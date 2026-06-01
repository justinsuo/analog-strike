#pragma once
#include "CoreMinimal.h"
#include "ASSecurityFrame.h"
#include "ASSniper.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASSniper : public AASSecurityFrame
{
	GENERATED_BODY()
public:
	AASSniper();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void ChasePlayer(float DeltaTime) override;
	virtual void AttackPlayer() override;

	// Sniper charges shot — laser sight warns player
	float ChargeTimer = 0.f;
	bool bIsCharging = false;
	float ChargeTime = 1.5f; // Time before shot fires
	FVector LaserTarget = FVector::ZeroVector;
};
