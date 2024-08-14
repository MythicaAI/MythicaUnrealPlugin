#include "MythicaEditorSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "FileUtilities/ZipArchiveReader.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "IImageWrapperModule.h"
#include "ImageCoreUtils.h"
#include "ImageUtils.h"
#include "MythicaDeveloperSettings.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"

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

bool FMythicaAssetVersion::operator<(const FMythicaAssetVersion& Other) const
{
    return Major < Other.Major
        || (Major == Other.Major && Minor < Other.Minor)
        || (Major == Other.Major && Minor == Other.Minor && Patch < Other.Patch);
}

void UMythicaEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LoadInstalledAssetList();
}

void UMythicaEditorSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

EMythicaSessionState UMythicaEditorSubsystem::GetSessionState()
{
    return SessionState;
}

bool UMythicaEditorSubsystem::CanInstallAssets()
{
    return FModuleManager::Get().ModuleExists(TEXT("HoudiniEngine"));
}

TArray<FMythicaAsset> UMythicaEditorSubsystem::GetAssetList()
{
    return AssetList;
}

TArray<FMythicaTool> UMythicaEditorSubsystem::GetToolList()
{
    return ToolList;
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

bool UMythicaEditorSubsystem::IsToolInterfaceLoaded(const FString& FileId)
{
    return true;
}

FMythicaParameters UMythicaEditorSubsystem::GetToolInterface(const FString& FileId)
{
    return FMythicaParameters();
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

    FString Url = FString::Printf(TEXT("%s/v1/profiles/start_session/%s"), *Settings->ServiceURL, *Settings->ProfileId);

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
    ToolList.Reset();

    TArray<TSharedPtr<FJsonValue>> Array = JsonValue->AsArray();
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

                    FString FileId = FileObject->GetStringField(TEXT("file_id"));
                    ToolList.Add({ FileId, FileName });
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
    FString PackagePath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), PackageId, PackageId + ".zip");

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

        FString FilePath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), PackageId, File);
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

    // Import HDA files into Unreal
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

    TArray<UObject*> ImportedObjects;
    FString BaseImportDirectory = GetUniqueImportDirectory(ObjectTools::SanitizeObjectName(Asset->Name));
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

void UMythicaEditorSubsystem::LoadToolInterface(const FString& FileId)
{

}

void UMythicaEditorSubsystem::GenerateMesh(const FString& FileId, const FMythicaParameters& Params, const FString& ImportName)
{
    if (SessionState != EMythicaSessionState::SessionCreated)
    {
        return;
    }

    // Create JSON payload
    TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject);
    for (const FMythicaParameter& Param : Params.Parameters)
    {
        ParamsObject->SetStringField(Param.Name, FString::SanitizeFloat(Param.Value));
    }

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("file_id"), FileId);
    JsonObject->SetObjectField(TEXT("params"), ParamsObject);

    FString ContentJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentJson);
    if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to create generate mesh JSON request object"));
        return;
    }

    // Send request
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/jobs/generate-mesh"), *Settings->ServiceURL);

    auto Callback = [this, FileId, ImportName](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnGenerateMeshResponse(Request, Response, bConnectedSuccessfully, FileId, ImportName);
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

void UMythicaEditorSubsystem::OnGenerateMeshResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileId, const FString& ImportName)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to request asset generation for file %s"), *FileId);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse create generate mesh JSON string"));
        return;
    }

    FString RequestId;
    if (!JsonObject->TryGetStringField(TEXT("event_id"), RequestId))
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get RequestId from JSON string"));
        return;
    }

    GenerateMeshRequests.Add(RequestId, ImportName);

    if (!GenerateMeshTimer.IsValid())
    {
        FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UMythicaEditorSubsystem::PollGenerateMeshStatus);
        GEditor->GetTimerManager()->SetTimer(GenerateMeshTimer, TimerDelegate, 1.0f, true);
    }
}

void UMythicaEditorSubsystem::PollGenerateMeshStatus()
{
    for (const TTuple<FString, FString>& RequestEntry : GenerateMeshRequests)
    {
        const FString& RequestId = RequestEntry.Key;
        const FString& ImportName = RequestEntry.Value;

        const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

        FString Url = FString::Printf(TEXT("%s/v1/jobs/generate-mesh/status/%s"), *Settings->ServiceURL, *RequestId);

        auto Callback = [this, RequestId, ImportName](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            OnGenerateMeshStatusResponse(Request, Response, bConnectedSuccessfully, RequestId, ImportName);
        };

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetVerb("GET");
        Request->SetHeader("Content-Type", "application/json");
        Request->OnProcessRequestComplete().BindLambda(Callback);

        Request->ProcessRequest();
    }
}

