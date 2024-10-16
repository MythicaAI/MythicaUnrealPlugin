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
#include "MythicaDeveloperSettings.h"
#include "MythicaUSDUtil.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"

#define MYTHICA_CLEAN_TEMP_FILES 1

DEFINE_LOG_CATEGORY(LogMythica);

const TCHAR* ConfigFile = TEXT("PackageInfo.ini");
const TCHAR* ConfigPackageInfoSection = TEXT("PackageInfo");
const TCHAR* ConfigPackageIdKey = TEXT("PackageId");

const TCHAR* ImportableFileExtensions[] = 
{ 
    TEXT("hda"), TEXT("hdalc"), TEXT("hdanc"),
    TEXT("otl"), TEXT("otllc"), TEXT("otlnc")
};

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

void UMythicaEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    LevelEditorModule.OnMapChanged().AddUObject(this, &UMythicaEditorSubsystem::OnMapChanged);

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
}

void UMythicaEditorSubsystem::OnMapChanged(UWorld* InWorld, EMapChangeType InMapChangeType)
{
    if (InMapChangeType == EMapChangeType::TearDownWorld)
    {
        ClearJobs();
    }
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

bool UMythicaEditorSubsystem::IsAssetInstalled(const FString& PackageId)
{
    return InstalledAssets.Contains(PackageId);
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
    if (Settings->ProfileId.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("Profile ID is empty"));
        SetSessionState(EMythicaSessionState::SessionFailed);
        return;
    }
    if (Settings->ServiceURL.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("ServiceURL is empty"));
        SetSessionState(EMythicaSessionState::SessionFailed);
        return;
    }

    FString Url = FString::Printf(TEXT("%s/v1/sessions/direct/%s"), *Settings->ServiceURL, *Settings->ProfileId);

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

    UpdateJobDefinitionList();
}

void UMythicaEditorSubsystem::UpdateAssetList()
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/assets/top"), *Settings->ServiceURL);

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
                    ThumbnailURL = FString::Printf(TEXT("%s/%s.%s"), *Settings->ImagesURL, *ContentHash, *FileExtension);
                    break;
                }
            }
        }

        AssetList.Push({ AssetId, PackageId, Name, Description, OrgName, AssetVersion, {}, ThumbnailURL, DigitalAssetCount });
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

    FString Url = FString::Printf(TEXT("%s/v1/download/info/%s"), *Settings->ServiceURL, *DownloadId);

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

    FString ImportPackagePath = FPaths::Combine(Settings->ImportDirectory, ObjectTools::SanitizeObjectName(Asset->Name));
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

void UMythicaEditorSubsystem::UpdateJobDefinitionList()
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/jobs/definitions"), *Settings->ServiceURL);

    auto Callback = [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnJobDefinitionsResponse(Request, Response, bConnectedSuccessfully);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("Get");
    Request->OnProcessRequestComplete().BindLambda(Callback);

    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnJobDefinitionsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get job definitions"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonValue> JsonValue;
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse job definitions JSON string"));
        return;
    }

    if (JsonValue->Type != EJson::Array)
    {
        UE_LOG(LogMythica, Error, TEXT("JSON value is not an array"));
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    JobDefinitionList.Reset();

    const TArray<TSharedPtr<FJsonValue>>& Array = JsonValue->AsArray();
    for (TSharedPtr<FJsonValue> Value : Array)
    {
        TSharedPtr<FJsonObject> JsonObject = Value->AsObject();
        if (!JsonObject.IsValid())
        {
            continue;
        }

        FString JobDefId = JsonObject->GetStringField(TEXT("job_def_id"));
        FString JobType = JsonObject->GetStringField(TEXT("job_type"));
        FString Name = JsonObject->GetStringField(TEXT("name"));
        FString Description = JsonObject->GetStringField(TEXT("description"));

        TSharedPtr<FJsonObject> ParamsSchema = JsonObject->GetObjectField(TEXT("params_schema"));
        TSharedPtr<FJsonObject> ParamsSet = ParamsSchema->GetObjectField(TEXT("params"));

        FMythicaInputs Inputs;
        FMythicaParameters Params;
        Mythica::ReadParameters(ParamsSet, Inputs, Params);

        JobDefinitionList.Push({ JobDefId, JobType, Name, Description, Inputs, Params });
    }

    OnJobDefinitionListUpdated.Broadcast();
}

