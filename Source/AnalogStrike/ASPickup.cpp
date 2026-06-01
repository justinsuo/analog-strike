#include "ASPickup.h"
#include "ASPlayerController.h"
#include "ASHUD.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"

AASPickup::AASPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	RootComponent = PickupMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded()) PickupMesh->SetStaticMesh(CubeMesh.Object);
	PickupMesh->SetWorldScale3D(FVector(0.3f));
	PickupMesh->SetCollisionProfileName(TEXT("OverlapAll"));

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetIntensity(2000.f);
	Glow->SetAttenuationRadius(300.f);

	PickupZone = CreateDefaultSubobject<USphereComponent>(TEXT("PickupZone"));
	PickupZone->SetupAttachment(RootComponent);
	PickupZone->SetSphereRadius(100.f);
	PickupZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AASPickup::BeginPlay()
{
	Super::BeginPlay();
	StartLocation = GetActorLocation();
	PickupZone->OnComponentBeginOverlap.AddDynamic(this, &AASPickup::OnOverlap);

	// Set glow color based on type
	switch (Type)
	{
	case EPickupType::Health: Glow->SetLightColor(FLinearColor(0.1f, 1.f, 0.1f)); break;
	case EPickupType::Ammo: Glow->SetLightColor(FLinearColor(1.f, 0.8f, 0.1f)); break;
	case EPickupType::Grenade: Glow->SetLightColor(FLinearColor(0.1f, 0.8f, 1.f)); break;
	case EPickupType::Shield: Glow->SetLightColor(FLinearColor(0.3f, 0.5f, 1.f)); break;
	case EPickupType::SpeedBoost: Glow->SetLightColor(FLinearColor(1.f, 0.5f, 0.1f)); break;
	}
}

void AASPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Bob up and down
	FVector Loc = StartLocation;
	Loc.Z += FMath::Sin(GetWorld()->GetTimeSeconds() * BobSpeed) * BobHeight;
	SetActorLocation(Loc);
	// Rotate slowly
	AddActorLocalRotation(FRotator(0.f, 45.f * DeltaTime, 0.f));
}

void AASPickup::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if it's the player pawn (BP_ThirdPersonCharacter)
	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!PlayerPawn || !PlayerPawn->IsPlayerControlled()) return;

	AASPlayerController* PC = Cast<AASPlayerController>(PlayerPawn->GetController());
	if (!PC) return;

	AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());

	switch (Type)
	{
	case EPickupType::Health:
		PC->Health = FMath::Min(PC->MaxHealth, PC->Health + HealthAmount);
		if (HUD) HUD->AddKillFeed(FString::Printf(TEXT("+%.0f HP"), HealthAmount), FColor(50, 255, 50));
		break;
	case EPickupType::Ammo:
		PC->Ammo = FMath::Min(PC->MaxAmmo, PC->Ammo + AmmoAmount);
		if (HUD) HUD->AddKillFeed(FString::Printf(TEXT("+%d Ammo"), AmmoAmount), FColor(255, 220, 50));
		break;
	case EPickupType::Grenade:
		PC->Grenades = FMath::Min(PC->Grenades + 1, 5);
		if (HUD) HUD->AddKillFeed(TEXT("+1 EMP Grenade"), FColor(50, 200, 255));
		break;
	case EPickupType::Shield:
		PC->Shield = FMath::Min(PC->MaxShield, PC->Shield + 25.f);
		if (HUD) HUD->AddKillFeed(TEXT("+25 Shield"), FColor(100, 150, 255));
		break;
	case EPickupType::SpeedBoost:
		PC->SpeedBoostTimer = 10.f;
		if (HUD) HUD->AddKillFeed(TEXT("SPEED BOOST! 10s"), FColor(255, 150, 50));
		break;
	}

	Destroy();
}
