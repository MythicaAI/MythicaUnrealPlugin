#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MythicaEditorSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythica, Log, All);

UENUM(BlueprintType)
enum EMythicaSessionState
{
	None,
	RequestingSession,
	SessionFailed,
	SessionCreated
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStateChanged, EMythicaSessionState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAssetListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThumbnailLoaded, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetInstalled, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetUninstalled, const FString&, PackageId);

USTRUCT(BlueprintType)
struct FMythicaAssetVersion
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Version")
	int32 Major = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Version")
	int32 Minor = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Version")
	int32 Patch = 0;

	bool operator<(const FMythicaAssetVersion& Other) const;
};

USTRUCT(BlueprintType)
struct FMythicaAsset
{
    GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString PackageId;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FMythicaAssetVersion Version;

	UPROPERTY()
	FString ThumbnailURL;
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
	EMythicaSessionState GetSessionState();

	UFUNCTION(BlueprintPure, Category = "Mythica")
	bool CanInstallAssets();

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	TArray<FMythicaAsset> GetAssetList();

	UFUNCTION(BlueprintPure, Category = "Mythica")
	bool IsAssetInstalled(const FString& PackageId);

	UFUNCTION(BlueprintPure, Category = "Mythica")
	UTexture2D* GetThumbnail(const FString& PackageId);

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
	FOnSessionStateChanged OnSessionStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnAssetListUpdated OnAssetListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnThumbnailLoaded OnThumbnailLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnAssetInstalled OnAssetInstalled;

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnAssetInstalled OnAssetUninstalled;

private:
	void OnCreateSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnGetAssetsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);
	void OnDownloadAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);

	void SetSessionState(EMythicaSessionState NewState);

	void LoadInstalledAssetList();
	FString GetUniqueImportDirectory(const FString& PackageName);
	void AddInstalledAsset(const FString& PackageId, const FString& ImportDirectory);

	void LoadThumbnails();
	void OnThumbnailDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);

	FMythicaAsset* FindAsset(const FString& PackageId);

	EMythicaSessionState SessionState = EMythicaSessionState::None;
	FString AuthToken;

	TMap<FString, FString> InstalledAssets;
	TArray<FMythicaAsset> AssetList;

	TMap<FString, UTexture2D*> ThumbnailCache;
};
