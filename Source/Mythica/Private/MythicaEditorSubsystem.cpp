#include "MythicaEditorSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "FileUtilities/ZipArchiveReader.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "ImageCoreUtils.h"
#include "ImageUtils.h"
#include "MythicaDeveloperSettings.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY(LogMythica);

const TCHAR* ConfigFile = TEXT("PackageInfo.ini");
const TCHAR* ConfigPackageInfoSection = TEXT("PackageInfo");
const TCHAR* ConfigPackageIdKey = TEXT("PackageId");

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

TArray<FMythicaAsset> UMythicaEditorSubsystem::GetAssetList()
{
    return AssetList;
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
    if (Settings->ServerHost.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("ServerHost is empty"));
        SetSessionState(EMythicaSessionState::SessionFailed);
        return;
    }

    FString Url = FString::Printf(TEXT("http://%s:%d/api/v1/profiles/start_session/%s"), *Settings->ServerHost, Settings->ServerPort, *Settings->ProfileId);

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
        UE_LOG(LogMythica, Error, TEXT("Failed to parse JSON string"));
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
    if (SessionState != EMythicaSessionState::SessionCreated)
    {
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("http://%s:%d/api/v1/assets/all"), *Settings->ServerHost, Settings->ServerPort);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
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
        UE_LOG(LogMythica, Error, TEXT("Failed to parse JSON string"));
        return;
    }

    if (JsonValue->Type != EJson::Array)
    {
        UE_LOG(LogMythica, Error, TEXT("JSON value is not an array"));
        return;
    }

    AssetList.Reset();

    TArray<TSharedPtr<FJsonValue>> Array = JsonValue->AsArray();
    for (TSharedPtr<FJsonValue> Value : Array)
    {
        TSharedPtr<FJsonObject> JsonObject = Value->AsObject();
        if (!JsonObject.IsValid())
        {
            continue;
        }

        FString PackageId = JsonObject->GetStringField(TEXT("asset_id"));
        FString Name = JsonObject->GetStringField(TEXT("name"));
        FString Description = JsonObject->GetStringField(TEXT("description"));

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

        FString ThumbnailFileId;
        TArray<TSharedPtr<FJsonValue>> ThumbnailObject = ContentsObject->GetArrayField(TEXT("thumbnails"));
        if (ThumbnailObject.Num() > 0)
        {
            ThumbnailFileId = ThumbnailObject[0]->AsObject()->GetStringField(TEXT("file_id"));
        }

        AssetList.Push({ PackageId, Name, Description, AssetVersion, ThumbnailFileId });
    }

    AssetList.Sort([](const FMythicaAsset& a, const FMythicaAsset& b)
    {
        int32 compare = a.Name.Compare(b.Name);
        return compare < 0 || compare == 0 && b.Version < a.Version;
    });

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
#if 1
    // Work around not having automated zip file generation
    DownloadId = "4a1e841d-29e5-4b30-9af7-de577479a438";
#endif

    FString Url = FString::Printf(TEXT("http://%s:%d/api/v1/download/info/%s"), *Settings->ServerHost, Settings->ServerPort, *DownloadId);

    auto Callback = [this, PackageId](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnDownloadInfoResponse(Request, Response, bConnectedSuccessfully, PackageId);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
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
    TArray<FString> HDAFilePaths;

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

        if (FPaths::GetExtension(FilePath) == TEXT("hda"))
        {
            HDAFilePaths.Add(FilePath);
        }
    }

    // Import HDA files into Unreal
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
    ImportData->bReplaceExisting = true;
    ImportData->DestinationPath = FPaths::Combine(Settings->ImportDirectory, ObjectTools::SanitizeObjectPath(Asset->Name));
    ImportData->Filenames = HDAFilePaths;

    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<UObject*> ImportedObject = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
    if (ImportedObject.Num() != HDAFilePaths.Num())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to import HDA from package %s"), *PackageId);
        return;
    }

    for (UObject* Object : ImportedObject)
    {
        UPackage* Package = Cast<UPackage>(Object->GetOuter());
        FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;

        UPackage::SavePackage(Package, nullptr, *Filename, SaveArgs);
    }

    AddInstalledAsset(PackageId, ImportData->DestinationPath);

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

    FString DirectoryRelative = FPackageName::LongPackageNameToFilename(ImportDirectory);
    FString DirectoryAbsolute = FPaths::ConvertRelativePathToFull(DirectoryRelative);
    FString FileAbsolute = FPaths::Combine(DirectoryAbsolute, ConfigFile);

    GConfig->SetString(ConfigPackageInfoSection, ConfigPackageIdKey, *PackageId, *FileAbsolute);
    GConfig->Flush(false, *FileAbsolute);

    InstalledAssets.Add(PackageId, DirectoryAbsolute);
}

void UMythicaEditorSubsystem::LoadThumbnails()
{
    for (FMythicaAsset& Asset : AssetList)
    {
        if (Asset.ThumbnailFileId.IsEmpty() || ThumbnailCache.Contains(Asset.PackageId))
        {
            continue;
        }

        const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

        FString Url = FString::Printf(TEXT("http://%s:%d/api/v1/download/info/%s"), *Settings->ServerHost, Settings->ServerPort, *Asset.ThumbnailFileId);

        auto Callback = [this, &Asset](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            OnThumbnailDownloadInfoResponse(Request, Response, bConnectedSuccessfully, Asset.PackageId, Asset.ThumbnailFileId);
        };

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->SetURL(Url);
        Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
        Request->SetVerb("GET");
        Request->SetHeader("Content-Type", "application/octet-stream");
        Request->OnProcessRequestComplete().BindLambda(Callback);

        Request->ProcessRequest();
    }
}

void UMythicaEditorSubsystem::OnThumbnailDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageID, const FString& ThumbnailFileID)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get download info for thumbnail %s"), *ThumbnailFileID);
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
        UE_LOG(LogMythica, Error, TEXT("Failed to get download URL for file %s"), *ThumbnailFileID);
        return;
    }

    auto Callback = [this, PackageID, ThumbnailFileID](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        OnThumbnailDownloadResponse(Request, Response, bConnectedSuccessfully, PackageID, ThumbnailFileID);
    };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> DownloadRequest = FHttpModule::Get().CreateRequest();
    DownloadRequest->SetURL(DownloadURL);
    DownloadRequest->SetVerb("GET");
    DownloadRequest->SetHeader("Content-Type", *ContentType);
    DownloadRequest->OnProcessRequestComplete().BindLambda(Callback);

    DownloadRequest->ProcessRequest();
}

void UMythicaEditorSubsystem::OnThumbnailDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageID, const FString& ThumbnailFileID)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to download thumbnail %s"), *ThumbnailFileID);
        return;
    }

    TArray<uint8> PackageData = Response->GetContent();

    FImage Image;
    bool Decompressed = FImageUtils::DecompressImage(PackageData.GetData(), PackageData.Num(), Image);
    if (!Decompressed)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to decompress image for thumbnail %s"), *ThumbnailFileID);
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
        UE_LOG(LogMythica, Error, TEXT("Failed to create texture for thumbnail %s"), *ThumbnailFileID);
        return;
    }

    ThumbnailCache.Add(PackageID, NewTexture);

    OnThumbnailLoaded.Broadcast(PackageID);
}

FMythicaAsset* UMythicaEditorSubsystem::FindAsset(const FString& PackageId)
{
    return AssetList.FindByPredicate([PackageId](const FMythicaAsset& InAsset) { return InAsset.PackageId == PackageId; });
}
