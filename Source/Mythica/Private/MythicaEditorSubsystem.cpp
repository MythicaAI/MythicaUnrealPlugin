#include "MythicaEditorSubsystem.h"

#include "AssetToolsModule.h"
#include "FileUtilities/ZipArchiveReader.h"
#include "HttpModule.h"
#include "MythicaDeveloperSettings.h"

DEFINE_LOG_CATEGORY(LogMythica);

void UMythicaEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
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

    FMythicaAsset* Asset = AssetList.FindByPredicate([PackageId](const FMythicaAsset& InAsset) { return InAsset.PackageId == PackageId; });
    if (!Asset)
    {
        UE_LOG(LogMythica, Error, TEXT("Unknown asset type %s"), *PackageId);
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("http://%s:%d/api/v1/asset/zip/%s"), *Settings->ServerHost, Settings->ServerPort, *PackageId);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/octet-stream");
    Request->OnProcessRequestComplete().BindUObject(this, &UMythicaEditorSubsystem::OnDownloadAssetResponse);

#if 0
    Request->ProcessRequest();
#else
    OnDownloadAssetResponse(Request, nullptr, true);
#endif
}

void UMythicaEditorSubsystem::OnDownloadAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
#if 0
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to download asset"));
        return;
    }

    TArray<uint8> PackageData = Response->GetContent();

#else
    FString TestPackage = "D:/TestPackage.zip";

    TArray<uint8> PackageData;
    bool PackageLoaded = FFileHelper::LoadFileToArray(PackageData, *TestPackage);
    if (!PackageLoaded)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to load test package %s"), *TestPackage);
        return;
    }
#endif

    // Save package to disk
    FString PackageId = FPaths::GetBaseFilename(Request->GetURL());
    FString PackagePath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), PackageId, PackageId + ".zip");

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
    
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

    TArray<UObject*> ImportedObject = AssetToolsModule.Get().ImportAssets({ HDAFilePaths }, Settings->ImportDirectory);
    if (ImportedObject.Num() != HDAFilePaths.Num())
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to import HDA from package %s"), *PackageId);
        return;
    }

    InstalledAssets.Add(PackageId);

    OnAssetInstalled.Broadcast(PackageId);
}
