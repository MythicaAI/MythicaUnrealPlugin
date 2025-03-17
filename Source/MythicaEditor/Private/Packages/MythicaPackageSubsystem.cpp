// Fill out your copyright notice in the Description page of Project Settings.


#include "MythicaPackageSubsystem.h"

#include "HttpModule.h"
#include "IImageWrapperModule.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "MythicaDeveloperSettings.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonValue.h"

#include "MythicaEditorPrivatePCH.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicaPackageSubsystem)

DEFINE_LOG_CATEGORY(LogMythicaPackages)

namespace MythicaPackages
{
    const TCHAR* ImportableFileExtensions[] =
    {
        TEXT("hda"), TEXT("hdalc"), TEXT("hdanc"),
        TEXT("otl"), TEXT("otllc"), TEXT("otlnc")
    };

    static bool CanImportAsset(const FString & FilePath)
    {
        FString Extension = FPaths::GetExtension(FilePath);

        for (const TCHAR* ImportableExtension : ImportableFileExtensions)
        {
            if (Extension == ImportableExtension)
            {
                return true;
            }
        }
        return false;
    }

    static bool CanDisplayImage(const FString & FileName)
    {
        IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
        EImageFormat ImageFormat = ImageWrapperModule.GetImageFormatFromExtension(*FileName);
        return ImageFormat != EImageFormat::Invalid;
    }
}

void UMythicaPackageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);


}

void UMythicaPackageSubsystem::Deinitialize()
{
    Super::Deinitialize();

}

bool UMythicaPackageSubsystem::CanInstallHdas() const
{
    return FModuleManager::Get().ModuleExists(TEXT("HoudiniEngine"));
}

TArray<FMythicaPackage> UMythicaPackageSubsystem::GetPackageList() const
{
    return PackageList;
}

UTexture2D* UMythicaPackageSubsystem::GetThumbnail(const FString& PackageId) const
{
    const TObjectPtr<UTexture2D>* Texture = ThumbnailCache.Find(PackageId);
    return (*Texture ? *Texture : nullptr);
}

bool UMythicaPackageSubsystem::IsPackageInstalled(const FString& PackageId) const
{
    return InstalledPackages.Contains(PackageId);
}

bool UMythicaPackageSubsystem::IsAssetFavorite(const FString& AssetId) const
{
    return FavoriteAssetIds.Contains(AssetId);
}

void UMythicaPackageSubsystem::UpdatePackageList()
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/assets/top"), *Settings->GetServiceURL());

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");
    Request->OnProcessRequestComplete().BindUObject(this, &UMythicaPackageSubsystem::OnGetAssetsResponse);

    Request->ProcessRequest();
}

void UMythicaPackageSubsystem::InstallPackage(const FString& PackageId)
{
}

void UMythicaPackageSubsystem::UninstallPackage(const FString& PackageId)
{
}

void UMythicaPackageSubsystem::FavoriteAsset(const FString& AssetId)
{
    ExecuteFavoriteAsset(AssetId, true);
}

void UMythicaPackageSubsystem::UnfavoriteAsset(const FString& AssetId)
{
    ExecuteFavoriteAsset(AssetId, false);
}

