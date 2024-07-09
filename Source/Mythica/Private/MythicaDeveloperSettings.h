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
	FString ServiceURL = TEXT("api.mythica.ai");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
	FString ImagesURL = TEXT("images.mythica.ai");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
	FString ImportDirectory = TEXT("/Game/Mythica");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = User)
	FString ProfileId;
};
