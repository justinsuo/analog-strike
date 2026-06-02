#include "ASPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "ASEnemyBase.h"
#include "ASControlledDoor.h"
#include "ASBreakerBox.h"
#include "ASBuilderUnit.h"
#include "ASValve.h"
#include "ASNPC.h"

AASPlayerCharacter::AASPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // ── CAMERA ──
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 220.f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = FVector(0.f, 70.f, 25.f);

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // ── CHARACTER MESH — Quinn with dark tactical look ──
    GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));

    // Use Quinn mesh (different look from default Manny)
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> QuinnMesh(
        TEXT("/Game/Characters/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (QuinnMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(QuinnMesh.Object);
    }
    else
    {
        // Fallback to Manny
        static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
            TEXT("/Game/Characters/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
        if (MannyMesh.Succeeded())
            GetMesh()->SetSkeletalMesh(MannyMesh.Object);
    }

    // Rifle animation blueprint — makes character hold weapons properly
    static ConstructorHelpers::FClassFinder<UAnimInstance> RifleABP(
        TEXT("/Game/Characters/Anims/Shooter/ABP_TP_Rifle.ABP_TP_Rifle_C"));
    if (RifleABP.Class)
    {
        GetMesh()->SetAnimInstanceClass(RifleABP.Class);
    }
    else
    {
        // Try Variant_Shooter path
        static ConstructorHelpers::FClassFinder<UAnimInstance> RifleABP2(
            TEXT("/Game/Variant_Shooter/Anims/ABP_TP_Rifle.ABP_TP_Rifle_C"));
        if (RifleABP2.Class)
            GetMesh()->SetAnimInstanceClass(RifleABP2.Class);
    }

    // ── MOVEMENT ──
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->JumpZVelocity = 600.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxAcceleration = 2048.f;
    JumpMaxCount = 2; // Double jump

    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
}

void AASPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
    CurrentStamina = MaxStamina;
    CurrentAmmo = MaxAmmo;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

    // Apply dark tactical materials to all mesh slots
    if (GetMesh())
    {
        for (int32 i = 0; i < GetMesh()->GetNumMaterials(); i++)
        {
            UMaterialInterface* BaseMat = GetMesh()->GetMaterial(i);
            if (BaseMat)
            {
                UMaterialInstanceDynamic* TacMat = UMaterialInstanceDynamic::Create(BaseMat, this);
                if (TacMat)
                {
                    FLinearColor DarkColor(0.04f, 0.05f, 0.07f, 1.f);
                    TacMat->SetVectorParameterValue(TEXT("BaseColor"), DarkColor);
                    TacMat->SetVectorParameterValue(TEXT("Tint"), DarkColor);
                    TacMat->SetVectorParameterValue(TEXT("Color"), DarkColor);
                    TacMat->SetScalarParameterValue(TEXT("Roughness"), 0.6f);
                    TacMat->SetScalarParameterValue(TEXT("Metallic"), 0.3f);
                    GetMesh()->SetMaterial(i, TacMat);
                }
            }
        }
    }

    // Spawn protection
    bIsInvincible = true;
    FTimerHandle SpawnProtHandle;
    GetWorldTimerManager().SetTimer(SpawnProtHandle, [this]()
    {
        bIsInvincible = false;
    }, SpawnProtectionTime, false);
}

void AASPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead) return;

    MissionTime += DeltaTime;
    TimeSinceLastDamage += DeltaTime;

    // Health regen when out of combat
    if (TimeSinceLastDamage > HealthRegenDelay && CurrentHealth < MaxHealth && CurrentHealth > 0.f)
    {
        CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealthRegenRate * DeltaTime);
    }

    // Hit marker fade
    if (bHitMarkerActive)
    {
        HitMarkerTimer -= DeltaTime;
        if (HitMarkerTimer <= 0.f) bHitMarkerActive = false;
    }

    // Compass
    if (Controller)
    {
        CompassYaw = Controller->GetControlRotation().Yaw;
    }

    // Stamina
    float RegenMod = bReflexConditioning ? ReflexStaminaPenalty : 1.f;
    if (bIsSprinting && CurrentStamina > 0.f)
    {
        CurrentStamina = FMath::Max(0.f, CurrentStamina - SprintStaminaDrain * DeltaTime);
        if (CurrentStamina <= 0.f) StopSprint();
    }
    else if (!bIsSprinting && CurrentStamina < MaxStamina)
    {
        CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + StaminaRegenRate * RegenMod * DeltaTime);
    }

    // Aim mode camera
    float TargetArm = bIsAiming ? 150.f : 300.f;
    FVector TargetOffset = bIsAiming ? FVector(0.f, 80.f, 40.f) : FVector(0.f, 60.f, 60.f);
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm, DeltaTime, 10.f);
    CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetOffset, DeltaTime, 10.f);

    if (bIsAiming)
    {
        bUseControllerRotationYaw = true;
        GetCharacterMovement()->bOrientRotationToMovement = false;
    }
    else
    {
        bUseControllerRotationYaw = false;
        GetCharacterMovement()->bOrientRotationToMovement = true;
    }

    // Damage flash fade
    if (DamageFlashAlpha > 0.f)
        DamageFlashAlpha = FMath::Max(0.f, DamageFlashAlpha - DeltaTime * 3.f);

    // Punch cooldown tracking
    if (!bPunchReady && PunchCooldownRemaining > 0.f)
        PunchCooldownRemaining = FMath::Max(0.f, PunchCooldownRemaining - DeltaTime);

    // Auto-reload when empty
    if (CurrentAmmo <= 0 && !bIsReloading && bCanFire)
        Reload();

    // Check for nearby interactables to show prompt
    bShowInteractPrompt = false;
    InteractPromptText = TEXT("");
    {
        FVector CamLoc;
        FRotator CamRot;
        if (Controller)
        {
            GetController()->GetPlayerViewPoint(CamLoc, CamRot);
            FHitResult Hit;
            FCollisionQueryParams Params;
            Params.AddIgnoredActor(this);
            if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, CamLoc + CamRot.Vector() * 300.f, ECC_Visibility, Params))
            {
                if (Hit.GetActor())
                {
                    if (Cast<AASControlledDoor>(Hit.GetActor()))
                    {
                        AASControlledDoor* Door = Cast<AASControlledDoor>(Hit.GetActor());
                        bShowInteractPrompt = true;
                        InteractPromptText = Door->bIsLocked ? TEXT("[E] Door (LOCKED)") : TEXT("[E] Open/Close Door");
                    }
                    else if (Cast<AASBreakerBox>(Hit.GetActor()))
                    {
                        AASBreakerBox* B = Cast<AASBreakerBox>(Hit.GetActor());
                        bShowInteractPrompt = true;
                        InteractPromptText = B->bIsActivated ? TEXT("Breaker (Active)") : TEXT("[E] Pull Breaker");
                    }
                    else if (Cast<AASValve>(Hit.GetActor()))
                    {
                        bShowInteractPrompt = true;
                        InteractPromptText = TEXT("[E] Turn Valve");
                    }
                    else if (Cast<AASNPC>(Hit.GetActor()))
                    {
                        AASNPC* N = Cast<AASNPC>(Hit.GetActor());
                        bShowInteractPrompt = true;
                        InteractPromptText = FString::Printf(TEXT("[E] Talk to %s"), *N->NPCName);
                    }
                }
            }
        }
    }
}

void AASPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AASPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AASPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AASPlayerCharacter::StartSprint);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AASPlayerCharacter::StopSprint);
    PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AASPlayerCharacter::StartFire);
    PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AASPlayerCharacter::StartAim);
    PlayerInputComponent->BindAction("Aim", IE_Released, this, &AASPlayerCharacter::StopAim);
    PlayerInputComponent->BindAction("Melee", IE_Pressed, this, &AASPlayerCharacter::StartMelee);
    PlayerInputComponent->BindAction("Grenade", IE_Pressed, this, &AASPlayerCharacter::StartGrenade);
    PlayerInputComponent->BindAction("Ability", IE_Pressed, this, &AASPlayerCharacter::StartAbility);
    PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AASPlayerCharacter::StartInteract);
    PlayerInputComponent->BindAction("Dodge", IE_Pressed, this, &AASPlayerCharacter::OnDodge);
    PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AASPlayerCharacter::StartReload);
    PlayerInputComponent->BindAction("Weapon1", IE_Pressed, this, &AASPlayerCharacter::SwitchToShotgun);
    PlayerInputComponent->BindAction("Weapon2", IE_Pressed, this, &AASPlayerCharacter::SwitchToRevolver);
}

void AASPlayerCharacter::SwitchToShotgun() { SwitchWeapon(0); }
void AASPlayerCharacter::SwitchToRevolver() { SwitchWeapon(1); }

void AASPlayerCharacter::SwitchWeapon(int32 Index)
{
    if (Index == CurrentWeaponIndex) return;
    CurrentWeaponIndex = Index;

    if (Index == 0)
    {
        // Pump Shotgun
        CurrentWeaponName = TEXT("Pump Shotgun");
        WeaponDamage = 15.f;
        WeaponRange = 5000.f;
        FireRate = 0.8f;
        MaxAmmo = 6;
        ShotgunPellets = 8;
        ShotgunSpread = 8.f;
        ReloadTime = 2.0f;
    }
    else
    {
        // Heavy Revolver
        CurrentWeaponName = TEXT("Heavy Revolver");
        WeaponDamage = 45.f;
        WeaponRange = 8000.f;
        FireRate = 0.5f;
        MaxAmmo = 6;
        ShotgunPellets = 1;
        ShotgunSpread = 1.f;
        ReloadTime = 2.5f;
    }

    CurrentAmmo = MaxAmmo;
    bIsReloading = false;
    bCanFire = true;

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::White,
            FString::Printf(TEXT("Switched to %s"), *CurrentWeaponName));
}

void AASPlayerCharacter::MoveForward(float Value)
{
    if (Value != 0.f && Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
    }
}

void AASPlayerCharacter::MoveRight(float Value)
{
    if (Value != 0.f && Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
    }
}

void AASPlayerCharacter::StartSprint()
{
    if (CurrentStamina > 0.f)
    {
        bIsSprinting = true;
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * SprintSpeedMultiplier;
    }
}

