#include "ASEnemyBase.h"
#include "ASPlayerController.h"
#include "ASPickup.h"
#include "ASHUD.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"

AASEnemyBase::AASEnemyBase()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);
    GetCapsuleComponent()->SetCapsuleRadius(42.f);

    // Give enemies a skeletal mesh (mannequin) — try actual paths that exist
    GetMesh()->SetRelativeLocation(FVector(0, 0, -96));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));

    // Try Manny Simple (the mesh that actually exists in this project)
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannySM(
        TEXT("/Game/Characters/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (MannySM.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MannySM.Object);
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

        // Unarmed ABP for enemies (don't want them holding rifles)
        static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedABP(
            TEXT("/Game/Characters/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
        if (UnarmedABP.Class)
            GetMesh()->SetAnimInstanceClass(UnarmedABP.Class);
    }
    // Hide the skeletal mesh since we use static mesh robot bodies instead
    GetMesh()->SetVisibility(false);

    // VISIBLE ROBOT BODY — static mesh so enemy is ALWAYS visible regardless of skeletal mesh
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylMesh.Object);
        BodyMesh->SetRelativeLocation(FVector(0, 0, 0));
        BodyMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 1.0f));
        BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    // Head sphere
    UStaticMeshComponent* HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        HeadMesh->SetStaticMesh(SphereMesh.Object);
        HeadMesh->SetRelativeLocation(FVector(0, 0, 85));
        HeadMesh->SetWorldScale3D(FVector(0.35f));
        HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    // Eye (small red sphere on front of head)
    UStaticMeshComponent* EyeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeMesh"));
    EyeMesh->SetupAttachment(RootComponent);
    if (SphereMesh.Succeeded())
    {
        EyeMesh->SetStaticMesh(SphereMesh.Object);
        EyeMesh->SetRelativeLocation(FVector(15, 0, 90));
        EyeMesh->SetWorldScale3D(FVector(0.08f));
        EyeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // Red glow so enemies are visible from a distance
    EnemyGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("EnemyGlow"));
    EnemyGlow->SetupAttachment(RootComponent);
    EnemyGlow->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    EnemyGlow->SetIntensity(5000.f);
    EnemyGlow->SetAttenuationRadius(500.f);
    EnemyGlow->SetLightColor(FLinearColor(1.f, 0.2f, 0.1f));
}

