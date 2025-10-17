#include "HexManager.h"
#include "HexTile.h"
#include "DrawDebugHelpers.h"
#include "HexGridSubsystem.h"

AHexManager::AHexManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bRunConstructionScriptOnDrag = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));

	GrassMeshComp = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassMeshComp"));
	GrassMeshComp->SetupAttachment(RootComponent);

	WaterMeshComp = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WaterMeshComp"));
	WaterMeshComp->SetupAttachment(RootComponent);

	GrassMeshComp->SetMobility(EComponentMobility::Movable);
	WaterMeshComp->SetMobility(EComponentMobility::Movable);

	Settings = GetMutableDefault<UHexGridSettings>();
	check(Settings);
}

void AHexManager::DestroyTiles()
{
	if (!IsValid(GrassMeshComp) || !IsValid(WaterMeshComp))
		return;

	if (WaterMeshComp)
	{
		WaterMeshComp->ClearInstances();
	}
	if (GrassMeshComp)
	{
		GrassMeshComp->ClearInstances();
	}

	// Cleanup previously spawned actors
	for (AActor* Spawned : SpawnedActors)
	{
		if (IsValid(Spawned))
		{
			Spawned->Destroy();
		}
	}
	SpawnedActors.Empty();
}

void AHexManager::GenerateHexGrid()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	UHexGridSubsystem* Subsystem = World->GetSubsystem<UHexGridSubsystem>();
	if (!Subsystem)
		return;

	UFastNoiseWrapper* NoiseWrapperLvl1 = Subsystem->NoiseWrapperLvl1;
	if (!NoiseWrapperLvl1)
		return;

	if (!GrassMesh || !WaterMesh)
		return;

	GrassMeshComp->SetStaticMesh(GrassMesh);
	WaterMeshComp->SetStaticMesh(WaterMesh);

	NoiseWrapperLvl1->SetupFastNoise(
		NoiseType, Seed, Frequency, Interp, Fractaltype,
		Octaves, Lacunarity, Gain, CellularJitter,
		CellularDistanceFunction, CellularReturnType);

	if (!NoiseWrapperLvl1->IsInitialized())
		return;

	DestroyTiles();

	for (int32 y = 0; y < GridHeight; y++)
	{
		for (int32 x = 0; x < GridWidth; x++)
		{
			const bool oddRow = y % 2 == 1;
			const float xPos = oddRow ? (x * Settings->TileHorizontalOffset) + Settings->OddRowHorizontalOffset
				: x * Settings->TileHorizontalOffset;
			const float yPos = y * Settings->TileVerticalOffset;

			UInstancedStaticMeshComponent* currentComp = nullptr;
			float randomHeight = 0.f;
			const float randomNoise = NoiseWrapperLvl1->GetNoise2D(xPos, yPos);

			if (randomNoise >= 0.f)
			{
				currentComp = GrassMeshComp;
				randomHeight = randomNoise;
			}
			else
			{
				currentComp = WaterMeshComp;
				randomHeight = randomNoise * 0.5f;
			}

			if (!currentComp)
				continue;

			// Local and world positions (computed once)
			const FVector localPos(xPos, yPos, randomHeight * HeightStrength);
			const FVector worldPos = GetActorLocation() + localPos;

			// Add the instance (local space)
			currentComp->AddInstance(FTransform(localPos));

			// One random roll for all spawns on this tile
			const float roll = FMath::FRand();

			// --- Enemies ---
			if (EnemyTypes.Num() > 0 && roll < EnemySpawnChance)
			{
				int32 Index = FMath::RandRange(0, EnemyTypes.Num() - 1);
				TSubclassOf<AHopperBaseCharacter> EnemyClass = EnemyTypes[Index];
				if (EnemyClass)
				{
					FTransform SpawnTransform(FRotator::ZeroRotator, worldPos + FVector(0.f, 0.f, 150.f));
					AActor* Spawned = World->SpawnActor<AActor>(EnemyClass, SpawnTransform);
					if (Spawned)
						SpawnedActors.Add(Spawned);
				}
			}
			// --- Pickups ---
			else if (PickupActors.Num() > 0 && roll < EnemySpawnChance + PickupSpawnChance)
			{
				int32 Index = FMath::RandRange(0, PickupActors.Num() - 1);
				TSubclassOf<AActor> PickupClass = PickupActors[Index];
				if (PickupClass)
				{
					FTransform SpawnTransform(FRotator::ZeroRotator, worldPos + FVector(0.f, 0.f, 100.f));
					AActor* Spawned = World->SpawnActor<AActor>(PickupClass, SpawnTransform);
					if (Spawned)
						SpawnedActors.Add(Spawned);
				}
			}
			// --- Props (trees, rocks, etc.) ---
			else if (PropActors.Num() > 0 && roll < EnemySpawnChance + PickupSpawnChance + PropSpawnChance)
			{
				int32 Index = FMath::RandRange(0, PropActors.Num() - 1);
				TSubclassOf<AActor> PropClass = PropActors[Index];
				if (PropClass)
				{
					FTransform SpawnTransform(FRotator::ZeroRotator, worldPos + FVector(0.f, 0.f, 80.f));
					AActor* Spawned = World->SpawnActor<AActor>(PropClass, SpawnTransform);
					if (Spawned)
						SpawnedActors.Add(Spawned);
				}
			}
		}
	}
}

void AHexManager::GenerateNewLoop(
	int32 InGridW,
	int32 InGridH,
	float InEnemySpawnChance,
	float InPickupSpawnChance,
	float InPropSpawnChance,
	EFastNoise_NoiseType InNoiseType,
	int32 InSeed,
	float InFrequency,
	int32 InOctaves,
	float InLacunarity,
	float InGain,
	float InCellularJitter)
{
	GridWidth = InGridW;
	GridHeight = InGridH;

	EnemySpawnChance = InEnemySpawnChance;
	PickupSpawnChance = InPickupSpawnChance;
	PropSpawnChance = InPropSpawnChance;

	NoiseType = InNoiseType;
	Seed = InSeed;
	Frequency = InFrequency;
	Octaves = InOctaves;
	Lacunarity = InLacunarity;
	Gain = InGain;
	CellularJitter = InCellularJitter;
	GenerateHexGrid();
}
