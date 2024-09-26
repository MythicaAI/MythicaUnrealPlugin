#include "MythicaDeveloperSettings.h"

UMythicaDeveloperSettings::UMythicaDeveloperSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Mythica");
}
