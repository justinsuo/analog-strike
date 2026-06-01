#include "ASEnemySpawner.h"
#include "ASEnemyBase.h"
#include "ASSecurityFrame.h"
#include "ASPlayerController.h"
#include "ASHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AASEnemySpawner::AASEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded()) BaseMesh->SetStaticMesh(Cube.Object);
	BaseMesh->SetWorldScale3D(FVector(1.f, 1.f, 0.5f));
	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));

	AntennaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AntennaMesh"));
	AntennaMesh->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder"));
	if (Cyl.Succeeded()) AntennaMesh->SetStaticMesh(Cyl.Object);
	AntennaMesh->SetRelativeLocation(FVector(0, 0, 80));
	AntennaMesh->SetWorldScale3D(FVector(0.05f, 0.05f, 1.5f));
	AntennaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(RootComponent);
	StatusLight->SetRelativeLocation(FVector(0, 0, 160));
	StatusLight->SetIntensity(8000.f);
	StatusLight->SetAttenuationRadius(800.f);
	StatusLight->SetLightColor(FLinearColor(1.f, 0.2f, 0.1f));
}

void AASEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	SpawnTimer = 5.f; // First spawn after 5s
}

void AASEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsActive) return;

	// Pulsing light
	float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f) * 2000.f;
	StatusLight->SetIntensity(6000.f + Pulse);

	// Spawn beam visual
	DrawDebugLine(GetWorld(), GetActorLocation() + FVector(0,0,160),
		GetActorLocation() + FVector(0,0,300),
		FColor(255, 50, 30, 150), false, -1.f, 0, 2.f);

	// Spawn enemies on timer
	SpawnTimer -= DeltaTime;
	if (SpawnTimer <= 0.f && TotalSpawned < MaxSpawned)
	{
		SpawnTimer = SpawnInterval;
		SpawnEnemy();
	}
}

void AASEnemySpawner::SpawnEnemy()
{
	FVector SpawnLoc = GetActorLocation() + FVector(
		FMath::RandRange(-SpawnRadius, SpawnRadius),
		FMath::RandRange(-SpawnRadius, SpawnRadius), 100.f);

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AASEnemyBase* Enemy = nullptr;
	if (FMath::FRand() < 0.6f)
		Enemy = GetWorld()->SpawnActor<AASSecurityFrame>(AASSecurityFrame::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SP);
	else
		Enemy = GetWorld()->SpawnActor<AASEnemyBase>(AASEnemyBase::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SP);

	if (Enemy)
	{
		TotalSpawned++;
		// Spawn effect
		DrawDebugSphere(GetWorld(), SpawnLoc, 50.f, 8, FColor(255, 100, 50), false, 1.f, 0, 2.f);
		for (int32 i = 0; i < 6; i++)
		{
			DrawDebugLine(GetWorld(), SpawnLoc, SpawnLoc + FMath::VRand() * 80.f,
				FColor(255, 150, 50), false, 0.5f, 0, 1.f);
		}
	}
}

float AASEnemySpawner::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!bIsActive) return 0.f;
	CurrentHealth -= Damage;

	// Hit feedback
	DrawDebugPoint(GetWorld(), GetActorLocation() + FVector(0,0,50), 10.f, FColor::White, false, 0.05f);

	if (CurrentHealth <= 0.f)
		OnDestroyed();

	return Damage;
}

void AASEnemySpawner::OnDestroyed()
{
	bIsActive = false;
	StatusLight->SetLightColor(FLinearColor(0.1f, 0.5f, 0.1f));
	StatusLight->SetIntensity(3000.f);

	// Explosion
	FVector Loc = GetActorLocation();
	DrawDebugSphere(GetWorld(), Loc, 150.f, 16, FColor(255, 100, 30), false, 1.f, 0, 3.f);
	for (int32 i = 0; i < 12; i++)
	{
		FVector Spark = FMath::VRand() * FMath::RandRange(50.f, 200.f);
		Spark.Z = FMath::Abs(Spark.Z);
		DrawDebugLine(GetWorld(), Loc, Loc + Spark,
			FColor(255, FMath::RandRange(100,255), FMath::RandRange(20,100)), false, 0.5f, 0, 1.5f);
	}

	// Award XP
	AASPlayerController* PC = Cast<AASPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC)
	{
		PC->XP += 50;
		AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());
		if (HUD)
		{
			HUD->AddKillFeed(TEXT("Spawner DESTROYED! +50 XP"), FColor(255, 200, 50));
		}
	}
}
