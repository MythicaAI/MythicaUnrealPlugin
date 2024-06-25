#include "MythicaEditorSubsystem.h"

#include "HttpModule.h"

void UMythicaEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMythicaEditorSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UMythicaEditorSubsystem::RequestFact()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL("https://catfact.ninja/fact");
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");
    Request->OnProcessRequestComplete().BindUObject(this, &UMythicaEditorSubsystem::OnResponseReceived);

    Request->ProcessRequest();
}

void UMythicaEditorSubsystem::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
        return;

    FString ResponseContent = Response->GetContentAsString();
    OnFactReceived.Broadcast(ResponseContent);
}
