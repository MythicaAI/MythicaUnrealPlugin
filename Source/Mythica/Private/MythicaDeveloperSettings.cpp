#include "MythicaDeveloperSettings.h"

const TCHAR* DefaultJobDefIdWhitelist[] =
{
    TEXT("jobdef_2crsbDnsSuFXgZouExzetpMDyDNq"),    // Crystal
    TEXT("jobdef_4Cf9gnbAjoTKdRsavFGAEXeDSLF9"),    // MeanderingVine
    TEXT("jobdef_36esG96RhZBu4R7bMXAkKNnjKtrE"),    // Rockify
    TEXT("jobdef_4V1RAwMKKbtbdqaZYVqAjCRWGdH8"),    // CrystalCluster
    TEXT("jobdef_44tBgRfFD49UiSKhcKotnD2SGrqc"),    // MossGrowth
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
