#pragma once

/*----------------------------------------------------------------------------
    Low level includes.
----------------------------------------------------------------------------*/

#include "CoreTypes.h"

/*----------------------------------------------------------------------------
    Forward declarations
----------------------------------------------------------------------------*/

#include "CoreFwd.h"
#include "UObject/UObjectHierarchyFwd.h"
#include "Containers/ContainersFwd.h"

/*----------------------------------------------------------------------------
    Common headers
----------------------------------------------------------------------------*/

// @TODO: Take out core minimal, used as a catch all.
#include "CoreMinimal.h"

#include "Logging/LogMacros.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Editor.h"
#include "LevelEditor.h"

/*----------------------------------------------------------------------------
    Pre-processor defines
----------------------------------------------------------------------------*/

// Module names for convenience.
#define MYTHICA_MODULE_EDITOR "MythicaEditor"
#define MYTHICA_MODULE_RUNTIME "MythicaRuntime"

// Defining the local namespace for the entire editor so it is all grouped together.
#define MYTHICA_LOCTEXT_NAMESPACE MYTHICA_MODULE_EDITOR

// Declare our editor log category.
DECLARE_LOG_CATEGORY_EXTERN(LogMythicaEditor, Log, All);

/*----------------------------------------------------------------------------
    Backward compatibile wrappers
----------------------------------------------------------------------------*/
