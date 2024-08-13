#pragma once

#include "CoreMinimal.h"

#include "MythicaTypes.generated.h"

USTRUCT(BlueprintType)
struct FMythicaParameters
{
    GENERATED_BODY()

    struct Parameter
    {
        FString Name;
        float Value;
    };
    TArray<Parameter> Parameters;
};
