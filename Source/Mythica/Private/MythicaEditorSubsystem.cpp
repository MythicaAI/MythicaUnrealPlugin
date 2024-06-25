#include "MythicaEditorSubsystem.h"

#include "HttpModule.h"
#include "MythicaDeveloperSettings.h"

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

void UMythicaEditorSubsystem::CreateSession()
{
    if (IsAuthenticated())
    {
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();
    if (Settings->ProfileId.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Profile ID is empty"));
        return;
    }
    if (Settings->ServerHost.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("ServerHost is empty"));
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
        UE_LOG(LogTemp, Error, TEXT("Failed to create session"));
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonObject> JsonObject;
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON string"));
        return;
    }

    FString Token;
    if (!JsonObject->TryGetStringField(TEXT("token"), Token))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get token from JSON string"));
    }

    AuthToken = Token;
}