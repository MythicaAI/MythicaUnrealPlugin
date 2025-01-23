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

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
    EMythicaEnvironment Environment = EMythicaEnvironment::Production;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, meta = (EditCondition = "Environment == EMythicaEnvironment::Production", EditConditionHides))
    FString ProductionAPIKey;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, meta = (EditCondition = "Environment == EMythicaEnvironment::Staging", EditConditionHides))
    FString StagingAPIKey;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server, meta = (EditCondition = "Environment == EMythicaEnvironment::Local", EditConditionHides))
    FString LocalAPIKey;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Import)
    FString PackageImportDirectory = TEXT("/Game/Mythica/Packages");
    
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Import)
    FString GeneratedAssetImportDirectory = TEXT("/Game/Mythica/Generated");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Tools)
    float JobTimeoutSeconds = 120.0f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Tools)
    bool UseToolWhitelist = true;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Tools, meta = (EditCondition = "UseToolWhitelist", EditConditionHides))
    TArray<FString> JobDefIdWhitelist;
};
