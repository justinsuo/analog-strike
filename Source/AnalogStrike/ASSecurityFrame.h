#pragma once
#include "CoreMinimal.h"
#include "ASEnemyBase.h"
#include "ASSecurityFrame.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASSecurityFrame : public AASEnemyBase
{
	GENERATED_BODY()
public:
	AASSecurityFrame();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category="Combat") float ShootRange = 2000.f;
	UPROPERTY(EditAnywhere, Category="Combat") float ShootSpread = 3.f;
	UPROPERTY(EditAnywhere, Category="Combat") float ShootCooldown = 0.8f;
	UPROPERTY(EditAnywhere, Category="Combat") int32 BurstCount = 3;
	UPROPERTY(EditAnywhere, Category="Combat") float BurstInterval = 0.15f;

protected:
	virtual void ChasePlayer(float DeltaTime) override;
	virtual void AttackPlayer() override;

	bool bCanShoot = true;
	int32 BurstShotsRemaining = 0;
	float RepositionTimer = 0.f;
	FTimerHandle ShootTimerHandle;
	FTimerHandle BurstTimerHandle;
};
