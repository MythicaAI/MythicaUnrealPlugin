// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/EngineSubsystem.h"

#include "MythicaPackage.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "MythicaPackageSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythicaPackages, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPackageListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFavoriteAssetUpdated);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThumbnailLoaded, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPackageInstalled, const FString&, PackageId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPackageUninstalled, const FString&, PackageId);

/**
 * Mythica Package Subsystem
 *
 * 
 */
UCLASS()
class UMythicaPackageSubsystem : public UEngineSubsystem
{

    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection);
    virtual void Deinitialize();

    /**  */
    UFUNCTION(BlueprintPure, Category = "Mythica|Packages")
    bool CanInstallHdas() const;

    /**  */
    UFUNCTION(BlueprintPure, Category = "Mythica|Packages")
    TArray<FMythicaPackage> GetPackageList() const;

    /**  */
    UFUNCTION(BlueprintPure, Category = "Mythica|Packages")
    UTexture2D* GetThumbnail(const FString& PackageId) const;

    /**  */
    UFUNCTION(BlueprintPure, Category = "Mythica|Packages")
    bool IsPackageInstalled(const FString& PackageId) const;

    /**  */
    UFUNCTION(BlueprintPure, Category = "Mythica|Packages")
    bool IsAssetFavorite(const FString& AssetId) const;

    UFUNCTION(BlueprintCallable, Category = "Mythica|Packages")
    void UpdatePackageList();

    UFUNCTION(BlueprintCallable, Category = "Mythica|Packages")
    void InstallPackage(const FString& PackageId);

    UFUNCTION(BlueprintCallable, Category = "Mythica|Packages")
    void UninstallPackage(const FString& PackageId);

    UFUNCTION(BlueprintCallable, Category = "Mythica|Packages")
    void FavoriteAsset(const FString& AssetId);

    UFUNCTION(BlueprintCallable, Category = "Mythica|Packages")
    void UnfavoriteAsset(const FString& AssetId);

private:

    void OnGetAssetsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void ExecuteFavoriteAsset(const FString& AssetId, bool bState);
    void OnFavortiteAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void LoadInstalledAssetList();
    void AddInstalledAsset(const FString& PackageId, const FString& ImportDirectory);

    void LoadThumbnails();
    void OnThumbnailDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId);

    FMythicaPackage* FindPackage(const FString& PackageId);

public:

    UPROPERTY(BlueprintAssignable, Category = "Mythica|Packages")
    FOnPackageListUpdated OnAssetListUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Mythica|Packages")
    FOnFavoriteAssetUpdated OnFavoriteAssetsUpdated;

    //UPROPERTY(BlueprintAssignable, Category = "Mythica|Packages")
    //FOnThumbnailLoaded OnThumbnailLoaded;

    UPROPERTY(BlueprintAssignable, Category = "Mythica|Packages")
    FOnPackageInstalled OnPackageInstalled;

    UPROPERTY(BlueprintAssignable, Category = "Mythica|Packages")
    FOnPackageInstalled OnPackageUninstalled;

private:

    TArray<FString> FavoriteAssetIds;

    TMap<FString, FString> InstalledPackages;

    TArray<FMythicaPackage> PackageList;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<class UTexture2D>> ThumbnailCache;

};
