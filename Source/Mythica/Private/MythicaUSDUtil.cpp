#pragma once

#include "MythicaUSDUtil.h"

#include "Components/SplineComponent.h"
#include "Exporters/Exporter.h"
#include "IPythonScriptPlugin.h"
#include "LevelExporterUSDOptions.h"
#include "StaticMeshExporterUSDOptions.h"
#include "UObject/GCObjectScopeGuard.h"
#include "UnrealUSDWrapper.h"
#include "UsdWrappers/SdfLayer.h"
#include "UsdWrappers/UsdStage.h"

#include "USDIncludesStart.h"
    #include "pxr/usd/usd/stage.h"
    #include "pxr/usd/usdGeom/basisCurves.h"
#include "USDIncludesEnd.h"

static bool ConvertUSDtoUSDZ(const FString& InFile, const FString& OutFile)
{
    if (!IPythonScriptPlugin::Get()->IsPythonAvailable())
    {
        return false;
    }

    FString Command = FString::Printf(TEXT("from pxr import UsdUtils; UsdUtils.CreateNewUsdzPackage('%s','%s')"), *InFile, *OutFile);
    IPythonScriptPlugin::Get()->ExecPythonCommand(*Command);
    return true;
}

FString Mythica::MakeUniquePath(const FString& AbsolutePath)
{
    FString UniquePath = AbsolutePath;

    uint32 Counter = 1;
    while (IFileManager::Get().DirectoryExists(*UniquePath))
    {
        UniquePath = FString::Printf(TEXT("%s_%d"), *AbsolutePath, Counter);
        Counter++;
    }

    return UniquePath;
}

bool Mythica::ExportMesh(UStaticMesh* Mesh, const FString& ExportPath)
{
    FString TempFolder = FPaths::Combine(FPaths::GetPath(ExportPath), "USDExport");
    FString UniqueTempFolder = MakeUniquePath(TempFolder);
    FString USDPath = FPaths::Combine(UniqueTempFolder, "Export.usd");

    UStaticMeshExporterUSDOptions* StaticMeshOptions = NewObject<UStaticMeshExporterUSDOptions>();
    StaticMeshOptions->StageOptions.MetersPerUnit = 1.0f;
    StaticMeshOptions->StageOptions.UpAxis = EUsdUpAxis::YAxis;

    UAssetExportTask* ExportTask = NewObject<UAssetExportTask>();
    FGCObjectScopeGuard ExportTaskGuard(ExportTask);
    ExportTask->Object = Mesh;
    ExportTask->Options = StaticMeshOptions;
    ExportTask->Exporter = nullptr;
    ExportTask->Filename = USDPath;
    ExportTask->bSelected = false;
    ExportTask->bReplaceIdentical = true;
    ExportTask->bPrompt = false;
    ExportTask->bUseFileArchive = false;
    ExportTask->bWriteEmptyFiles = false;
    ExportTask->bAutomated = true;

    if (!UExporter::RunAssetExportTask(ExportTask))
    {
        return false;
    }

    return ConvertUSDtoUSDZ(USDPath, ExportPath);
}

bool Mythica::ExportActors(const TArray<AActor*> Actors, const FString& ExportPath)
{
    for (AActor* Actor : Actors)
    {
        USplineComponent* SplineComponent = Actor->FindComponentByClass<USplineComponent>();
        if (SplineComponent)
        {
            return ExportSpline(SplineComponent, ExportPath);
        }
    }

    FString TempFolder = FPaths::Combine(FPaths::GetPath(ExportPath), "USDExport");
    FString UniqueTempFolder = MakeUniquePath(TempFolder);
    FString USDPath = FPaths::Combine(UniqueTempFolder, "Export.usd");

    GEditor->SelectNone(false, true, false);
    for (AActor* Actor : Actors)
    {
        GEditor->SelectActor(Actor, true, true, true);
    }
    GEditor->NoteSelectionChange();

    ULevelExporterUSDOptions* LevelOptions = GetMutableDefault<ULevelExporterUSDOptions>();
    LevelOptions->StageOptions.MetersPerUnit = 1.0f;
    LevelOptions->StageOptions.UpAxis = EUsdUpAxis::YAxis;

    UAssetExportTask* ExportTask = NewObject<UAssetExportTask>();
    FGCObjectScopeGuard ExportTaskGuard(ExportTask);
    ExportTask->Object = GWorld;
    ExportTask->Options = LevelOptions;
    ExportTask->Exporter = nullptr;
    ExportTask->Filename = USDPath;
    ExportTask->bSelected = true;
    ExportTask->bReplaceIdentical = true;
    ExportTask->bPrompt = false;
    ExportTask->bUseFileArchive = false;
    ExportTask->bWriteEmptyFiles = false;
    ExportTask->bAutomated = true;

    if (!UExporter::RunAssetExportTask(ExportTask))
    {
        return false;
    }

    return ConvertUSDtoUSDZ(USDPath, ExportPath);
}

bool Mythica::ExportSpline(USplineComponent* SplineComponent, const FString& ExportPath)
{
    FString TempFolder = FPaths::Combine(FPaths::GetPath(ExportPath), "USDExport");
    FString UniqueTempFolder = MakeUniquePath(TempFolder);
    FString USDPath = FPaths::Combine(UniqueTempFolder, "Export.usd");

    // Gather point data
    int NumPoints = SplineComponent->GetNumberOfSplinePoints();

    pxr::VtArray<pxr::GfVec3f> UsdPoints;
    UsdPoints.reserve(NumPoints);

    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector Point = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        pxr::GfVec3f UsdPoint(Point.X, Point.Y, Point.Z);
        UsdPoints.push_back(UsdPoint);
    }

    // Create curve primitive
    UE::FUsdStage Stage = UnrealUSDWrapper::NewStage(*USDPath);
    if (!Stage)
    {
        return false;
    }

    pxr::UsdGeomBasisCurves Curves = pxr::UsdGeomBasisCurves::Define(Stage, pxr::SdfPath("/SplineCurve"));

    Curves.CreateCurveVertexCountsAttr().Set(pxr::VtArray<int>{NumPoints});
    Curves.CreatePointsAttr().Set(UsdPoints);

    Curves.CreateTypeAttr().Set(pxr::TfToken("cubic"));
    Curves.CreateBasisAttr().Set(pxr::TfToken("bezier"));
    Curves.CreateWrapAttr().Set(pxr::TfToken("nonperiodic"));

    Stage.GetRootLayer().Save();

    return ConvertUSDtoUSDZ(USDPath, ExportPath);
}
