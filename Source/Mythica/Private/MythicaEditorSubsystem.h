#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MythicaTypes.h"
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

UENUM(BlueprintType)
enum EMythicaGenerateMeshState
{
	Invalid,
	Queued,
	Processing,
    Failed,
    Completed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStateChanged, EMythicaSessionState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAssetListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThumbnailLoaded, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetInstalled, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetUninstalled, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolInterfaceLoaded, const FString&, FileId);

USTRUCT(BlueprintType)
struct FMythicaStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stat")
	int32 TotalPackages = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stat")
	int32 TotalDigitalAssets = 0;
};

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
struct FMythicaTool
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString FileId;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString Name;
};

USTRUCT(BlueprintType)
struct FMythicaAsset
{
    GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString AssetId;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString PackageId;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FString OrgName;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FMythicaAssetVersion Version;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	TArray<FString> Tags;

	UPROPERTY()
	FString ThumbnailURL;

	UPROPERTY()
	int32 DigitalAssetCount;
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

	UFUNCTION(BlueprintPure, Category = "Mythica")
	bool CanGenerateMeshes();

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	TArray<FMythicaAsset> GetAssetList();

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	TArray<FMythicaTool> GetToolList();

	UFUNCTION(BlueprintPure, Category = "Mythica")
	bool IsAssetInstalled(const FString& PackageId);

	UFUNCTION(BlueprintPure, Category = "Mythica")
	UTexture2D* GetThumbnail(const FString& PackageId);

	UFUNCTION(BlueprintPure, Category = "Mythica")
	FMythicaStats GetStats();

	UFUNCTION(BlueprintPure, Category = "Mythica")
	bool IsToolInterfaceLoaded(const FString& FileId);

	UFUNCTION(BlueprintPure, Category = "Mythica")
	FMythicaParameters GetToolInterface(const FString& FileId);

	// Requests
	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void CreateSession();

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void UpdateAssetList();

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void InstallAsset(const FString& PackageId);

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void UninstallAsset(const FString& PackageId);

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void LoadToolInterface(const FString& FileId);

	UFUNCTION(BlueprintCallable, Category = "Mythica")
	void GenerateMesh(const FString& FileId, const FMythicaParameters& Params, const FString& ImportName);

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

	UPROPERTY(BlueprintAssignable, Category = "Mythica")
	FOnToolInterfaceLoaded OnToolInterfaceLoaded;

private:
	void OnCreateSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnGetAssetsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);
	void OnDownloadAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);

	void OnGenerateMeshResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileID, const FString& ImportName);
	void OnGenerateMeshStatusResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& RequestId, const FString& ImportName);
	void OnMeshDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileId, const FString& ImportName);
	void OnMeshDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileId, const FString& ImportName);

	void OnInterfaceResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileId);
	void OnInterfaceDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileId);
	void OnInterfaceDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& FileId);

	void AddGenerateMeshRequest(const FString& RequestId, const FString& ImportName);
	void RemoveGenerateMeshRequest(const FString& RequestId);
	void PollGenerateMeshStatus();

	void SetSessionState(EMythicaSessionState NewState);

	void LoadInstalledAssetList();
	FString GetUniqueImportDirectory(const FString& PackageName);
	void AddInstalledAsset(const FString& PackageId, const FString& ImportDirectory);
	void UpdateStats();

	void LoadThumbnails();
	void OnThumbnailDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);

	FMythicaAsset* FindAsset(const FString& PackageId);

	EMythicaSessionState SessionState = EMythicaSessionState::None;
	FString AuthToken;

	TMap<FString, FString> GenerateMeshRequests;
	FTimerHandle GenerateMeshTimer;

	TMap<FString, FString> InstalledAssets;
	TArray<FMythicaAsset> AssetList;
	TArray<FMythicaTool> ToolList;
	FMythicaStats Stats;

	TMap<FString, FMythicaParameters> ToolInterfaces;

	UPROPERTY()
	TMap<FString, UTexture2D*> ThumbnailCache;
};
