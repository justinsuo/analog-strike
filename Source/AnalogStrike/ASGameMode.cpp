#include "ASGameMode.h"
#include "ASPlayerCharacter.h"
#include "ASPlayerController.h"
#include "ASEnemyBase.h"
#include "ASSecurityFrame.h"
#include "ASScoutDrone.h"
#include "ASSniper.h"
#include "ASRelayNode.h"
#include "ASExtractionZone.h"
#include "ASHUD.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AASGameMode::AASGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // Use our OWN character class — Quinn mesh with rifle animations and dark tactical look
    // This is a C++ class, not a Blueprint, so the mesh is guaranteed to be set correctly
    DefaultPawnClass = AASPlayerCharacter::StaticClass();

    // Our controller handles Enhanced Input mapping + combat via key polling
    PlayerControllerClass = AASPlayerController::StaticClass();
    HUDClass = AASHUD::StaticClass();

    Objectives.Add(TEXT("Explore the valley — find enemies nearby"));
    Objectives.Add(TEXT("Clear the checkpoint area"));
    Objectives.Add(TEXT("Enter the pine forest"));
    Objectives.Add(TEXT("Investigate the research station"));
    Objectives.Add(TEXT("Find the cave system"));
    Objectives.Add(TEXT("Reach the relay core"));
    Objectives.Add(TEXT("DESTROY the Relay Node"));
    Objectives.Add(TEXT("Extract — reach the helipad"));
}

void AASGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Spawn initial enemies immediately so player sees combat right away
    FTimerHandle SpawnHandle;
    GetWorldTimerManager().SetTimer(SpawnHandle, this, &AASGameMode::SpawnInitialEnemies, 2.f, false);
}

void AASGameMode::SpawnInitialEnemies()
{
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!Player) return;

    FVector PLoc = Player->GetActorLocation();
    FActorSpawnParameters SP;
    SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Spawn 4 enemies right near the player (500-1200 units)
    for (int32 i = 0; i < 4; i++)
    {
        float Angle = i * 90.f + FMath::RandRange(-20.f, 20.f);
        float Dist = FMath::RandRange(500.f, 1200.f);
        FVector Loc = PLoc + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist, 50.f);

        if (i < 3)
        {
            GetWorld()->SpawnActor<AASSecurityFrame>(AASSecurityFrame::StaticClass(), Loc, FRotator::ZeroRotator, SP);
        }
        else
        {
            GetWorld()->SpawnActor<AASEnemyBase>(AASEnemyBase::StaticClass(), Loc, FRotator::ZeroRotator, SP);
        }
    }

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("WARNING: Hostile robots detected nearby!"));
}

void AASGameMode::AdvanceObjective()
{
    ObjectiveIndex++;
    if (ObjectiveIndex < Objectives.Num())
        CurrentObjective = Objectives[ObjectiveIndex];
    else
    {
        bMissionComplete = true;
        CurrentObjective = TEXT("MISSION COMPLETE");
    }
}

void AASGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bMissionComplete) return;

    // Update objective waypoint on HUD
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());
        if (HUD)
        {
            // Point to relay node or extraction zone
            if (!bMissionComplete)
            {
                TArray<AActor*> RelayNodes;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASRelayNode::StaticClass(), RelayNodes);
                if (RelayNodes.Num() > 0)
                {
                    HUD->ObjectiveWorldPos = RelayNodes[0]->GetActorLocation();
                    HUD->bShowObjectiveWaypoint = true;
                }
            }
            else
            {
                TArray<AActor*> ExtZones;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASExtractionZone::StaticClass(), ExtZones);
                if (ExtZones.Num() > 0)
                {
                    HUD->ObjectiveWorldPos = ExtZones[0]->GetActorLocation();
                    HUD->bShowObjectiveWaypoint = true;
                }
            }
        }
    }

    // Difficulty scaling — gets harder every 2 minutes
    GameTime += DeltaTime;
    DifficultyMultiplier = 1.f + GameTime / 120.f * 0.2f; // +20% per 2 minutes
    // Spawn more enemies as difficulty rises
    MinActiveEnemies = 6 + (int32)(GameTime / 180.f); // +1 enemy per 3 minutes
    MinActiveEnemies = FMath::Min(MinActiveEnemies, 15); // Cap at 15

    WaveTimer -= DeltaTime;
    if (WaveTimer <= 0.f)
    {
        WaveTimer = WaveCheckInterval;

        // Count living enemies
        TArray<AActor*> Enemies;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASEnemyBase::StaticClass(), Enemies);
        int32 AliveCount = 0;
        for (AActor* E : Enemies)
        {
            AASEnemyBase* Enemy = Cast<AASEnemyBase>(E);
            if (Enemy && Enemy->CurrentHealth > 0) AliveCount++;
        }

        if (AliveCount < MinActiveEnemies)
        {
            int32 ToSpawn = MinActiveEnemies - AliveCount;
            SpawnWaveEnemies(ToSpawn);

            // Wave announcement
            AASHUD* HUD = PC ? Cast<AASHUD>(PC->GetHUD()) : nullptr;
            if (HUD && ToSpawn >= 3)
            {
                FString WaveMsg;
                if (DifficultyMultiplier > 2.f) WaveMsg = TEXT("HEAVY WAVE INCOMING!");
                else if (DifficultyMultiplier > 1.5f) WaveMsg = TEXT("REINFORCEMENTS ARRIVING!");
                else WaveMsg = TEXT("Hostiles detected nearby.");
                HUD->ShowDialogue(TEXT("SYSTEM"), WaveMsg, 3.f);
            }
        }
    }
}

