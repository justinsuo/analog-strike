#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASRelayNode.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASRelayNode : public AActor
{
    GENERATED_BODY()

public:
    AASRelayNode();

    virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* NodeMesh;
    UPROPERTY(VisibleAnywhere) class UPointLightComponent* NodeLight;

    UPROPERTY(EditAnywhere, Category="Relay") float MaxHealth = 300.f;
    UPROPERTY(BlueprintReadWrite, Category="Relay") float CurrentHealth = 300.f;
    UPROPERTY(BlueprintReadOnly) bool bIsActive = true;

    UFUNCTION(BlueprintCallable) void DestroyRelay();

protected:
    virtual void BeginPlay() override;
};
