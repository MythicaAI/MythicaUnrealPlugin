#pragma once

#include "MythicaUSDUtil.h"

#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Components/SplineComponent.h"
#include "Exporters/Exporter.h"
#include "IPythonScriptPlugin.h"
#include "LevelExporterUSDOptions.h"
#include "Selection.h"
#include "StaticMeshExporterUSDOptions.h"
#include "UObject/GCObjectScopeGuard.h"
#include "UnrealUSDWrapper.h"
#include "USDConversionUtils.h"
#include "USDStageImportOptions.h"
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

    FString Command;
    Command.Append(TEXT("from mythica import usd_util\n"));
    Command.Appendf(TEXT("usd_util.convert_usd_to_usdz('%s', '%s')\n"), *InFile, *OutFile);

    IPythonScriptPlugin::Get()->ExecPythonCommand(*Command);
    return true;
}

static bool CreateOffsetScene(const FString& InFile, const FString& OutFile, const FVector& Offset)
{
    if (!IPythonScriptPlugin::Get()->IsPythonAvailable())
    {
        return false;
    }

    FString Command;
    Command.Append(TEXT("from mythica import usd_util\n"));
    Command.Appendf(TEXT("usd_util.create_offset_scene('%s', '%s', (%f, %f, %f))\n"), *InFile, *OutFile, Offset.X, Offset.Y, Offset.Z);

    IPythonScriptPlugin::Get()->ExecPythonCommand(*Command);
    return true;
}

