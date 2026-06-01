#include "ASValve.h"
#include "ASSteamVent.h"
#include "Components/StaticMeshComponent.h"

AASValve::AASValve()
{
	ValveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ValveMesh"));
	RootComponent = ValveMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder"));
	if (Cyl.Succeeded()) ValveMesh->SetStaticMesh(Cyl.Object);
	ValveMesh->SetWorldScale3D(FVector(0.3f, 0.3f, 0.1f));

	Tags.Add(TEXT("Interactable"));
}

void AASValve::BeginPlay() { Super::BeginPlay(); Tags.AddUnique(TEXT("Interactable")); }

void AASValve::Interact()
{
	bIsActivated = !bIsActivated;
	for (AActor* A : LinkedVents)
	{
		AASSteamVent* Vent = Cast<AASSteamVent>(A);
		if (Vent) { bIsActivated ? Vent->ShutOff() : Vent->TurnOn(); }
	}
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			bIsActivated ? TEXT("Valve CLOSED - Steam off") : TEXT("Valve OPENED - Steam on"));
}
