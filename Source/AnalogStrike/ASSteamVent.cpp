#include "ASSteamVent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AASSteamVent::AASSteamVent()
{
	PrimaryActorTick.bCanEverTick = true;

	VentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VentMesh"));
	RootComponent = VentMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded()) VentMesh->SetStaticMesh(Cube.Object);
	VentMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.1f));

	DamageZone = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageZone"));
	DamageZone->SetupAttachment(RootComponent);
	DamageZone->SetBoxExtent(FVector(100.f, 100.f, 150.f));
	DamageZone->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	DamageZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	SteamGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("SteamGlow"));
	SteamGlow->SetupAttachment(RootComponent);
	SteamGlow->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	SteamGlow->SetIntensity(3000.f);
	SteamGlow->SetAttenuationRadius(400.f);
	SteamGlow->SetLightColor(FLinearColor(1.f, 0.9f, 0.7f));

	Tags.Add(TEXT("Interactable"));
}

void AASSteamVent::BeginPlay()
{
	Super::BeginPlay();
	DamageZone->OnComponentBeginOverlap.AddDynamic(this, &AASSteamVent::OnOverlapBegin);
	DamageZone->OnComponentEndOverlap.AddDynamic(this, &AASSteamVent::OnOverlapEnd);
}

void AASSteamVent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsActive) return;

	// Visual steam indicator
	DrawDebugBox(GetWorld(), GetActorLocation() + FVector(0, 0, 100),
		FVector(100, 100, 150), FColor(255, 200, 100, 40), false, 0.f, 0, 1.f);

	// Damage overlapping actors
	for (AActor* A : OverlappingActors)
	{
		if (A && !A->IsPendingKillPending())
		{
			UGameplayStatics::ApplyDamage(A, DamagePerSecond * DeltaTime,
				nullptr, this, UDamageType::StaticClass());
		}
	}
}

void AASSteamVent::OnOverlapBegin(UPrimitiveComponent* Comp, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 Idx, bool bSweep, const FHitResult& Hit)
{
	if (bIsActive && Other) OverlappingActors.Add(Other);
}

void AASSteamVent::OnOverlapEnd(UPrimitiveComponent* Comp, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 Idx)
{
	OverlappingActors.Remove(Other);
}

void AASSteamVent::ShutOff()
{
	bIsActive = false;
	SteamGlow->SetIntensity(0.f);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Steam vent SHUT OFF"));
}

void AASSteamVent::TurnOn()
{
	bIsActive = true;
	SteamGlow->SetIntensity(3000.f);
}