void AASEnemyBase::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;

    // Configure movement
    UCharacterMovementComponent* CMC = GetCharacterMovement();
    CMC->MaxWalkSpeed = MoveSpeed;
    CMC->bOrientRotationToMovement = false; // We control rotation manually
    CMC->GravityScale = 1.f;
    CMC->MaxAcceleration = 2048.f;
    CMC->BrakingDecelerationWalking = 512.f;
    CMC->GroundFriction = 8.f;
    CMC->SetMovementMode(MOVE_Walking);

    // Color the robot body based on enemy type (each class gets unique color)
    if (BodyMesh && BodyMesh->GetMaterial(0))
    {
        UMaterialInstanceDynamic* BodyMat = UMaterialInstanceDynamic::Create(BodyMesh->GetMaterial(0), this);
        if (BodyMat)
        {
            FString ClassName = GetClass()->GetName();
            FLinearColor BodyColor(0.3f, 0.08f, 0.05f); // Default dark red

            if (ClassName.Contains(TEXT("SecurityFrame")))
                BodyColor = FLinearColor(0.15f, 0.15f, 0.2f); // Dark steel blue
            else if (ClassName.Contains(TEXT("Sniper")))
                BodyColor = FLinearColor(0.1f, 0.2f, 0.1f); // Dark green (camouflage)
            else if (ClassName.Contains(TEXT("Warden")))
                BodyColor = FLinearColor(0.25f, 0.05f, 0.2f); // Dark purple (boss)
            else if (ClassName.Contains(TEXT("Builder")))
                BodyColor = FLinearColor(0.25f, 0.2f, 0.05f); // Dark gold (construction)
            else if (ClassName.Contains(TEXT("ScoutDrone")))
                BodyColor = FLinearColor(0.2f, 0.2f, 0.05f); // Yellow-ish
            else
                BodyColor = FLinearColor(0.3f, 0.08f, 0.05f); // Default red-ish

            BodyMat->SetVectorParameterValue(TEXT("BaseColor"), BodyColor);
            BodyMat->SetScalarParameterValue(TEXT("Metallic"), 0.6f);
            BodyMat->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
            BodyMesh->SetMaterial(0, BodyMat);
        }
    }
    // Color head
    TArray<UStaticMeshComponent*> StaticMeshes;
    GetComponents<UStaticMeshComponent>(StaticMeshes);
    for (UStaticMeshComponent* SM : StaticMeshes)
    {
        if (SM == BodyMesh) continue;
        if (SM->GetMaterial(0))
        {
            UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(SM->GetMaterial(0), this);
            if (Mat)
            {
                if (SM->GetName().Contains(TEXT("Eye")))
                    Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(5.f, 0.3f, 0.1f)); // Glowing red eye
                else
                    Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.2f, 0.2f, 0.25f)); // Dark metal head
                SM->SetMaterial(0, Mat);
            }
        }
    }

    PlayerRef = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void AASEnemyBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Retry finding player if not found yet
    if (!PlayerRef)
    {
        PlayerRef = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        if (!PlayerRef) return;
    }
    if (bIsStunned || bIsEMPDisabled) return;

    // Check if player is in detection range
    float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());
    bPlayerDetected = (DistToPlayer <= DetectionRange);

    if (bPlayerDetected)
    {
        // Low HP enemies seek cover instead of rushing
        float HPct = CurrentHealth / MaxHealth;
        if (HPct < 0.3f && !bSeekingCover)
        {
            CoverCheckTimer -= DeltaTime;
            if (CoverCheckTimer <= 0.f)
            {
                bSeekingCover = true;
                CoverCheckTimer = 3.f;
            }
        }

        if (bSeekingCover && HPct < 0.3f)
        {
            TakeCover(DeltaTime);
            // Still attack if in range
            if (DistToPlayer <= AttackRange * 1.2f) AttackPlayer();
        }
        else if (DistToPlayer <= AttackRange * 1.2f)
        {
            AttackPlayer();
            StrafeAroundPlayer(DeltaTime);
        }
        else
        {
            ChasePlayer(DeltaTime);
        }

        // Shield visual (blue sphere around shielded enemies)
        if (bHasShield && ShieldHP > 0.f)
        {
            float ShieldPulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 4.f) * 0.3f + 0.7f;
            DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0,0,50), 70.f, 8,
                FColor(50, 100, 255, (int32)(40 * ShieldPulse)), false, -1.f, 0, 1.f);
        }

        // Draw line to player when detected (subtle, helps debug)
        if (DistToPlayer < 1000.f)
        {
            DrawDebugLine(GetWorld(), GetActorLocation() + FVector(0,0,50),
                PlayerRef->GetActorLocation() + FVector(0,0,50),
                FColor(255, 50, 30, 30), false, -1.f, 0, 0.3f);
        }
    }
}

void AASEnemyBase::ChasePlayer(float DeltaTime)
{
    if (!PlayerRef) return;

    FVector ToPlayer = PlayerRef->GetActorLocation() - GetActorLocation();
    float Dist = ToPlayer.Size();
    FVector Direction = ToPlayer.GetSafeNormal();

    // Varied approach: mix of zigzag and flanking based on enemy ID
    float T = GetWorld()->GetTimeSeconds();
    uint32 ID = GetUniqueID();
    float ZigFreq = 1.5f + (ID % 5) * 0.3f;  // Different frequencies per enemy
    float ZigAmp = 0.3f + (ID % 3) * 0.15f;
    float ZigZag = FMath::Sin(T * ZigFreq + ID) * ZigAmp;

    // Some enemies flank wider
    bool bFlanker = (ID % 4 == 0);
    if (bFlanker && Dist > AttackRange * 1.5f)
        ZigZag += (ID % 2 == 0 ? 0.6f : -0.6f);

    FVector StrafeDir = FVector::CrossProduct(Direction, FVector::UpVector) * ZigZag;
    FVector MoveDir = (Direction + StrafeDir).GetSafeNormal();

    // Speed up when far, slow down when close
    float SpeedMult = FMath::Clamp(Dist / AttackRange, 0.6f, 1.2f);
    AddMovementInput(MoveDir, SpeedMult);

    // Face the player
    FRotator LookAt = Direction.Rotation();
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAt, DeltaTime, 5.f));
}

