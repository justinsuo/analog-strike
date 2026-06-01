#include "ASSecurityFrame.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AASSecurityFrame::AASSecurityFrame()
{
	MaxHealth = 150.f;
	CurrentHealth = 150.f;
	MoveSpeed = 350.f;
	AttackDamage = 15.f;
	AttackRange = ShootRange;
	DetectionRange = 2500.f;
	AttackCooldown = ShootCooldown;
}

void AASSecurityFrame::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (RepositionTimer > 0) RepositionTimer -= DeltaTime;
}

void AASSecurityFrame::ChasePlayer(float DeltaTime)
{
	if (!PlayerRef) return;
	float Dist = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());

	// Stay at medium range — advance if too far, retreat if too close
	if (Dist > ShootRange * 0.7f)
	{
		FVector Dir = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		AddMovementInput(Dir, 0.7f);
	}
	else if (Dist < ShootRange * 0.3f)
	{
		// Too close — back away
		FVector Dir = (GetActorLocation() - PlayerRef->GetActorLocation()).GetSafeNormal();
		AddMovementInput(Dir, 0.8f);
	}

	// Periodically reposition — seek cover between shots
	if (RepositionTimer <= 0.f && Dist < ShootRange * 0.8f && bCanShoot)
	{
		FVector ToPlayer = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		FVector StrafeDir = FVector::CrossProduct(ToPlayer, FVector::UpVector);
		float StrafeSign = (GetUniqueID() % 2 == 0) ? 1.f : -1.f;
		if (FMath::Sin(GetWorld()->GetTimeSeconds() * 0.5f + GetUniqueID()) > 0) StrafeSign *= -1.f;
		AddMovementInput(StrafeDir * StrafeSign, 0.5f);
	}

	// Always face player
	FVector ToPlayer = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FRotator LookAt = ToPlayer.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAt, DeltaTime, 5.f));
}

void AASSecurityFrame::AttackPlayer()
{
	if (!bCanShoot || !PlayerRef || bIsStunned || bIsEMPDisabled) return;
	bCanShoot = false;

	// Start burst fire sequence
	BurstShotsRemaining = BurstCount;
	auto FireOneBurst = [this]()
	{
		if (!IsValid(this) || !PlayerRef || BurstShotsRemaining <= 0) return;
		BurstShotsRemaining--;

		FVector MuzzleLoc = GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0, 0, 50);
		FVector TargetLoc = PlayerRef->GetActorLocation();
		FVector Dir = (TargetLoc - MuzzleLoc).GetSafeNormal();
		Dir = FMath::VRandCone(Dir, FMath::DegreesToRadians(ShootSpread));

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		FVector TraceEnd = MuzzleLoc + Dir * ShootRange;
		if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLoc, TraceEnd, ECC_Visibility, Params))
		{
			DrawDebugLine(GetWorld(), MuzzleLoc, Hit.ImpactPoint, FColor(255, 80, 30), false, 0.1f, 0, 0.6f);
			DrawDebugLine(GetWorld(), MuzzleLoc, Hit.ImpactPoint, FColor(255, 40, 10, 80), false, 0.15f, 0, 1.5f);
			DrawDebugPoint(GetWorld(), MuzzleLoc, 6.f, FColor(255, 150, 50), false, 0.03f);
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 4.f, FColor(255, 100, 50), false, 0.06f);

			if (Hit.GetActor())
				UGameplayStatics::ApplyDamage(Hit.GetActor(), AttackDamage, GetController(), this, UDamageType::StaticClass());
		}
		else
		{
			DrawDebugLine(GetWorld(), MuzzleLoc, TraceEnd, FColor(255, 80, 30, 100), false, 0.08f, 0, 0.4f);
		}
	};

	// Fire first shot immediately
	FireOneBurst();

	// Remaining shots in burst
	if (BurstCount > 1)
	{
		GetWorldTimerManager().SetTimer(BurstTimerHandle, FireOneBurst, BurstInterval, true, BurstInterval);
		// Stop burst after all shots
		FTimerHandle StopBurst;
		GetWorldTimerManager().SetTimer(StopBurst, [this]()
		{
			GetWorldTimerManager().ClearTimer(BurstTimerHandle);
		}, BurstInterval * (BurstCount - 1) + 0.01f, false);
	}

	// Reposition after burst
	RepositionTimer = ShootCooldown * 0.5f;
	GetWorldTimerManager().SetTimer(ShootTimerHandle, [this]() { bCanShoot = true; }, ShootCooldown, false);
}
