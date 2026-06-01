#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASValve.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASValve : public AActor
{
	GENERATED_BODY()
public:
	AASValve();

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* ValveMesh;
	UPROPERTY(EditAnywhere, Category="Valve") TArray<AActor*> LinkedVents;
	UPROPERTY(BlueprintReadOnly) bool bIsActivated = false;

	UFUNCTION(BlueprintCallable) void Interact();
	virtual void BeginPlay() override;
};
