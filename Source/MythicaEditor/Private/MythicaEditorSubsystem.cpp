#include "MythicaEditorSubsystem.h"

#include "AssetExportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "FileUtilities/ZipArchiveReader.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "IImageWrapperModule.h"
#include "ImageCoreUtils.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "LevelEditor.h"
#include "MythicaComponent.h"
#include "MythicaDeveloperSettings.h"
#include "MythicaInputSelectionVolume.h"
#include "MythicaUSDUtil.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"
#include "WebSocketsModule.h"

#define MYTHICA_CLEAN_TEMP_FILES 1
#define MYTHICA_ENABLE_WEBSOCKETS 0

DEFINE_LOG_CATEGORY(LogMythica);

const TCHAR* ConfigFile = TEXT("PackageInfo.ini");
const TCHAR* ConfigPackageInfoSection = TEXT("PackageInfo");
const TCHAR* ConfigPackageIdKey = TEXT("PackageId");

const TCHAR* ImportableFileExtensions[] = 
{ 
    TEXT("hda"), TEXT("hdalc"), TEXT("hdanc"),
    TEXT("otl"), TEXT("otllc"), TEXT("otlnc")
};

static bool JobWaitingForStreamItems(EMythicaJobState State)
{
    return State == EMythicaJobState::Queued || State == EMythicaJobState::Processing;
}

static bool CanImportAsset(const FString& FilePath)
{
    FString Extension = FPaths::GetExtension(FilePath);

    for (const TCHAR* ImportableExtension : ImportableFileExtensions)
    {
        if (Extension == ImportableExtension)
        {
            return true;
        }
    }
    return false;
}

static bool CanDisplayImage(const FString& FileName)
{
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    EImageFormat ImageFormat = ImageWrapperModule.GetImageFormatFromExtension(*FileName);
    return ImageFormat != EImageFormat::Invalid;
}

static FString MakeUniquePath(const FString& AbsolutePath)
{
    FString UniquePath = AbsolutePath;

    uint32 Counter = 1;
    while (IFileManager::Get().DirectoryExists(*UniquePath))
    {
        UniquePath = FString::Printf(TEXT("%s_%d"), *AbsolutePath, Counter);
        Counter++;
    }

    return UniquePath;
}

bool FMythicaAssetVersion::operator<(const FMythicaAssetVersion& Other) const
{
    return Major < Other.Major
        || (Major == Other.Major && Minor < Other.Minor)
        || (Major == Other.Major && Minor == Other.Minor && Patch < Other.Patch);
}

bool FMythicaAssetVersion::operator==(const FMythicaAssetVersion& Other) const
{
    return Major == Other.Major && Minor == Other.Minor && Patch == Other.Patch;
}

bool FMythicaAssetVersion::IsValid() const
{
    return Major > 0 || Minor > 0 || Patch > 0;
}

FString FMythicaAssetVersion::ToString() const
{
    return IsValid() ? FString::Printf(TEXT("%d.%d.%d"), Major, Minor, Patch) : "";
}

bool FMythicaAssetVersionEntryPointReference::IsValid() const
{
    return !AssetId.IsEmpty();
}

bool FMythicaAssetVersionEntryPointReference::Compare(const FMythicaAssetVersionEntryPointReference& Other) const
{
    return AssetId == Other.AssetId 
        && FileName == Other.FileName
        && EntryPoint == Other.EntryPoint;
}

void UMythicaEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    LevelEditorModule.OnMapChanged().AddUObject(this, &UMythicaEditorSubsystem::OnMapChanged);

    UMythicaDeveloperSettings* Settings = GetMutableDefault<UMythicaDeveloperSettings>();
    Settings->OnSettingChanged().AddUObject(this, &UMythicaEditorSubsystem::OnSettingsChanged);

    CreateSession();

    LoadInstalledAssetList();
}

void UMythicaEditorSubsystem::Deinitialize()
{
    Super::Deinitialize();

    if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
    {
        FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
        LevelEditorModule.OnMapChanged().RemoveAll(this);
    }

    UMythicaDeveloperSettings* Settings = GetMutableDefault<UMythicaDeveloperSettings>();
    Settings->OnSettingChanged().RemoveAll(this);
}

void UMythicaEditorSubsystem::ResetSession()
{
    ClearJobs();

    DestroySessionWebSocket();
    AuthToken.Empty();
    SetSessionState(EMythicaSessionState::None);

    JobDefinitionList.Reset();
    AssetList.Reset();
    UpdateStats();

    CreateSession();
}

void UMythicaEditorSubsystem::OnMapChanged(UWorld* InWorld, EMapChangeType InMapChangeType)
{
    if (InMapChangeType == EMapChangeType::TearDownWorld)
    {
        ClearJobs();
    }
}

void UMythicaEditorSubsystem::OnSettingsChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent)
{
    ResetSession();
}

EMythicaSessionState UMythicaEditorSubsystem::GetSessionState()
{
    return SessionState;
}

bool UMythicaEditorSubsystem::CanInstallAssets()
{
    return FModuleManager::Get().ModuleExists(TEXT("HoudiniEngine"));
}

bool UMythicaEditorSubsystem::CanGenerateMeshes()
{
    return IPluginManager::Get().FindEnabledPlugin(TEXT("USDImporter")).IsValid();
}

TArray<FMythicaAsset> UMythicaEditorSubsystem::GetAssetList()
{
    return AssetList;
}

TArray<FMythicaJobDefinition> UMythicaEditorSubsystem::GetJobDefinitionList(const FString& JobType)
{
    UMythicaDeveloperSettings* Settings = GetMutableDefault<UMythicaDeveloperSettings>();

    TArray<FMythicaJobDefinition> Definitions;
    for (const FMythicaJobDefinition& Definition : JobDefinitionList)
    {
        if (Definition.JobType == JobType)
        {
            Definitions.Add(Definition);
        }
    }
    return Definitions;
}

FMythicaJobDefinition UMythicaEditorSubsystem::GetJobDefinitionById(const FString& JobDefId)
{
    for (const FMythicaJobDefinition& Definition : JobDefinitionList)
    {
        if (Definition.JobDefId == JobDefId)
        {
            return Definition;
        }
    }

    return {};
}

FMythicaJobDefinition UMythicaEditorSubsystem::GetJobDefinitionLatest(const FMythicaAssetVersionEntryPointReference& EntryPointReference)
{
    if (!EntryPointReference.IsValid())
    {
        return {};
    }

    FMythicaJobDefinition LatestDefinition = {};
    for (const FMythicaJobDefinition& Definition : JobDefinitionList)
    {
        if (EntryPointReference.Compare(Definition.Source) && LatestDefinition.Source.Version < Definition.Source.Version)
        {
            return LatestDefinition = Definition;
        }
    }

    return LatestDefinition;
}

TMap<int, FMythicaJob> UMythicaEditorSubsystem::GetActiveJobsList() const
{
    return Jobs;
}

bool UMythicaEditorSubsystem::IsAssetInstalled(const FString& PackageId)
{
    return InstalledAssets.Contains(PackageId);
}

