#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASExtractionZone.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASExtractionZone : public AActor
{
	GENERATED_BODY()
public:
	AASExtractionZone();

	UPROPERTY(VisibleAnywhere) class UBoxComponent* TriggerZone;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* BeaconLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsActive = false;

	UFUNCTION() void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable) void ActivateExtraction();

protected:
	virtual void BeginPlay() override;
};
