#include "ASBreakerBox.h"
#include "ASControlledDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

AASBreakerBox::AASBreakerBox()
{
	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	RootComponent = BoxMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded()) BoxMesh->SetStaticMesh(CubeMesh.Object);
	BoxMesh->SetWorldScale3D(FVector(0.3f, 0.3f, 0.4f));

	IndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndicatorLight"));
	IndicatorLight->SetupAttachment(RootComponent);
	IndicatorLight->SetRelativeLocation(FVector(20.f, 0.f, 0.f));
	IndicatorLight->SetIntensity(500.f);
	IndicatorLight->SetAttenuationRadius(200.f);
	IndicatorLight->SetLightColor(FLinearColor(1.f, 0.f, 0.f));

	Tags.Add(TEXT("Interactable"));
}

void AASBreakerBox::BeginPlay()
{
	Super::BeginPlay();
	Tags.AddUnique(TEXT("Interactable"));
}

void AASBreakerBox::Interact()
{
	if (bIsActivated) return;
	bIsActivated = true;

	IndicatorLight->SetLightColor(FLinearColor(0.f, 1.f, 0.f));

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
			FString::Printf(TEXT("BREAKER ACTIVATED! Systems restored for %.0f seconds"), TemporaryDuration));

	// Unlock ALL doors in the level temporarily
	TArray<AActor*> AllDoors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASControlledDoor::StaticClass(), AllDoors);
	for (AActor* A : AllDoors)
	{
		AASControlledDoor* Door = Cast<AASControlledDoor>(A);
		if (Door) Door->UnlockDoor();
	}

	// Set timer to revert
	GetWorldTimerManager().SetTimer(RevertTimerHandle, this, &AASBreakerBox::RevertBreaker, TemporaryDuration, false);
}

void AASBreakerBox::RevertBreaker()
{
	bIsActivated = false;
	IndicatorLight->SetLightColor(FLinearColor(1.f, 0.f, 0.f));

	TArray<AActor*> AllDoors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASControlledDoor::StaticClass(), AllDoors);
	for (AActor* A : AllDoors)
	{
		AASControlledDoor* Door = Cast<AASControlledDoor>(A);
		if (Door) Door->LockDoor();
	}

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Breaker expired! Systems locked again."));
}
