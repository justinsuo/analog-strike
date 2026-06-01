#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASPickup.generated.h"

UENUM(BlueprintType)
enum class EPickupType : uint8 { Health, Ammo, Grenade, Shield, SpeedBoost };

UCLASS()
class ANALOGSTRIKE_API AASPickup : public AActor
{
	GENERATED_BODY()
public:
	AASPickup();

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* PickupMesh;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* Glow;
	UPROPERTY(VisibleAnywhere) class USphereComponent* PickupZone;

	UPROPERTY(EditAnywhere, Category="Pickup") EPickupType Type = EPickupType::Health;
	UPROPERTY(EditAnywhere, Category="Pickup") float HealthAmount = 25.f;
	UPROPERTY(EditAnywhere, Category="Pickup") int32 AmmoAmount = 6;
	UPROPERTY(EditAnywhere, Category="Pickup") float BobSpeed = 2.f;
	UPROPERTY(EditAnywhere, Category="Pickup") float BobHeight = 20.f;

	virtual void Tick(float DeltaTime) override;

	UFUNCTION() void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
	virtual void BeginPlay() override;
	FVector StartLocation;
};
