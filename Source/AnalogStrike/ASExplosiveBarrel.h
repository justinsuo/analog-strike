#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASExplosiveBarrel.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASExplosiveBarrel : public AActor
{
	GENERATED_BODY()
public:
	AASExplosiveBarrel();
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* BarrelMesh;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* WarningLight;

	UPROPERTY(EditAnywhere, Category="Explosion") float ExplosionDamage = 120.f;
	UPROPERTY(EditAnywhere, Category="Explosion") float ExplosionRadius = 600.f;
	UPROPERTY(EditAnywhere, Category="Explosion") float Health = 50.f;

private:
	bool bExploded = false;
	void Explode();
};