bool UMythicaEditorSubsystem::IsAssetFavorite(const FString& AssetId)
{
    return FavoriteAssetIds.Contains(AssetId);
}

UTexture2D* UMythicaEditorSubsystem::GetThumbnail(const FString& PackageId)
{
    UTexture2D** Texture = ThumbnailCache.Find(PackageId);
    return Texture ? *Texture : nullptr;
}

FMythicaStats UMythicaEditorSubsystem::GetStats()
{
    return Stats;
}

EMythicaJobState UMythicaEditorSubsystem::GetRequestState(int RequestId)
{
    FMythicaJob* RequestData = Jobs.Find(RequestId);
    return RequestData ? RequestData->State : EMythicaJobState::Invalid;
}

FString UMythicaEditorSubsystem::GetImportDirectory(int RequestId)
{
    FMythicaJob* RequestData = Jobs.Find(RequestId);
    return RequestData ? RequestData->ImportDirectory : FString();
}

void UMythicaEditorSubsystem::CreateSession()
{
    if (SessionState != EMythicaSessionState::None && SessionState != EMythicaSessionState::SessionFailed)
    {
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();
    FString APIKey = Settings->GetAPIKey();
    if (APIKey.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("API key not setup"));
        SetSessionState(EMythicaSessionState::SessionFailed);
        return;
    }

    FString Url = FString::Printf(TEXT("%s/v1/sessions/key/%s"), *Settings->GetServiceURL(), *APIKey);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");
    Request->OnProcessRequestComplete().BindUObject(this, &UMythicaEditorSubsystem::OnCreateSessionResponse);

    Request->ProcessRequest();

    SetSessionState(EMythicaSessionState::RequestingSession);
}

void UMythicaEditorSubsystem::OnCreateSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to create session"));
        SetSessionState(EMythicaSessionState::SessionFailed);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse create session JSON string"));
        SetSessionState(EMythicaSessionState::SessionFailed);
        return;
    }

    FString Token;
    if (!JsonObject->TryGetStringField(TEXT("token"), Token))
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get token from JSON string"));
        SetSessionState(EMythicaSessionState::SessionFailed);
        return;
    }

    AuthToken = Token;

    SetSessionState(EMythicaSessionState::SessionCreated);

    CreateSessionWebSocket();
    UpdateJobDefinitionList();
}

void UMythicaEditorSubsystem::UpdateAssetList()
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/assets/top"), *Settings->GetServiceURL());

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");
    Request->OnProcessRequestComplete().BindUObject(this, &UMythicaEditorSubsystem::OnGetAssetsResponse);

    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnGetAssetsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get assets"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonValue> JsonValue;
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse get assets JSON string"));
        return;
    }

    if (JsonValue->Type != EJson::Array)
    {
        UE_LOG(LogMythica, Error, TEXT("JSON value is not an array"));
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();
    
    AssetList.Reset();

    const TArray<TSharedPtr<FJsonValue>>& Array = JsonValue->AsArray();
    for (TSharedPtr<FJsonValue> Value : Array)
    {
        TSharedPtr<FJsonObject> JsonObject = Value->AsObject();
        if (!JsonObject.IsValid())
        {
            continue;
        }

        FString AssetId = JsonObject->GetStringField(TEXT("asset_id"));
        FString PackageId = JsonObject->GetStringField(TEXT("package_id"));
        FString Name = JsonObject->GetStringField(TEXT("name"));
        FString Description = JsonObject->GetStringField(TEXT("description"));
        FString OrgName = JsonObject->GetStringField(TEXT("org_name"));
        if (PackageId.IsEmpty())
        {
            UE_LOG(LogMythica, Error, TEXT("Missing PackageId for package: %s"), *Name);
            continue;
        }

        TArray<TSharedPtr<FJsonValue>> Version = JsonObject->GetArrayField(TEXT("version"));
        if (Version.Num() != 3)
        {
            continue;
        }

        FMythicaAssetVersion AssetVersion = {
            Version[0]->AsNumber(), 
            Version[1]->AsNumber(), 
            Version[2]->AsNumber() 
        };

        TSharedPtr<FJsonObject> ContentsObject = JsonObject->GetObjectField(TEXT("contents"));
        if (!ContentsObject.IsValid())
        {
            continue;
        }

        int32 DigitalAssetCount = 0;

        const TArray<TSharedPtr<FJsonValue>>* FileArray = nullptr;
        ContentsObject->TryGetArrayField(TEXT("files"), FileArray);
        if (FileArray)
        {
            for (const TSharedPtr<FJsonValue>& FileValue : *FileArray)
            {
                TSharedPtr<FJsonObject> FileObject = FileValue->AsObject();

                FString FileName = FileObject->GetStringField(TEXT("file_name"));
                if (CanImportAsset(FileName))
                {
                    DigitalAssetCount++;
                }
            }
        }

        FString ThumbnailURL;
        const TArray<TSharedPtr<FJsonValue>>* ThumbnailArray = nullptr;
        ContentsObject->TryGetArrayField(TEXT("thumbnails"), ThumbnailArray);
        if (ThumbnailArray)
        {
            for (const TSharedPtr<FJsonValue>& ThumbnailValue : *ThumbnailArray)
            {
                TSharedPtr<FJsonObject> ThumbnailObject = ThumbnailValue->AsObject();

                FString FileName = ThumbnailObject->GetStringField(TEXT("file_name"));
                if (CanDisplayImage(FileName))
                {
                    FString FileExtension = FPaths::GetExtension(FileName);
                    FString ContentHash = ThumbnailObject->GetStringField(TEXT("content_hash"));
                    ThumbnailURL = FString::Printf(TEXT("%s/%s.%s"), *Settings->GetImagesURL(), *ContentHash, *FileExtension);
                    break;
                }
            }
        }

        FString PackageURL;
        PackageURL = FString::Printf(TEXT("%s/package-view/%s/versions/%d.%d.%d"), *Settings->GetServiceURL(), *AssetId, AssetVersion.Major, AssetVersion.Minor, AssetVersion.Patch);

        AssetList.Push({ AssetId, PackageId, Name, Description, OrgName, AssetVersion, {}, ThumbnailURL, PackageURL, DigitalAssetCount });
    }

    UpdateStats();

    OnAssetListUpdated.Broadcast();

    LoadThumbnails();
}

