// Fill out your copyright notice in the Description page of Project Settings.


#include "MythicaPackage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicaPackage)

bool FMythicaPackageVersion::IsValid() const
{
    return Major > 0 || Minor > 0 || Patch > 0;
}

FString FMythicaPackageVersion::ToString() const
{
    return IsValid() ? FString::Printf(TEXT("%d.%d.%d"), Major, Minor, Patch) : "";
}

bool FMythicaPackageVersion::operator<(const FMythicaPackageVersion& Other) const
{
    return Major < Other.Major
        || (Major == Other.Major && Minor < Other.Minor)
        || (Major == Other.Major && Minor == Other.Minor && Patch < Other.Patch);
}

bool FMythicaPackageVersion::operator==(const FMythicaPackageVersion& Other) const
{
    return Major == Other.Major && Minor == Other.Minor && Patch == Other.Patch;
}



bool FMythicaPackageVersionEntryPointReference::IsValid() const
{
    return !AssetId.IsEmpty();
}

bool FMythicaPackageVersionEntryPointReference::Compare(const FMythicaPackageVersionEntryPointReference& Other) const
{
    return AssetId == Other.AssetId
        && FileName == Other.FileName
        && EntryPoint == Other.EntryPoint;
}
