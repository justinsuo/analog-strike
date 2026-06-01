#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASAmmoCrate.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASAmmoCrate : public AActor
{
	GENERATED_BODY()
public:
	AASAmmoCrate();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* CrateMesh;
	UPROPERTY(VisibleAnywhere) class USphereComponent* InteractZone;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* CrateGlow;

	UPROPERTY(EditAnywhere) float RespawnTime = 30.f;
	UPROPERTY(EditAnywhere) bool bRefillGrenades = false;

protected:
	virtual void BeginPlay() override;
	UFUNCTION() void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bIsAvailable = true;
	float RespawnTimer = 0.f;
};
