#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASTurret.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASTurret : public AActor
{
	GENERATED_BODY()
public:
	AASTurret();
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* BaseMesh;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* BarrelMesh;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* StatusLight;

	UPROPERTY(EditAnywhere, Category="Turret") float MaxHealth = 200.f;
	UPROPERTY(EditAnywhere, Category="Turret") float TurretDamage = 8.f;
	UPROPERTY(EditAnywhere, Category="Turret") float Range = 2500.f;
	UPROPERTY(EditAnywhere, Category="Turret") float FireRate = 0.3f;
	UPROPERTY(EditAnywhere, Category="Turret") float RotationSpeed = 90.f;
	UPROPERTY(EditAnywhere, Category="Turret") bool bIsActive = true;
	UPROPERTY(BlueprintReadWrite) float CurrentHealth;

	UFUNCTION(BlueprintCallable) void Deactivate();
	UFUNCTION(BlueprintCallable) void Activate();

protected:
	virtual void BeginPlay() override;
	bool bCanFire = true;
	FTimerHandle FireHandle;
	UPROPERTY() ACharacter* PlayerRef;
};