void AASGameMode::SpawnWaveEnemies(int32 Count)
{
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!Player) return;

    FVector PlayerLoc = Player->GetActorLocation();

    for (int32 i = 0; i < Count; i++)
    {
        // Spawn at random positions around the player
        // First wave spawns closer so player sees enemies immediately
        float Angle = FMath::RandRange(0.f, 360.f);
        float MinDist = (TotalWaveEnemiesSpawned < 10) ? 800.f : 1500.f;
        float MaxDist = (TotalWaveEnemiesSpawned < 10) ? 1800.f : 3000.f;
        float Dist = FMath::RandRange(MinDist, MaxDist);
        FVector SpawnLoc = PlayerLoc + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Dist,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Dist,
            50.f
        );

        FActorSpawnParameters SP;
        SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        // Mix of enemy types: 45% SecurityFrame, 20% melee, 15% ScoutDrone, 10% Sniper, 10% melee rush
        AASEnemyBase* Enemy = nullptr;
        float Roll = FMath::FRand();
        if (Roll < 0.45f)
        {
            Enemy = GetWorld()->SpawnActor<AASSecurityFrame>(AASSecurityFrame::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SP);
        }
        else if (Roll < 0.65f)
        {
            Enemy = GetWorld()->SpawnActor<AASEnemyBase>(AASEnemyBase::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SP);
        }
        else if (Roll < 0.80f)
        {
            SpawnLoc.Z += 200.f;
            Enemy = GetWorld()->SpawnActor<AASScoutDrone>(AASScoutDrone::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SP);
        }
        else if (Roll < 0.90f)
        {
            // Sniper — spawn farther away
            float FarAngle = FMath::RandRange(0.f, 360.f);
            SpawnLoc = PlayerLoc + FVector(
                FMath::Cos(FMath::DegreesToRadians(FarAngle)) * 4000.f,
                FMath::Sin(FMath::DegreesToRadians(FarAngle)) * 4000.f, 100.f);
            Enemy = GetWorld()->SpawnActor<AASSniper>(AASSniper::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SP);
        }
        else
        {
            // Fast melee rusher
            Enemy = GetWorld()->SpawnActor<AASEnemyBase>(AASEnemyBase::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SP);
            if (Enemy)
            {
                Enemy->MoveSpeed = 600.f;
                Enemy->AttackDamage = 20.f;
                Enemy->MaxHealth = 60.f;
                Enemy->CurrentHealth = 60.f;
            }
        }

        if (Enemy)
        {
            TotalWaveEnemiesSpawned++;
            // Scale with global difficulty
            Enemy->MaxHealth *= DifficultyMultiplier;
            Enemy->CurrentHealth = Enemy->MaxHealth;
            Enemy->AttackDamage *= FMath::Min(DifficultyMultiplier, 2.5f);
            Enemy->MoveSpeed *= FMath::Min(1.f + (DifficultyMultiplier - 1.f) * 0.3f, 1.5f);

            // 20% chance for shielded enemies (increases with difficulty)
            float ShieldChance = 0.15f + (DifficultyMultiplier - 1.f) * 0.1f;
            if (FMath::FRand() < FMath::Min(ShieldChance, 0.4f))
            {
                Enemy->bHasShield = true;
                Enemy->ShieldHP = 50.f * DifficultyMultiplier;
            }
        }
    }
}

void AASGameMode::CheckZoneObjective(const FString& ZoneName)
{
    if (bMissionComplete) return;
    if (VisitedZones.Contains(ZoneName)) return;
    VisitedZones.Add(ZoneName);

    if (ZoneName.Contains(TEXT("CHECKPOINT")) && ObjectiveIndex < 1) { ObjectiveIndex = 0; AdvanceObjective(); }
    else if (ZoneName.Contains(TEXT("FOREST")) && ObjectiveIndex < 2) { ObjectiveIndex = 1; AdvanceObjective(); }
    else if (ZoneName.Contains(TEXT("RESEARCH")) && ObjectiveIndex < 3) { ObjectiveIndex = 2; AdvanceObjective(); }
    else if (ZoneName.Contains(TEXT("CAVE")) && ObjectiveIndex < 4) { ObjectiveIndex = 3; AdvanceObjective(); }
}
