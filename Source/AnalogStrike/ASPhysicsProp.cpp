#include "ASPhysicsProp.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

AASPhysicsProp::AASPhysicsProp()
{
	PropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PropMesh"));
	RootComponent = PropMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded())
		PropMesh->SetStaticMesh(Cube.Object);

	// PHYSICS enabled — these are rigid bodies
	PropMesh->SetSimulatePhysics(true);
	PropMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	PropMesh->SetMassOverrideInKg(NAME_None, 50.f);
	PropMesh->SetLinearDamping(0.5f);
	PropMesh->SetAngularDamping(0.5f);
}

void AASPhysicsProp::BeginPlay()
{
	Super::BeginPlay();

	// Set size and shape based on type
	switch (PropType)
	{
	case EPropType::Crate:
		PropMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));
		PropMesh->SetMassOverrideInKg(NAME_None, 40.f);
		break;
	case EPropType::Barrel:
	{
		static UStaticMesh* CylMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder"));
		if (CylMesh) PropMesh->SetStaticMesh(CylMesh);
		PropMesh->SetWorldScale3D(FVector(0.3f, 0.3f, 0.5f));
		PropMesh->SetMassOverrideInKg(NAME_None, 60.f);
		break;
	}
	case EPropType::Debris:
		PropMesh->SetWorldScale3D(FVector(FMath::RandRange(0.1f, 0.4f), FMath::RandRange(0.1f, 0.3f), FMath::RandRange(0.05f, 0.15f)));
		PropMesh->SetMassOverrideInKg(NAME_None, 10.f);
		break;
	case EPropType::Chair:
		PropMesh->SetWorldScale3D(FVector(0.35f, 0.35f, 0.6f));
		PropMesh->SetMassOverrideInKg(NAME_None, 15.f);
		break;
	case EPropType::Table:
		PropMesh->SetWorldScale3D(FVector(0.8f, 0.5f, 0.05f));
		PropMesh->SetRelativeLocation(FVector(0, 0, 40));
		PropMesh->SetMassOverrideInKg(NAME_None, 80.f);
		break;
	}

	// Color the props
	if (PropMesh->GetMaterial(0))
	{
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(PropMesh->GetMaterial(0), this);
		if (Mat)
		{
			switch (PropType)
			{
			case EPropType::Crate:
				Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.4f, 0.3f, 0.15f)); // Wood
				break;
			case EPropType::Barrel:
				Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.15f, 0.3f, 0.15f)); // Green metal
				break;
			case EPropType::Debris:
				Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.3f, 0.3f, 0.32f)); // Grey
				break;
			case EPropType::Chair:
				Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.25f, 0.15f, 0.08f)); // Brown
				break;
			case EPropType::Table:
				Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.35f, 0.25f, 0.12f)); // Light wood
				break;
			}
			PropMesh->SetMaterial(0, Mat);
		}
	}
}

float AASPhysicsProp::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	Health -= Damage;

	// Apply impulse when shot (knockback physics)
	if (DamageCauser)
	{
		FVector ImpulseDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
		PropMesh->AddImpulse(ImpulseDir * Damage * 50.f + FVector(0, 0, Damage * 20.f));
	}

	if (Health <= 0.f)
	{
		// Break apart — spawn debris
		FVector Loc = GetActorLocation();
		for (int32 i = 0; i < 4; i++)
		{
			DrawDebugLine(GetWorld(), Loc, Loc + FMath::VRand() * FMath::RandRange(30.f, 80.f),
				FColor(150, 130, 100), false, 1.f, 0, 1.f);
		}
		Destroy();
	}

	return Damage;
}
