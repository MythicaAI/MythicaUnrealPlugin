#include "MythicaInputSelectionVolume.h"

#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "MythicaComponent.h"

static bool ActorHasAnyCollision(AActor* InActor)
{
    TArray<UPrimitiveComponent*> PrimitiveComponents;
    InActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    for (UPrimitiveComponent* Component : PrimitiveComponents)
    {
        if (Component && Component->IsCollisionEnabled())
        {
            return true;
        }
    }

    return false;
}

static bool ActorReferencingVolume(AActor* InActor, const AActor* InVolume)
{
    TArray<UMythicaComponent*> MythicaComponents;
    InActor->GetComponents<UMythicaComponent>(MythicaComponents);

    for (UMythicaComponent* Component : MythicaComponents)
    {
        if (!Component)
        {
            continue;
        }

        for (const FMythicaParameter& Parameter : Component->Parameters.Parameters)
        {
            if (Parameter.Type != EMythicaParameterType::File)
            {
                continue;
            }

            const FMythicaParameterFile& Input = Parameter.ValueFile;
            if (Input.Type == EMythicaInputType::Volume && Input.VolumeActor == InVolume)
            {
                return true;
            }
        }
    }

    return false;
}

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

        if (!Actor->GetComponentsBoundingBox().Intersect(GetComponentsBoundingBox()))
        {
            continue;
        }

        if (!ActorHasAnyCollision(Actor))
        {
            continue;
        }

        if (Actor->IsA(ALandscapeProxy::StaticClass()))
        {
            continue;
        }

        if (ActorReferencingVolume(Actor, this))
        {
            continue;
        }

        Actors.Add(Actor);
    }
}
