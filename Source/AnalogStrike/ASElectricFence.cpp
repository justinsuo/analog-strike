#include "ASElectricFence.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AASElectricFence::AASElectricFence()
{
	PrimaryActorTick.bCanEverTick = true;

	PostMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PostMesh"));
	RootComponent = PostMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded()) PostMesh->SetStaticMesh(Cube.Object);
	PostMesh->SetWorldScale3D(FVector(0.1f, 0.1f, 1.5f));

	DamageZone = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageZone"));
	DamageZone->SetupAttachment(RootComponent);
	DamageZone->SetBoxExtent(FVector(150.f, 20.f, 80.f));
	DamageZone->SetRelativeLocation(FVector(150.f, 0.f, 0.f));
	DamageZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	ElectricGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("ElectricGlow"));
	ElectricGlow->SetupAttachment(RootComponent);
	ElectricGlow->SetRelativeLocation(FVector(0, 0, 50));
	ElectricGlow->SetIntensity(4000.f);
	ElectricGlow->SetAttenuationRadius(300.f);
	ElectricGlow->SetLightColor(FLinearColor(0.3f, 0.5f, 1.f)); // Blue = electric
}

void AASElectricFence::BeginPlay()
{
	Super::BeginPlay();
	DamageZone->OnComponentBeginOverlap.AddDynamic(this, &AASElectricFence::OnOverlapBegin);
	DamageZone->OnComponentEndOverlap.AddDynamic(this, &AASElectricFence::OnOverlapEnd);
}

void AASElectricFence::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsActive) return;

	// Electric arc visual between posts
	FVector Start = GetActorLocation() + FVector(0, 0, 50);
	FVector End = Start + GetActorForwardVector() * FenceLength;

	// Flickering electric arcs
	for (int32 i = 0; i < 3; i++)
	{
		FVector Mid = FMath::Lerp(Start, End, FMath::FRand());
		Mid += FVector(FMath::RandRange(-15.f, 15.f), FMath::RandRange(-15.f, 15.f),
			FMath::RandRange(-10.f, 30.f));
		DrawDebugLine(GetWorld(), Start, Mid, FColor(100, 150, 255, 200), false, -1.f, 0, 1.5f);
		DrawDebugLine(GetWorld(), Mid, End, FColor(100, 150, 255, 200), false, -1.f, 0, 1.5f);
	}

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

void AASElectricFence::OnOverlapBegin(UPrimitiveComponent* Comp, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 Idx, bool bSweep, const FHitResult& Hit)
{
	if (bIsActive && Other) OverlappingActors.Add(Other);
}

void AASElectricFence::OnOverlapEnd(UPrimitiveComponent* Comp, AActor* Other,
	UPrimitiveComponent* OtherComp, int32 Idx)
{
	OverlappingActors.Remove(Other);
}

void AASElectricFence::Deactivate()
{
	bIsActive = false;
	ElectricGlow->SetIntensity(0.f);
}
