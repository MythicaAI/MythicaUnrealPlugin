#pragma once

#include "CoreMinimal.h"

#include "MythicaDeveloperSettings.generated.h"

UCLASS(config = Plugins, defaultconfig, meta = (DisplayName = "Mythica"))
class MYTHICA_API UMythicaDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMythicaDeveloperSettings(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(Config, EditAnywhere, Category = Server)
	FString PackageServerHost = TEXT("localhost");

	UPROPERTY(Config, EditAnywhere, Category = Server)
	int32 PackageServerPort = 5055;

	UPROPERTY(Config, EditAnywhere, Category = Server)
	FString PackageInstallDirectory = TEXT("/Game/Mythica");

	UPROPERTY(Config, EditAnywhere, Category = User)
	FString Username;

	UPROPERTY(Config, EditAnywhere, Category = User)
	FString AuthKey;
};
