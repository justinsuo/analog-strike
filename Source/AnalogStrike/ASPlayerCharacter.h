#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ASPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class AASValve;
class AASNPC;

UCLASS()
class ANALOGSTRIKE_API AASPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AASPlayerCharacter();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) USpringArmComponent* CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) UCameraComponent* FollowCamera;

    // Health
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats") float MaxHealth = 100.f;
    UPROPERTY(BlueprintReadWrite, Category="Stats") float CurrentHealth = 100.f;

    // Stamina
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats") float MaxStamina = 100.f;
    UPROPERTY(BlueprintReadWrite, Category="Stats") float CurrentStamina = 100.f;
    UPROPERTY(EditAnywhere, Category="Stats") float StaminaRegenRate = 15.f;
    UPROPERTY(EditAnywhere, Category="Stats") float SprintStaminaDrain = 20.f;

    // Genetic Modifier: Reflex Conditioning
    UPROPERTY(EditAnywhere, Category="Genetic") bool bReflexConditioning = true;
    UPROPERTY(EditAnywhere, Category="Genetic") float ReflexDodgeBonus = 0.3f; // wider dodge
    UPROPERTY(EditAnywhere, Category="Genetic") float ReflexStaminaPenalty = 0.7f; // 70% stamina regen

    // Movement
    UPROPERTY(EditAnywhere, Category="Movement") float WalkSpeed = 400.f;
    UPROPERTY(EditAnywhere, Category="Movement") float SprintSpeedMultiplier = 2.f;
    UPROPERTY(EditAnywhere, Category="Movement") float DodgeCost = 25.f;
    UPROPERTY(EditAnywhere, Category="Movement") float DodgeForce = 1000.f;
    UPROPERTY(BlueprintReadOnly) bool bIsSprinting = false;
    UPROPERTY(BlueprintReadOnly) bool bIsAiming = false;
    UPROPERTY(BlueprintReadOnly) bool bIsDodging = false;

    // Weapon - Pump Shotgun
    UPROPERTY(EditAnywhere, Category="Weapon") float WeaponDamage = 15.f; // per pellet
    UPROPERTY(EditAnywhere, Category="Weapon") float WeaponRange = 5000.f;
    UPROPERTY(EditAnywhere, Category="Weapon") float FireRate = 0.8f;
    UPROPERTY(EditAnywhere, Category="Weapon") int32 MaxAmmo = 6;
    UPROPERTY(BlueprintReadWrite, Category="Weapon") int32 CurrentAmmo = 6;
    UPROPERTY(EditAnywhere, Category="Weapon") int32 ShotgunPellets = 8;
    UPROPERTY(EditAnywhere, Category="Weapon") float ShotgunSpread = 8.f;
    UPROPERTY(EditAnywhere, Category="Weapon") float ReloadTime = 2.0f;
    UPROPERTY(BlueprintReadOnly) bool bIsReloading = false;

    // Melee - Breaching Hammer
    UPROPERTY(EditAnywhere, Category="Melee") float MeleeDamage = 50.f;
    UPROPERTY(EditAnywhere, Category="Melee") float MeleeRange = 200.f;

    // Ability - Hydraulic Arm Punch
    UPROPERTY(EditAnywhere, Category="Ability") float PunchDamage = 80.f;
    UPROPERTY(EditAnywhere, Category="Ability") float PunchRange = 250.f;
    UPROPERTY(EditAnywhere, Category="Ability") float PunchCooldown = 8.f;
    UPROPERTY(EditAnywhere, Category="Ability") float PunchForce = 2000.f;
    UPROPERTY(BlueprintReadOnly) float PunchCooldownRemaining = 0.f;

    // EMP Grenade
    UPROPERTY(EditAnywhere, Category="Grenade") int32 GrenadeCount = 3;
    UPROPERTY(EditAnywhere, Category="Grenade") float EMPRadius = 500.f;
    UPROPERTY(EditAnywhere, Category="Grenade") float EMPDuration = 5.f;
    UPROPERTY(EditAnywhere, Category="Grenade") float GrenadeFuseTime = 2.5f;
    UPROPERTY(EditAnywhere, Category="Grenade") float GrenadeThrowForce = 1500.f;

    // Damage feedback
    UPROPERTY(BlueprintReadOnly) float DamageFlashAlpha = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 EnemiesKilled = 0;
    UPROPERTY(BlueprintReadOnly) float MissionTime = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bHitMarkerActive = false;
    UPROPERTY(BlueprintReadOnly) float HitMarkerTimer = 0.f;

    // Survival
    UPROPERTY(EditAnywhere, Category="Survival") float HealthRegenDelay = 5.f;
    UPROPERTY(EditAnywhere, Category="Survival") float HealthRegenRate = 8.f;
    UPROPERTY(EditAnywhere, Category="Survival") float SpawnProtectionTime = 3.f;
    UPROPERTY(BlueprintReadOnly) bool bIsInvincible = false;
    UPROPERTY(BlueprintReadOnly) bool bIsDead = false;
    float TimeSinceLastDamage = 0.f;

    // Interaction prompt
    UPROPERTY(BlueprintReadOnly) bool bShowInteractPrompt = false;
    UPROPERTY(BlueprintReadOnly) FString InteractPromptText;

    // Weapon system
    UPROPERTY(BlueprintReadOnly) int32 CurrentWeaponIndex = 0;
    UPROPERTY(BlueprintReadOnly) FString CurrentWeaponName = TEXT("Pump Shotgun");

    // Compass
    UPROPERTY(BlueprintReadOnly) float CompassYaw = 0.f;

    UFUNCTION(BlueprintCallable) void Fire();
    UFUNCTION(BlueprintCallable) void Reload();
    UFUNCTION(BlueprintCallable) void MeleeAttack();
    UFUNCTION(BlueprintCallable) void ThrowEMPGrenade();
    UFUNCTION(BlueprintCallable) void HydraulicPunch();
    UFUNCTION(BlueprintCallable) void Interact();
    UFUNCTION(BlueprintCallable) void AddHealth(float Amount);
    UFUNCTION(BlueprintCallable) void AddAmmo(int32 Amount);

protected:
    virtual void BeginPlay() override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartSprint();
    void StopSprint();
    void StartAim();
    void StopAim();
    void StartFire();
    void StartMelee();
    void StartGrenade();
    void StartAbility();
    void StartInteract();
    void StartReload();
    void OnDodge();
    void SwitchToShotgun();
    void SwitchToRevolver();
    void SwitchWeapon(int32 Index);

    bool bCanFire = true;
    bool bCanMelee = true;
    bool bPunchReady = true;

    FTimerHandle FireTimerHandle;
    FTimerHandle MeleeTimerHandle;
    FTimerHandle PunchTimerHandle;
    FTimerHandle ReloadTimerHandle;
    FTimerHandle DodgeTimerHandle;
};
