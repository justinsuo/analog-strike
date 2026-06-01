#include "ASExplosiveBarrel.h"
#include "ASEnemyBase.h"
#include "ASPlayerController.h"
#include "ASHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AASExplosiveBarrel::AASExplosiveBarrel()
{
	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	RootComponent = BarrelMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder"));
	if (Cyl.Succeeded()) BarrelMesh->SetStaticMesh(Cyl.Object);
	BarrelMesh->SetWorldScale3D(FVector(0.4f, 0.4f, 0.6f));
	BarrelMesh->SetCollisionProfileName(TEXT("BlockAll"));

	WarningLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WarningLight"));
	WarningLight->SetupAttachment(RootComponent);
	WarningLight->SetRelativeLocation(FVector(0, 0, 40));
	WarningLight->SetIntensity(2000.f);
	WarningLight->SetAttenuationRadius(200.f);
	WarningLight->SetLightColor(FLinearColor(1.f, 0.4f, 0.1f)); // Orange = explosive
}

float AASExplosiveBarrel::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bExploded) return 0.f;
	Health -= Damage;
	if (Health <= 0.f) Explode();
	return Damage;
}

void AASExplosiveBarrel::Explode()
{
	bExploded = true;
	FVector Loc = GetActorLocation();

	// Visual explosion
	DrawDebugSphere(GetWorld(), Loc, ExplosionRadius * 0.3f, 16, FColor(255, 150, 30), false, 0.5f, 0, 4.f);
	DrawDebugSphere(GetWorld(), Loc, ExplosionRadius * 0.6f, 20, FColor(255, 100, 20, 150), false, 0.8f, 0, 2.f);
	DrawDebugSphere(GetWorld(), Loc, ExplosionRadius, 24, FColor(255, 50, 10, 80), false, 1.2f, 0, 1.f);

	// Debris/sparks
	for (int32 i = 0; i < 16; i++)
	{
		FVector SparkDir = FMath::VRand() * FMath::RandRange(100.f, ExplosionRadius * 0.8f);
		SparkDir.Z = FMath::Abs(SparkDir.Z); // Upward bias
		DrawDebugLine(GetWorld(), Loc, Loc + SparkDir,
			FColor(255, FMath::RandRange(100, 255), FMath::RandRange(20, 80)),
			false, 0.6f, 0, FMath::RandRange(0.5f, 2.f));
	}

	// AoE damage
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetWorld()->SweepMultiByChannel(Hits, Loc, Loc, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(ExplosionRadius), Params))
	{
		TSet<AActor*> DamagedActors;
		for (auto& H : Hits)
		{
			AActor* HitActor = H.GetActor();
			if (!HitActor || DamagedActors.Contains(HitActor)) continue;
			DamagedActors.Add(HitActor);

			// Damage falloff by distance
			float Dist = FVector::Dist(Loc, HitActor->GetActorLocation());
			float Falloff = 1.f - FMath::Clamp(Dist / ExplosionRadius, 0.f, 1.f);
			float FinalDmg = ExplosionDamage * Falloff;

			UGameplayStatics::ApplyDamage(HitActor, FinalDmg, nullptr, this, UDamageType::StaticClass());

			// Knockback
			ACharacter* C = Cast<ACharacter>(HitActor);
			if (C)
			{
				FVector KnockDir = (C->GetActorLocation() - Loc).GetSafeNormal();
				C->LaunchCharacter(KnockDir * 1200.f * Falloff + FVector(0, 0, 400 * Falloff), true, true);
			}
		}
	}

	// Kill feed + screen shake
	AASPlayerController* PC = Cast<AASPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (PC)
	{
		AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());
		if (HUD)
		{
			HUD->AddKillFeed(TEXT("Barrel EXPLODED!"), FColor(255, 150, 30));
			HUD->DamageFlash = 0.3f; // Brief flash from explosion light
		}

		// Camera shake proportional to distance
		if (PC->GetPawn())
		{
			float PlayerDist = FVector::Dist(Loc, PC->GetPawn()->GetActorLocation());
			if (PlayerDist < ExplosionRadius * 2.f)
			{
				float ShakeIntensity = 1.f - FMath::Clamp(PlayerDist / (ExplosionRadius * 2.f), 0.f, 1.f);
				PC->AddPitchInput(FMath::RandRange(-ShakeIntensity * 2.f, ShakeIntensity * 2.f));
				PC->AddYawInput(FMath::RandRange(-ShakeIntensity, ShakeIntensity));
			}
		}
	}

	// Chain reaction — damage nearby barrels
	TArray<AActor*> NearbyBarrels;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASExplosiveBarrel::StaticClass(), NearbyBarrels);
	for (AActor* B : NearbyBarrels)
	{
		if (B == this) continue;
		float BDist = FVector::Dist(Loc, B->GetActorLocation());
		if (BDist < ExplosionRadius * 0.8f)
		{
			// Delay chain explosion slightly
			FTimerHandle ChainTimer;
			GetWorldTimerManager().SetTimer(ChainTimer, [B]()
			{
				AASExplosiveBarrel* OtherBarrel = Cast<AASExplosiveBarrel>(B);
				if (OtherBarrel && !OtherBarrel->bExploded)
					OtherBarrel->Explode();
			}, FMath::RandRange(0.1f, 0.3f), false);
		}
	}

	// Hide the barrel mesh
	BarrelMesh->SetVisibility(false);
	BarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WarningLight->SetIntensity(0.f);

	// Destroy after effects finish
	SetLifeSpan(2.f);
}
