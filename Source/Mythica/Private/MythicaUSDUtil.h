#pragma once

UENUM(BlueprintType)
enum class EMythicaExportTransformType : uint8
{
    World,
    Relative,
    Centered
};

namespace Mythica
{
    bool ExportMesh(UStaticMesh* Mesh, const FString& ExportPath);
    bool ExportActors(const TArray<AActor*> Actors, const FString& ExportPath, const FVector& Origin, EMythicaExportTransformType TransformType);
    bool ExportSpline(AActor* SplineActor, const FString& ExportPath, const FVector& Origin, EMythicaExportTransformType TransformType);

    bool ImportMesh(const FString& FilePath, const FString& ImportDirectory);
}