bool UMythicaEditorSubsystem::PrepareInputFiles(const FMythicaInputs& Inputs, TMap<int, FString>& InputFiles, FString& ExportDirectory, const FVector& Origin)
{
    FString DesiredDirectory = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), TEXT("ExportCache"), TEXT("Export"));
    ExportDirectory = MakeUniquePath(DesiredDirectory);

    for (int i = 0; i < Inputs.Inputs.Num(); i++)
    {
        const FMythicaInput& Input = Inputs.Inputs[i];
        if (Input.Type == EMythicaInputType::Mesh && Input.Mesh != nullptr)
        {
            FString FilePath = FPaths::Combine(ExportDirectory, FString::Format(TEXT("Input{0}"), { i }), "Mesh.usdz");
            bool Success = Mythica::ExportMesh(Input.Mesh, FilePath);
            if (!Success)
            {
                UE_LOG(LogMythica, Error, TEXT("Failed to export mesh %s"), *Input.Mesh->GetName());
                return false;
            }

            InputFiles.Add(i, FilePath);
        }
        else if (Input.Type == EMythicaInputType::World && !Input.Actors.IsEmpty())
        {
            FString FilePath = FPaths::Combine(ExportDirectory, FString::Format(TEXT("Input{0}"), { i }), "Mesh.usdz");
            bool Success = Mythica::ExportActors(Input.Actors, FilePath, Origin);
            if (!Success)
            {
                UE_LOG(LogMythica, Error, TEXT("Failed to export actors"));
                return false;
            }

            InputFiles.Add(i, FilePath);
        }
        else if (Input.Type == EMythicaInputType::Spline && Input.SplineActor != nullptr)
        {
            FString FilePath = FPaths::Combine(ExportDirectory, FString::Format(TEXT("Input{0}"), { i }), "Mesh.usdz");
            bool Success = Mythica::ExportSpline(Input.SplineActor, FilePath, Origin);
            if (!Success)
            {
                UE_LOG(LogMythica, Error, TEXT("Failed to export actors"));
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

    FString Url = FString::Printf(TEXT("%s/v1/upload/store"), *Settings->ServiceURL);

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
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse upload inputs JSON string"));
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    TArray<TSharedPtr<FJsonValue>> Files = JsonObject->GetArrayField(TEXT("files"));
    if (Files.Num() != InputFiles.Num())
    {
        UE_LOG(LogMythica, Error, TEXT("Unexpected number of uploaded files"));
        SetJobState(RequestId, EMythicaJobState::Failed);
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
    const FMythicaInputs& Inputs, 
    const FMythicaParameters& Params, 
    const FString& ImportName, 
    const FVector& Origin
) {
    if (SessionState != EMythicaSessionState::SessionCreated)
    {
        UE_LOG(LogMythica, Error, TEXT("Unable to create job due to session not created"));
        return -1;
    }

    FString ExportDirectory;
    TMap<int, FString> InputFiles;
    bool Success = PrepareInputFiles(Inputs, InputFiles, ExportDirectory, Origin);
    if (!Success)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to prepare job input files"));
        return -1;
    }

    int RequestId = CreateJob(JobDefId, Inputs, Params, ImportName);

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
    TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject);
    Mythica::WriteParameters(RequestData->Inputs, RequestData->InputFileIds, RequestData->Params, ParamsObject);
    ParamsSetObject->SetObjectField(TEXT("params"), ParamsObject);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("job_def_id"), RequestData->JobDefId);
    JsonObject->SetObjectField(TEXT("params"), ParamsSetObject);

    FString ContentJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentJson);
    bool Success = FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    check(Success);

    // Send request
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/jobs/"), *Settings->ServiceURL);

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
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse create job JSON string"));
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    FString JobId;
    if (!JsonObject->TryGetStringField(TEXT("job_id"), JobId))
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get JobId from JSON string"));
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    RequestData->JobId = JobId;
    SetJobState(RequestId, EMythicaJobState::Queued);
}

int UMythicaEditorSubsystem::CreateJob(const FString& JobDefId, const FMythicaInputs& Inputs, const FMythicaParameters& Params, const FString& ImportName)
{
    int RequestId = NextRequestId++;
    Jobs.Add(RequestId, { JobDefId, Inputs, {}, Params, ImportName });

    if (!JobPollTimer.IsValid())
    {
        FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UMythicaEditorSubsystem::PollJobStatus);
        GEditor->GetTimerManager()->SetTimer(JobPollTimer, TimerDelegate, 1.0f, true);
    }

    return RequestId;
}

