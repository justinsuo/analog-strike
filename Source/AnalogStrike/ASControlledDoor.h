#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASControlledDoor.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASControlledDoor : public AActor
{
	GENERATED_BODY()
public:
	AASControlledDoor();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* DoorFrame;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* DoorPanel;
	UPROPERTY(VisibleAnywhere) class UBoxComponent* InteractZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door") bool bIsLocked = true;
	UPROPERTY(BlueprintReadOnly) bool bIsOpen = false;

	UFUNCTION(BlueprintCallable) void Interact();
	UFUNCTION(BlueprintCallable) void UnlockDoor();
	UFUNCTION(BlueprintCallable) void LockDoor();

	// Tag as interactable
	virtual void BeginPlay() override;

private:
	FVector ClosedOffset;
	FVector OpenOffset;
	float DoorLerpAlpha = 0.f;
};
