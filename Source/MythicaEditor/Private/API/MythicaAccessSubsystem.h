// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/EngineSubsystem.h"

#include "MythicaAccessSubsystem.generated.h"

//UENUM(BlueprintType)
//enum class EMythicaSessionState : uint8
//{
//    MSS_None        UMETA(DisplayName = "None"),
//    MSS_Requesting  UMETA(DisplayName = "Requesting"),
//    MSS_Failed      UMETA(DisplayName = "Failed"),
//    MSS_Created     UMETA(DisplayName = "Created")
//};
//
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStateChanged, EMythicaSessionState, State);

/**
 * Mythica Access Subsystem
 *
 * A way to connect to the Mythica backend. This allows users to link to their profile and push/pull data from mythica's databases.
 */
UCLASS()
class UMythicaAccessSubsystem : public UEngineSubsystem
{

    GENERATED_BODY()

//public:
//
//    UFUNCTION(BlueprintPure, Category = "Mythica|API|Session")
//    EMythicaSessionState GetSessionState() const { return SessionState; }
//
//private:
//
//    void OnCreateSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
//
//public:
//
//    UPROPERTY(BlueprintAssignable, Category = "Mythica|API|Session")
//    FOnSessionStateChanged OnSessionStateChanged;
//
//private:
//
//    UPROPERTY(Transient)
//    EMythicaSessionState SessionState = EMythicaSessionState::None;
//
//    UPROPERTY(Transient)
//    FString AuthToken = FString();
//
//    UPROPERTY(Transient)
//    TSharedPtr<class IWebSocket> WebSocket = nullptr;

};