void UMythicaEditorSubsystem::OnGenerateMeshStatusResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& RequestId, const FString& ImportName)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get generate mesh status for request %s"), *RequestId);
        return;
    }

    if (!GenerateMeshRequests.Contains(RequestId))
    {
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse generate mesh status JSON string"));
        return;
    }

    FString FileId;
    if (!JsonObject->TryGetStringField(TEXT("file_id"), FileId))
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get FileId from JSON string"));
        return;
    }

    // Job has not been completed yet
    if (FileId.IsEmpty())
    {
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/download/info/%s"), *Settings->ServiceURL, *FileId);

    auto Callback = [this, FileId, ImportName](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnMeshDownloadInfoResponse(Request, Response, bConnectedSuccessfully, FileId, ImportName);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> DownloadInfoRequest = FHttpModule::Get().CreateRequest();
    DownloadInfoRequest->SetURL(Url);
    DownloadInfoRequest->SetVerb("GET");
    DownloadInfoRequest->SetHeader("Content-Type", "application/octet-stream");
    DownloadInfoRequest->OnProcessRequestComplete().BindLambda(Callback);

    DownloadInfoRequest->ProcessRequest();

    GenerateMeshRequests.Remove(RequestId);
    if (GenerateMeshRequests.Num() == 0)
    {
        GEditor->GetTimerManager()->ClearTimer(GenerateMeshTimer);
    }
}

void UMythicaEditorSubsystem::OnMeshDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileId, const FString& ImportName)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get mesh download info for file %s"), *FileId);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse mesh download info JSON string"));
        return;
    }

    FString DownloadURL = JsonObject->GetStringField(TEXT("url"));
    FString ContentType = JsonObject->GetStringField(TEXT("content_type"));
    if (DownloadURL.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get download URL for mesh %s"), *FileId);
        return;
    }

    auto Callback = [this, FileId, ImportName](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnMeshDownloadResponse(Request, Response, bConnectedSuccessfully, FileId, ImportName);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> DownloadRequest = FHttpModule::Get().CreateRequest();
    DownloadRequest->SetURL(DownloadURL);
    DownloadRequest->SetVerb("GET");
    DownloadRequest->SetHeader("Content-Type", *ContentType);
    DownloadRequest->OnProcessRequestComplete().BindLambda(Callback);

    DownloadRequest->ProcessRequest();
}

void UMythicaEditorSubsystem::OnMeshDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileId, const FString& ImportName)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to download asset"));
        return;
    }

    // Save package to disk
    FString FilePath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), TEXT("GenerateMeshCache"), ImportName + ".fbx");

    TArray<uint8> PackageData = Response->GetContent();
    bool PackageWritten = FFileHelper::SaveArrayToFile(PackageData, *FilePath);
    if (!PackageWritten)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to write mesh file %s"), *FilePath);
        return;
    }

    // Import the mesh
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString DirectoryRelative = FPackageName::LongPackageNameToFilename(Settings->ImportDirectory);
    FString DirectoryAbsolute = FPaths::ConvertRelativePathToFull(DirectoryRelative);

    UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
    ImportData->bReplaceExisting = true;
    ImportData->DestinationPath = DirectoryAbsolute;
    ImportData->Filenames = { FilePath };

    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<UObject*> Objects = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
    if (Objects.Num() != 1)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to import generated mesh for file %s"), *FileId);
        return;
    }

    // Save the imported asset
    UPackage* Package = Cast<UPackage>(Objects[0]->GetOuter());
    FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
    UPackage::SavePackage(Package, nullptr, *Filename, SaveArgs);
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

FString UMythicaEditorSubsystem::GetUniqueImportDirectory(const FString& PackageName)
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString PackagePath = FPaths::Combine(Settings->ImportDirectory, PackageName);
    FString DirectoryRelative = FPackageName::LongPackageNameToFilename(PackagePath);
    FString DirectoryAbsolute = FPaths::ConvertRelativePathToFull(DirectoryRelative);

    FString UniqueDirectory = DirectoryAbsolute;
    uint32 Counter = 1;
    while (IFileManager::Get().DirectoryExists(*UniqueDirectory))
    {
        UniqueDirectory = FString::Printf(TEXT("%s_%d"), *DirectoryAbsolute, Counter);
        Counter++;
    }

    return UniqueDirectory;
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