void UMythicaEditorSubsystem::InstallAsset(const FString& PackageId)
{
    if (InstalledAssets.Contains(PackageId))
    {
        UE_LOG(LogMythica, Error, TEXT("Package already installed %s"), *PackageId);
        return;
    }

    FMythicaAsset* Asset = FindAsset(PackageId);
    if (!Asset)
    {
        UE_LOG(LogMythica, Error, TEXT("Unknown asset type %s"), *PackageId);
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString DownloadId = PackageId;

    FString Url = FString::Printf(TEXT("%s/v1/download/info/%s"), *Settings->GetServiceURL(), *DownloadId);

    auto Callback = [this, PackageId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnDownloadInfoResponse(Request, Response, bConnectedSuccessfully, PackageId);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/octet-stream");
    Request->OnProcessRequestComplete().BindLambda(Callback);

    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get download info for package %s"), *PackageId);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse download info JSON string"));
        return;
    }

    FString DownloadURL = JsonObject->GetStringField(TEXT("url"));
    FString ContentType = JsonObject->GetStringField(TEXT("content_type"));
    if (DownloadURL.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get download URL for package %s"), *PackageId);
        return;
    }   

    auto Callback = [this, PackageId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnDownloadAssetResponse(Request, Response, bConnectedSuccessfully, PackageId);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> DownloadRequest = FHttpModule::Get().CreateRequest();
    DownloadRequest->SetURL(DownloadURL);
    DownloadRequest->SetVerb("GET");
    DownloadRequest->SetHeader("Content-Type", *ContentType);
    DownloadRequest->OnProcessRequestComplete().BindLambda(Callback);

    DownloadRequest->ProcessRequest();
}

struct FFileImportData
{
    FString FilePath;
    FString RelativeImportPath;
};

void UMythicaEditorSubsystem::OnDownloadAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to download asset"));
        return;
    }

    if (InstalledAssets.Contains(PackageId))
    {
        UE_LOG(LogMythica, Error, TEXT("Package already installed %s"), *PackageId);
        return;
    }

    FMythicaAsset* Asset = FindAsset(PackageId);
    if (!Asset)
    {
        UE_LOG(LogMythica, Error, TEXT("Unknown asset type %s"), *PackageId);
        return;
    }

    // Save package to disk
    FString PackagePath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), TEXT("PackageCache"), PackageId, PackageId + ".zip");

    TArray<uint8> PackageData = Response->GetContent();
    bool PackageWritten = FFileHelper::SaveArrayToFile(PackageData, *PackagePath);
    if (!PackageWritten)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to write package file %s"), *PackagePath);
        return;
    }

    // Unzip package
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    IFileHandle* FileHandle = PlatformFile.OpenRead(*PackagePath);
    if (!FileHandle)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to package file %s"), *PackagePath);
        return;
    }

    FZipArchiveReader Reader(FileHandle);
    if (!Reader.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to open zip archive %s"), *PackagePath);
        return;
    }

    TArray<FFileImportData> ImportFiles;

    TArray<FString> Files = Reader.GetFileNames();
    for (const FString& File : Files)
    {
        TArray<uint8> FileData;
        bool FileRead = Reader.TryReadFile(File, FileData);
        if (!FileRead)
        {
            UE_LOG(LogMythica, Error, TEXT("Failed to read file from archive %s"), *File);
            continue;
        }

        FString FilePath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), TEXT("PackageCache"), PackageId, File);
        bool FileWritten = FFileHelper::SaveArrayToFile(FileData, *FilePath);
        if (!FileWritten)
        {
            UE_LOG(LogMythica, Error, TEXT("Failed to write file %s"), *FilePath);
            continue;
        }

        if (CanImportAsset(FilePath))
        {
            ImportFiles.Add({ FilePath, File });
        }
    }

    // Get unique import directory
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString ImportPackagePath = FPaths::Combine(Settings->PackageImportDirectory, ObjectTools::SanitizeObjectName(Asset->Name));
    FString DirectoryRelative = FPackageName::LongPackageNameToFilename(ImportPackagePath);
    FString DirectoryAbsolute = FPaths::ConvertRelativePathToFull(DirectoryRelative);
    FString BaseImportDirectory = MakeUniquePath(DirectoryAbsolute);

    // Import HDA files into Unreal
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

    TArray<UObject*> ImportedObjects;
    for (const FFileImportData& Data : ImportFiles)
    {
        FString FullImportDirectory = FPaths::GetPath(FPaths::Combine(BaseImportDirectory, Data.RelativeImportPath));
        
        UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
        ImportData->bReplaceExisting = true;
        ImportData->DestinationPath = FullImportDirectory;
        ImportData->Filenames = { Data.FilePath };

        TArray<UObject*> Objects = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
        if (Objects.Num() != 1)
        {
            UE_LOG(LogMythica, Error, TEXT("Failed to import HDA from package %s"), *PackageId);
            return;
        }

        ImportedObjects.Add(Objects[0]);
    }

    for (UObject* Object : ImportedObjects)
    {
        UPackage* Package = Cast<UPackage>(Object->GetOuter());
        FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;

        UPackage::SavePackage(Package, nullptr, *Filename, SaveArgs);
    }

    AddInstalledAsset(PackageId, BaseImportDirectory);

    OnAssetInstalled.Broadcast(PackageId);
}

void UMythicaEditorSubsystem::UninstallAsset(const FString& PackageId)
{
    FString* InstallDirectory = InstalledAssets.Find(PackageId);
    if (!InstallDirectory)
    {
        UE_LOG(LogMythica, Error, TEXT("Trying to uninstall package that isn't installed %s"), *PackageId);
        return;
    }

    FString PackagePath = FPackageName::FilenameToLongPackageName(*InstallDirectory);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssetsByPath(*PackagePath, Assets, true, false);

    int32 NumDeleted = ObjectTools::DeleteAssets(Assets, false);
    if (NumDeleted != Assets.Num())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to delete assets from %s"), **InstallDirectory);
        return;
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    bool DirectoryDeleted = PlatformFile.DeleteDirectoryRecursively(**InstallDirectory);
    if (!DirectoryDeleted)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to delete directory %s"), **InstallDirectory);
        return;
    }

    FString ConfigFileAbsolute = FPaths::Combine(*InstallDirectory, ConfigFile);
    GConfig->UnloadFile(ConfigFileAbsolute);

    InstalledAssets.Remove(PackageId);

    OnAssetUninstalled.Broadcast(PackageId);
}

void UMythicaEditorSubsystem::FavoriteAsset(const FString& AssetId)
{
    ExecuteFavoriteAsset(AssetId, true);
}

void UMythicaEditorSubsystem::UnfavoriteAsset(const FString& AssetId)
{
    ExecuteFavoriteAsset(AssetId, false);
}

void UMythicaEditorSubsystem::ExecuteFavoriteAsset(const FString& AssetId, bool State)
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/assets/g/unreal/%s/versions/0.0.0"), *Settings->GetServiceURL(), *AssetId);

    auto Callback = [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnFavortiteAssetResponse(Request, Response, bConnectedSuccessfully);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(State ? "Post" : "Delete");
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
    Request->OnProcessRequestComplete().BindLambda(Callback);

    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnFavortiteAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to change asset group"));
        return;
    }

    UpdateJobDefinitionList();
}

