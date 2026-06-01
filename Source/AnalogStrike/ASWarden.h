#pragma once
#include "CoreMinimal.h"
#include "ASSecurityFrame.h"
#include "ASWarden.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASWarden : public AASSecurityFrame
{
	GENERATED_BODY()
public:
	AASWarden();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category="Boss") float DroneSpawnInterval = 30.f;
	UPROPERTY(EditAnywhere, Category="Boss") int32 MaxDrones = 4;
	UPROPERTY(EditAnywhere, Category="Boss") TSubclassOf<class AASScoutDrone> DroneClass;

	virtual void OnDeath() override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;

	bool bFightStarted = false;
	bool bShieldActive = false;
	float ShieldTimer = 0.f;
	float SlamCooldown = 0.f;
	int32 PhaseIndex = 0; // 0 = full HP, 1 = half HP, 2 = low HP
	int32 DronesSpawned = 0;
	FTimerHandle DroneSpawnTimerHandle;

	void SpawnDrone();
	void StartBossFight();
	void DoGroundSlam();
};