void UMythicaEditorSubsystem::SetJobState(int RequestId, EMythicaJobState State)
{
    FMythicaJob* JobData = Jobs.Find(RequestId);
    if (!JobData || JobData->State == State)
    {
        return;
    }

    JobData->State = State;
    OnJobStateChange.Broadcast(RequestId, State);

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
    for (const TTuple<int, FMythicaJob>& JobEntry : Jobs)
    {
        int RequestId = JobEntry.Key;
        const FMythicaJob& JobData = JobEntry.Value;

        if (JobData.State != EMythicaJobState::Queued && JobData.State != EMythicaJobState::Processing)
        {
            continue;
        }

        const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

        FString Url = FString::Printf(TEXT("%s/v1/jobs/results/%s"), *Settings->ServiceURL, *JobData.JobId);

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

    FString FileId;

    TArray<TSharedPtr<FJsonValue>> Results = JsonObject->GetArrayField(TEXT("results"));
    for (TSharedPtr<FJsonValue> Value : Results)
    {
        TSharedPtr<FJsonObject> ResultObject = Value->AsObject();
        TSharedPtr<FJsonObject> ResultDataObject = ResultObject->GetObjectField(TEXT("result_data"));
        
        if (ResultDataObject->TryGetStringField(TEXT("file_id"), FileId))
        {
            break;
        }
    }

    if (FileId.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("Job failed %d"), RequestId);
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;

    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/download/info/%s"), *Settings->ServiceURL, *FileId);

    auto Callback = [this, FileId, RequestId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
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

void UMythicaEditorSubsystem::OnMeshDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get mesh download info"));
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse mesh download info JSON string"));
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    FString DownloadURL = JsonObject->GetStringField(TEXT("url"));
    FString ContentType = JsonObject->GetStringField(TEXT("content_type"));
    if (DownloadURL.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get download URL for mesh"));
        SetJobState(RequestId, EMythicaJobState::Failed);
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
    FMythicaJob* RequestData = Jobs.Find(RequestId);
    if (!RequestData)
    {
        return;
    }

    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to download asset"));
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    // Generate a unique import directory
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString JobName = FPaths::GetBaseFilename(ObjectTools::SanitizeObjectName(RequestData->ImportName));
    FString ImportDirectory = FPaths::Combine(Settings->ImportDirectory, TEXT("GeneratedMeshes"), JobName);

    FString DirectoryPackageName = FPaths::Combine(ImportDirectory, TEXT("Run"));
    FString DirectoryRelative = FPackageName::LongPackageNameToFilename(DirectoryPackageName);
    FString DirectoryAbsolute = FPaths::ConvertRelativePathToFull(DirectoryRelative);
    FString UniqueDirectoryAbsolute = MakeUniquePath(DirectoryAbsolute);
    FString UniqueDirectoryName = FPaths::GetBaseFilename(UniqueDirectoryAbsolute);

    // Save package to disk
    FString FilePath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), TEXT("GenerateMeshCache"), UniqueDirectoryName + ".usdz");

    TArray<uint8> PackageData = Response->GetContent();
    bool PackageWritten = FFileHelper::SaveArrayToFile(PackageData, *FilePath);
    if (!PackageWritten)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to write mesh file %s"), *FilePath);
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    // Import the mesh
    UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
    ImportData->bReplaceExisting = true;
    ImportData->DestinationPath = ImportDirectory;
    ImportData->Filenames = { FilePath };

    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<UObject*> Objects = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
    if (Objects.Num() != 1)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to import generated mesh"));
        SetJobState(RequestId, EMythicaJobState::Failed);
        return;
    }

    // Save the imported assets
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FString CreatedImportDirectory = FPaths::Combine(ImportDirectory, UniqueDirectoryName);

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssetsByPath(*CreatedImportDirectory, Assets, true, false);

    TArray<UPackage*> Packages;
    for (const FAssetData& Asset : Assets)
    {
        Packages.Add(Asset.GetPackage());
    }

    bool PrevSilent = GIsSilent;
    GIsSilent = true;
    UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
    FTimerHandle DummyHandle;
    GEditor->GetTimerManager()->SetTimer(DummyHandle, [=]() { GIsSilent = PrevSilent; }, 0.1f, false); // Delay until after asset validation

    RequestData->ImportDirectory = CreatedImportDirectory;
    SetJobState(RequestId, EMythicaJobState::Completed);

    if (MYTHICA_CLEAN_TEMP_FILES)
    {
        IFileManager::Get().Delete(*FilePath);
    }
}

void UMythicaEditorSubsystem::SetSessionState(EMythicaSessionState NewState)
{
    SessionState = NewState;
    OnSessionStateChanged.Broadcast(NewState);
}

void UMythicaEditorSubsystem::LoadInstalledAssetList()
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString ImportFolderRelative = FPackageName::LongPackageNameToFilename(Settings->ImportDirectory);
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
