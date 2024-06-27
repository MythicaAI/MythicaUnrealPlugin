#include "MythicaEditorSubsystem.h"

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

    AssetList.Push({ "Mythica Flora", "Library of plants and trees." });
    AssetList.Push({ "Stair Tool", "Converts geometry into stairs." });

    OnAssetListUpdated.Broadcast();
}

void UMythicaEditorSubsystem::InstallAsset(const FString& Name)
{
    FMythicaAsset* Asset = AssetList.FindByPredicate([Name](const FMythicaAsset& InAsset) { return InAsset.Name == Name; });
    if (!Asset)
    {
        UE_LOG(LogMythica, Error, TEXT("Unknown asset type %s"), *Name);
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("http://%s:%d/api/v1/asset/zip/%s"), *Settings->ServerHost, Settings->ServerPort, *Name);

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
    FString Name = FPaths::GetBaseFilename(Request->GetURL());
    FString PackagePath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), Name + ".zip");

    bool FileWritten = FFileHelper::SaveArrayToFile(PackageData, *PackagePath);
    if (!FileWritten)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to write file %s"), *PackagePath);
        return;
    }

    // Unzip package
#if 0
    // TODO: Link in a zip library
#else
    FString TestHDA = "D:/TestHDA.hda";

    TArray<uint8> HDAData;
    bool HDALoaded = FFileHelper::LoadFileToArray(HDAData, *TestHDA);
    if (!HDALoaded)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to load test package %s"), *TestPackage);
        return;
    }

    FString HDAName = FPaths::GetBaseFilename(TestHDA);
    FString HDAPath = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("MythicaCache"), HDAName + ".hda");

    bool HDAWritten = FFileHelper::SaveArrayToFile(HDAData, *HDAPath);
    if (!HDAWritten)
    {
        UE_LOG(LogMythica, Error, TEXT("Failed to write HDA file %s"), *HDAPath);
        return;
    }
#endif

    // Import HDA files into Unreal

}
