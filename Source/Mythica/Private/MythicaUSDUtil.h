#pragma once

class AActor;
class UStaticMesh;
class USplineComponent;

namespace Mythica
{
    FString MakeUniquePath(const FString& AbsolutePath);
    
    bool ExportMesh(UStaticMesh* Mesh, const FString& ExportPath);
    bool ExportActors(const TArray<AActor*> Actors, const FString& ExportPath);
    bool ExportSpline(USplineComponent* SplineComponent, const FString& ExportPath);
}
