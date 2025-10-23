#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexTile.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "HexGridSettings.h"
#include "FastNoiseWrapper.h"
#include "Actors/HopperBaseCharacter.h"
#include "NavigationSystem.h"
#include "HexManager.generated.h"

USTRUCT(BlueprintType)
struct FSpawnableData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Spawn")
    TSubclassOf<AActor> ActorClass;

    UPROPERTY(EditAnywhere, Category = "Spawn")
    float MinHeightOffset = 100.f;

    UPROPERTY(EditAnywhere, Category = "Spawn")
    float MaxHeightOffset = 200.f;

    UPROPERTY(EditAnywhere, Category = "Spawn")
    bool bAllowStacking = false;

    UPROPERTY(EditAnywhere, Category = "Spawn", meta = (EditCondition = "bAllowStacking", ClampMin = "0.0", ClampMax = "1.0"))
    float StackChance = 0.25f;

    UPROPERTY(EditAnywhere, Category="Spawn")
    bool bRandomRotate = true;

    UPROPERTY(EditAnywhere, Category="Spawn|Behavior")
    bool bNaturalPlacement = false;

        // --- Spawn amount control ---
    UPROPERTY(EditAnywhere, Category = "Spawn|Amount")
    int32 MinSpawnAmount = 1;

    UPROPERTY(EditAnywhere, Category = "Spawn|Amount")
    int32 MaxSpawnAmount = 5;

    // --- Natural placement cluster control ---
    UPROPERTY(EditAnywhere, Category = "Spawn|Behavior", meta=(EditCondition="bNaturalPlacement"))
    int32 MinClusterSize = 2;

    UPROPERTY(EditAnywhere, Category = "Spawn|Behavior", meta=(EditCondition="bNaturalPlacement"))
    int32 MaxClusterSize = 5;

    UPROPERTY(EditAnywhere, Category="Spawn|Behavior")
    bool bSetsTargetCoinCount = false;
};

UCLASS()
class CONTRACTRENEWED_API AHexManager : public AActor
{
    GENERATED_BODY()

public:
    AHexManager();

     UPROPERTY(VisibleAnywhere, Category="HexGrid|Setup")
    int32 TargetCoinCount = 10;

    UPROPERTY(EditAnywhere, Category="HexGrid|Materials")
    UMaterialInstance* GlitchMaterialInstance;

protected:
    virtual void BeginPlay() override;

    void DestroyTiles();

        // --- Island Gap Options ---
    UPROPERTY(EditAnywhere, Category="HexGrid|Gaps")
    bool bCreateRandomGap = false;

    UPROPERTY(EditAnywhere, Category="HexGrid|Gaps", meta=(EditCondition="bCreateRandomGap"))
    float GapRadius = 400.f;

    UPROPERTY(EditAnywhere, Category="HexGrid|Gaps", meta=(EditCondition="bCreateRandomGap"))
    float GapDepth = -1000.f; // used only for reference, not spawning

    UFUNCTION(BlueprintCallable, Category = "HexGrid|Testing")
    void GenerateHexGrid();

    void SpawnEnemiesAfterNavMeshReady();

    // Unified spawn system
    UFUNCTION(BlueprintCallable, Category = "HexGrid")
    void SpawnAllActors(const TArray<FSpawnableData>& InSpawnables);

    UFUNCTION(BlueprintCallable, Category = "HexGrid")
    int32 GetTargetCoinCount() {return TargetCoinCount;};

    UFUNCTION(BlueprintCallable, Category = "HexGrid|Testing")
    void SpawnAllActorsInEditor();

    // --- Tile & Grid Data ---
    UPROPERTY(EditAnywhere, Category = "HexGrid|Layout")
    int32 GridWidth = 3;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Layout")
    int32 GridHeight = 5;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Setup")
    float HeightStrength = 1.f;

    UPROPERTY(EditAnywhere, Category = "HexGrid")
    UStaticMesh* GrassMesh;

    UPROPERTY(EditAnywhere, Category = "HexGrid")
    UStaticMesh* WaterMesh;

    UPROPERTY(VisibleDefaultsOnly, Category = "Hex", meta = (AllowPrivateAccess = "true"))
    UHierarchicalInstancedStaticMeshComponent* GrassMeshComp;

    UPROPERTY(VisibleDefaultsOnly, Category = "Hex", meta = (AllowPrivateAccess = "true"))
    UHierarchicalInstancedStaticMeshComponent* WaterMeshComp;

    // --- Spawning Data ---
    UPROPERTY(EditAnywhere, Category = "Spawning")
    TArray<FSpawnableData> Spawnables; // replaces PickupSpawnData, PropActors, EnemyTypes arrays

    UPROPERTY()
    TArray<AActor*> SpawnedActors;

    // --- Noise Settings ---
    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    EFastNoise_NoiseType NoiseType = EFastNoise_NoiseType::Simplex;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    int32 Seed = 1337;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    float Frequency = 0.01f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    EFastNoise_Interp Interp = EFastNoise_Interp::Quintic;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    EFastNoise_FractalType Fractaltype = EFastNoise_FractalType::FBM;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    int32 Octaves = 3;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    float Gain = 0.5f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    float CellularJitter = 0.45f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    EFastNoise_CellularDistanceFunction CellularDistanceFunction = EFastNoise_CellularDistanceFunction::Euclidean;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Noise")
    EFastNoise_CellularReturnType CellularReturnType = EFastNoise_CellularReturnType::CellValue;

private:
    UPROPERTY()
    UHexGridSettings* Settings;
    TArray<FVector> TilePositions;
};
