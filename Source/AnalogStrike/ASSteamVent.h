#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASSteamVent.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASSteamVent : public AActor
{
	GENERATED_BODY()
public:
	AASSteamVent();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* VentMesh;
	UPROPERTY(VisibleAnywhere) class UBoxComponent* DamageZone;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* SteamGlow;

	UPROPERTY(EditAnywhere, Category="Steam") float DamagePerSecond = 15.f;
	UPROPERTY(EditAnywhere, Category="Steam") bool bIsActive = true;

	UFUNCTION(BlueprintCallable) void ShutOff();
	UFUNCTION(BlueprintCallable) void TurnOn();

protected:
	virtual void BeginPlay() override;

	UFUNCTION() void OnOverlapBegin(UPrimitiveComponent* Comp, AActor* Other,
		UPrimitiveComponent* OtherComp, int32 Idx, bool bSweep, const FHitResult& Hit);
	UFUNCTION() void OnOverlapEnd(UPrimitiveComponent* Comp, AActor* Other,
		UPrimitiveComponent* OtherComp, int32 Idx);

	TSet<AActor*> OverlappingActors;
};
