#include "MythicaEditorSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "FileUtilities/ZipArchiveReader.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "MythicaDeveloperSettings.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY(LogMythica);

const TCHAR* ConfigFile = TEXT("PackageInfo.ini");
const TCHAR* ConfigPackageInfoSection = TEXT("PackageInfo");
const TCHAR* ConfigPackageIdKey = TEXT("PackageId");

void UMythicaEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LoadInstalledAssetList();
}

void UMythicaEditorSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

bool UMythicaEditorSubsystem::IsAuthenticated()
{
    return !AuthToken.IsEmpty();
}

TArray<FMythicaAsset> UMythicaEditorSubsystem::GetAssetList()
{
    return AssetList;
}

bool UMythicaEditorSubsystem::IsAssetInstalled(const FString& PackageId)
{
    return InstalledAssets.Contains(PackageId);
}

void UMythicaEditorSubsystem::CreateSession()
{
    if (IsAuthenticated())
    {
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();
    if (Settings->ProfileId.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("Profile ID is empty"));
        return;
    }
    if (Settings->ServerHost.IsEmpty())
    {
        UE_LOG(LogMythica, Error, TEXT("ServerHost is empty"));
        return;
    }

    FString Url = FString::Printf(TEXT("http://%s:%d/api/v1/profiles/start_session/%s"), *Settings->ServerHost, Settings->ServerPort, *Settings->ProfileId);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");
    Request->OnProcessRequestComplete().BindUObject(this, &UMythicaEditorSubsystem::OnCreateSessionResponse);

    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnCreateSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to create session"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to parse JSON string"));
        return;
    }

    FString Token;
    if (!JsonObject->TryGetStringField(TEXT("token"), Token))
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to get token from JSON string"));
    }

    AuthToken = Token;

    OnSessionCreated.Broadcast();
}

void UMythicaEditorSubsystem::UpdateAssetList()
{
    if (!IsAuthenticated())
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

        FString Name = JsonObject->GetStringField(TEXT("name"));
        FString Description = JsonObject->GetStringField(TEXT("description"));

        AssetList.Push({ Name, Description });
    }

    AssetList.Push({ "921f6530-da50-469a-90e7-983332ac6a0c", "Mythica Flora", "Library of plants and trees." });
    AssetList.Push({ "b249f5ee-5edf-4377-a5a3-6bf244915b97", "Stair Tool", "Converts geometry into stairs." });

    OnAssetListUpdated.Broadcast();
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

FMythicaAsset* UMythicaEditorSubsystem::FindAsset(const FString& PackageId)
{
    return AssetList.FindByPredicate([PackageId](const FMythicaAsset& InAsset) { return InAsset.PackageId == PackageId; });
}
