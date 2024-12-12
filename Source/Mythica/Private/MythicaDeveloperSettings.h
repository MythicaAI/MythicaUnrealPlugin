#pragma once

#include "CoreMinimal.h"

#include "MythicaDeveloperSettings.generated.h"

UCLASS(config = Plugins, defaultconfig, meta = (DisplayName = "Mythica"))
class MYTHICA_API UMythicaDeveloperSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMythicaDeveloperSettings(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
    FString ServiceURL = TEXT("https://api.mythica.ai");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
    FString ImagesURL = TEXT("https://api.mythica.ai/images");

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