void AASEnemyBase::StrafeAroundPlayer(float DeltaTime)
{
    if (!PlayerRef) return;

    FVector ToPlayer = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    // Strafe perpendicular to player direction
    FVector StrafeDir = FVector::CrossProduct(ToPlayer, FVector::UpVector);
    // Alternate direction based on time
    float StrafeSign = FMath::Sin(GetWorld()->GetTimeSeconds() * 1.5f + GetUniqueID()) > 0 ? 1.f : -1.f;
    AddMovementInput(StrafeDir * StrafeSign, 0.5f);
}

void AASEnemyBase::AttackPlayer()
{
    if (!bCanAttack || !PlayerRef) return;
    bCanAttack = false;

    UGameplayStatics::ApplyDamage(PlayerRef, AttackDamage,
        GetController(), this, UDamageType::StaticClass());

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Orange,
            FString::Printf(TEXT("%s attacks! (%.0f dmg)"), *GetName(), AttackDamage));

    GetWorldTimerManager().SetTimer(AttackTimerHandle,
        [this]() { bCanAttack = true; }, AttackCooldown, false);
}

void AASEnemyBase::TakeCover(float DeltaTime)
{
    if (!PlayerRef) return;

    // Move perpendicular to player to find cover
    FVector ToPlayer = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    FVector CoverDir = FVector::CrossProduct(ToPlayer, FVector::UpVector).GetSafeNormal();

    // Alternate direction based on ID
    if (GetUniqueID() % 2 == 0) CoverDir *= -1.f;

    // Also move away slightly
    FVector AwayDir = -ToPlayer * 0.3f;
    AddMovementInput((CoverDir + AwayDir).GetSafeNormal(), 0.8f);

    // Check if we have line of sight cover
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(PlayerRef);
    bool bInCover = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation() + FVector(0,0,50),
        PlayerRef->GetActorLocation() + FVector(0,0,50), ECC_WorldStatic, Params);

    if (bInCover)
    {
        // Found cover! Stop seeking
        bSeekingCover = false;
        CoverCheckTimer = 5.f;
    }

    // Face player even while retreating
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToPlayer.Rotation(), DeltaTime, 5.f));
}

void AASEnemyBase::AlertNearbyEnemies(float Radius)
{
    TArray<AActor*> NearbyEnemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASEnemyBase::StaticClass(), NearbyEnemies);
    int32 Alerted = 0;
    for (AActor* A : NearbyEnemies)
    {
        if (A == this) continue;
        AASEnemyBase* E = Cast<AASEnemyBase>(A);
        if (E && !E->bPlayerDetected && FVector::Dist(GetActorLocation(), E->GetActorLocation()) < Radius)
        {
            E->bPlayerDetected = true;
            Alerted++;
        }
    }
    if (Alerted > 0)
    {
        DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0,0,100), Radius * 0.1f, 8,
            FColor(255, 200, 50, 80), false, 1.f, 0, 1.f);
    }
}

void AASEnemyBase::OnEMPHit(float Duration)
{
    bIsEMPDisabled = true;
    GetCharacterMovement()->StopMovementImmediately();

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
            FString::Printf(TEXT("%s EMP DISABLED for %.1fs"), *GetName(), Duration));

    GetWorldTimerManager().SetTimer(EMPTimerHandle,
        [this]() { bIsEMPDisabled = false; }, Duration, false);
}

