#include "HexManager.h"
#include "HexGridSubsystem.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "AIController.h"

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

void AHexManager::BeginPlay()
{
    Super::BeginPlay();
}

void AHexManager::DestroyTiles()
{
    if (!IsValid(GrassMeshComp) || !IsValid(WaterMeshComp))
        return;

    GrassMeshComp->ClearInstances();
    WaterMeshComp->ClearInstances();

    for (AActor* Spawned : SpawnedActors)
    {
        if (IsValid(Spawned))
            Spawned->Destroy();
    }

    SpawnedActors.Empty();
    TilePositions.Empty();
}

void AHexManager::GenerateHexGrid()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UHexGridSubsystem* Subsystem = World->GetSubsystem<UHexGridSubsystem>();
    if (!Subsystem) return;

    UFastNoiseWrapper* NoiseWrapper = Subsystem->NoiseWrapperLvl1;
    if (!NoiseWrapper || !GrassMesh || !WaterMesh) return;

    GrassMeshComp->SetStaticMesh(GrassMesh);
    WaterMeshComp->SetStaticMesh(WaterMesh);

    NoiseWrapper->SetupFastNoise(
        NoiseType, Seed, Frequency, Interp, Fractaltype,
        Octaves, Lacunarity, Gain, CellularJitter,
        CellularDistanceFunction, CellularReturnType);

    if (!NoiseWrapper->IsInitialized()) return;

    DestroyTiles();

    for (int32 y = 0; y < GridHeight; ++y)
    {
        for (int32 x = 0; x < GridWidth; ++x)
        {
            const bool bOddRow = (y % 2 == 1);
            const float XPos = bOddRow
                ? (x * Settings->TileHorizontalOffset) + Settings->OddRowHorizontalOffset
                : x * Settings->TileHorizontalOffset;
            const float YPos = y * Settings->TileVerticalOffset;

            const float NoiseValue = NoiseWrapper->GetNoise2D(XPos, YPos);
            const FVector LocalPos(XPos, YPos, NoiseValue * HeightStrength);
            const FVector WorldPos = GetActorLocation() + LocalPos;
            TilePositions.Add(WorldPos);

            UInstancedStaticMeshComponent* MeshComp = NoiseValue >= 0.f ? GrassMeshComp : WaterMeshComp;
            MeshComp->AddInstance(FTransform(LocalPos));
        }
    }

    // Delay until navmesh is ready
    GetWorldTimerManager().SetTimerForNextTick(this, &AHexManager::SpawnEnemiesAfterNavMeshReady);
}

void AHexManager::SpawnEnemiesAfterNavMeshReady()
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return;

    if (NavSys->IsNavigationBeingBuiltOrLocked(GetWorld()))
    {
        FTimerHandle RetryHandle;
        GetWorldTimerManager().SetTimer(RetryHandle, this, &AHexManager::SpawnEnemiesAfterNavMeshReady, 0.25f, false);
        return;
    }

    // Once navmesh ready, spawn everything
    SpawnAllActorsInEditor();
}

void AHexManager::SpawnAllActors(const TArray<FSpawnableData>& InSpawnables)
{
    UWorld* World = GetWorld();
    if (!World || TilePositions.IsEmpty()) return;

    // Track how many actors are stacked per tile
    TMap<int32, int32> TileStackCounts;
    // Track which tiles are already occupied
    TSet<int32> UsedTiles;

    for (const FSpawnableData& Data : InSpawnables)
    {
        if (!Data.ActorClass || Data.SpawnAmount <= 0) continue;

        for (int32 i = 0; i < Data.SpawnAmount; ++i)
        {
            int32 TileIndex = -1;

            // If stacking chance succeeds and there are already used tiles, pick one to stack on
            if (Data.bAllowStacking && FMath::FRand() < Data.StackChance && UsedTiles.Num() > 0)
            {
                // Pick a random used tile to stack on
                TileIndex = UsedTiles.Array()[FMath::RandRange(0, UsedTiles.Num() - 1)];
            }
            else
            {
                // Pick a completely new tile (not used yet)
                int32 SafetyCounter = 0;
                do
                {
                    TileIndex = FMath::RandRange(0, TilePositions.Num() - 1);
                    SafetyCounter++;
                } while (UsedTiles.Contains(TileIndex) && SafetyCounter < 200);

                if (SafetyCounter >= 200)
                {
                    UE_LOG(LogTemp, Warning, TEXT("SpawnAllActors: ran out of unique tiles for %s"), *GetNameSafe(Data.ActorClass));
                    break;
                }

                UsedTiles.Add(TileIndex);
            }

            // Base position of that tile
            FVector BasePos = TilePositions[TileIndex];
            float HeightOffset = FMath::FRandRange(Data.MinHeightOffset, Data.MaxHeightOffset);

            // Stack on previous ones if tile was already used
            int32& StackCount = TileStackCounts.FindOrAdd(TileIndex);
            if (StackCount > 0)
            {
                HeightOffset += StackCount * 100.f;
            }

            FVector SpawnLoc = BasePos + FVector(0, 0, HeightOffset);
            FRotator SpawnRot = Data.bRandomRotate
                ? FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f)
                : FRotator::ZeroRotator;

            if (AActor* Spawned = World->SpawnActor<AActor>(Data.ActorClass, SpawnLoc, SpawnRot))
            {
                SpawnedActors.Add(Spawned);
                StackCount++;
            }
        }
    }
}

void AHexManager::SpawnAllActorsInEditor()
{
	if (Spawnables.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("No spawn data defined."));
		return;
	}

	SpawnAllActors(Spawnables);
}
