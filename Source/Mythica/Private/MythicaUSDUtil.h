#pragma once

class AActor;
class UStaticMesh;

namespace Mythica
{
    FString MakeUniquePath(const FString& AbsolutePath);
    
    bool ExportMesh(UStaticMesh* Mesh, const FString& ExportPath);
    bool ExportActors(const TArray<AActor*> Actors, const FString& ExportPath);
    bool ExportSpline(AActor* SplineActor, const FString& ExportPath);
}
