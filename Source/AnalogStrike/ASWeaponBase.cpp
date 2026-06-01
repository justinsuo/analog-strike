#include "ASWeaponBase.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"

AASWeaponBase::AASWeaponBase()
{
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded()) WeaponMesh->SetStaticMesh(CubeMesh.Object);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WeaponMesh);
	MuzzlePoint->SetRelativeLocation(FVector(60.f, 0.f, 0.f));
}

void AASWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentAmmo = MaxAmmo;
}

void AASWeaponBase::FireWeapon(AController* WeaponInstigator, FVector CamLoc, FRotator CamRot)
{
	if (!bCanFire || bIsReloading) return;
	if (CurrentAmmo <= 0) { Reload(); return; }

	bCanFire = false;
	CurrentAmmo--;

	for (int32 i = 0; i < FMath::Max(1, PelletCount); i++)
	{
		FVector Dir = CamRot.Vector();
		if (SpreadAngle > 0.f)
			Dir = FMath::VRandCone(Dir, FMath::DegreesToRadians(SpreadAngle));

		FVector TraceEnd = CamLoc + Dir * Range;
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (GetOwner()) Params.AddIgnoredActor(GetOwner());

		bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_Visibility, Params);

		if (bHit)
		{
			// Tracer line
			DrawDebugLine(GetWorld(), CamLoc, Hit.ImpactPoint, FColor::Red, false, 0.2f, 0, 1.5f);
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 8.f, FColor::Yellow, false, 0.2f);

			// Weak point damage multiplier
			float DmgMult = 1.f;
			if (Hit.GetActor())
			{
				FVector Local = Hit.GetActor()->GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
				// Head/optics = top 20% = 3x
				if (Local.Z > 150.f) { DmgMult = 3.f; DrawDebugString(GetWorld(), Hit.ImpactPoint, TEXT("CRIT!"), nullptr, FColor::Red, 0.3f); }
				// Back hit = power cell = 2x
				else if (FVector::DotProduct(Dir, Hit.GetActor()->GetActorForwardVector()) > 0.5f) { DmgMult = 2.f; }

				UGameplayStatics::ApplyDamage(Hit.GetActor(), Damage * DmgMult, WeaponInstigator, this, UDamageType::StaticClass());
			}
		}
		else
		{
			DrawDebugLine(GetWorld(), CamLoc, TraceEnd, FColor::Orange, false, 0.1f, 0, 0.5f);
		}
	}

	// Recoil
	ApplyRecoil(WeaponInstigator);

	// Fire rate cooldown
	GetWorldTimerManager().SetTimer(FireCooldownHandle, [this]() { bCanFire = true; }, FireRate, false);
}

void AASWeaponBase::Reload()
{
	if (bIsReloading || CurrentAmmo >= MaxAmmo) return;
	bIsReloading = true;

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::White,
		FString::Printf(TEXT("Reloading %s..."), *WeaponName));

	GetWorldTimerManager().SetTimer(ReloadHandle, [this]()
	{
		CurrentAmmo = MaxAmmo;
		bIsReloading = false;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, TEXT("Reloaded"));
	}, ReloadTime, false);
}

void AASWeaponBase::StopReload()
{
	GetWorldTimerManager().ClearTimer(ReloadHandle);
	bIsReloading = false;
}

void AASWeaponBase::ApplyRecoil(AController* C)
{
	APlayerController* PC = Cast<APlayerController>(C);
	if (PC)
	{
		float Pitch = -RecoilPitch + FMath::RandRange(-RecoilPitch * 0.2f, 0.f);
		float Yaw = FMath::RandRange(-RecoilYaw, RecoilYaw);
		PC->AddPitchInput(Pitch);
		PC->AddYawInput(Yaw);
	}
}