void UMythicaEditorSubsystem::UpdateJobDefinitionList()
{
    JobDefinitionList.Reset();
    FavoriteAssetIds.Reset();

    // Whitelist job definitions
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    for (const FString& JobDefId : Settings->GetJobDefIdWhitelist())
    {
        FString Url = FString::Printf(TEXT("%s/v1/jobs/definitions/%s"), *Settings->GetServiceURL(), *JobDefId);

        auto Callback = [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            OnJobDefinitionResponse(Request, Response, bConnectedSuccessfully);
        };

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb("Get");
        Request->OnProcessRequestComplete().BindLambda(Callback);

        Request->ProcessRequest();
    }

    // Whitelist assets
    for (const FString& AssetId : Settings->GetAssetWhitelist())
    {
        FString Url = FString::Printf(TEXT("%s/v1/assets/%s"), *Settings->GetServiceURL(), *AssetId);

        auto Callback = [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            OnAssetResponse(Request, Response, bConnectedSuccessfully);
        };

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb("Get");
        Request->OnProcessRequestComplete().BindLambda(Callback);

        Request->ProcessRequest();
    }

    // Asset group assets
    FString Url = FString::Printf(TEXT("%s/v1/assets/g/unreal"), *Settings->GetServiceURL());

    auto Callback = [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnAssetGroupResponse(Request, Response, bConnectedSuccessfully);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("Get");
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
    Request->OnProcessRequestComplete().BindLambda(Callback);

    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnJobDefinitionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get job definition"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonValue> JsonValue;
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse job definition JSON string"));
        return;
    }

    TSharedPtr<FJsonObject> JsonObject = JsonValue->AsObject();
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse job definition object"));
        return;
    }

    FString JobDefId = JsonObject->GetStringField(TEXT("job_def_id"));
    if (JobDefId.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get job definition ID"));
        return;
    }

    auto MatchJobDefId = [JobDefId](const FMythicaJobDefinition& Definition) { return Definition.JobDefId == JobDefId; };
    if (JobDefinitionList.ContainsByPredicate(MatchJobDefId))
    {
        return;
    }

    FString JobType = JsonObject->GetStringField(TEXT("job_type"));
    FString Name = JsonObject->GetStringField(TEXT("name"));
    FString Description = JsonObject->GetStringField(TEXT("description"));

    TSharedPtr<FJsonObject> ParamsSchema = JsonObject->GetObjectField(TEXT("params_schema"));
    TSharedPtr<FJsonObject> ParamsSet = ParamsSchema->GetObjectField(TEXT("params"));

    FMythicaParameters Params;
    Mythica::ReadParameters(ParamsSet, Params);

    JobDefinitionList.Push({ JobDefId, JobType, Name, Description, Params, {}, {}, {} });
}

void UMythicaEditorSubsystem::OnAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get asset"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonValue> JsonValue;
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse asset JSON string"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>& Array = JsonValue->AsArray();
    if (Array.Num() == 0)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get asset version"));
        return;
    }

    TSharedPtr<FJsonValue> VersionValue = Array[0];
    TSharedPtr<FJsonObject> VersionObject = VersionValue->AsObject();

    RequestJobDefsForAssetVersion(VersionObject);
}

void UMythicaEditorSubsystem::RequestJobDefsForAssetVersion(TSharedPtr<FJsonObject> AssetVersion)
{
    FString AssetId = AssetVersion->GetStringField(TEXT("asset_id"));
    FString Name = AssetVersion->GetStringField(TEXT("name"));
    FString Owner = AssetVersion->GetStringField(TEXT("owner_name"));

    TArray<TSharedPtr<FJsonValue>> Version = AssetVersion->GetArrayField(TEXT("version"));
    int32 Major = Version[0]->AsNumber();
    int32 Minor = Version[1]->AsNumber();
    int32 Patch = Version[2]->AsNumber();

    TMap<FString, FString> FileNames;

    TSharedPtr<FJsonObject> ContentsObject = AssetVersion->GetObjectField(TEXT("contents"));
    const TArray<TSharedPtr<FJsonValue>>* FileArray = nullptr;
    ContentsObject->TryGetArrayField(TEXT("files"), FileArray);
    if (FileArray)
    {
        for (const TSharedPtr<FJsonValue>& FileValue : *FileArray)
        {
            TSharedPtr<FJsonObject> FileObject = FileValue->AsObject();

            FString FileId = FileObject->GetStringField(TEXT("file_id"));
            FString FileName = FileObject->GetStringField(TEXT("file_name"));
            FileNames.Add(FileId, FileName);
        }
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/jobs/definitions/by_asset/%s/versions/%d/%d/%d"), *Settings->GetServiceURL(), *AssetId, Major, Minor, Patch);

    auto Callback = [this, AssetId, Name, Owner, FileNames](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnAssetJobDefsResponse(Request, Response, bConnectedSuccessfully, Name, Owner, FileNames);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> AssetJobDefsRequest = FHttpModule::Get().CreateRequest();
    AssetJobDefsRequest->SetURL(Url);
    AssetJobDefsRequest->SetVerb("Get");
    AssetJobDefsRequest->OnProcessRequestComplete().BindLambda(Callback);

    AssetJobDefsRequest->ProcessRequest();
}

void UMythicaEditorSubsystem::OnAssetJobDefsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& SourceName, const FString& SourceOwner, const TMap<FString, FString>& FileNames)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get asset job definitions"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonValue> JsonValue;
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse asset job definition JSON string"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>& Array = JsonValue->AsArray();
    for (TSharedPtr<FJsonValue> Value : Array)
    {
        TSharedPtr<FJsonObject> JsonObject = Value->AsObject();
        if (!JsonObject.IsValid())
        {
            UE_LOG(LogMythica, Error, TEXT("Failed to parse asset job definition object"));
            continue;
        }

        FString JobDefId = JsonObject->GetStringField(TEXT("job_def_id"));
        if (JobDefId.IsEmpty())
        {
            UE_LOG(LogMythica, Error, TEXT("Failed to get asset job definition ID"));
            continue;
        }

        auto MatchJobDefId = [JobDefId](const FMythicaJobDefinition& Definition) { return Definition.JobDefId == JobDefId; };
        if (JobDefinitionList.ContainsByPredicate(MatchJobDefId))
        {
            continue;
        }

        FString JobType = JsonObject->GetStringField(TEXT("job_type"));
        FString Name = JsonObject->GetStringField(TEXT("name"));
        FString Description = JsonObject->GetStringField(TEXT("description"));

        TSharedPtr<FJsonObject> ParamsSchema = JsonObject->GetObjectField(TEXT("params_schema"));
        TSharedPtr<FJsonObject> ParamsSet = ParamsSchema->GetObjectField(TEXT("params"));

        FMythicaParameters Params;
        Mythica::ReadParameters(ParamsSet, Params);

        TSharedPtr<FJsonObject> SourceObject = JsonObject->GetObjectField(TEXT("source"));
        FString FileId = SourceObject->GetStringField(TEXT("file_id"));

        const FString* FileName = FileNames.Find(FileId);
        if (!FileName)
        {
            UE_LOG(LogMythica, Error, TEXT("Asset contains job with invalid source file_id"));
            continue;
        }

        FMythicaAssetVersionEntryPointReference Source;
        Source.AssetId = SourceObject->GetStringField(TEXT("asset_id"));
        Source.Version.Major = SourceObject->GetNumberField(TEXT("major"));
        Source.Version.Minor = SourceObject->GetNumberField(TEXT("minor"));
        Source.Version.Patch = SourceObject->GetNumberField(TEXT("patch"));
        Source.FileId = FileId;
        Source.FileName = *FileName;
        Source.EntryPoint = SourceObject->GetStringField(TEXT("entry_point"));

        JobDefinitionList.Push({ JobDefId, JobType, Name, Description, Params, Source, SourceName, SourceOwner });
    }
}