void AASPlayerCharacter::StopSprint()
{
    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AASPlayerCharacter::StartAim() { bIsAiming = true; }
void AASPlayerCharacter::StopAim() { bIsAiming = false; }
void AASPlayerCharacter::StartFire() { Fire(); }
void AASPlayerCharacter::StartMelee() { MeleeAttack(); }
void AASPlayerCharacter::StartGrenade() { ThrowEMPGrenade(); }
void AASPlayerCharacter::StartAbility() { HydraulicPunch(); }
void AASPlayerCharacter::StartInteract() { Interact(); }
void AASPlayerCharacter::StartReload() { Reload(); }

// ── FIRE (Pump Shotgun) ─────────────────────────────────────────────────────
void AASPlayerCharacter::Fire()
{
    if (!bCanFire || bIsReloading) return;
    if (CurrentAmmo <= 0) { Reload(); return; }

    bCanFire = false;
    CurrentAmmo--;

    FVector CamLoc;
    FRotator CamRot;
    GetController()->GetPlayerViewPoint(CamLoc, CamRot);

    for (int32 i = 0; i < ShotgunPellets; i++)
    {
        FVector SpreadDir = FMath::VRandCone(CamRot.Vector(), FMath::DegreesToRadians(ShotgunSpread));
        FVector PelletEnd = CamLoc + SpreadDir * WeaponRange;

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, PelletEnd, ECC_Visibility, Params))
        {
            DrawDebugLine(GetWorld(), CamLoc, Hit.ImpactPoint, FColor::Red, false, 0.3f, 0, 1.f);
            DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 8.f, FColor::Yellow, false, 0.3f);

            if (Hit.GetActor())
            {
                // Weak point check based on hit bone/location
                float DamageMult = 1.f;
                FVector HitLocal = Hit.GetActor()->GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
                float HitHeight = HitLocal.Z;
                float ActorHeight = 180.f; // approximate

                // Head/optics = 3x (top 20%)
                if (HitHeight > ActorHeight * 0.8f)
                {
                    DamageMult = 3.f;
                    DrawDebugString(GetWorld(), Hit.ImpactPoint, TEXT("CRITICAL!"), nullptr, FColor::Red, 0.5f);
                }
                // Power cell = back hits 2x (check dot product)
                else if (FVector::DotProduct(CamRot.Vector(), Hit.GetActor()->GetActorForwardVector()) > 0.5f)
                {
                    DamageMult = 2.f;
                    DrawDebugString(GetWorld(), Hit.ImpactPoint, TEXT("WEAK POINT!"), nullptr, FColor::Orange, 0.5f);
                }

                UGameplayStatics::ApplyDamage(Hit.GetActor(), WeaponDamage * DamageMult,
                    GetController(), this, UDamageType::StaticClass());
                // Hit marker
                bHitMarkerActive = true;
                HitMarkerTimer = 0.3f;
            }
        }
        else
        {
            DrawDebugLine(GetWorld(), CamLoc, PelletEnd, FColor::Orange, false, 0.15f, 0, 0.3f);
        }
    }

    // Recoil kick
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        float RecoilPitch = (CurrentWeaponIndex == 0) ? -1.5f : -2.5f;
        float RecoilYaw = FMath::RandRange(-0.3f, 0.3f);
        PC->AddPitchInput(RecoilPitch);
        PC->AddYawInput(RecoilYaw);
    }

    // Muzzle flash light (brief)
    FVector MuzzleLoc = GetActorLocation() + GetActorForwardVector() * 100.f + FVector(0, 0, 60);
    DrawDebugPoint(GetWorld(), MuzzleLoc, 30.f, FColor::Yellow, false, 0.05f);

    GetWorldTimerManager().SetTimer(FireTimerHandle, [this]() { bCanFire = true; }, FireRate, false);
}

// ── RELOAD ──────────────────────────────────────────────────────────────────
void AASPlayerCharacter::Reload()
{
    if (bIsReloading || CurrentAmmo >= MaxAmmo) return;
    bIsReloading = true;

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::White, TEXT("RELOADING..."));

    GetWorldTimerManager().SetTimer(ReloadTimerHandle, [this]()
    {
        CurrentAmmo = MaxAmmo;
        bIsReloading = false;
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("RELOADED"));
    }, ReloadTime, false);
}

// ── MELEE (Breaching Hammer) ────────────────────────────────────────────────
void AASPlayerCharacter::MeleeAttack()
{
    if (!bCanMelee) return;
    bCanMelee = false;

    FVector Start = GetActorLocation();
    FVector End = Start + GetActorForwardVector() * MeleeRange;
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
        ECC_Pawn, FCollisionShape::MakeSphere(60.f), Params))
    {
        if (Hit.GetActor())
        {
            UGameplayStatics::ApplyDamage(Hit.GetActor(), MeleeDamage,
                GetController(), this, UDamageType::StaticClass());

            // Check if it's a door - hammer can force doors
            AASControlledDoor* Door = Cast<AASControlledDoor>(Hit.GetActor());
            if (Door && !Door->bIsLocked)
            {
                Door->Interact();
            }

            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Red,
                    FString::Printf(TEXT("HAMMER STRIKE! %s"), *Hit.GetActor()->GetName()));
        }
    }

    DrawDebugLine(GetWorld(), Start, End, FColor::White, false, 0.3f, 0, 3.f);
    GetWorldTimerManager().SetTimer(MeleeTimerHandle, [this]() { bCanMelee = true; }, 0.8f, false);
}

