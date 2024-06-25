#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MythicaEditorSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFactReceived, const FString&, Fact);

UCLASS()
class UMythicaEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection);
	virtual void Deinitialize();

public:

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void RequestFact();

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnFactReceived OnFactReceived;

private:
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