void UMythicaEditorSubsystem::OnAssetGroupResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get asset group"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonValue> JsonValue;
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse asset group JSON string"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>& Array = JsonValue->AsArray();
    for (TSharedPtr<FJsonValue> Value : Array)
    {
        TSharedPtr<FJsonObject> JsonObject = Value->AsObject();
        if (!JsonObject.IsValid())
        {
            UE_LOG(LogMythica, Error, TEXT("Failed to parse asset group object"));
            continue;
        }

        FString AssetId = JsonObject->GetStringField(TEXT("asset_id"));
        FavoriteAssetIds.Add(AssetId);

        RequestJobDefsForAssetVersion(JsonObject);
    }

    OnFavoriteAssetsUpdated.Broadcast();
}

bool UMythicaEditorSubsystem::PrepareInputFiles(const FMythicaParameters& Params, TMap<int, FString>& InputFiles, FString& ExportDirectory, const FVector& Origin)
{
    FString DesiredDirectory = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), TEXT("ExportCache"), TEXT("Export"));
    ExportDirectory = MakeUniquePath(DesiredDirectory);

    for (int i = 0; i < Params.Parameters.Num(); i++)
    {
        if (Params.Parameters[i].Type != EMythicaParameterType::File)
        {
            continue;
        }
        
        const FMythicaParameterFile& Input = Params.Parameters[i].ValueFile;
        if (Input.Type == EMythicaInputType::Mesh)
        {
            if (!Input.Mesh)
            {
                continue;
            }

            FString FilePath = FPaths::Combine(ExportDirectory, FString::Format(TEXT("Input{0}"), { i }), "Mesh.usdz");
            bool Success = Mythica::ExportMesh(Input.Mesh, FilePath);
            if (!Success)
            {
                UE_LOG(LogMythica, Error, TEXT("Failed to export mesh %s"), *Input.Mesh->GetName());
                return false;
            }

            InputFiles.Add(i, FilePath);
        }
        else if (Input.Type == EMythicaInputType::World)
        {
            TArray<AActor*> Actors = Input.Actors;
            Actors.RemoveAll([](AActor* Actor) { return !Actor; });
            if (Actors.IsEmpty())
            {
                continue;
            }

            FString FilePath = FPaths::Combine(ExportDirectory, FString::Format(TEXT("Input{0}"), { i }), "Mesh.usdz");
            bool Success = Mythica::ExportActors(Actors, FilePath, Origin, Input.Settings.TransformType);
            if (!Success)
            {
                UE_LOG(LogMythica, Error, TEXT("Failed to export actors"));
                return false;
            }

            InputFiles.Add(i, FilePath);
        }
        else if (Input.Type == EMythicaInputType::Spline)
        {
            if (!Input.SplineActor)
            {
                continue;
            }

            FString FilePath = FPaths::Combine(ExportDirectory, FString::Format(TEXT("Input{0}"), { i }), "Mesh.usdz");
            bool Success = Mythica::ExportSpline(Input.SplineActor, FilePath, Origin, Input.Settings.TransformType);
            if (!Success)
            {
                UE_LOG(LogMythica, Error, TEXT("Failed to export spline"));
                return false;
            }

            InputFiles.Add(i, FilePath);
        }
        else if (Input.Type == EMythicaInputType::Volume)
        {
            if (!Input.VolumeActor)
            {
                continue;
            }

            TArray<AActor*> Actors;
            Input.VolumeActor->GetActors(Actors);
            if (Actors.IsEmpty())
            {
                continue;
            }

            FString FilePath = FPaths::Combine(ExportDirectory, FString::Format(TEXT("Input{0}"), { i }), "Mesh.usdz");
            bool Success = Mythica::ExportActors(Actors, FilePath, Origin, Input.Settings.TransformType);
            if (!Success)
            {
                UE_LOG(LogMythica, Error, TEXT("Failed to export volume actors"));
                return false;
            }

            InputFiles.Add(i, FilePath);
        }
    }

    return true;
}

void UMythicaEditorSubsystem::UploadInputFiles(int RequestId, const TMap<int, FString>& InputFiles)
{
    // Construct the upload request
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/upload/store"), *Settings->GetServiceURL());

    auto Callback = [this, RequestId, InputFiles](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnUploadInputFilesResponse(Request, Response, bConnectedSuccessfully, RequestId, InputFiles);
    };

    FString NewLine = "\r\n";
    FString Boundary = "---------------------------" + FString::FromInt(FDateTime::Now().GetTicks());

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("POST");
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
    Request->SetHeader(TEXT("Content-Type"), TEXT("multipart/form-data; boundary =" + Boundary));
    Request->OnProcessRequestComplete().BindLambda(Callback);

    // Construct the payload
    TArray<uint8> RequestBodyBytes;

    for (TMap<int, FString>::TConstIterator It(InputFiles); It; ++It)
    {
        const FString& File = It.Value();

        TArray<uint8> FileBytes;
        bool PackageWritten = FFileHelper::LoadFileToArray(FileBytes, *File);

        FString RequestContent;
        RequestContent += "--" + Boundary + NewLine;
        RequestContent += "Content-Disposition: form-data; name=\"files\"; filename=\"" + FPaths::GetCleanFilename(File) + "\"" + NewLine;
        RequestContent += "Content-Type: application/octet-stream" + NewLine + NewLine;

        RequestBodyBytes.Append((uint8*)TCHAR_TO_ANSI(*RequestContent), RequestContent.Len());
        RequestBodyBytes.Append(FileBytes);
        RequestBodyBytes.Append((uint8*)TCHAR_TO_ANSI(*NewLine), NewLine.Len());
    }

    FString ClosingBoundary = "--" + Boundary + "--" + NewLine;
    RequestBodyBytes.Append((uint8*)TCHAR_TO_ANSI(*ClosingBoundary), ClosingBoundary.Len());

    Request->SetContent(RequestBodyBytes);

    // Send the request
    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnUploadInputFilesResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId, const TMap<int, FString>& InputFiles)
{
    FMythicaJob* RequestData = Jobs.Find(RequestId);
    if (!RequestData)
    {
        return;
    }

    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to upload inputs"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to upload input data 1"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse upload inputs JSON string"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to upload input data 2"));
        return;
    }

    TArray<TSharedPtr<FJsonValue>> Files = JsonObject->GetArrayField(TEXT("files"));
    if (Files.Num() != InputFiles.Num())
    {
        UE_LOG(LogMythica, Error, TEXT("Unexpected number of uploaded files"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to upload input data 3"));
        return;
    }

    TArray<FString> InputFileIds;

    int FileIndex = 0;
    for (TMap<int, FString>::TConstIterator It(InputFiles); It; ++It, ++FileIndex)
    {
        int InputIndex = It.Key();
        const FString& File = It.Value();

        TSharedPtr<FJsonObject> FileObject = Files[FileIndex]->AsObject();
        FString FileId = FileObject->GetStringField(TEXT("file_id"));

        InputFileIds.SetNum(InputIndex + 1, false);
        InputFileIds[InputIndex] = FileId;
    }

    RequestData->InputFileIds = InputFileIds;

    SendJobRequest(RequestId);
}

int UMythicaEditorSubsystem::ExecuteJob(
    const FString& JobDefId, 
    const FMythicaParameters& Params, 
    const FString& ImportPath,
    const FVector& Origin,
    UMythicaComponent* ExecutingComp
) {
    if (SessionState != EMythicaSessionState::SessionCreated)
    {
        UE_LOG(LogMythica, Error, TEXT("Unable to create job due to session not created"));
        return -1;
    }

    FString ExportDirectory;
    TMap<int, FString> InputFiles;
    bool bSuccess = PrepareInputFiles(Params, InputFiles, ExportDirectory, Origin);
    if (!bSuccess)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to prepare job input files"));
        return -1;
    }

    int RequestId = CreateJob(JobDefId, Params, ImportPath, ExecutingComp);

    if (InputFiles.IsEmpty())
    {
        SendJobRequest(RequestId);
    }
    else
    {
        UploadInputFiles(RequestId, InputFiles);
        if (MYTHICA_CLEAN_TEMP_FILES)
        {
            IFileManager::Get().DeleteDirectory(*ExportDirectory, false, true);
        }
    }

    return RequestId;
}

