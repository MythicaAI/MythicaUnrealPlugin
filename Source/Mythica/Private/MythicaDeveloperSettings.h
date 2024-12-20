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

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
    EMythicaEnvironment Environment = EMythicaEnvironment::Production;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
    FString PackageImportDirectory = TEXT("/Game/Mythica/Packages");
    
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
    FString GeneratedAssetImportDirectory = TEXT("/Game/Mythica/Generated");
 
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = User)
    FString APIKey;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Tools)
    bool UseToolWhitelist = true;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Tools, meta = (EditCondition = "UseToolWhitelist", EditConditionHides))
    TArray<FString> JobDefIdWhitelist;
};
