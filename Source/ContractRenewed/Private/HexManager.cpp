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

    UFastNoiseWrapper* NoiseWrapperLvl1 = Subsystem->NoiseWrapperLvl1;
    if (!NoiseWrapperLvl1 || !GrassMesh || !WaterMesh) return;

    GrassMeshComp->SetStaticMesh(GrassMesh);
    WaterMeshComp->SetStaticMesh(WaterMesh);

    NoiseWrapperLvl1->SetupFastNoise(
        NoiseType, Seed, Frequency, Interp, Fractaltype,
        Octaves, Lacunarity, Gain, CellularJitter,
        CellularDistanceFunction, CellularReturnType);

    if (!NoiseWrapperLvl1->IsInitialized()) return;

    DestroyTiles();

    for (int32 y = 0; y < GridHeight; y++)
    {
        for (int32 x = 0; x < GridWidth; x++)
        {
            const bool oddRow = y % 2 == 1;
            const float xPos = oddRow
                ? (x * Settings->TileHorizontalOffset) + Settings->OddRowHorizontalOffset
                : x * Settings->TileHorizontalOffset;
            const float yPos = y * Settings->TileVerticalOffset;

            const float noiseValue = NoiseWrapperLvl1->GetNoise2D(xPos, yPos);
            const FVector localPos(xPos, yPos, noiseValue * HeightStrength);
            const FVector worldPos = GetActorLocation() + localPos;
            TilePositions.Add(worldPos);

            UInstancedStaticMeshComponent* Comp = noiseValue >= 0.f ? GrassMeshComp : WaterMeshComp;
            Comp->AddInstance(FTransform(localPos));
        }
    }

    // Start delayed spawn after navmesh rebuild
    GetWorldTimerManager().SetTimerForNextTick(this, &AHexManager::SpawnEnemiesAfterNavMeshReady);
}

void AHexManager::SpawnEnemiesAfterNavMeshReady()
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return;

    if (NavSys->IsNavigationBeingBuiltOrLocked(GetWorld()))
    {
        // NavMesh is still locked (building), retry soon
        FTimerHandle RetryHandle;
        GetWorldTimerManager().SetTimer(RetryHandle, this, &AHexManager::SpawnEnemiesAfterNavMeshReady, 0.2f, false);
        return;
    }


    SpawnPickups();
    SpawnProps();
    SpawnEnemies();
}

void AHexManager::SpawnEnemies()
{
    UWorld* World = GetWorld();
    if (!World || TilePositions.Num() == 0 || EnemyTypes.Num() == 0) return;

    for (int32 i = 0; i < EnemySpawnAmount; ++i)
    {
        int32 TileIndex = FMath::RandRange(0, TilePositions.Num() - 1);
        FVector SpawnLoc = TilePositions[TileIndex] + FVector(0, 0, 150.f);

        int32 EnemyIndex = FMath::RandRange(0, EnemyTypes.Num() - 1);
        TSubclassOf<AHopperBaseCharacter> EnemyClass = EnemyTypes[EnemyIndex];
        if (!EnemyClass) continue;

        AActor* Spawned = World->SpawnActor<AActor>(EnemyClass, SpawnLoc, FRotator::ZeroRotator);
        if (Spawned)
            SpawnedActors.Add(Spawned);
    }
}


void AHexManager::SpawnPickups()
{
    UWorld* World = GetWorld();
    if (!World || TilePositions.Num() == 0) return;

    for (const FPickupSpawnData& Data : PickupSpawnData)
    {
        if (!Data.PickupClass || Data.SpawnAmount <= 0) continue;

        for (int32 i = 0; i < Data.SpawnAmount; ++i)
        {
            const int32 TileIndex = FMath::RandRange(0, TilePositions.Num() - 1);
            const FVector BasePos = TilePositions[TileIndex];

            const float HeightOffset = FMath::FRandRange(Data.MinHeightOffset, Data.MaxHeightOffset);
            const FVector SpawnLoc = BasePos + FVector(0, 0, HeightOffset);

            AActor* Spawned = World->SpawnActor<AActor>(
                Data.PickupClass,
                SpawnLoc,
                FRotator::ZeroRotator
            );

            if (Spawned)
                SpawnedActors.Add(Spawned);
        }
    }
}

void AHexManager::SpawnProps()
{
    UWorld* World = GetWorld();
    if (!World || PropActors.Num() == 0 || TilePositions.Num() == 0) return;

    for (int32 i = 0; i < PropSpawnAmount; ++i)
    {
        int32 TileIndex = FMath::RandRange(0, TilePositions.Num() - 1);
        FVector SpawnLoc = TilePositions[TileIndex];

        int32 PropIndex = FMath::RandRange(0, PropActors.Num() - 1);
        TSubclassOf<AActor> PropClass = PropActors[PropIndex];
        if (!PropClass) continue;

        AActor* Spawned = World->SpawnActor<AActor>(PropClass, SpawnLoc, FRotator::ZeroRotator);
        if (Spawned)
            SpawnedActors.Add(Spawned);
    }
}

void AHexManager::GenerateNewLoop(
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
)
{
    GridWidth = InGridW;
    GridHeight = InGridH;
    EnemySpawnAmount = InEnemySpawnAmount;
    PropSpawnAmount = InPropSpawnAmount;
    NoiseType = InNoiseType;
    Seed = InSeed;
    Frequency = InFrequency;
    Octaves = InOctaves;
    Lacunarity = InLacunarity;
    Gain = InGain;
    CellularJitter = InCellularJitter;
    PickupSpawnData = InPickupSpawnData;

    DestroyTiles();
    GenerateHexGrid();
}
