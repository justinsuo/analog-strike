#include "ASSniper.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AASSniper::AASSniper()
{
	MaxHealth = 80.f;
	CurrentHealth = 80.f;
	MoveSpeed = 250.f;
	AttackDamage = 45.f;
	ShootRange = 5000.f;
	ShootSpread = 0.5f;
	ShootCooldown = 3.f;
	DetectionRange = 5000.f;
	BurstCount = 1;
}

void AASSniper::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Charging shot — show laser sight warning
	if (bIsCharging && PlayerRef)
	{
		ChargeTimer += DeltaTime;
		FVector MuzzleLoc = GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0, 0, 50);
		LaserTarget = PlayerRef->GetActorLocation();

		// Laser sight gets brighter as it charges
		float ChargePct = FMath::Clamp(ChargeTimer / ChargeTime, 0.f, 1.f);
		int32 Alpha = (int32)(50 + ChargePct * 200);
		float Thickness = 0.5f + ChargePct * 2.f;
		DrawDebugLine(GetWorld(), MuzzleLoc, LaserTarget,
			FColor(255, 30, 30, Alpha), false, -1.f, 0, Thickness);

		// Pulsing dot on target
		if (ChargePct > 0.5f)
		{
			float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 15.f) * 0.5f + 0.5f;
			DrawDebugPoint(GetWorld(), LaserTarget, 4.f + Pulse * 4.f,
				FColor(255, 50, 30), false, -1.f);
		}

		// Fire when charged
		if (ChargeTimer >= ChargeTime)
		{
			bIsCharging = false;
			ChargeTimer = 0.f;

			// Execute the actual shot
			FVector Dir = (LaserTarget - MuzzleLoc).GetSafeNormal();
			Dir = FMath::VRandCone(Dir, FMath::DegreesToRadians(ShootSpread));

			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			FVector TraceEnd = MuzzleLoc + Dir * ShootRange;
			if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLoc, TraceEnd, ECC_Visibility, Params))
			{
				// Bright sniper tracer (white-hot)
				DrawDebugLine(GetWorld(), MuzzleLoc, Hit.ImpactPoint, FColor(255, 255, 200), false, 0.2f, 0, 1.5f);
				DrawDebugLine(GetWorld(), MuzzleLoc, Hit.ImpactPoint, FColor(255, 100, 50, 100), false, 0.3f, 0, 3.f);
				// Big muzzle flash
				DrawDebugPoint(GetWorld(), MuzzleLoc, 10.f, FColor(255, 200, 100), false, 0.05f);
				// Impact
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 6.f, FColor(255, 200, 100), false, 0.1f);
				DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 15.f, 8, FColor(255, 150, 50), false, 0.15f);

				if (Hit.GetActor())
				{
					UGameplayStatics::ApplyDamage(Hit.GetActor(), AttackDamage, GetController(), this, UDamageType::StaticClass());
				}
			}
			else
			{
				DrawDebugLine(GetWorld(), MuzzleLoc, TraceEnd, FColor(255, 255, 200, 150), false, 0.15f, 0, 1.f);
			}

			// Long cooldown after shot
			bCanShoot = false;
			GetWorldTimerManager().SetTimer(ShootTimerHandle, [this]() { bCanShoot = true; }, ShootCooldown, false);
		}
	}
}

void AASSniper::ChasePlayer(float DeltaTime)
{
	if (!PlayerRef) return;
	float Dist = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());

	// Snipers prefer to STAY FAR — retreat if player gets close
	if (Dist < ShootRange * 0.3f)
	{
		FVector Away = (GetActorLocation() - PlayerRef->GetActorLocation()).GetSafeNormal();
		AddMovementInput(Away, 1.0f);
	}
	else if (Dist > ShootRange * 0.8f)
	{
		FVector Toward = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		AddMovementInput(Toward, 0.5f);
	}
	// Otherwise hold position

	// Always face player
	FVector ToPlayer = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FRotator LookAt = ToPlayer.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookAt, DeltaTime, 3.f));
}

void AASSniper::AttackPlayer()
{
	if (!bCanShoot || !PlayerRef || bIsStunned || bIsEMPDisabled) return;
	if (bIsCharging) return; // Already charging

	// Start charging instead of firing immediately
	bIsCharging = true;
	ChargeTimer = 0.f;
	bCanShoot = false;
}