// ── EMP GRENADE ─────────────────────────────────────────────────────────────
void AASPlayerCharacter::ThrowEMPGrenade()
{
    if (GrenadeCount <= 0) return;
    GrenadeCount--;

    // Throw forward with arc
    FVector CamLoc;
    FRotator CamRot;
    GetController()->GetPlayerViewPoint(CamLoc, CamRot);
    FVector ThrowTarget = CamLoc + CamRot.Vector() * 800.f;

    // Delayed EMP effect (simulating fuse)
    FTimerHandle FuseHandle;
    FVector EMPCenter = ThrowTarget;
    GetWorldTimerManager().SetTimer(FuseHandle, [this, EMPCenter]()
    {
        // Visual feedback
        DrawDebugSphere(GetWorld(), EMPCenter, EMPRadius, 24, FColor::Cyan, false, 2.f, 0, 3.f);

        // Find and disable enemies
        TArray<FHitResult> Hits;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(EMPRadius);
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        if (GetWorld()->SweepMultiByChannel(Hits, EMPCenter, EMPCenter, FQuat::Identity,
            ECC_Pawn, Sphere, Params))
        {
            int32 Disabled = 0;
            for (auto& Hit : Hits)
            {
                AASEnemyBase* Enemy = Cast<AASEnemyBase>(Hit.GetActor());
                if (Enemy)
                {
                    Enemy->OnEMPHit(EMPDuration);
                    Disabled++;
                }
            }
            if (GEngine && Disabled > 0)
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
                    FString::Printf(TEXT("EMP: %d enemies disabled for %.1fs!"), Disabled, EMPDuration));
        }
    }, GrenadeFuseTime, false);

    // Throw indicator
    DrawDebugSphere(GetWorld(), ThrowTarget, 30.f, 8, FColor::Blue, false, GrenadeFuseTime, 0, 2.f);

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan,
            FString::Printf(TEXT("EMP THROWN! Detonates in %.1fs. Remaining: %d"), GrenadeFuseTime, GrenadeCount));
}

// ── HYDRAULIC ARM PUNCH ─────────────────────────────────────────────────────
void AASPlayerCharacter::HydraulicPunch()
{
    if (!bPunchReady) return;
    bPunchReady = false;
    PunchCooldownRemaining = PunchCooldown;

    FVector Start = GetActorLocation();
    FVector End = Start + GetActorForwardVector() * PunchRange;
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.5f, 0, 5.f);

    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
        ECC_Pawn, FCollisionShape::MakeSphere(60.f), Params))
    {
        if (Hit.GetActor())
        {
            UGameplayStatics::ApplyDamage(Hit.GetActor(), PunchDamage,
                GetController(), this, UDamageType::StaticClass());

            AASBuilderUnit* Builder = Cast<AASBuilderUnit>(Hit.GetActor());
            if (Builder)
            {
                Builder->OnStagger();
                if (GEngine)
                    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
                        TEXT("HYDRAULIC PUNCH: Builder Unit STAGGERED!"));
            }
            else
            {
                ACharacter* HitChar = Cast<ACharacter>(Hit.GetActor());
                if (HitChar)
                {
                    FVector KnockDir = (Hit.GetActor()->GetActorLocation() - GetActorLocation()).GetSafeNormal();
                    HitChar->LaunchCharacter(KnockDir * PunchForce + FVector(0, 0, 400), true, true);
                }
                if (GEngine)
                    GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
                        TEXT("HYDRAULIC PUNCH: Enemy knocked back!"));
            }
        }
    }

    GetWorldTimerManager().SetTimer(PunchTimerHandle, [this]()
    {
        bPunchReady = true;
        PunchCooldownRemaining = 0.f;
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Hydraulic Punch READY"));
    }, PunchCooldown, false);
}