void UMythicaEditorSubsystem::SendJobRequest(int RequestId)
{
    FMythicaJob* RequestData = Jobs.Find(RequestId);
    if (!RequestData)
    {
        return;
    }

    // Create JSON payload
    TSharedPtr<FJsonObject> ParamsSetObject = MakeShareable(new FJsonObject);
    Mythica::WriteParameters(RequestData->InputFileIds, RequestData->Params, ParamsSetObject);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("job_def_id"), RequestData->JobDefId);
    JsonObject->SetObjectField(TEXT("params"), ParamsSetObject);

    FString ContentJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentJson);
    bool Success = FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    check(Success);

    // Send request
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/jobs/"), *Settings->GetServiceURL());

    auto Callback = [this, RequestId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnExecuteJobResponse(Request, Response, bConnectedSuccessfully, RequestId);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("POST");
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(ContentJson);
    Request->OnProcessRequestComplete().BindLambda(Callback);

    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnExecuteJobResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId)
{
    FMythicaJob* RequestData = Jobs.Find(RequestId);
    if (!RequestData)
    {
        return;
    }

    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to request job"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to request job 1"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse create job JSON string"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to request job 2"));
        return;
    }

    FString JobId;
    if (!JsonObject->TryGetStringField(TEXT("job_id"), JobId))
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get JobId from JSON string"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to request job 3"));
        return;
    }

    RequestData->JobId = JobId;
    SetJobState(RequestId, EMythicaJobState::Queued);
}

int UMythicaEditorSubsystem::CreateJob(const FString& JobDefId, const FMythicaParameters& Params, const FString& ImportPath, UMythicaComponent* Component)
{
    int RequestId = NextRequestId++;
    FMythicaJob& Job = Jobs.Add(RequestId, { JobDefId, {}, Params, ImportPath, Component, Component->GetReadableName()}); // UKismetSystemLibrary::GetDisplayName(Component) 

    Job.StartTime = FDateTime::Now();

    if (!JobPollTimer.IsValid())
    {
        FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UMythicaEditorSubsystem::PollJobStatus);
        GEditor->GetTimerManager()->SetTimer(JobPollTimer, TimerDelegate, 1.0f, true);
    }

    return RequestId;
}

void UMythicaEditorSubsystem::SetJobState(int RequestId, EMythicaJobState State, FText Message)
{
    FMythicaJob* JobData = Jobs.Find(RequestId);
    if (!JobData || JobData->State == State)
    {
        return;
    }

    JobData->State = State;
    OnJobStateChange.Broadcast(RequestId, State, Message);

    if (State == EMythicaJobState::Queued)
    {
        const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();
        FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UMythicaEditorSubsystem::OnJobTimeout, RequestId);
        GEditor->GetTimerManager()->SetTimer(JobData->TimeoutTimer, TimerDelegate, Settings->JobTimeoutSeconds, false);
    }
    else if (State == EMythicaJobState::Importing || State == EMythicaJobState::Failed)
    {
        GEditor->GetTimerManager()->ClearTimer(JobData->TimeoutTimer);
    }
    else if (State == EMythicaJobState::Completed)
    {
        JobData->EndTime = FDateTime::Now();
    }

    // TODO: Expire old request data

    if (Jobs.Num() == 0)
    {
        GEditor->GetTimerManager()->ClearTimer(JobPollTimer);
    }
}

void UMythicaEditorSubsystem::ClearJobs()
{
    Jobs.Reset();
    GEditor->GetTimerManager()->ClearTimer(JobPollTimer);
}

void UMythicaEditorSubsystem::PollJobStatus()
{
    if (MYTHICA_ENABLE_WEBSOCKETS)
    {
        return;
    }

    for (const TTuple<int, FMythicaJob>& JobEntry : Jobs)
    {
        int RequestId = JobEntry.Key;
        const FMythicaJob& JobData = JobEntry.Value;

        if (!JobWaitingForStreamItems(JobData.State))
        {
            continue;
        }

        const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

        FString Url = FString::Printf(TEXT("%s/v1/jobs/results/%s"), *Settings->GetServiceURL(), *JobData.JobId);

        auto Callback = [this, RequestId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            OnJobResultsResponse(Request, Response, bConnectedSuccessfully, RequestId);
        };

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb("GET");
        Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
        Request->SetHeader("Content-Type", "application/json");
        Request->OnProcessRequestComplete().BindLambda(Callback);

        Request->ProcessRequest();
    }
}

void UMythicaEditorSubsystem::OnJobTimeout(int RequestId)
{
    FMythicaJob* JobData = Jobs.Find(RequestId);
    if (!JobData)
    {
        return;
    }

    check(JobWaitingForStreamItems(JobData->State));
    SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Timed out"));
}

void UMythicaEditorSubsystem::OnJobResultsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId)
{
    FMythicaJob* RequestData = Jobs.Find(RequestId);
    if (!RequestData)
    {
        return;
    }

    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get job for request %d"), RequestId);
        return;
    }
    
    // Request may have already been completed for other reasons like timeout
    if (!JobWaitingForStreamItems(RequestData->State))
    {
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse job status JSON string"));
        return;
    }

    bool Completed = JsonObject->GetBoolField(TEXT("completed"));
    if (!Completed)
    {
        // TODO: Expose processing event in API
        if (RequestData->State == EMythicaJobState::Queued)
        {
            SetJobState(RequestId, EMythicaJobState::Processing);
        }

        return;
    }

    TArray<TSharedPtr<FJsonValue>> Results = JsonObject->GetArrayField(TEXT("results"));
    for (TSharedPtr<FJsonValue> Value : Results)
    {
        TSharedPtr<FJsonObject> ResultObject = Value->AsObject();
        TSharedPtr<FJsonObject> ResultDataObject = ResultObject->GetObjectField(TEXT("result_data"));

        OnStreamItem(ResultDataObject);
        if (!JobWaitingForStreamItems(RequestData->State))
        {
            break;
        }
    }
 
    // Verify request is satisfied if no additional stream items are expected
    if (JobWaitingForStreamItems(RequestData->State))
    {
        UE_LOG(LogMythica, Error, TEXT("Job failed %d"), RequestId);
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to produce result mesh"));
    }
}

