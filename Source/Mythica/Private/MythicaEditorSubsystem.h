#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MythicaEditorSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythica, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAssetListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetInstalled, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetUninstalled, const FString&, PackageId);

USTRUCT(BlueprintType)
struct FMythicaAsset
{
    GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PackageId;
	
	UPROPERTY(BlueprintReadOnly)
    FString Name;

	UPROPERTY(BlueprintReadOnly)
	FString Description;
};

UCLASS()
class UMythicaEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection);
	virtual void Deinitialize();

public:
	// Getters
	UFUNCTION(BlueprintPure, Category = "Mythica")
	bool IsAuthenticated();

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	TArray<FMythicaAsset> GetAssetList();

	UFUNCTION(BlueprintPure, Category = "Mythica")
	bool IsAssetInstalled(const FString& PackageId);

	// Requests
	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void CreateSession();

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void UpdateAssetList();

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void InstallAsset(const FString& PackageId);

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void UninstallAsset(const FString& PackageId);

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnSessionCreated OnSessionCreated;

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnAssetListUpdated OnAssetListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnAssetInstalled OnAssetInstalled;

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnAssetInstalled OnAssetUninstalled;

private:
	void OnCreateSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnGetAssetsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDownloadAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void LoadInstalledAssetList();
	void AddInstalledAsset(const FString& PackageId, const FString& ImportDirectory);

	FMythicaAsset* FindAsset(const FString& PackageId);

	FString AuthToken;
	TMap<FString, FString> InstalledAssets;
	TArray<FMythicaAsset> AssetList;
};