bool Mythica::ExportMesh(UStaticMesh* Mesh, const FString& ExportPath)
{
    FString TempFolder = FPaths::Combine(FPaths::GetPath(ExportPath), "USDExport");
    FString USDPath = FPaths::Combine(TempFolder, "Export.usd");

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

bool Mythica::ExportActors(const TArray<AActor*> Actors, const FString& ExportPath, const FVector& Origin)
{
    FString TempFolder = FPaths::Combine(FPaths::GetPath(ExportPath), "USDExport");
    FString USDPath = FPaths::Combine(TempFolder, "Export.usd");

    // Save original selection
    TArray<AActor*> OriginalSelection;
    TArray<UActorComponent*> OriginalComponentSelection;
    GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(OriginalSelection);
    GEditor->GetSelectedComponents()->GetSelectedObjects<UActorComponent>(OriginalComponentSelection);

    // Set selection to target actors
    GEditor->SelectNone(false, false, false);
    for (AActor* Actor : Actors)
    {
        GEditor->SelectActor(Actor, true, false, true);
    }

    // Export selected actors
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

    bool PrevSilent = GIsSilent;
    GIsSilent = true;
    bool Success = UExporter::RunAssetExportTask(ExportTask);
    GIsSilent = PrevSilent;

    // Restore original selection
    GEditor->SelectNone(false, false, false);
    for (AActor* Actor : OriginalSelection)
    {
        GEditor->SelectActor(Actor, true, false, true);
    }
    for (UActorComponent* Component : OriginalComponentSelection)
    {
        GEditor->SelectComponent(Component, true, false, true);
    }
    GEditor->NoteSelectionChange();

    // Modify scene to be relative to the desired origin
    FString OffsetUSDPath = FPaths::Combine(TempFolder, "Export_Offset.usd");
    FVector USDOrigin(Origin.X / 100.0f, Origin.Z / 100.0f, Origin.Y / 100.0f);
    if (!CreateOffsetScene(USDPath, OffsetUSDPath, -USDOrigin))
    {
        return false;
    }

    return Success ? ConvertUSDtoUSDZ(OffsetUSDPath, ExportPath) : false;
}

bool Mythica::ExportSpline(AActor* SplineActor, const FString& ExportPath, const FVector& Origin)
{
    FString TempFolder = FPaths::Combine(FPaths::GetPath(ExportPath), "USDExport");
    FString USDPath = FPaths::Combine(TempFolder, "Export.usd");

    USplineComponent* SplineComponent = SplineActor->FindComponentByClass<USplineComponent>();
    if (!SplineComponent)
    {
        return false;
    }

    // Gather point data
    int NumPoints = SplineComponent->GetNumberOfSplinePoints();

    pxr::VtArray<pxr::GfVec3f> Points;
    Points.reserve(NumPoints);

    for (int i = 0; i < NumPoints; ++i)
    {
        // Unreal: Z-up, left handed, 1cm per unit
        // USD: Y-up right handed, 1m per unit
        FVector Point = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector RelativePoint = Point - Origin;
        pxr::GfVec3f UsdPoint(RelativePoint.X / 100.0f, -RelativePoint.Z / 100.0f, RelativePoint.Y / 100.0f);
        Points.push_back(UsdPoint);
    }

    // Create curve primitive
    UE::FUsdStage Stage = UnrealUSDWrapper::NewStage(*USDPath);
    if (!Stage)
    {
        return false;
    }

    UsdUtils::SetUsdStageMetersPerUnit(Stage, 1.0f);
    UsdUtils::SetUsdStageUpAxis(Stage, pxr::TfToken("Y"));

    pxr::UsdGeomBasisCurves Curves = pxr::UsdGeomBasisCurves::Define(Stage, pxr::SdfPath("/SplineCurve"));

    Curves.CreateCurveVertexCountsAttr().Set(pxr::VtArray<int>{NumPoints});
    Curves.CreatePointsAttr().Set(Points);

    Curves.CreateTypeAttr().Set(pxr::TfToken("cubic"));
    Curves.CreateBasisAttr().Set(pxr::TfToken("bezier"));
    Curves.CreateWrapAttr().Set(pxr::TfToken("nonperiodic"));

    Stage.GetRootLayer().Save();

    return ConvertUSDtoUSDZ(USDPath, ExportPath);
}

bool Mythica::ImportMesh(const FString& FilePath, const FString& ImportDirectory)
{
    // Setup import options
    UUsdStageImportOptions* ImportOptions = NewObject<UUsdStageImportOptions>();
    FGCObjectScopeGuard OptionsGuard(ImportOptions);

    ImportOptions->bImportActors = false;
    ImportOptions->bImportGeometry = true;
    ImportOptions->bImportSkeletalAnimations = false;
    ImportOptions->bImportLevelSequences = false;
    ImportOptions->bImportMaterials = true;
    ImportOptions->bImportGroomAssets = false;
    ImportOptions->bImportSparseVolumeTextures = false;
    ImportOptions->bImportOnlyUsedMaterials = false;

    ImportOptions->PrimsToImport = TArray<FString>{ TEXT("/") };
    ImportOptions->PurposesToImport = (int32)(EUsdPurpose::Default | EUsdPurpose::Proxy | EUsdPurpose::Render | EUsdPurpose::Guide);
    ImportOptions->NaniteTriangleThreshold = INT32_MAX;
    ImportOptions->RenderContextToImport = NAME_None;
    ImportOptions->MaterialPurpose = *UnrealIdentifiers::MaterialPreviewPurpose;
    ImportOptions->RootMotionHandling = EUsdRootMotionHandling::NoAdditionalRootMotion;
    ImportOptions->SubdivisionLevel = 0;
    ImportOptions->MetadataOptions = FUsdMetadataImportOptions{
        true,  /* bCollectMetadata */
        true,  /* bCollectFromEntireSubtrees */
        false, /* bCollectOnComponents */
        {},    /* BlockedPrefixFilters */
        false  /* bInvertFilters */
    };
    ImportOptions->bOverrideStageOptions = false;
    ImportOptions->StageOptions.MetersPerUnit = 0.01f;
    ImportOptions->StageOptions.UpAxis = EUsdUpAxis::ZAxis;
    ImportOptions->bImportAtSpecificTimeCode = false;
    ImportOptions->ImportTimeCode = 0.0f;

    ImportOptions->GroomInterpolationSettings = TArray<FHairGroupsInterpolation>();
    ImportOptions->ExistingActorPolicy = EReplaceActorPolicy::Replace;
    ImportOptions->ExistingAssetPolicy = EReplaceAssetPolicy::Replace;
    ImportOptions->bReuseIdenticalAssets = true;

    ImportOptions->bPrimPathFolderStructure = false;
    ImportOptions->KindsToCollapse = (int32)(EUsdDefaultKind::Component | EUsdDefaultKind::Subcomponent);
    ImportOptions->bMergeIdenticalMaterialSlots = true;
    ImportOptions->bInterpretLODs = true;

    // Import mesh
    UAssetImportTask* Task = NewObject<UAssetImportTask>();
    FGCObjectScopeGuard TaskGuard(Task);

    Task->bAutomated = true;
    Task->bReplaceExisting = true;
    Task->DestinationPath = ImportDirectory;
    Task->bSave = false;
    Task->Options = ImportOptions;
    Task->Filename = FilePath;
    
    TArray<UAssetImportTask*> Tasks;
    Tasks.Add(Task);

    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

    bool PrevSilent = GIsSilent;
    GIsSilent = true;
    AssetToolsModule.Get().ImportAssetTasks(Tasks);
    GIsSilent = PrevSilent;

    return Task->GetObjects().Num() > 0;
}
