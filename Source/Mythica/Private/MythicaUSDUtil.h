#pragma once

namespace Mythica
{
    FString MakeUniquePath(const FString& AbsolutePath);
    
    bool ExportMesh(UStaticMesh* Mesh, const FString& ExportPath);
    bool ExportActors(const TArray<AActor*> Actors, const FString& ExportPath);
}
