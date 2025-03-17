// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MythicaTypes.h"

#include "Engine/TimerHandle.h"

#include "MythicaJob.generated.h"

//UENUM(BlueprintType)
//enum class EMythicaJobState : uint8
//{
//    Invalid,
//    Requesting,        // Request has been sent to the server
//    Queued,            // Request acknowledged by the server and queued for processing
//    Processing,        // Request is being processed by the server
//    Importing,         // Request has finished processing and the result is being downloaded
//    Completed,         // Request has been completed
//    Failed             // Request has failed
//};

//USTRUCT(BlueprintType)
//struct FMythicaJobDefinitionId
//{
//    GENERATED_BODY()
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Data")
//    FString JobDefId;
//};
//
//USTRUCT(BlueprintType)
//struct FMythicaJobDefinition
//{
//    GENERATED_BODY()
//
//    UPROPERTY(BlueprintReadOnly, Category = "Data")
//    FString JobDefId;
//
//    UPROPERTY(BlueprintReadOnly, Category = "Data")
//    FString JobType;
//
//    UPROPERTY(BlueprintReadOnly, Category = "Data")
//    FString Name;
//
//    UPROPERTY(BlueprintReadOnly, Category = "Data")
//    FString Description;
//
//    UPROPERTY(BlueprintReadOnly, Category = "Data")
//    FMythicaParameters Parameters;
//
//    UPROPERTY(BlueprintReadOnly, Category = "Data")
//    FMythicaAssetVersionEntryPointReference Source;
//
//    UPROPERTY(BlueprintReadOnly, Category = "Data")
//    FString SourceAssetName;
//
//    UPROPERTY(BlueprintReadOnly, Category = "Data")
//    FString SourceAssetOwner;
//};

USTRUCT(BlueprintType)
struct FMythicaJobRecord
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString JobDefId = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    TArray<FString> InputFileIds = TArray<FString>();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FMythicaParameters Params = FMythicaParameters();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString ImportPath = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    TWeakObjectPtr<class UMythicaComponent> OwningComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString OwningComponentName = FString();

    //UPROPERTY(BlueprintReadOnly, Category = "Data")
    //EMythicaJobState State = EMythicaJobState::Requesting;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FTimerHandle TimeoutTimer;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString JobId = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString ImportDirectory = FString();

    /** The system time of the machine at the time the job was created */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FDateTime StartTime = FDateTime();

    /** The system time of the machine at the time the job was completed */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FDateTime EndTime = FDateTime();

};
