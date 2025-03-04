#include "MythicaDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicaDeveloperSettings)

const TCHAR* DefaultAssetIdIdWhitelist[] =
{
    TEXT("asset_2gcVbJghPhWLznuAxXeiapVKvQQi")  // Rockify
};

UMythicaDeveloperSettings::UMythicaDeveloperSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Mythica");

    for (const TCHAR* AssetId : DefaultAssetIdIdWhitelist)
    {
        ProductionAssetIdWhitelist.Add(AssetId);
    }
}

FString UMythicaDeveloperSettings::GetServiceURL() const
{
    switch (Environment)
    {
        case EMythicaEnvironment::Production:
            return TEXT("https://api.mythica.gg");
        case EMythicaEnvironment::Staging:
            return TEXT("https://api-staging.mythica.gg");
        case EMythicaEnvironment::Local:
            return TEXT("http://localhost:8080");
    }

    return TEXT("");
}

FString UMythicaDeveloperSettings::GetWebSocketURL() const
{
    switch (Environment)
    {
        case EMythicaEnvironment::Production:
            return TEXT("wss://api.mythica.gg");
        case EMythicaEnvironment::Staging:
            return TEXT("wss://api-staging.mythica.gg");
        case EMythicaEnvironment::Local:
            return TEXT("ws://localhost:8080");
    }

    return TEXT("");
}

FString UMythicaDeveloperSettings::GetImagesURL() const
{
    switch (Environment)
    {
        case EMythicaEnvironment::Production:
            return TEXT("https://api.mythica.gg/images");
        case EMythicaEnvironment::Staging:
            return TEXT("https://api-staging.mythica.gg/images");
        case EMythicaEnvironment::Local:
            return TEXT("http://localhost:8080/images");
    }

    return TEXT("");
}

FString UMythicaDeveloperSettings::GetAPIKey() const
{
    switch (Environment)
    {
        case EMythicaEnvironment::Production:
            return ProductionAPIKey;
        case EMythicaEnvironment::Staging:
            return StagingAPIKey;
        case EMythicaEnvironment::Local:
            return LocalAPIKey;
    }

    return TEXT("");
}

const TArray<FString>& UMythicaDeveloperSettings::GetJobDefIdWhitelist() const
{
    switch (Environment)
    {
        case EMythicaEnvironment::Production:
            return ProductionJobDefIdWhitelist;
        case EMythicaEnvironment::Staging:
            return StagingJobDefIdWhitelist;
        case EMythicaEnvironment::Local:
            return LocalJobDefIdWhitelist;
    }

    static const TArray<FString> EmptyArray;
    return EmptyArray;
}

const TArray<FString>& UMythicaDeveloperSettings::GetAssetWhitelist() const
{
    switch (Environment)
    {
        case EMythicaEnvironment::Production:
            return ProductionAssetIdWhitelist;
        case EMythicaEnvironment::Staging:
            return StagingAssetIdWhitelist;
        case EMythicaEnvironment::Local:
            return LocalAssetIdWhitelist;
    }

    static const TArray<FString> EmptyArray;
    return EmptyArray;
}
