#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MythicaTypes.h"
#include "MythicaEditorSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythica, Log, All);

UENUM(BlueprintType)
enum class EMythicaSessionState : uint8
{
    None,
    RequestingSession,
    SessionFailed,
    SessionCreated
};

UENUM(BlueprintType)
enum class EMythicaJobState : uint8
{
    Invalid,
    Requesting,        // Request has been sent to the server
    Queued,            // Request acknowledged by the server and queued for processing
    Processing,        // Request is being processed by the server
    Importing,         // Request has finished processing and the result is being downloaded
    Completed,         // Request has been completed
    Failed             // Request has failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStateChanged, EMythicaSessionState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAssetListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThumbnailLoaded, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetInstalled, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetUninstalled, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJobDefinitionListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnJobStateChanged, int, RequestId, EMythicaJobState, State, FText, Message);

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
struct FMythicaJobDefinitionId
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Data")
    FString JobDefId;
};

USTRUCT(BlueprintType)
struct FMythicaJobDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString JobDefId;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString JobType;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString Description;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FMythicaParameters Parameters;
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

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString ThumbnailURL;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString PackageURL;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    int32 DigitalAssetCount;
};

USTRUCT()
struct FMythicaJob
{
    GENERATED_BODY()

    UPROPERTY()
    FString JobDefId;

    UPROPERTY()
    TArray<FString> InputFileIds;

    UPROPERTY()
    FMythicaParameters Params;

    UPROPERTY()
    FString ImportPath;

    UPROPERTY()
    EMythicaJobState State = EMythicaJobState::Requesting;

    UPROPERTY()
    FTimerHandle TimeoutTimer;

    UPROPERTY()
    FString JobId;

    UPROPERTY()
    FString ImportDirectory;
};

UCLASS()
class UMythicaEditorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

    virtual void Initialize(FSubsystemCollectionBase& Collection);
    virtual void Deinitialize();
    void ResetSession();

    void OnMapChanged(UWorld* InWorld, EMapChangeType InMapChangeType);
    void OnSettingsChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent);

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
    TArray<FMythicaJobDefinition> GetJobDefinitionList(const FString& JobType);

    UFUNCTION(BlueprintCallable, Category = "Mythica")
    FMythicaJobDefinition GetJobDefinitionById(const FString& JobDefId);

    UFUNCTION(BlueprintPure, Category = "Mythica")
    bool IsAssetInstalled(const FString& PackageId);

    UFUNCTION(BlueprintPure, Category = "Mythica")
    UTexture2D* GetThumbnail(const FString& PackageId);

    UFUNCTION(BlueprintPure, Category = "Mythica")
    FMythicaStats GetStats();

    UFUNCTION(BlueprintPure, Category = "Mythica")
    FString GetImportDirectory(int RequestId);

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
    void UpdateJobDefinitionList();

    UFUNCTION(BlueprintCallable, Category = "Mythica")
    int ExecuteJob(
        const FString& JobDefId, 
        const FMythicaParameters& Params, 
        const FString& ImportPath, 
        const FVector& Origin);

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
    FOnJobDefinitionListUpdated OnJobDefinitionListUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Mythica")
    FOnJobStateChanged OnJobStateChange;

private:
    void OnCreateSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnGetAssetsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);
    void OnDownloadAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);

    void OnJobDefinitionsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    bool PrepareInputFiles(const FMythicaParameters& Params, TMap<int, FString>& InputFiles, FString& ExportDirectory, const FVector& Origin);
    void UploadInputFiles(int RequestId, const TMap<int, FString>& InputFiles);
    void OnUploadInputFilesResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId, const TMap<int, FString>& InputFiles);
    void SendJobRequest(int RequestId);

    void OnExecuteJobResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId);
    void OnJobResultsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId);
    void OnMeshDownloadInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId);
    void OnMeshDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, int RequestId);

    int CreateJob(const FString& JobDefId, const FMythicaParameters& Params, const FString& ImportName);
    void SetJobState(int RequestId, EMythicaJobState State, FText Message = FText::GetEmpty());
    void PollJobStatus();
    void OnJobTimeout(int RequestId);
    void ClearJobs();

    void SetSessionState(EMythicaSessionState NewState);

    void LoadInstalledAssetList();
    void AddInstalledAsset(const FString& PackageId, const FString& ImportDirectory);
    void UpdateStats();

    void LoadThumbnails();
    void OnThumbnailDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);

    FMythicaAsset* FindAsset(const FString& PackageId);

    EMythicaSessionState SessionState = EMythicaSessionState::None;
    FString AuthToken;

    TArray<FMythicaJobDefinition> JobDefinitionList;

    UPROPERTY()
    TMap<int, FMythicaJob> Jobs;
    FTimerHandle JobPollTimer;
    int NextRequestId = 1;

    TMap<FString, FString> InstalledAssets;
    TArray<FMythicaAsset> AssetList;
    FMythicaStats Stats;

    UPROPERTY()
    TMap<FString, UTexture2D*> ThumbnailCache;
};
