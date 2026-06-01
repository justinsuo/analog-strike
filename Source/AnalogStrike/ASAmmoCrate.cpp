#include "ASAmmoCrate.h"
#include "ASPlayerController.h"
#include "ASHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AASAmmoCrate::AASAmmoCrate()
{
	PrimaryActorTick.bCanEverTick = true;

	CrateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrateMesh"));
	RootComponent = CrateMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded()) CrateMesh->SetStaticMesh(Cube.Object);
	CrateMesh->SetWorldScale3D(FVector(0.5f, 0.35f, 0.3f));
	CrateMesh->SetCollisionProfileName(TEXT("OverlapAll"));

	InteractZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractZone"));
	InteractZone->SetupAttachment(RootComponent);
	InteractZone->SetSphereRadius(120.f);
	InteractZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	CrateGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("CrateGlow"));
	CrateGlow->SetupAttachment(RootComponent);
	CrateGlow->SetRelativeLocation(FVector(0, 0, 30));
	CrateGlow->SetIntensity(3000.f);
	CrateGlow->SetAttenuationRadius(250.f);
	CrateGlow->SetLightColor(FLinearColor(1.f, 0.8f, 0.2f)); // Yellow = ammo
}

void AASAmmoCrate::BeginPlay()
{
	Super::BeginPlay();
	InteractZone->OnComponentBeginOverlap.AddDynamic(this, &AASAmmoCrate::OnOverlap);
}

void AASAmmoCrate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAvailable)
	{
		RespawnTimer -= DeltaTime;
		if (RespawnTimer <= 0.f)
		{
			bIsAvailable = true;
			CrateMesh->SetVisibility(true);
			CrateGlow->SetIntensity(3000.f);
		}
	}
	else
	{
		// Subtle bob
		float Bob = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f + GetUniqueID()) * 2.f;
		CrateGlow->SetRelativeLocation(FVector(0, 0, 30 + Bob));
	}
}

void AASAmmoCrate::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsAvailable) return;

	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!PlayerPawn || !PlayerPawn->IsPlayerControlled()) return;

	AASPlayerController* PC = Cast<AASPlayerController>(PlayerPawn->GetController());
	if (!PC) return;

	// Refill ammo
	PC->Ammo = PC->MaxAmmo;
	PC->bIsReloading = false;

	// Optionally refill grenades
	if (bRefillGrenades)
	{
		PC->Grenades = FMath::Min(PC->Grenades + 2, 5);
	}

	AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());
	if (HUD)
	{
		if (bRefillGrenades)
			HUD->AddKillFeed(TEXT("Ammo + Grenades refilled!"), FColor(255, 220, 50));
		else
			HUD->AddKillFeed(TEXT("Ammo refilled!"), FColor(255, 220, 50));
	}

	// Disappear and respawn later
	bIsAvailable = false;
	RespawnTimer = RespawnTime;
	CrateMesh->SetVisibility(false);
	CrateGlow->SetIntensity(0.f);
}
