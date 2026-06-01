#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASEnemySpawner.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASEnemySpawner : public AActor
{
	GENERATED_BODY()
public:
	AASEnemySpawner();
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* BaseMesh;
	UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* AntennaMesh;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* StatusLight;

	UPROPERTY(EditAnywhere) float MaxHealth = 200.f;
	UPROPERTY(EditAnywhere) float SpawnInterval = 20.f;
	UPROPERTY(EditAnywhere) int32 MaxSpawned = 5;
	UPROPERTY(EditAnywhere) float SpawnRadius = 300.f;

protected:
	virtual void BeginPlay() override;
	float CurrentHealth = 200.f;
	float SpawnTimer = 0.f;
	int32 TotalSpawned = 0;
	bool bIsActive = true;

	void SpawnEnemy();
	void OnDestroyed();
};
