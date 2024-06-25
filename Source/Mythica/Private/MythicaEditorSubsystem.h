#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "MythicaEditorSubsystem.generated.h"

UCLASS()
class UMythicaEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection);
	virtual void Deinitialize();

public:

};
