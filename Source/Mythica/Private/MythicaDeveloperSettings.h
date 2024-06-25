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
	FString ServerHost = TEXT("localhost");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
	int32 ServerPort = 50555;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Server)
	FString PackageInstallDirectory = TEXT("/Game/Mythica");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = User)
	FString ProfileId;
};
