#pragma once
#include "CoreMinimal.h"
#include "ASEnemyBase.h"
#include "ASScoutDrone.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASScoutDrone : public AASEnemyBase
{
	GENERATED_BODY()
public:
	AASScoutDrone();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category="Drone") float OrbitRadius = 400.f;
	UPROPERTY(EditAnywhere, Category="Drone") float OrbitSpeed = 2.f;
	UPROPERTY(EditAnywhere, Category="Drone") float AlertRadius = 1500.f;

protected:
	virtual void BeginPlay() override;
	virtual void ChasePlayer(float DeltaTime) override;
	virtual void AttackPlayer() override;

	float OrbitAngle = 0.f;
	bool bHasAlerted = false;
};
