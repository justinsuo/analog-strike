#include "ASWarden.h"
#include "ASPlayerController.h"
#include "ASScoutDrone.h"
#include "ASControlledDoor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "DrawDebugHelpers.h"

AASWarden::AASWarden()
{
	MaxHealth = 500.f;
	CurrentHealth = 500.f;
	MoveSpeed = 300.f;
	AttackDamage = 25.f;
	ShootRange = 2500.f;
	ShootCooldown = 0.5f;
	DetectionRange = 3000.f;

	GetCapsuleComponent()->SetCapsuleHalfHeight(110.f);
	GetCapsuleComponent()->SetCapsuleRadius(50.f);

	DroneClass = AASScoutDrone::StaticClass();
}

void AASWarden::BeginPlay()
{
	Super::BeginPlay();

	// Make Warden visually bigger and more menacing
	if (BodyMesh) BodyMesh->SetWorldScale3D(FVector(0.8f, 0.8f, 1.5f));
	// Bigger glow — purple/red for boss
	if (EnemyGlow)
	{
		EnemyGlow->SetLightColor(FLinearColor(0.8f, 0.1f, 0.6f));
		EnemyGlow->SetIntensity(10000.f);
		EnemyGlow->SetAttenuationRadius(800.f);
	}
	// Scale head bigger
	TArray<UStaticMeshComponent*> AllSM;
	GetComponents<UStaticMeshComponent>(AllSM);
	for (auto* SM : AllSM)
	{
		if (SM != BodyMesh)
			SM->SetWorldScale3D(SM->GetComponentScale() * 1.5f);
	}
}

void AASWarden::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bPlayerDetected && !bFightStarted)
	{
		StartBossFight();
	}

	if (!bFightStarted) return;

	// Shield visual effect
	if (bShieldActive)
	{
		ShieldTimer -= DeltaTime;
		// Pulsing shield sphere
		float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 8.f) * 10.f;
		DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0,0,50), 120.f + Pulse, 16,
			FColor(50, 150, 255, 100), false, -1.f, 0, 2.f);
		if (ShieldTimer <= 0.f)
		{
			bShieldActive = false;
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("WARDEN SHIELD DOWN!"));
		}
	}

	// Ground slam when player is close
	if (SlamCooldown > 0) SlamCooldown -= DeltaTime;
	if (PlayerRef && SlamCooldown <= 0.f)
	{
		float Dist = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());
		if (Dist < 400.f)
		{
			DoGroundSlam();
		}
	}

	// Phase transitions
	float HPct = CurrentHealth / MaxHealth;
	if (HPct < 0.5f && PhaseIndex == 0)
	{
		PhaseIndex = 1;
		bShieldActive = true;
		ShieldTimer = 5.f;
		ShootCooldown = 0.35f; // Faster attacks
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("WARDEN: \"Activating defense protocol!\""));
	}
	else if (HPct < 0.25f && PhaseIndex == 1)
	{
		PhaseIndex = 2;
		bShieldActive = true;
		ShieldTimer = 3.f;
		ShootCooldown = 0.25f; // Even faster
		BurstCount = 5;
		DroneSpawnInterval = 15.f; // Faster drone spawns
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("WARDEN: \"MAXIMUM FORCE AUTHORIZED!\""));
	}
}

void AASWarden::DoGroundSlam()
{
	SlamCooldown = 8.f;
	FVector SlamPos = GetActorLocation();

	// Visual effect - expanding ring
	for (int32 r = 0; r < 4; r++)
	{
		float Radius = 100.f + r * 100.f;
		DrawDebugCircle(GetWorld(), SlamPos + FVector(0,0,5), Radius, 24,
			FColor(255, 100, 30), false, 0.5f - r * 0.1f, 0, 3.f - r * 0.5f,
			FVector(1,0,0), FVector(0,1,0), false);
	}

	// Damage and knockback
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetWorld()->SweepMultiByChannel(Hits, SlamPos, SlamPos, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(400.f), Params))
	{
		for (auto& H : Hits)
		{
			if (ACharacter* C = Cast<ACharacter>(H.GetActor()))
			{
				if (C != this)
				{
					UGameplayStatics::ApplyDamage(C, 40.f, GetController(), this, UDamageType::StaticClass());
					FVector KnockDir = (C->GetActorLocation() - SlamPos).GetSafeNormal();
					C->LaunchCharacter(KnockDir * 1500.f + FVector(0,0,500), true, true);
				}
			}
		}
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, TEXT("GROUND SLAM!"));

	// Screen shake for player
	AASPlayerController* PC = Cast<AASPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC && PC->GetPawn())
	{
		float PlayerDist = FVector::Dist(SlamPos, PC->GetPawn()->GetActorLocation());
		if (PlayerDist < 600.f)
		{
			float Intensity = 1.f - PlayerDist / 600.f;
			PC->AddPitchInput(FMath::RandRange(-Intensity * 3.f, Intensity * 3.f));
			PC->AddYawInput(FMath::RandRange(-Intensity * 2.f, Intensity * 2.f));
		}
	}
}

float AASWarden::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bShieldActive)
	{
		// Shield absorbs 80% of damage
		Damage *= 0.2f;
		DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0,0,50), 130.f, 12,
			FColor(100, 200, 255), false, 0.1f, 0, 3.f);
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void AASWarden::StartBossFight()
{
	bFightStarted = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("=== WARDEN DETECTED ==="));
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("\"You won't leave this room alive, human.\""));
	}

	// Lock nearby doors
	TArray<AActor*> AllDoors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASControlledDoor::StaticClass(), AllDoors);
	for (AActor* A : AllDoors)
	{
		AASControlledDoor* Door = Cast<AASControlledDoor>(A);
		if (Door && FVector::Dist(GetActorLocation(), Door->GetActorLocation()) < 3000.f)
		{
			Door->LockDoor();
		}
	}

	// Start spawning drones on timer
	GetWorldTimerManager().SetTimer(DroneSpawnTimerHandle, this, &AASWarden::SpawnDrone, DroneSpawnInterval, true, 5.f);
}

void AASWarden::SpawnDrone()
{
	if (DronesSpawned >= MaxDrones || !DroneClass) return;

	FVector SpawnLoc = GetActorLocation() + FVector(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), 200.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AASScoutDrone* Drone = GetWorld()->SpawnActor<AASScoutDrone>(DroneClass, SpawnLoc, FRotator::ZeroRotator, Params);
	if (Drone)
	{
		DronesSpawned++;
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
				FString::Printf(TEXT("WARDEN spawns Scout Drone! (%d/%d)"), DronesSpawned, MaxDrones));
	}
}

void AASWarden::OnDeath()
{
	GetWorldTimerManager().ClearTimer(DroneSpawnTimerHandle);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("=== WARDEN DESTROYED ==="));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("The sector's defenses are failing..."));
	}

	Super::OnDeath();
}
