#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASHealStation.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASHealStation : public AActor
{
	GENERATED_BODY()
public:
	AASHealStation();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* StationMesh;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* HealGlow;
	UPROPERTY(VisibleAnywhere) class USphereComponent* HealZone;

	UPROPERTY(EditAnywhere) float HealRate = 15.f;
	UPROPERTY(EditAnywhere) float HealRadius = 300.f;
	UPROPERTY(EditAnywhere) float MaxCharge = 200.f;
	UPROPERTY(EditAnywhere) float RechargeRate = 5.f;

protected:
	virtual void BeginPlay() override;
	float CurrentCharge = 200.f;
};