void AASEnemyBase::OnDeath()
{
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
            FString::Printf(TEXT("%s DESTROYED!"), *GetName()));

    // Track kill count and update kill feed
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    AASPlayerController* ASPC = Cast<AASPlayerController>(PC);
    if (ASPC)
    {
        ASPC->Kills++;

        // Kill feed entry
        AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());
        if (HUD)
        {
            FString EnemyType = GetClass()->GetName().Replace(TEXT("AS"), TEXT(""));
            HUD->AddKillFeed(FString::Printf(TEXT("Destroyed %s"), *EnemyType), FColor(255, 100, 50));

            // Kill streak notifications
            if (ASPC->KillStreak >= 3)
            {
                FString StreakMsg;
                if (ASPC->KillStreak == 3) StreakMsg = TEXT("TRIPLE KILL!");
                else if (ASPC->KillStreak == 5) StreakMsg = TEXT("KILLING SPREE!");
                else if (ASPC->KillStreak >= 7) StreakMsg = TEXT("UNSTOPPABLE!");
                else StreakMsg = FString::Printf(TEXT("%d KILL STREAK!"), ASPC->KillStreak);
                HUD->KillStreakText = StreakMsg;
                HUD->KillStreakDisplayTimer = 2.5f;
            }
        }
    }

    // Death explosion effect — big, satisfying
    FVector DeathLoc = GetActorLocation();
    // Inner explosion flash
    DrawDebugSphere(GetWorld(), DeathLoc + FVector(0,0,50), 40.f, 8, FColor(255, 255, 200), false, 0.08f, 0, 3.f);
    // Outer explosion
    DrawDebugSphere(GetWorld(), DeathLoc + FVector(0,0,50), 100.f, 16, FColor(255, 100, 30), false, 0.4f, 0, 2.f);
    DrawDebugSphere(GetWorld(), DeathLoc + FVector(0,0,50), 60.f, 12, FColor(255, 150, 50), false, 0.6f, 0, 1.5f);
    // Spark shower
    for (int32 i = 0; i < 15; i++)
    {
        FVector SparkDir = FMath::VRand() * FMath::RandRange(50.f, 200.f);
        SparkDir.Z = FMath::Abs(SparkDir.Z) + 30.f; // Upward bias
        FColor SparkColor(255, FMath::RandRange(100,255), FMath::RandRange(20,100));
        DrawDebugLine(GetWorld(), DeathLoc + FVector(0,0,50), DeathLoc + SparkDir + FVector(0,0,50),
            SparkColor, false, FMath::RandRange(0.2f, 0.5f), 0, FMath::RandRange(0.5f, 1.5f));
    }
    // Debris chunks flying out
    for (int32 d = 0; d < 6; d++)
    {
        FVector DebrisDir = FMath::VRand() * FMath::RandRange(80.f, 180.f);
        DebrisDir.Z = FMath::Abs(DebrisDir.Z);
        DrawDebugPoint(GetWorld(), DeathLoc + DebrisDir + FVector(0,0,50), FMath::RandRange(3.f, 8.f),
            FColor(100, 90, 80), false, 0.8f);
    }
    // Ground scorch mark
    DrawDebugCircle(GetWorld(), DeathLoc + FVector(0,0,2), 70.f, 16,
        FColor(30, 25, 20), false, 8.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);

    // Spawn loot drops (60% chance, type varies by enemy)
    if (FMath::FRand() < 0.6f)
    {
        UClass* PickupClass = LoadClass<AActor>(nullptr, TEXT("/Script/AnalogStrike.ASPickup"));
        if (PickupClass)
        {
            FActorSpawnParameters SP;
            SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            AASPickup* Pickup = GetWorld()->SpawnActor<AASPickup>(AASPickup::StaticClass(), DeathLoc + FVector(0,0,30), FRotator::ZeroRotator, SP);
            if (Pickup)
            {
                // Drop type based on what killed us and random chance
                float Roll = FMath::FRand();
                if (Roll < 0.35f) Pickup->Type = EPickupType::Ammo;
                else if (Roll < 0.55f) Pickup->Type = EPickupType::Health;
                else if (Roll < 0.70f) Pickup->Type = EPickupType::Grenade;
                else if (Roll < 0.85f) Pickup->Type = EPickupType::Shield;
                else Pickup->Type = EPickupType::SpeedBoost;

                Pickup->SetActorLabel(TEXT("DroppedLoot"));
            }
        }
    }
    // Small chance to drop a second item (15%)
    if (FMath::FRand() < 0.15f)
    {
        AASPickup* BonusDrop = GetWorld()->SpawnActor<AASPickup>(AASPickup::StaticClass(),
            DeathLoc + FVector(FMath::RandRange(-50.f,50.f), FMath::RandRange(-50.f,50.f), 30), FRotator::ZeroRotator);
        if (BonusDrop)
        {
            BonusDrop->Type = (FMath::FRand() < 0.5f) ? EPickupType::Health : EPickupType::Ammo;
            BonusDrop->SetActorLabel(TEXT("BonusDrop"));
        }
    }

    Destroy();
}

