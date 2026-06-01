#include "ASTurret.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

AASTurret::AASTurret()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	RootComponent = BaseMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded()) BaseMesh->SetStaticMesh(Cube.Object);
	BaseMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.8f));

	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	BarrelMesh->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder"));
	if (Cyl.Succeeded()) BarrelMesh->SetStaticMesh(Cyl.Object);
	BarrelMesh->SetRelativeLocation(FVector(30.f, 0.f, 40.f));
	BarrelMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	BarrelMesh->SetWorldScale3D(FVector(0.08f, 0.08f, 0.6f));

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(RootComponent);
	StatusLight->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	StatusLight->SetIntensity(3000.f);
	StatusLight->SetAttenuationRadius(300.f);
	StatusLight->SetLightColor(FLinearColor(1.f, 0.f, 0.f));
}

void AASTurret::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	PlayerRef = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void AASTurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsActive || !PlayerRef) return;

	float Dist = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());
	if (Dist > Range) return;

	// Rotate toward player
	FVector ToPlayer = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FRotator TargetRot = ToPlayer.Rotation();
	FRotator Current = GetActorRotation();
	FRotator NewRot = FMath::RInterpConstantTo(Current, TargetRot, DeltaTime, RotationSpeed);
	SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));

	// Laser sight (thin red line showing where turret aims)
	FVector LaserStart = GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0, 0, 40);
	FVector LaserEnd = LaserStart + GetActorForwardVector() * Range;
	FHitResult LaserHit;
	FCollisionQueryParams LaserParams;
	LaserParams.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(LaserHit, LaserStart, LaserEnd, ECC_Visibility, LaserParams))
		LaserEnd = LaserHit.ImpactPoint;
	DrawDebugLine(GetWorld(), LaserStart, LaserEnd, FColor(255, 0, 0, 40), false, -1.f, 0, 0.3f);

	// Fire if facing player
	float Angle = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRot.Yaw, TargetRot.Yaw));
	if (Angle < 15.f && bCanFire)
	{
		bCanFire = false;
		FVector MuzzleLoc = GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0, 0, 40);
		FVector Dir = FMath::VRandCone(ToPlayer, FMath::DegreesToRadians(3.f));
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLoc, MuzzleLoc + Dir * Range, ECC_Visibility, Params))
		{
			// Red turret laser beam
			DrawDebugLine(GetWorld(), MuzzleLoc, Hit.ImpactPoint, FColor(255, 20, 10), false, 0.12f, 0, 1.0f);
			DrawDebugLine(GetWorld(), MuzzleLoc, Hit.ImpactPoint, FColor(255, 50, 30, 60), false, 0.18f, 0, 2.5f);
			DrawDebugPoint(GetWorld(), MuzzleLoc, 5.f, FColor(255, 100, 50), false, 0.03f);
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 3.f, FColor(255, 80, 30), false, 0.08f);
			if (Hit.GetActor())
				UGameplayStatics::ApplyDamage(Hit.GetActor(), TurretDamage, nullptr, this, UDamageType::StaticClass());
		}

		GetWorldTimerManager().SetTimer(FireHandle, [this]() { bCanFire = true; }, FireRate, false);
	}
}

float AASTurret::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
	AController* Instigator, AActor* Causer)
{
	if (!bIsActive) return 0.f;
	CurrentHealth -= Damage;
	if (CurrentHealth <= 0.f) Deactivate();
	return Damage;
}

void AASTurret::Deactivate()
{
	bIsActive = false;
	StatusLight->SetLightColor(FLinearColor(0.1f, 0.1f, 0.1f));
	StatusLight->SetIntensity(500.f);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Turret DESTROYED"));
}

void AASTurret::Activate()
{
	bIsActive = true;
	CurrentHealth = MaxHealth;
	StatusLight->SetLightColor(FLinearColor(1.f, 0.f, 0.f));
	StatusLight->SetIntensity(3000.f);
}
