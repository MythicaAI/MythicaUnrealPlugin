#pragma once

#include "MythicaUSDUtil.h"

#include "Exporters/Exporter.h"
#include "IPythonScriptPlugin.h"
#include "LevelExporterUSDOptions.h"
#include "StaticMeshExporterUSDOptions.h"
#include "UObject/GCObjectScopeGuard.h"

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

