#include "ASControlledDoor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

AASControlledDoor::AASControlledDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrame"));
	RootComponent = DoorFrame;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded()) DoorFrame->SetStaticMesh(CubeMesh.Object);
	DoorFrame->SetWorldScale3D(FVector(0.1f, 2.f, 3.f));

	DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanel"));
	DoorPanel->SetupAttachment(RootComponent);
	if (CubeMesh.Succeeded()) DoorPanel->SetStaticMesh(CubeMesh.Object);
	DoorPanel->SetRelativeLocation(FVector(20.f, 0.f, 0.f));
	DoorPanel->SetWorldScale3D(FVector(0.05f, 1.8f, 2.8f));

	InteractZone = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractZone"));
	InteractZone->SetupAttachment(RootComponent);
	InteractZone->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	InteractZone->SetCollisionProfileName(TEXT("OverlapAll"));

	ClosedOffset = FVector(20.f, 0.f, 0.f);
	OpenOffset = FVector(20.f, 200.f, 0.f);

	Tags.Add(TEXT("Interactable"));
}

void AASControlledDoor::BeginPlay()
{
	Super::BeginPlay();
	Tags.AddUnique(TEXT("Interactable"));
}

void AASControlledDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetAlpha = bIsOpen ? 1.f : 0.f;
	DoorLerpAlpha = FMath::FInterpTo(DoorLerpAlpha, TargetAlpha, DeltaTime, 4.f);
	FVector NewPos = FMath::Lerp(ClosedOffset, OpenOffset, DoorLerpAlpha);
	DoorPanel->SetRelativeLocation(NewPos);
}

void AASControlledDoor::Interact()
{
	if (bIsLocked)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("DOOR LOCKED - Find the breaker panel"));
		return;
	}
	bIsOpen = !bIsOpen;
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green,
			bIsOpen ? TEXT("Door opened") : TEXT("Door closed"));
}

void AASControlledDoor::UnlockDoor()
{
	bIsLocked = false;
	bIsOpen = true;
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Door UNLOCKED"));
}

void AASControlledDoor::LockDoor()
{
	bIsLocked = true;
	bIsOpen = false;
}
