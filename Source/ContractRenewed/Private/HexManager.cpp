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

    // --- Center point for radial falloff ---
    const float GridHalfWidth = GridWidth * Settings->TileHorizontalOffset * 0.5f;
    const float GridHalfHeight = GridHeight * Settings->TileVerticalOffset * 0.5f;
    const FVector2D Center(GridHalfWidth, GridHalfHeight);
    const float MaxDist = FMath::Min(GridHalfWidth, GridHalfHeight) * 0.95f; // keeps circular radius a bit inside bounds

    for (int32 y = 0; y < GridHeight; ++y)
    {
        for (int32 x = 0; x < GridWidth; ++x)
        {
            const bool bOddRow = (y % 2 == 1);
            const float XPos = bOddRow
                ? (x * Settings->TileHorizontalOffset) + Settings->OddRowHorizontalOffset
                : x * Settings->TileHorizontalOffset;
            const float YPos = y * Settings->TileVerticalOffset;

            // --- Distance from center ---
            const float Dist = FVector2D::Distance(FVector2D(XPos, YPos), Center);

            // --- Skip tiles completely outside circular island ---
            if (Dist > MaxDist * 1.05f)
                continue; // do not create any tile here

            // --- Base noise ---
            const float NoiseValue = NoiseWrapper->GetNoise2D(XPos, YPos);

            // --- Radial falloff ---
            const float Falloff = FMath::Clamp(1.0f - (Dist / MaxDist), 0.0f, 1.0f);

            // --- Blend noise and falloff ---
            const float HeightValue = (NoiseValue * 0.4f + Falloff * 0.6f) * HeightStrength;

            const FVector LocalPos(XPos, YPos, HeightValue);
            const FVector WorldPos = GetActorLocation() + LocalPos;
            TilePositions.Add(WorldPos);

            // --- Choose sand or water mesh ---
            UInstancedStaticMeshComponent* MeshComp = (HeightValue >= 0.f) ? GrassMeshComp : WaterMeshComp;
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

    TMap<int32, int32> TileStackCounts;
    TSet<int32> UsedTiles;

    // --- Precompute island center and radius for bias ---
    const FVector2D Center(
        GridWidth * Settings->TileHorizontalOffset * 0.5f,
        GridHeight * Settings->TileVerticalOffset * 0.5f
    );
    const float MaxRadius = FMath::Min(GridWidth, GridHeight) * Settings->TileHorizontalOffset * 0.5f;

    for (const FSpawnableData& Data : InSpawnables)
    {
        if (!Data.ActorClass || Data.SpawnAmount <= 0) continue;

        for (int32 i = 0; i < Data.SpawnAmount; ++i)
        {
            int32 TileIndex = -1;
            const int32 MaxAttempts = 200;

            // --- Pick a valid central tile ---
            for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
            {
                const int32 Candidate = FMath::RandRange(0, TilePositions.Num() - 1);
                const FVector TilePos = TilePositions[Candidate];

                if (UsedTiles.Contains(Candidate))
                    continue;

                // Skip near-water tiles
                if (TilePos.Z < 20.f)
                    continue;

                // Bias toward center (ignore outer 25%)
                const FVector2D TileXY(TilePos.X, TilePos.Y);
                const float DistFromCenter = FVector2D::Distance(TileXY, Center);
                if (DistFromCenter > MaxRadius * 0.75f)
                    continue;

                // Accept candidate
                TileIndex = Candidate;
                UsedTiles.Add(TileIndex);
                break;
            }

            // Fallback if none found
            if (TileIndex == -1)
            {
                TileIndex = FMath::RandRange(0, TilePositions.Num() - 1);
                UsedTiles.Add(TileIndex);
            }

            FVector BasePos = TilePositions[TileIndex];
            float HeightOffset = FMath::FRandRange(Data.MinHeightOffset, Data.MaxHeightOffset);

            // --- NATURAL PLACEMENT LOGIC ---
            if (Data.bNaturalPlacement)
            {
                int32 ClusterSize = FMath::RandRange(2, 5);
                FVector ClusterCenter = BasePos;

                for (int32 j = 0; j < ClusterSize; ++j)
                {
                    FVector ClusterOffset = FMath::VRand() * FMath::RandRange(80.f, 150.f);
                    FVector SpawnLoc = ClusterCenter + ClusterOffset + FVector(0, 0, HeightOffset);
                    FRotator SpawnRot = Data.bRandomRotate
                        ? FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f)
                        : FRotator::ZeroRotator;

                    // Glitch placement
                    if (FMath::FRand() < 0.1f)
                    {
                        SpawnLoc.Z -= 50.f;
                        SpawnRot.Pitch += FMath::RandRange(-30.f, 30.f);
                        SpawnRot.Roll += FMath::RandRange(-15.f, 15.f);
                    }

                    // --- Align to terrain ---
                    FHitResult Hit;
                    FVector TraceStart = SpawnLoc + FVector(0, 0, 500.f);
                    FVector TraceEnd = SpawnLoc - FVector(0, 0, 1000.f);

                    if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic))
                    {
                        SpawnLoc = Hit.ImpactPoint;

                        // Rotation based on surface normal + random yaw
                        FRotator SurfaceRot = Hit.ImpactNormal.Rotation();
                        float RandomYaw = FMath::FRandRange(0.f, 360.f);
                        FRotator RandomRot(0.f, RandomYaw, 0.f);
                        SpawnRot = (SurfaceRot.Quaternion() * RandomRot.Quaternion()).Rotator();

                        // Reduce tilt so props stay upright
                        SpawnRot.Pitch *= 0.5f;
                        SpawnRot.Roll *= 0.5f;
                    }
                    else
                    {
                        // Fallback if no hit
                        SpawnLoc.Z += HeightOffset;
                    }

                    if (AActor* Spawned = World->SpawnActor<AActor>(Data.ActorClass, SpawnLoc, SpawnRot))
                    {
                        SpawnedActors.Add(Spawned);
                    }
                }
            }
            else
            {
                // --- DEFAULT SIMPLE SPAWN ---
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
