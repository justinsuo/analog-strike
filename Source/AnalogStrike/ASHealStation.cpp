#include "ASHealStation.h"
#include "ASPlayerController.h"
#include "ASHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AASHealStation::AASHealStation()
{
	PrimaryActorTick.bCanEverTick = true;

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	RootComponent = StationMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder"));
	if (Cyl.Succeeded()) StationMesh->SetStaticMesh(Cyl.Object);
	StationMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.8f));
	StationMesh->SetCollisionProfileName(TEXT("BlockAll"));

	HealGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("HealGlow"));
	HealGlow->SetupAttachment(RootComponent);
	HealGlow->SetRelativeLocation(FVector(0, 0, 60));
	HealGlow->SetIntensity(5000.f);
	HealGlow->SetAttenuationRadius(400.f);
	HealGlow->SetLightColor(FLinearColor(0.1f, 1.f, 0.3f));

	HealZone = CreateDefaultSubobject<USphereComponent>(TEXT("HealZone"));
	HealZone->SetupAttachment(RootComponent);
	HealZone->SetSphereRadius(300.f);
	HealZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AASHealStation::BeginPlay()
{
	Super::BeginPlay();
	CurrentCharge = MaxCharge;
}

void AASHealStation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Recharge when no player nearby
	bool bHealing = false;

	// Check if player is in range
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	AASPlayerController* ASPC = PC ? Cast<AASPlayerController>(PC) : nullptr;
	if (ASPC && ASPC->GetPawn() && CurrentCharge > 0.f)
	{
		float Dist = FVector::Dist(GetActorLocation(), ASPC->GetPawn()->GetActorLocation());
		if (Dist < HealRadius && ASPC->Health < ASPC->MaxHealth)
		{
			float HealAmount = FMath::Min(HealRate * DeltaTime, CurrentCharge);
			HealAmount = FMath::Min(HealAmount, ASPC->MaxHealth - ASPC->Health);
			ASPC->Health += HealAmount;
			CurrentCharge -= HealAmount;
			bHealing = true;

			// Heal visual — green particles rising
			for (int32 p = 0; p < 2; p++)
			{
				FVector Part = GetActorLocation() + FVector(FMath::RandRange(-30.f, 30.f),
					FMath::RandRange(-30.f, 30.f), FMath::RandRange(20.f, 80.f));
				DrawDebugPoint(GetWorld(), Part, 3.f, FColor(50, 255, 100, 150), false, 0.3f);
			}
		}
	}

	// Recharge when not healing
	if (!bHealing && CurrentCharge < MaxCharge)
		CurrentCharge = FMath::Min(MaxCharge, CurrentCharge + RechargeRate * DeltaTime);

	// Visual — glow intensity based on charge
	float ChargePct = CurrentCharge / MaxCharge;
	HealGlow->SetIntensity(5000.f * ChargePct);

	// Charge ring
	if (ChargePct > 0.1f)
	{
		DrawDebugCircle(GetWorld(), GetActorLocation() + FVector(0,0,5), HealRadius, 24,
			FColor(50, 255, 100, (int32)(30 * ChargePct)), false, -1.f, 0, 1.f,
			FVector(1,0,0), FVector(0,1,0), false);
	}
}
