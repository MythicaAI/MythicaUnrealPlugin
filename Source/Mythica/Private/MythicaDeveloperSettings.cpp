#include "MythicaDeveloperSettings.h"

const TCHAR* DefaultJobDefIdWhitelist[] =
{
    TEXT("jobdef_2crsbDnsSuFXgZouExzetpMDyDNq"),    // Crystal
    TEXT("jobdef_2ksoWHctAoAFcNN5roDzmSZ9iN62"),    // MeanderingVine
    TEXT("jobdef_3o41eujjxzNCSiMWkEXteaeUEEEM"),    // Rockify
};

UMythicaDeveloperSettings::UMythicaDeveloperSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Mythica");

    for (const TCHAR* JobDefId : DefaultJobDefIdWhitelist)
    {
        JobDefIdWhitelist.Add(JobDefId);
    }
}