// ── INTERACT ────────────────────────────────────────────────────────────────
void AASPlayerCharacter::Interact()
{
    FVector Start = GetActorLocation();
    FVector CamLoc;
    FRotator CamRot;
    GetController()->GetPlayerViewPoint(CamLoc, CamRot);
    FVector End = CamLoc + CamRot.Vector() * 300.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    // Line trace from camera
    if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params))
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor)
        {
            AASControlledDoor* Door = Cast<AASControlledDoor>(HitActor);
            if (Door) { Door->Interact(); return; }
            AASBreakerBox* Breaker = Cast<AASBreakerBox>(HitActor);
            if (Breaker) { Breaker->Interact(); return; }
        }
    }

    // Sphere sweep fallback for nearby interactables
    TArray<FHitResult> Hits;
    if (GetWorld()->SweepMultiByChannel(Hits, Start, Start + FVector(0,0,1), FQuat::Identity,
        ECC_Visibility, FCollisionShape::MakeSphere(250.f), Params))
    {
        for (auto& H : Hits)
        {
            if (!H.GetActor()) continue;
            AASControlledDoor* Door = Cast<AASControlledDoor>(H.GetActor());
            if (Door) { Door->Interact(); return; }
            AASBreakerBox* Breaker = Cast<AASBreakerBox>(H.GetActor());
            if (Breaker) { Breaker->Interact(); return; }
            AASValve* Valve = Cast<AASValve>(H.GetActor());
            if (Valve) { Valve->Interact(); return; }
            AASNPC* NPC = Cast<AASNPC>(H.GetActor());
            if (NPC) { NPC->Interact(); return; }
        }
    }
}

// ── DODGE / ROLL ────────────────────────────────────────────────────────────
void AASPlayerCharacter::OnDodge()
{
    if (bIsDodging || CurrentStamina < DodgeCost) return;
    bIsDodging = true;
    CurrentStamina -= DodgeCost;

    float DodgeMult = bReflexConditioning ? (1.f + ReflexDodgeBonus) : 1.f;
    FVector DodgeDir = GetLastMovementInputVector();
    if (DodgeDir.IsNearlyZero())
        DodgeDir = GetActorForwardVector();

    LaunchCharacter(DodgeDir.GetSafeNormal() * DodgeForce * DodgeMult + FVector(0, 0, 150), true, true);

    GetWorldTimerManager().SetTimer(DodgeTimerHandle, [this]() { bIsDodging = false; }, 0.5f, false);
}

// ── TAKE DAMAGE ─────────────────────────────────────────────────────────────
float AASPlayerCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDodging || bIsInvincible || bIsDead) return 0.f;

    float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
    CurrentHealth = FMath::Max(0.f, CurrentHealth - ActualDamage);
    DamageFlashAlpha = 0.8f;
    TimeSinceLastDamage = 0.f;

    if (CurrentHealth <= 0.f && !bIsDead)
    {
        bIsDead = true;
        // Respawn after 3 seconds
        FTimerHandle RespawnHandle;
        GetWorldTimerManager().SetTimer(RespawnHandle, [this]()
        {
            CurrentHealth = MaxHealth;
            CurrentStamina = MaxStamina;
            CurrentAmmo = MaxAmmo;
            bIsDead = false;
            bIsInvincible = true;
            DamageFlashAlpha = 0.f;

            // Teleport back to a safe location
            SetActorLocation(FVector(0, 0, 200));

            FTimerHandle InvHandle;
            GetWorldTimerManager().SetTimer(InvHandle, [this]() { bIsInvincible = false; }, SpawnProtectionTime, false);

            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("RESPAWNED — Stay sharp, operative."));
        }, 3.f, false);
    }

    return ActualDamage;
}

// ── PICKUPS ─────────────────────────────────────────────────────────────────
void AASPlayerCharacter::AddHealth(float Amount)
{
    CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green,
            FString::Printf(TEXT("+%.0f HP (%.0f/%.0f)"), Amount, CurrentHealth, MaxHealth));
}

void AASPlayerCharacter::AddAmmo(int32 Amount)
{
    CurrentAmmo = FMath::Min(MaxAmmo, CurrentAmmo + Amount);
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::White,
            FString::Printf(TEXT("+%d Ammo (%d/%d)"), Amount, CurrentAmmo, MaxAmmo));
}
