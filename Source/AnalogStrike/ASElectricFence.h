#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASElectricFence.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASElectricFence : public AActor
{
	GENERATED_BODY()
public:
	AASElectricFence();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* PostMesh;
	UPROPERTY(VisibleAnywhere) class UBoxComponent* DamageZone;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* ElectricGlow;

	UPROPERTY(EditAnywhere) float DamagePerSecond = 25.f;
	UPROPERTY(EditAnywhere) float FenceLength = 300.f;
	UPROPERTY(EditAnywhere) bool bIsActive = true;

	UFUNCTION(BlueprintCallable) void Deactivate();

protected:
	virtual void BeginPlay() override;

	UFUNCTION() void OnOverlapBegin(UPrimitiveComponent* Comp, AActor* Other,
		UPrimitiveComponent* OtherComp, int32 Idx, bool bSweep, const FHitResult& Hit);
	UFUNCTION() void OnOverlapEnd(UPrimitiveComponent* Comp, AActor* Other,
		UPrimitiveComponent* OtherComp, int32 Idx);

	TArray<AActor*> OverlappingActors;
};