float AASEnemyBase::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

    // Enemy shield absorbs damage
    if (bHasShield && ShieldHP > 0.f)
    {
        float Absorbed = FMath::Min(ShieldHP, ActualDamage);
        ShieldHP -= Absorbed;
        ActualDamage -= Absorbed;
        DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0,0,50), 60.f, 8,
            FColor(50, 100, 255), false, 0.1f, 0, 2.f);
        if (ShieldHP <= 0.f)
        {
            bHasShield = false;
            DrawDebugString(GetWorld(), GetActorLocation() + FVector(0,0,160), TEXT("SHIELD BROKEN!"),
                nullptr, FColor::Cyan, 1.f);
        }
        if (ActualDamage <= 0.f) return 0.f;
    }

    CurrentHealth -= ActualDamage;

    // Floating damage number — multiple stacked for big hits
    FVector DmgLoc = GetActorLocation() + FVector(FMath::RandRange(-40.f, 40.f), FMath::RandRange(-40.f, 40.f), 160.f);
    FColor DmgColor = (ActualDamage >= 80.f) ? FColor(255, 50, 50) :
        ((ActualDamage >= 40.f) ? FColor::Red : ((ActualDamage >= 20.f) ? FColor::Yellow : FColor::White));
    float DmgScale = (ActualDamage >= 80.f) ? 1.5f : 1.f;
    DrawDebugString(GetWorld(), DmgLoc, FString::Printf(TEXT("%.0f"), ActualDamage),
        nullptr, DmgColor, 1.0f);
    // Add "CRIT" text for big hits
    if (ActualDamage >= 60.f)
        DrawDebugString(GetWorld(), DmgLoc + FVector(0, 0, 20), TEXT("CRIT!"),
            nullptr, FColor::Red, 0.6f);

    // Hit flash on enemy
    DrawDebugPoint(GetWorld(), GetActorLocation() + FVector(0,0,90), 15.f, FColor::White, false, 0.05f);

    // Stagger on heavy hits (>30 damage) — briefly stop moving
    if (ActualDamage > 30.f && !bIsStunned)
    {
        bIsStunned = true;
        GetCharacterMovement()->StopMovementImmediately();
        FTimerHandle StaggerHandle;
        GetWorldTimerManager().SetTimer(StaggerHandle, [this]()
        {
            if (IsValid(this)) bIsStunned = false;
        }, 0.5f, false);

        // Knockback
        if (DamageCauser)
        {
            FVector KnockDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
            LaunchCharacter(KnockDir * ActualDamage * 5.f, true, false);
        }
    }

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
            FString::Printf(TEXT("%s hit! HP: %.0f/%.0f"), *GetName(), CurrentHealth, MaxHealth));

    // Alert nearby enemies when hit (robots communicate)
    if (!bPlayerDetected)
    {
        bPlayerDetected = true;
        AlertNearbyEnemies(1500.f);
    }

    if (CurrentHealth <= 0.f)
    {
        OnDeath();
    }

    return ActualDamage;
}
