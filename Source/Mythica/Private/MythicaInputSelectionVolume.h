#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "MythicaInputSelectionVolume.generated.h"

UCLASS()
class MYTHICA_API AMythicaInputSelectionVolume : public AVolume
{
    GENERATED_BODY()
public:

    void GetActors(TArray<AActor*>& Actors) const;
};
