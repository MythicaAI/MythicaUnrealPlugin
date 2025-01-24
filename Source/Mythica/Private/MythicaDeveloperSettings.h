#pragma once

#include "CoreMinimal.h"

#include "MythicaDeveloperSettings.generated.h"

UENUM(BlueprintType)
enum class EMythicaEnvironment : uint8
{
    Production,
    Staging,
    Local
};

UCLASS(config = Plugins, defaultconfig, meta = (DisplayName = "Mythica"))
class MYTHICA_API UMythicaDeveloperSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMythicaDeveloperSettings(const FObjectInitializer& ObjectInitializer);

    FString GetServiceURL() const;
    FString GetImagesURL() const;
    FString GetAPIKey() const;
    const TArray<FString>& GetJobDefIdWhitelist() const;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
    EMythicaEnvironment Environment = EMythicaEnvironment::Production;

    // Production Settings
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, DisplayName = "APIKey", meta = (EditCondition = "Environment == EMythicaEnvironment::Production", EditConditionHides))
    FString ProductionAPIKey;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, DisplayName = "JobDefIdWhitelist", meta = (EditCondition = "Environment == EMythicaEnvironment::Production", EditConditionHides))
    TArray<FString> ProductionJobDefIdWhitelist;

    // Staging Settings
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, DisplayName = "APIKey", meta = (EditCondition = "Environment == EMythicaEnvironment::Staging", EditConditionHides))
    FString StagingAPIKey;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, DisplayName = "JobDefIdWhitelist", meta = (EditCondition = "Environment == EMythicaEnvironment::Staging", EditConditionHides))
    TArray<FString> StagingJobDefIdWhitelist;

    // Local Settings
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, DisplayName = "APIKey", meta = (EditCondition = "Environment == EMythicaEnvironment::Local", EditConditionHides))
    FString LocalAPIKey;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, DisplayName = "JobDefIdWhitelist", meta = (EditCondition = "Environment == EMythicaEnvironment::Local", EditConditionHides))
    TArray<FString> LocalJobDefIdWhitelist;

    // Global Settings
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Import)
    FString PackageImportDirectory = TEXT("/Game/Mythica/Packages");
    
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Import)
    FString GeneratedAssetImportDirectory = TEXT("/Game/Mythica/Generated");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Settings)
    float JobTimeoutSeconds = 120.0f;
};
