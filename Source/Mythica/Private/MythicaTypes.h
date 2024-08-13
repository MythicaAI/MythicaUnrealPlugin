#pragma once

#include "CoreMinimal.h"

#include "MythicaTypes.generated.h"

USTRUCT(BlueprintType)
struct FMythicaParameter
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString Name;

    UPROPERTY(BlueprintReadWrite)
    float Value;
};

USTRUCT(BlueprintType)
struct FMythicaParameters
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicaParameter> Parameters;
};
