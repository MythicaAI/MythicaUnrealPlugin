#pragma once

namespace Mythica
{
    bool ExportMesh(UStaticMesh* Mesh, const FString& ExportPath);
    bool ExportActors(const TArray<AActor*> Actors, const FString& ExportPath, const FVector& Origin);
    bool ExportSpline(AActor* SplineActor, const FString& ExportPath, const FVector& Origin);
}
