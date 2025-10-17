#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexTile.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "HexGridSettings.h"
#include "FastNoiseWrapper.h"
#include "Actors/HopperBaseCharacter.h"
#include "HexManager.generated.h"

struct FDebugInfo
{
    FString Info;
    FVector Location;
};

UCLASS()
class CONTRACTRENEWED_API AHexManager : public AActor
{
    GENERATED_BODY()

public:
    AHexManager();

protected:

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TArray<TSubclassOf<AHopperBaseCharacter>> EnemyTypes;

    UPROPERTY(EditAnywhere, Category="Spawning")
    TArray<TSubclassOf<AActor>> PickupActors; 

	UPROPERTY()
	TArray<AActor*> SpawnedActors;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float EnemySpawnChance = 0.03f;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float PickupSpawnChance = 0.04f;

    UPROPERTY(EditAnywhere, Category="Spawning")
    TArray<TSubclassOf<AActor>> PropActors;

	UPROPERTY(EditAnywhere, Category = "Spawning")
    float PropSpawnChance = 0.10f;

    void DestroyTiles();

    UPROPERTY(EditAnywhere, Category = "HexGrid|Layout")
    int32 GridWidth = 3;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Layout")
    int32 GridHeight = 5;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Setup")
    float ChanceOfWater = 0.f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|Setup")
    float HeightStrength = 1.f;

    UFUNCTION(CallInEditor, Category = "HexGrid")
    void GenerateHexGrid();

	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	void GenerateNewLoop(
		int32 InGridW = 5,
		int32 InGridH = 5,
		float InEnemySpawnChance = 0.03f,
		float InPickupSpawnChance = 0.04f,
		float InPropSpawnChance = 0.10f,
		EFastNoise_NoiseType InNoiseType = EFastNoise_NoiseType::Simplex,
		int32 InSeed = 1337,
		float InFrequency = 0.01f,
		int32 InOctaves = 3,
		float InLacunarity = 2.0f,
		float InGain = 0.5f,
		float InCellularJitter = 0.45f);


    UPROPERTY(EditAnywhere, Category="HexGrid")
    UStaticMesh* GrassMesh;

    UPROPERTY(EditAnywhere, Category="HexGrid")
    UStaticMesh* WaterMesh;

    UPROPERTY(VisibleDefaultsOnly, Category="Hex", meta=(AllowPrivateAccess="true"))
    UHierarchicalInstancedStaticMeshComponent* GrassMeshComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Hex", meta = (AllowPrivateAccess = "true"))
	UHierarchicalInstancedStaticMeshComponent* WaterMeshComp;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    EFastNoise_NoiseType NoiseType = EFastNoise_NoiseType::Simplex;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    int32 Seed = 1337;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    float Frequency = 0.01f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    EFastNoise_Interp Interp = EFastNoise_Interp::Quintic;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    EFastNoise_FractalType Fractaltype = EFastNoise_FractalType::FBM;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    int32 Octaves = 3;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    float Gain = 0.5f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    float CellularJitter = 0.45f;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    EFastNoise_CellularDistanceFunction CellularDistanceFunction = EFastNoise_CellularDistanceFunction::Euclidean;

    UPROPERTY(EditAnywhere, Category = "HexGrid|NoiseLvl1")
    EFastNoise_CellularReturnType CellularReturnType = EFastNoise_CellularReturnType::CellValue;

private:

    UHexGridSettings* Settings;
};
