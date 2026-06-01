#include "ASBuilderUnit.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

AASBuilderUnit::AASBuilderUnit()
{
	MaxHealth = 400.f;
	CurrentHealth = 400.f;
	MoveSpeed = 200.f;
	AttackDamage = 40.f;
	AttackRange = 250.f;
	DetectionRange = 1500.f;
	AttackCooldown = 2.5f;
	OriginalSpeed = MoveSpeed;

	GetCapsuleComponent()->SetCapsuleHalfHeight(120.f);
	GetCapsuleComponent()->SetCapsuleRadius(60.f);
}

void AASBuilderUnit::AttackPlayer()
{
	if (!bCanAttack || !PlayerRef || bIsStunned || bIsEMPDisabled) return;
	bCanAttack = false;

	// Heavy slam - area damage
	DrawDebugSphere(GetWorld(), GetActorLocation(), SlamRadius, 16, FColor::Red, false, 1.f, 0, 3.f);

	float Dist = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());
	if (Dist <= SlamRadius)
	{
		UGameplayStatics::ApplyDamage(PlayerRef, AttackDamage, GetController(), this, UDamageType::StaticClass());

		// Knockback player
		FVector KnockDir = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		ACharacter* PlayerChar = Cast<ACharacter>(PlayerRef);
		if (PlayerChar)
		{
			PlayerChar->LaunchCharacter(KnockDir * SlamForce + FVector(0, 0, 400), true, true);
		}

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("BUILDER UNIT: GROUND SLAM!"));
	}

	GetWorldTimerManager().SetTimer(AttackTimerHandle, [this]() { bCanAttack = true; }, AttackCooldown, false);
}

void AASBuilderUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsStunned || bIsEMPDisabled) return;

	// Repair nearby damaged allies
	RepairTimer -= DeltaTime;
	if (RepairTimer <= 0.f)
	{
		RepairTimer = RepairCooldown;
		TArray<AActor*> AllEnemies;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASEnemyBase::StaticClass(), AllEnemies);
		for (AActor* A : AllEnemies)
		{
			if (A == this) continue;
			AASEnemyBase* Ally = Cast<AASEnemyBase>(A);
			if (!Ally || Ally->CurrentHealth <= 0) continue;
			if (Ally->CurrentHealth >= Ally->MaxHealth) continue;

			float Dist = FVector::Dist(GetActorLocation(), Ally->GetActorLocation());
			if (Dist < RepairRange)
			{
				Ally->CurrentHealth = FMath::Min(Ally->MaxHealth, Ally->CurrentHealth + RepairAmount);
				// Repair beam visual
				DrawDebugLine(GetWorld(), GetActorLocation() + FVector(0,0,80),
					Ally->GetActorLocation() + FVector(0,0,80),
					FColor(50, 200, 255), false, RepairCooldown * 0.5f, 0, 1.5f);
				DrawDebugString(GetWorld(), Ally->GetActorLocation() + FVector(0,0,160),
					TEXT("+REPAIR"), nullptr, FColor::Cyan, 1.f);
				break; // Only repair one ally per cycle
			}
		}
	}
}

void AASBuilderUnit::OnStagger()
{
	bIsStunned = true;
	GetCharacterMovement()->MaxWalkSpeed = 0.f;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("BUILDER UNIT STAGGERED!"));

	GetWorldTimerManager().SetTimer(StaggerTimerHandle, [this]()
	{
		bIsStunned = false;
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	}, 2.f, false);
}
