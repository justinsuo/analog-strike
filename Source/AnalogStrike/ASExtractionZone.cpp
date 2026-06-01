#include "ASExtractionZone.h"
#include "ASGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"

AASExtractionZone::AASExtractionZone()
{
	TriggerZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerZone"));
	RootComponent = TriggerZone;
	TriggerZone->SetBoxExtent(FVector(300.f, 300.f, 200.f));
	TriggerZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
	BeaconLight->SetupAttachment(RootComponent);
	BeaconLight->SetIntensity(0.f); // Off until active
	BeaconLight->SetLightColor(FLinearColor(0.1f, 1.f, 0.1f));
	BeaconLight->SetAttenuationRadius(1000.f);
}

void AASExtractionZone::BeginPlay()
{
	Super::BeginPlay();
	TriggerZone->OnComponentBeginOverlap.AddDynamic(this, &AASExtractionZone::OnOverlapBegin);
}

void AASExtractionZone::ActivateExtraction()
{
	bIsActive = true;
	BeaconLight->SetIntensity(15000.f);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("EXTRACTION ZONE ACTIVE - Get to the green light!"));
}

void AASExtractionZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsActive) return;

	APawn* Player = Cast<APawn>(OtherActor);
	if (Player && Player->IsPlayerControlled())
	{
		AASGameMode* GM = Cast<AASGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
		if (GM)
		{
			GM->bMissionComplete = true;
			GM->CurrentObjective = TEXT("MISSION COMPLETE");
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT(""));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("========================================"));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("     MISSION COMPLETE - EXTRACTED"));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("========================================"));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT(""));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, TEXT("The sector has been reclaimed."));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, TEXT("Analog beats machine."));
		}
	}
}
