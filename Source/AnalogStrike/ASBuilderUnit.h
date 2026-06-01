#pragma once
#include "CoreMinimal.h"
#include "ASEnemyBase.h"
#include "ASBuilderUnit.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASBuilderUnit : public AASEnemyBase
{
	GENERATED_BODY()
public:
	AASBuilderUnit();

	UPROPERTY(EditAnywhere, Category="Combat") float SlamRadius = 300.f;
	UPROPERTY(EditAnywhere, Category="Combat") float SlamForce = 1500.f;

	UFUNCTION(BlueprintCallable) void OnStagger();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void AttackPlayer() override;
	FTimerHandle StaggerTimerHandle;
	float OriginalSpeed;
	float RepairTimer = 0.f;
	float RepairCooldown = 3.f;
	float RepairAmount = 20.f;
	float RepairRange = 800.f;
};
