// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MythicaPackage.generated.h"

/**
 * Mythica Pacakge Version
 *
 * Each asset can have multiple package versions. The versioning allows us to know which package we want.
 */
USTRUCT(BlueprintType)
struct FMythicaPackageVersion
{

    GENERATED_BODY()

public:

    bool IsValid() const;
    FString ToString() const;

    bool operator<(const FMythicaPackageVersion& Other) const;
    bool operator==(const FMythicaPackageVersion& Other) const;

public:

    UPROPERTY(BlueprintReadOnly, Category = "Version")
    int32 Major = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Version")
    int32 Minor = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Version")
    int32 Patch = 0;

};

/**
 * Mythica Package Version Entry Point Reference
 *
 * 
 */
USTRUCT(BlueprintType)
struct FMythicaPackageVersionEntryPointReference
{

    GENERATED_BODY()

public:

    bool IsValid() const;
    bool Compare(const FMythicaPackageVersionEntryPointReference& Other) const;

public:

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString AssetId;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FMythicaPackageVersion Version;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString FileId;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString FileName;

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString EntryPoint;

};

/**
 * Mythica Package
 *
 * A package is a collection of files that can be downloaded and installed into unreal engine.
 */
USTRUCT(BlueprintType)
struct FMythicaPackage
{

    GENERATED_BODY()

public:

    /** The ID of an asset in the Mythica API that encompasses a collection of versioned packages. */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString AssetId = FString();

    /** The ID of a downloadable, versioned package of files stored in the Mythica API. */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString PackageId = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString Name = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString Description = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString OrgName = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FMythicaPackageVersion Version = FMythicaPackageVersion();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    TArray<FString> Tags = TArray<FString>();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString ThumbnailUrl = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString PackageUrl = FString();

    UPROPERTY(BlueprintReadOnly, Category = "Data")
    int32 DigitalAssetCount = 0;

};
