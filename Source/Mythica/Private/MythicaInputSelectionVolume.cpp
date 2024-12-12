#include "MythicaInputSelectionVolume.h"

#include "EngineUtils.h"

void AMythicaInputSelectionVolume::GetActors(TArray<AActor*>& Actors) const
{
    UWorld* World = GetWorld();

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor == this)
        {
            continue;
        }

        if (Actor->GetComponentsBoundingBox().Intersect(GetComponentsBoundingBox()))
        {
            Actors.Add(Actor);
        }
    }
}
