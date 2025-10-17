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
struct FPickupSpawnData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Pickup")
    TSubclassOf<AActor> PickupClass;

    UPROPERTY(EditAnywhere, Category = "Pickup")
    int32 SpawnAmount = 5;

    UPROPERTY(EditAnywhere, Category = "Pickup")
    float MinHeightOffset = 100.f;

    UPROPERTY(EditAnywhere, Category = "Pickup")
    float MaxHeightOffset = 200.f;
};

UCLASS()
class CONTRACTRENEWED_API AHexManager : public AActor
{
    GENERATED_BODY()

public:
    AHexManager();

	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	void GenerateNewLoop(
		int32 InGridW,
		int32 InGridH,
		int32 InEnemySpawnAmount,
		int32 InPropSpawnAmount,
		EFastNoise_NoiseType InNoiseType,
		int32 InSeed,
		float InFrequency,
		int32 InOctaves,
		float InLacunarity,
		float InGain,
		float InCellularJitter,
		TArray<FPickupSpawnData> InPickupSpawnData
	);


protected:
    virtual void BeginPlay() override;

    void DestroyTiles();

    UFUNCTION(CallInEditor, Category="HexGrid|Testing")
    void GenerateHexGrid();
    void SpawnEnemiesAfterNavMeshReady();
    void SpawnEnemies();

    // --- Tile & Grid Data ---
    UPROPERTY(EditAnywhere, Category = "HexGrid|Layout")
    int32 GridWidth = 3;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Layout")
    int32 GridHeight = 5;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Setup")
    float HeightStrength = 1.f;

    UPROPERTY(EditAnywhere, Category="HexGrid")
    UStaticMesh* GrassMesh;

    UPROPERTY(EditAnywhere, Category="HexGrid")
    UStaticMesh* WaterMesh;

    UPROPERTY(VisibleDefaultsOnly, Category="Hex", meta=(AllowPrivateAccess="true"))
    UHierarchicalInstancedStaticMeshComponent* GrassMeshComp;

    UPROPERTY(VisibleDefaultsOnly, Category = "Hex", meta = (AllowPrivateAccess = "true"))
    UHierarchicalInstancedStaticMeshComponent* WaterMeshComp;

    // --- Spawning Data ---
    UPROPERTY(EditAnywhere, Category = "Spawning")
    TArray<TSubclassOf<AHopperBaseCharacter>> EnemyTypes;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<FPickupSpawnData> PickupSpawnData;

    UPROPERTY(EditAnywhere, Category="Spawning")
    TArray<TSubclassOf<AActor>> PropActors;

    void SpawnPickups();
    void SpawnProps();

   UPROPERTY(EditAnywhere, Category = "Enemy")
    int32 EnemySpawnAmount = 5;

    UPROPERTY(EditAnywhere, Category = "Enemy")
    int32 PropSpawnAmount = 5;

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
    UHexGridSettings* Settings;

    TArray<FVector> TilePositions;
};
