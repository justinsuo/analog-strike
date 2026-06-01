#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASPhysicsProp.generated.h"

UENUM(BlueprintType)
enum class EPropType : uint8
{
	Crate,
	Barrel,
	Debris,
	Chair,
	Table
};

UCLASS()
class ANALOGSTRIKE_API AASPhysicsProp : public AActor
{
	GENERATED_BODY()
public:
	AASPhysicsProp();

	UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* PropMesh;
	UPROPERTY(EditAnywhere) EPropType PropType = EPropType::Crate;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	float Health = 30.f;
};
