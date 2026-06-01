#include "ASScoutDrone.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ASEnemyBase.h"

AASScoutDrone::AASScoutDrone()
{
	MaxHealth = 30.f;
	CurrentHealth = 30.f;
	MoveSpeed = 700.f;
	AttackDamage = 5.f;
	AttackRange = 500.f;
	DetectionRange = 2000.f;

	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCapsuleComponent()->SetCapsuleHalfHeight(30.f);
	GetCapsuleComponent()->SetCapsuleRadius(30.f);

	// Drone shouldn't use humanoid mesh — hide all inherited meshes
	GetMesh()->SetVisibility(false);
	if (BodyMesh) BodyMesh->SetVisibility(false);
	// Hide all static mesh components (head, eye, body)
	TArray<UStaticMeshComponent*> AllSM;
	GetComponents<UStaticMeshComponent>(AllSM);
	for (auto* SM : AllSM) SM->SetVisibility(false);
	// Make drone glow yellow instead of red
	if (EnemyGlow)
	{
		EnemyGlow->SetLightColor(FLinearColor(1.f, 0.8f, 0.2f));
		EnemyGlow->SetIntensity(3000.f);
	}
}

void AASScoutDrone::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void AASScoutDrone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Hover effect
	FVector Loc = GetActorLocation();
	Loc.Z += FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f) * 0.5f;
	SetActorLocation(Loc);

	// Draw drone body visually (sphere + propeller lines)
	DrawDebugSphere(GetWorld(), Loc, 25.f, 8, FColor(200, 180, 50), false, -1.f, 0, 1.5f);
	float T = GetWorld()->GetTimeSeconds() * 20.f;
	for (int32 p = 0; p < 4; p++)
	{
		float PA = T + p * 90.f;
		FVector PropEnd = Loc + FVector(FMath::Cos(FMath::DegreesToRadians(PA)) * 35.f,
			FMath::Sin(FMath::DegreesToRadians(PA)) * 35.f, 5.f);
		DrawDebugLine(GetWorld(), Loc + FVector(0,0,5), PropEnd, FColor(150, 150, 150), false, -1.f, 0, 0.8f);
	}
}

void AASScoutDrone::ChasePlayer(float DeltaTime)
{
	if (!PlayerRef) return;
	float Dist = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());

	if (Dist > OrbitRadius)
	{
		// Fly toward player
		FVector Dir = (PlayerRef->GetActorLocation() + FVector(0, 0, 200) - GetActorLocation()).GetSafeNormal();
		AddMovementInput(Dir, 1.0f);
	}
	else
	{
		// Orbit around player
		OrbitAngle += OrbitSpeed * DeltaTime;
		FVector OrbitPos = PlayerRef->GetActorLocation()
			+ FVector(FMath::Cos(OrbitAngle) * OrbitRadius, FMath::Sin(OrbitAngle) * OrbitRadius, 200.f);
		FVector Dir = (OrbitPos - GetActorLocation()).GetSafeNormal();
		AddMovementInput(Dir, 0.8f);
	}

	// Face player
	FVector ToPlayer = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToPlayer.Rotation(), DeltaTime, 5.f));
}

void AASScoutDrone::AttackPlayer()
{
	if (!bHasAlerted && PlayerRef)
	{
		bHasAlerted = true;
		// Alert nearby enemies
		TArray<AActor*> Nearby;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASEnemyBase::StaticClass(), Nearby);
		for (AActor* A : Nearby)
		{
			if (A != this && FVector::Dist(GetActorLocation(), A->GetActorLocation()) < AlertRadius)
			{
				AASEnemyBase* Enemy = Cast<AASEnemyBase>(A);
				if (Enemy) Enemy->bPlayerDetected = true;
			}
		}
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("SCOUT DRONE: Player detected! Alerting nearby enemies!"));
	}

	// Light melee attack if very close
	Super::AttackPlayer();
}