void UMythicaEditorSubsystem::OnStreamItem(TSharedPtr<FJsonObject> StreamItem)
{
    // Find associated job request
    FString JobId;
    if (!StreamItem->TryGetStringField(TEXT("job_id"), JobId))
    {
        return;
    }

    int RequestId = -1;
    FMythicaJob* RequestData = nullptr;
    for (TTuple<int, FMythicaJob>& JobEntry : Jobs)
    {
        if (JobEntry.Value.JobId == JobId)
        {
            RequestId = JobEntry.Key;
            RequestData = &JobEntry.Value;
            break;
        }
    }
    if (!RequestData)
    {
        return;
    }

    if (!JobWaitingForStreamItems(RequestData->State))
    {
        return;
    }

    // Process stram item
    FString ItemType = StreamItem->GetStringField(TEXT("item_type"));
    if (ItemType == "progress")
    {
        if (RequestData->State == EMythicaJobState::Queued)
        {
            SetJobState(RequestId, EMythicaJobState::Processing);
        }
    }
    else if (ItemType == "file")
    {
        TSharedPtr<FJsonObject> FilesObject = StreamItem->GetObjectField(TEXT("files"));
        TArray<TSharedPtr<FJsonValue>> FilesArray = FilesObject->GetArrayField(TEXT("mesh"));
        if (FilesArray.Num() == 0)
        {
            UE_LOG(LogMythica, Error, TEXT("File result contains no mesh data"));
            return;
        }

        FString FileValue = FilesArray[0]->AsString();  
        if (FileValue.StartsWith("file_"))
        {
            const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

            FString Url = FString::Printf(TEXT("%s/v1/download/info/%s"), *Settings->GetServiceURL(), *FileValue);

            auto Callback = [this, FileValue, RequestId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
            {
                OnMeshDownloadInfoResponse(Request, Response, bConnectedSuccessfully, RequestId);
            };

            TSharedRef<IHttpRequest, ESPMode::ThreadSafe> DownloadInfoRequest = FHttpModule::Get().CreateRequest();
            DownloadInfoRequest->SetURL(Url);
            DownloadInfoRequest->SetVerb("GET");
            DownloadInfoRequest->SetHeader("Content-Type", "application/octet-stream");
            DownloadInfoRequest->OnProcessRequestComplete().BindLambda(Callback);

            DownloadInfoRequest->ProcessRequest();

            SetJobState(RequestId, EMythicaJobState::Importing);
        }
        else
        {
            TArray<uint8> FileData;
            bool Success = FBase64::Decode(FileValue, FileData);
            if (!Success)
            {
                UE_LOG(LogMythica, Error, TEXT("Failed to decode result mesh data"));
                SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to decode result mesh data"));
                return;
            }

            OnResultMeshData(FileData, RequestId);
        }
    }
    else if (ItemType == "completed")
    {
        UE_LOG(LogMythica, Error, TEXT("Job failed %d"), RequestId);
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to produce result mesh"));
    }
}

void UMythicaEditorSubsystem::OnMeshDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get mesh download info"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to download result mesh 1"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse mesh download info JSON string"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to download result mesh 2"));
        return;
    }

    FString DownloadURL = JsonObject->GetStringField(TEXT("url"));
    FString ContentType = JsonObject->GetStringField(TEXT("content_type"));
    if (DownloadURL.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get download URL for mesh"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to download result mesh 3"));
        return;
    }

    auto Callback = [this, RequestId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnMeshDownloadResponse(Request, Response, bConnectedSuccessfully, RequestId);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> DownloadRequest = FHttpModule::Get().CreateRequest();
    DownloadRequest->SetURL(DownloadURL);
    DownloadRequest->SetVerb("GET");
    DownloadRequest->SetHeader("Content-Type", *ContentType);
    DownloadRequest->OnProcessRequestComplete().BindLambda(Callback);

    DownloadRequest->ProcessRequest();
}

void UMythicaEditorSubsystem::OnMeshDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to download asset"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to download result mesh 4"));
        return;
    }

    TArray<uint8> FileData = Response->GetContent();
    OnResultMeshData(FileData, RequestId);
}

void UMythicaEditorSubsystem::OnResultMeshData(const TArray<uint8>& FileData, int RequestId)
{
    FMythicaJob* RequestData = Jobs.Find(RequestId);
    if (!RequestData)
    {
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    // Import over any existing mesh for this component
    FString ImportDirectory = FPaths::Combine(Settings->GeneratedAssetImportDirectory, RequestData->ImportPath);
    FString ImportDirectoryParent = FPaths::GetPath(ImportDirectory);
    FString ImportName = FPaths::GetBaseFilename(RequestData->ImportPath);

    // Store unique file in cache for each import to avoid file locking issues
    // Name of file must match the desired import folder name
    FString CacheImportDirectory = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), TEXT("GenerateMeshCache"), RequestData->ImportPath);
    FString UniqueCacheImportDirectory = MakeUniquePath(CacheImportDirectory);
    FString CacheImportFile = FPaths::Combine(UniqueCacheImportDirectory, ImportName + ".usdz");

    // Save package to disk
    bool PackageWritten = FFileHelper::SaveArrayToFile(FileData, *CacheImportFile);
    if (!PackageWritten)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to write mesh file %s"), *CacheImportFile);
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to import result mesh 1"));
        return;
    }

    // Import the mesh
    bool Success = Mythica::ImportMesh(CacheImportFile, ImportDirectoryParent);
    if (!Success)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to import generated mesh"));
        SetJobState(RequestId, EMythicaJobState::Failed, FText::FromString("Failed to import result mesh 2"));
        return;
    }

    RequestData->ImportDirectory = ImportDirectory;
    SetJobState(RequestId, EMythicaJobState::Completed);

    if (MYTHICA_CLEAN_TEMP_FILES)
    {
        IFileManager::Get().Delete(*CacheImportFile);
    }
}