void UMythicaPackageSubsystem::OnGetAssetsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    ensure(Response.IsValid());

    if (!bWasSuccessful)
    {
        EHttpFailureReason Reason = Response->GetFailureReason();

        FString ReasonAsString;
        switch (Reason)
        {
        case EHttpFailureReason::ConnectionError:
            ReasonAsString = TEXT("Connection Error");
            break;
        case EHttpFailureReason::Cancelled:
            ReasonAsString = TEXT("Cancelled");
            break;
        case EHttpFailureReason::TimedOut:
            ReasonAsString = TEXT("Timed Out");
            break;
        case EHttpFailureReason::Other:
            ReasonAsString = TEXT("Other");
            break;
        default:
            ReasonAsString = TEXT("");
            break;
        }

        UE_LOG(LogMythicaPackages, Error, TEXT("Failed to get assets because %s"), *ReasonAsString);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

    TSharedPtr<FJsonValue> JsonValue;
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMythicaPackages, Error, TEXT("Failed to parse get assets JSON string"));
        return;
    }

    if (JsonValue->Type != EJson::Array)
    {
        UE_LOG(LogMythicaPackages, Error, TEXT("JSON value is not an array"));
        return;
    }

    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();
    ensure(Settings);

    PackageList.Reset();

    const TArray<TSharedPtr<FJsonValue>>& Array = JsonValue->AsArray();
    for (TSharedPtr<FJsonValue> Value : Array)
    {
        TSharedPtr<FJsonObject> JsonObject = Value->AsObject();
        if (!JsonObject.IsValid())
        {
            continue;
        }

        FString AssetId = JsonObject->GetStringField(TEXT("asset_id"));
        FString PackageId = JsonObject->GetStringField(TEXT("package_id"));
        FString Name = JsonObject->GetStringField(TEXT("name"));
        FString Description = JsonObject->GetStringField(TEXT("description"));
        FString OrgName = JsonObject->GetStringField(TEXT("org_name"));
        if (PackageId.IsEmpty())
        {
            UE_LOG(LogMythicaPackages, Error, TEXT("Missing PackageId for package: %s"), *Name);
            continue;
        }

        TArray<TSharedPtr<FJsonValue>> Version = JsonObject->GetArrayField(TEXT("version"));
        if (Version.Num() != 3)
        {
            continue;
        }

        FMythicaPackageVersion AssetVersion = {
            Version[0]->AsNumber(),
            Version[1]->AsNumber(),
            Version[2]->AsNumber()
        };

        TSharedPtr<FJsonObject> ContentsObject = JsonObject->GetObjectField(TEXT("contents"));
        if (!ContentsObject.IsValid())
        {
            continue;
        }

        int32 DigitalAssetCount = 0;

        const TArray<TSharedPtr<FJsonValue>>* FileArray = nullptr;
        ContentsObject->TryGetArrayField(TEXT("files"), FileArray);
        if (FileArray)
        {
            for (const TSharedPtr<FJsonValue>& FileValue : *FileArray)
            {
                TSharedPtr<FJsonObject> FileObject = FileValue->AsObject();

                FString FileName = FileObject->GetStringField(TEXT("file_name"));
                if (MythicaPackages::CanImportAsset(FileName))
                {
                    DigitalAssetCount++;
                }
            }
        }

        FString ThumbnailUrl;
        const TArray<TSharedPtr<FJsonValue>>* ThumbnailArray = nullptr;
        ContentsObject->TryGetArrayField(TEXT("thumbnails"), ThumbnailArray);
        if (ThumbnailArray)
        {
            for (const TSharedPtr<FJsonValue>& ThumbnailValue : *ThumbnailArray)
            {
                TSharedPtr<FJsonObject> ThumbnailObject = ThumbnailValue->AsObject();

                FString FileName = ThumbnailObject->GetStringField(TEXT("file_name"));
                if (MythicaPackages::CanDisplayImage(FileName))
                {
                    FString FileExtension = FPaths::GetExtension(FileName);
                    FString ContentHash = ThumbnailObject->GetStringField(TEXT("content_hash"));
                    ThumbnailUrl = FString::Printf(TEXT("%s/%s.%s"), *Settings->GetImagesURL(), *ContentHash, *FileExtension);
                    break;
                }
            }
        }

        FString PackageUrl;
        PackageUrl = FString::Printf(TEXT("%s/package-view/%s/versions/%d.%d.%d"), *Settings->GetServiceURL(), *AssetId, AssetVersion.Major, AssetVersion.Minor, AssetVersion.Patch);

        PackageList.Push({ AssetId, PackageId, Name, Description, OrgName, AssetVersion, {}, ThumbnailUrl, PackageUrl, DigitalAssetCount });
    }

    // @TODO: Update stats by registering to the package list update callback
    // UpdateStats();

    OnAssetListUpdated.Broadcast();

    LoadThumbnails();
}

void UMythicaPackageSubsystem::ExecuteFavoriteAsset(const FString& AssetId, bool bState)
{
    const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();

    FString Url = FString::Printf(TEXT("%s/v1/assets/g/unreal/%s/versions/0.0.0"), *Settings->GetServiceURL(), *AssetId);

    auto Callback = [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
        {
            OnFavortiteAssetResponse(Request, Response, bConnectedSuccessfully);
        };

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(bState ? "Post" : "Delete");
    //Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
    Request->OnProcessRequestComplete().BindLambda(Callback);

    Request->ProcessRequest();
}

void UMythicaPackageSubsystem::OnFavortiteAssetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogMythicaPackages, Error, TEXT("Failed to change asset group"));
        return;
    }

    OnFavoriteAssetsUpdated.Broadcast();
}

void UMythicaPackageSubsystem::LoadInstalledAssetList()
{
}

void UMythicaPackageSubsystem::AddInstalledAsset(const FString& PackageId, const FString& ImportDirectory)
{
}

void UMythicaPackageSubsystem::LoadThumbnails()
{
}

void UMythicaPackageSubsystem::OnThumbnailDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& PackageId)
{
}

FMythicaPackage* UMythicaPackageSubsystem::FindPackage(const FString& PackageId)
{
    return nullptr;
}
