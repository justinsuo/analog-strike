#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ASPlayerController.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AASPlayerController();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	// Health
	float Health = 100.f;
	float MaxHealth = 100.f;
	float HealthRegenDelay = 5.f;
	float HealthRegenRate = 10.f;
	float TimeSinceLastDamage = 99.f;
	bool bIsDead = false;

	// Stamina
	float Stamina = 100.f;
	float MaxStamina = 100.f;
	float StaminaDrainRate = 20.f;
	float StaminaRegenRate = 15.f;

	// XP / Level system
	int32 XP = 0;
	int32 Level = 1;
	int32 XPToNextLevel = 100;
	float Shield = 0.f;
	float MaxShield = 50.f;
	float SpeedBoostTimer = 0.f;
	int32 DeployableTurrets = 2; // Player starts with 2 turrets
	bool bLastDeployTurret = false;
	int32 HeadshotCount = 0;
	int32 TotalShotsFired = 0;
	int32 TotalHits = 0;
	float DamageMultiplier = 1.f; // Increases with kills (weapon upgrade)

	// Combat state (read by HUD)
	int32 Ammo = 6;
	int32 MaxAmmo = 6;
	int32 Grenades = 3;
	int32 WeaponIndex = 0;
	float PunchCooldown = 0.f;
	bool bIsReloading = false;
	bool bCanFire = true;
	int32 Kills = 0;
	FString WeaponName = TEXT("Assault Rifle");

	// Shooting state
	float CurrentSpread = 3.f;
	float BaseSpread = 3.f;
	float AimSpread = 0.5f;
	float SprintSpreadMult = 2.f;
	bool bIsAiming = false;
	bool bIsSprinting = false;
	int32 KillStreak = 0;
	float KillStreakTimer = 0.f;
	float LastDamageDealt = 0.f;
	bool bLastShotWasHeadshot = false;

protected:
	virtual void BeginPlay() override;

private:
	bool bPunchReady = true;
	bool bLastFire, bLastReload, bLastGrenade, bLastPunch;
	bool bLastWeapon1, bLastWeapon2, bLastWeapon3, bLastInteract, bLastDodge;
	bool bLastAim, bLastSprint, bLastPause, bLastCrouch, bLastScrollUp, bLastScrollDown;
	bool bIsCrouching = false;
	bool bIsWallRunning = false;
	float WallRunTimer = 0.f;
	FVector WallNormal = FVector::ZeroVector;
	float MeleeTimer = 0.f;
	float FireHoldTime = 0.f;
	float ARSpinUp = 0.f; // Assault Rifle spin-up: fire rate increases while holding
	FTimerHandle FireTimer, ReloadTimer;
	bool bWeaponSetupDone = false;

	UPROPERTY() AActor* WeaponMeshActor = nullptr;

	// TPS aiming
	bool GetCrosshairWorldRay(FVector& OutStart, FVector& OutDir);
	bool GetAimHitLocation(FVector& OutHitLocation);
	FVector GetMuzzleLocation();

	void SpawnWeaponMesh();
	void DoFire();
	void DoReload();
	void DoKnifeAttack();
	void DoGrenade();
	void DoPunch();
	void DoDodge();
	void DoInteract();
	void SwitchWeapon(int32 Index);
	void ToggleFlashlight();
	void DoGrapple();
	// Bullet time (slow-mo)
	bool bBulletTime = false;
	float BulletTimeDuration = 0.f;
	float BulletTimeCooldown = 0.f;
	bool bLastBulletTime = false;

	bool bGrappling = false;
	FVector GrappleTarget = FVector::ZeroVector;
	float GrappleTimer = 0.f;
	float GrappleCooldown = 0.f;
	bool bLastGrapple = false;
	UPROPERTY() class USpotLightComponent* Flashlight = nullptr;
	bool bFlashlightOn = false;
	bool bLastFlashlight = false;

	UFUNCTION()
	void OnPawnTakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
	bool bDamageBound = false;

	// Melee combo
	int32 MeleeCombo = 0;
	float MeleeComboResetTimer = 0.f;

	// Weapon animation state
	float WeaponBobTime = 0.f;
	float RecoilRecovery = 0.f;
	FVector WeaponRecoilOffset = FVector::ZeroVector;
	float ReloadAnimProgress = 0.f; // 0 = not reloading, 0->1 = animation progress

	// Weapon configs
	struct FWeaponConfig {
		FString Name;
		int32 MaxAmmo;
		int32 Pellets;
		float BaseDamage;
		float BaseSpread;
		float AimSpread;
		float FireRate;
		float ReloadTime;
		float RecoilPitch;
		float RecoilYaw;
		bool bIsAutomatic;
		bool bIsMelee;
	};
	TArray<FWeaponConfig> WeaponConfigs;
};