void UMythicaEditorSubsystem::CreateSessionWebSocket()
{
    if (!MYTHICA_ENABLE_WEBSOCKETS)
    {
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/readers/connect"), *Settings->GetWebSocketURL());

    TMap<FString, FString> Headers;
    Headers.Add("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));

    WebSocket = FWebSocketsModule::Get().CreateWebSocket(Url, FString(), Headers);

    WebSocket->OnConnected().AddUObject(this, &UMythicaEditorSubsystem::OnConnected);
    WebSocket->OnConnectionError().AddUObject(this, &UMythicaEditorSubsystem::OnConnectionError);
    WebSocket->OnMessage().AddUObject(this, &UMythicaEditorSubsystem::OnMessage);
    WebSocket->OnClosed().AddUObject(this, &UMythicaEditorSubsystem::OnClosed);
    WebSocket->OnBinaryMessage().AddUObject(this, &UMythicaEditorSubsystem::OnBinaryMessage);

    WebSocket->Connect();
}

void UMythicaEditorSubsystem::DestroySessionWebSocket()
{
    if (!WebSocket)
    {
        return;
    }

    WebSocket->OnConnected().RemoveAll(this);
    WebSocket->OnConnectionError().RemoveAll(this);
    WebSocket->OnMessage().RemoveAll(this);
    WebSocket->OnClosed().RemoveAll(this);
    WebSocket->OnBinaryMessage().RemoveAll(this);

    WebSocket->Close();
    WebSocket.Reset();
}

void UMythicaEditorSubsystem::OnConnected()
{
    UE_LOG(LogMythica, Error, TEXT("WebSocket: Connected"));
}

void UMythicaEditorSubsystem::OnConnectionError(const FString& Error)
{
    UE_LOG(LogMythica, Error, TEXT("WebSocket: Connection Error %s"), *Error);
}

void UMythicaEditorSubsystem::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogMythica, Error, TEXT("WebSocket: Closed %d %s"), StatusCode, *Reason);
}

void UMythicaEditorSubsystem::OnMessage(const FString& Msg)
{
    UE_LOG(LogMythica, Error, TEXT("WebSocket: Message %s"), *Msg);
}

void UMythicaEditorSubsystem::OnBinaryMessage(const void* Data, SIZE_T Length, bool bIsLastFragment)
{
    UE_LOG(LogMythica, Error, TEXT("WebSocket: BinaryMessage %d"), Length);
}

void UMythicaEditorSubsystem::SetSessionState(EMythicaSessionState NewState)
{
    SessionState = NewState;
    OnSessionStateChanged.Broadcast(NewState);
}

void UMythicaEditorSubsystem::LoadInstalledAssetList()
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString ImportFolderRelative = FPackageName::LongPackageNameToFilename(Settings->PackageImportDirectory);
    FString ImportFolderAbsolute = FPaths::ConvertRelativePathToFull(ImportFolderRelative);
    FString MatchPattern = ImportFolderAbsolute + "/*";

    TArray<FString> Directories;
    IFileManager::Get().FindFiles(Directories, *MatchPattern, false, true);
    for (const FString& Directory : Directories)
    {
        FString DirectoryAbsolute = FPaths::Combine(ImportFolderAbsolute, Directory);
        FString FileAbsolute = FPaths::Combine(DirectoryAbsolute, ConfigFile);
        if (!FPaths::FileExists(FileAbsolute))
        {
            UE_LOG(LogMythica, Error, TEXT("Failed to get package info file from %s"), *FileAbsolute);
            continue;
        }

        FString PackageId;
        bool Result = GConfig->GetString(ConfigPackageInfoSection, ConfigPackageIdKey, PackageId, FileAbsolute);
        if (!Result)
        {
            UE_LOG(LogMythica, Error, TEXT("Failed to read PackageId from %s"), *FileAbsolute);
            continue;
        }

        if (InstalledAssets.Contains(PackageId))
        {
            UE_LOG(LogMythica, Error, TEXT("Duplicate package found at %s"), *FileAbsolute);
            continue;
        }

        InstalledAssets.Add(PackageId, DirectoryAbsolute);
    }
}

void UMythicaEditorSubsystem::AddInstalledAsset(const FString& PackageId, const FString& ImportDirectory)
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString FileAbsolute = FPaths::Combine(ImportDirectory, ConfigFile);

    GConfig->SetString(ConfigPackageInfoSection, ConfigPackageIdKey, *PackageId, *FileAbsolute);
    GConfig->Flush(false, *FileAbsolute);

    InstalledAssets.Add(PackageId, ImportDirectory);
}

void UMythicaEditorSubsystem::UpdateStats()
{
    // Get the latest version of each asset
    TMap<FString, FMythicaAsset*> LatestVersions;

    for (FMythicaAsset& Asset : AssetList)
    {
        FMythicaAsset** ExistingAsset = LatestVersions.Find(Asset.AssetId);
        if (!ExistingAsset)
        {
            LatestVersions.Add(Asset.AssetId, &Asset);
        }
        else
        {
            FMythicaAsset* Existing = *ExistingAsset;
            if (Existing->Version < Asset.Version)
            {
                LatestVersions[Asset.AssetId] = &Asset;
            }
        }
    }

    // Accumulate stats
    Stats.TotalPackages = LatestVersions.Num();

    Stats.TotalDigitalAssets = 0;
    for (const TPair<FString, FMythicaAsset*>& Pair : LatestVersions)
    {
        Stats.TotalDigitalAssets += Pair.Value->DigitalAssetCount;
    }
 }

void UMythicaEditorSubsystem::LoadThumbnails()
{
    for (FMythicaAsset& Asset : AssetList)
    {
        if (Asset.ThumbnailURL.IsEmpty() || ThumbnailCache.Contains(Asset.PackageId))
        {
            continue;
        }

        const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

        auto Callback = [this, &Asset](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            OnThumbnailDownloadResponse(Request, Response, bConnectedSuccessfully, Asset.PackageId);
        };

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(Asset.ThumbnailURL);
        Request->SetVerb("GET");
        Request->SetHeader("Content-Type", "application/octet-stream");
        Request->OnProcessRequestComplete().BindLambda(Callback);

        Request->ProcessRequest();
    }
}

void UMythicaEditorSubsystem::OnThumbnailDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageID)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to download thumbnail for package %s"), *PackageID);
        return;
    }

    TArray<uint8> PackageData = Response->GetContent();

    FImage Image;
    bool Decompressed = FImageUtils::DecompressImage(PackageData.GetData(), PackageData.Num(), Image);
    if (!Decompressed)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to decompress image for thumbnail for package  %s"), *PackageID);
        return;
    }

    ERawImageFormat::Type NewFormat;
    EPixelFormat PixelFormat = FImageCoreUtils::GetPixelFormatForRawImageFormat(Image.Format, &NewFormat);
    if (Image.Format != NewFormat)
    {
        Image.ChangeFormat(NewFormat, ERawImageFormat::GetDefaultGammaSpace(NewFormat));
        PixelFormat = FImageCoreUtils::GetPixelFormatForRawImageFormat(Image.Format);
    }

    const int32 Width = Image.GetWidth();
    const int32 Height = Image.GetHeight();
    UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PixelFormat, NAME_None, Image.RawData);
    if (!NewTexture)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to create thumbnail texture for package package %s"), *PackageID);
        return;
    }

    ThumbnailCache.Add(PackageID, NewTexture);

    OnThumbnailLoaded.Broadcast(PackageID);
}

FMythicaAsset* UMythicaEditorSubsystem::FindAsset(const FString& PackageId)
{
    return AssetList.FindByPredicate([PackageId](const FMythicaAsset& InAsset) { return InAsset.PackageId == PackageId; });
}
