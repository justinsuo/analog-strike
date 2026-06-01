#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASBreakerBox.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASBreakerBox : public AActor
{
	GENERATED_BODY()
public:
	AASBreakerBox();

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* BoxMesh;
	UPROPERTY(VisibleAnywhere) class UPointLightComponent* IndicatorLight;

	UPROPERTY(EditAnywhere, Category="Breaker") float TemporaryDuration = 30.f;
	UPROPERTY(BlueprintReadOnly) bool bIsActivated = false;

	UFUNCTION(BlueprintCallable) void Interact();

	virtual void BeginPlay() override;

private:
	FTimerHandle RevertTimerHandle;
	void RevertBreaker();
};
