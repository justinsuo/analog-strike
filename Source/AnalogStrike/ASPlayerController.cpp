#include "ASPlayerController.h"
#include "ASHUD.h"
#include "ASEnemyBase.h"
#include "Components/SpotLightComponent.h"
#include "ASControlledDoor.h"
#include "ASBreakerBox.h"
#include "ASValve.h"
#include "ASNPC.h"
#include "ASBuilderUnit.h"
#include "ASTurret.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"

AASPlayerController::AASPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bLastFire = bLastReload = bLastGrenade = bLastPunch = false;
	bLastWeapon1 = bLastWeapon2 = bLastWeapon3 = bLastInteract = bLastDodge = false;
	bLastAim = bLastSprint = bLastPause = bLastCrouch = bLastScrollUp = bLastScrollDown = false;

	// Define weapon configurations
	// 0: Assault Rifle
	WeaponConfigs.Add({TEXT("Assault Rifle"), 30, 1, 18.f, 2.5f, 0.5f, 0.12f, 2.0f, 0.6f, 0.2f, true, false});
	// 1: Heavy Revolver
	WeaponConfigs.Add({TEXT("Heavy Revolver"), 6, 1, 60.f, 1.0f, 0.2f, 0.5f, 2.5f, 2.0f, 0.4f, false, false});
	// 2: Shotgun
	WeaponConfigs.Add({TEXT("Pump Shotgun"), 8, 8, 12.f, 5.f, 3.f, 0.7f, 2.2f, 1.5f, 0.5f, false, false});
	// 3: Combat Knife
	WeaponConfigs.Add({TEXT("Combat Knife"), 0, 0, 65.f, 0.f, 0.f, 0.4f, 0.f, 0.f, 0.f, false, true});
	// 4: Sniper Rifle (high damage, slow, precise)
	WeaponConfigs.Add({TEXT("Sniper Rifle"), 5, 1, 150.f, 0.3f, 0.05f, 1.2f, 3.0f, 3.5f, 0.2f, false, false});
	// 5: Grenade Launcher (explosive AoE)
	WeaponConfigs.Add({TEXT("Grenade Launcher"), 4, 1, 80.f, 2.f, 1.f, 1.5f, 3.5f, 4.f, 0.5f, false, false});
}

void AASPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (UEnhancedInputLocalPlayerSubsystem* EIS =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Default.IMC_Default"));
		if (IMC) EIS->AddMappingContext(IMC, 0);
		UInputMappingContext* IMC2 = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
		if (IMC2) EIS->AddMappingContext(IMC2, 1);
	}

	// Start with Assault Rifle — set initial state directly since SwitchWeapon skips if same index
	WeaponIndex = -1; // Force switch
	SwitchWeapon(0);
}

void AASPlayerController::SetupInputComponent() { Super::SetupInputComponent(); }

// ═══════════════════════════════════════════════════════════════════════════════
// TPS AIMING
// ═══════════════════════════════════════════════════════════════════════════════

bool AASPlayerController::GetCrosshairWorldRay(FVector& OutStart, FVector& OutDir)
{
	FVector2D VP;
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(VP);
	else return false;
	FVector Pos, Dir;
	bool OK = UGameplayStatics::DeprojectScreenToWorld(this, FVector2D(VP.X/2, VP.Y/2), Pos, Dir);
	if (OK) { OutStart = Pos; OutDir = Dir; }
	return OK;
}

bool AASPlayerController::GetAimHitLocation(FVector& Out)
{
	// Reference: github.com/intrxx/Multiplayer-Shooter TraceUnderCrosshair
	FVector CrosshairStart, CrosshairDir;
	if (!GetCrosshairWorldRay(CrosshairStart, CrosshairDir)) return false;

	// KEY FIX: Start the trace PAST the character to avoid self-hits
	// This is what the reference project does
	APawn* PP = GetPawn();
	if (PP)
	{
		float DistToChar = (PP->GetActorLocation() - CrosshairStart).Size();
		CrosshairStart += CrosshairDir * (DistToChar + 100.f);
	}

	FVector TraceEnd = CrosshairStart + CrosshairDir * 50000.f;
	FHitResult Hit;
	FCollisionQueryParams P;
	if (PP) P.AddIgnoredActor(PP);
	if (WeaponMeshActor) P.AddIgnoredActor(WeaponMeshActor);

	if (GetWorld()->LineTraceSingleByChannel(Hit, CrosshairStart, TraceEnd, ECC_Visibility, P))
	{
		Out = Hit.Location;
		return true;
	}
	Out = TraceEnd;
	return false;
}

FVector AASPlayerController::GetMuzzleLocation()
{
	// Get muzzle from CHARACTER's forward direction + height offset
	// This ensures tracers come from where the weapon visually is
	APawn* PP = GetPawn();
	if (PP)
	{
		FVector CharLoc = PP->GetActorLocation();
		FVector CharFwd = PP->GetActorForwardVector();
		FVector CharRight = PP->GetActorRightVector();
		// Muzzle: forward from character, offset right (weapon is in right hand)
		// and up to chest/shoulder height
		return CharLoc + CharFwd * 60.f + CharRight * 25.f + FVector(0, 0, 55.f);
	}
	return FVector::ZeroVector;
}

// ═══════════════════════════════════════════════════════════════════════════════
// WEAPON MESH
// ═══════════════════════════════════════════════════════════════════════════════

void AASPlayerController::SpawnWeaponMesh()
{
	APawn* P = GetPawn();
	if (!P || WeaponMeshActor) return;
	ACharacter* Ch = Cast<ACharacter>(P);
	if (!Ch || !Ch->GetMesh()) return;

	FActorSpawnParameters SP;
	SP.Owner = P;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	WeaponMeshActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SP);
	if (!WeaponMeshActor) return;

	UStaticMeshComponent* Gun = NewObject<UStaticMeshComponent>(WeaponMeshActor);
	Gun->RegisterComponent();
	WeaponMeshActor->SetRootComponent(Gun);

	// Try to load the REAL rifle mesh from Fab first, then fallback to procedural
	FString MeshNames[] = {TEXT("SM_RealRifle"), TEXT("SM_Pistol"), TEXT("SM_RealRifle"), TEXT("SM_Knife"), TEXT("SM_RealRifle"), TEXT("SM_RealRifle")};
	FString MN = (WeaponIndex < 6) ? MeshNames[WeaponIndex] : TEXT("SM_RealRifle");

	UStaticMesh* WM = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/AnalogStrike/Weapons/%s.%s"), *MN, *MN));
	// Fallback to procedural mesh
	if (!WM) WM = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/AnalogStrike/Weapons/SM_Rifle.SM_Rifle")));
	if (!WM) WM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));

	bool bFallback = WM && (WM->GetName() == TEXT("Cube") || WM->GetName() == TEXT("Cylinder"));
	if (!WM)
	{
		// If absolutely nothing loaded, try cylinder for barrel-like shape
		WM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder"));
		bFallback = true;
	}
	if (WM)
	{
		Gun->SetStaticMesh(WM);
		if (bFallback)
		{
			switch (WeaponIndex)
			{
			case 0: // AR — long cylinder
				Gun->SetWorldScale3D(FVector(0.03f, 0.03f, 0.5f));
				Gun->SetRelativeRotation(FRotator(90, 0, 0));
				break;
			case 1: // Revolver — short cylinder
				Gun->SetWorldScale3D(FVector(0.035f, 0.035f, 0.25f));
				Gun->SetRelativeRotation(FRotator(90, 0, 0));
				break;
			case 2: // Shotgun — thick cylinder
				Gun->SetWorldScale3D(FVector(0.04f, 0.04f, 0.5f));
				Gun->SetRelativeRotation(FRotator(90, 0, 0));
				break;
			case 3: // Knife — thin flat
				Gun->SetWorldScale3D(FVector(0.01f, 0.015f, 0.25f));
				Gun->SetRelativeRotation(FRotator(90, 0, 0));
				break;
			case 4: // Sniper — extra long
				Gun->SetWorldScale3D(FVector(0.025f, 0.025f, 0.7f));
				Gun->SetRelativeRotation(FRotator(90, 0, 0));
				break;
			case 5: // Grenade Launcher — thick short barrel
				Gun->SetWorldScale3D(FVector(0.05f, 0.05f, 0.35f));
				Gun->SetRelativeRotation(FRotator(90, 0, 0));
				break;
			default:
				Gun->SetWorldScale3D(FVector(0.03f, 0.03f, 0.4f));
				break;
			}
		}
		else
		{
			Gun->SetWorldScale3D(FVector(1.f));
		}
		Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Color the weapon dark gunmetal
		if (Gun->GetMaterial(0))
		{
			UMaterialInstanceDynamic* GunMat = UMaterialInstanceDynamic::Create(Gun->GetMaterial(0), WeaponMeshActor);
			if (GunMat)
			{
				if (WeaponIndex == 3) // Knife — silver
				{
					GunMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.6f, 0.6f, 0.65f));
					GunMat->SetScalarParameterValue(TEXT("Metallic"), 0.9f);
					GunMat->SetScalarParameterValue(TEXT("Roughness"), 0.2f);
				}
				else // Guns — dark gunmetal
				{
					GunMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.08f, 0.08f, 0.1f));
					GunMat->SetScalarParameterValue(TEXT("Metallic"), 0.7f);
					GunMat->SetScalarParameterValue(TEXT("Roughness"), 0.4f);
				}
				Gun->SetMaterial(0, GunMat);
			}
		}
	}

	// Try multiple socket names — different skeletons use different names
	FName SocketNames[] = {TEXT("hand_r"), TEXT("ik_hand_gun"), TEXT("GripPoint"), TEXT("weapon_r"), TEXT("RightHand")};
	bool bAttached = false;
	for (const FName& Sock : SocketNames)
	{
		const USkeletalMeshSocket* S = Ch->GetMesh()->GetSocketByName(Sock);
		if (S)
		{
			S->AttachActor(WeaponMeshActor, Ch->GetMesh());
			bAttached = true;
			break;
		}
	}
	if (!bAttached)
	{
		// Fallback: attach to hand_r bone directly
		WeaponMeshActor->AttachToComponent(Ch->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("hand_r"));
	}

	// Position weapon so barrel points where character faces
	// The Fab rifle FBX has Y-forward (bounds: Y -27 to +57)
	// On hand_r socket we need to rotate so Y aligns with the hand forward direction
	// Try multiple rotations based on weapon type
	if (WeaponIndex == 3) // Knife
	{
		WeaponMeshActor->SetActorRelativeLocation(FVector(5, 0, 0));
		WeaponMeshActor->SetActorRelativeRotation(FRotator(0, 90, 0));
	}
	else // Guns
	{
		WeaponMeshActor->SetActorRelativeLocation(FVector(0, 5, -3));
		WeaponMeshActor->SetActorRelativeRotation(FRotator(-10, 90, 0));
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// TICK
// ═══════════════════════════════════════════════════════════════════════════════

void AASPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!GetPawn() || !IsLocalController()) return;

	ACharacter* MyChar = Cast<ACharacter>(GetPawn());

	// Bind damage delegate (once pawn is available) — retry every tick until bound
	if (!bDamageBound)
	{
		APawn* MyPawn = GetPawn();
		if (MyPawn)
		{
			MyPawn->OnTakeAnyDamage.AddDynamic(this, &AASPlayerController::OnPawnTakeDamage);
			bDamageBound = true;

			// Also set double jump here (one-time)
			ACharacter* JC = Cast<ACharacter>(MyPawn);
			if (JC)
			{
				JC->JumpMaxCount = 2;
				JC->JumpMaxHoldTime = 0.2f;
			}
		}
	}

	// One-time setup
	if (!bWeaponSetupDone && MyChar && MyChar->GetMesh() && MyChar->GetMesh()->GetNumBones() > 0)
	{
		SpawnWeaponMesh();
		MyChar->bUseControllerRotationYaw = true;
		MyChar->GetCharacterMovement()->bOrientRotationToMovement = false;

		// Fix camera: force look forward on spawn
		SetControlRotation(FRotator(-10.f, GetControlRotation().Yaw, 0.f));
		// Also reset spring arm rotation
		TArray<USpringArmComponent*> Arms;
		MyChar->GetComponents<USpringArmComponent>(Arms);
		for (USpringArmComponent* Arm : Arms)
		{
			Arm->SetRelativeRotation(FRotator(-10.f, 0.f, 0.f));
			Arm->SocketOffset = FVector(0.f, 70.f, 25.f);
			Arm->TargetArmLength = 220.f;
		}

		// ── TACTICAL GEAR ATTACHMENTS ──
		// (Character mesh/anim is now set in ASPlayerCharacter constructor)
		if (MyChar && MyChar->GetMesh())
		{
			USkeletalMeshComponent* CharMesh = MyChar->GetMesh();

			// Add tactical gear attachments (shoulder pads, helmet visor, backpack)
			UStaticMesh* CubeSM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
			UStaticMesh* SphereSM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
			UStaticMesh* CylSM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder"));

			if (CubeSM)
			{
				// Shoulder armor (left)
				UStaticMeshComponent* LShoulder = NewObject<UStaticMeshComponent>(MyChar);
				LShoulder->RegisterComponent();
				LShoulder->SetStaticMesh(CubeSM);
				LShoulder->SetWorldScale3D(FVector(0.12f, 0.15f, 0.06f));
				LShoulder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				LShoulder->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("spine_05"));
				LShoulder->SetRelativeLocation(FVector(0, -20, 10));

				// Shoulder armor (right)
				UStaticMeshComponent* RShoulder = NewObject<UStaticMeshComponent>(MyChar);
				RShoulder->RegisterComponent();
				RShoulder->SetStaticMesh(CubeSM);
				RShoulder->SetWorldScale3D(FVector(0.12f, 0.15f, 0.06f));
				RShoulder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				RShoulder->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("spine_05"));
				RShoulder->SetRelativeLocation(FVector(0, 20, 10));

				// Backpack
				UStaticMeshComponent* Backpack = NewObject<UStaticMeshComponent>(MyChar);
				Backpack->RegisterComponent();
				Backpack->SetStaticMesh(CubeSM);
				Backpack->SetWorldScale3D(FVector(0.08f, 0.18f, 0.22f));
				Backpack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Backpack->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("spine_04"));
				Backpack->SetRelativeLocation(FVector(-15, 0, 0));

				// Knee pads
				for (FName Knee : {FName("calf_l"), FName("calf_r")})
				{
					UStaticMeshComponent* Pad = NewObject<UStaticMeshComponent>(MyChar);
					Pad->RegisterComponent();
					Pad->SetStaticMesh(CubeSM);
					Pad->SetWorldScale3D(FVector(0.06f, 0.08f, 0.05f));
					Pad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Pad->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Knee);
					Pad->SetRelativeLocation(FVector(8, 0, 25));
				}

				// Belt pouches
				UStaticMeshComponent* Pouch1 = NewObject<UStaticMeshComponent>(MyChar);
				Pouch1->RegisterComponent();
				Pouch1->SetStaticMesh(CubeSM);
				Pouch1->SetWorldScale3D(FVector(0.04f, 0.06f, 0.05f));
				Pouch1->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Pouch1->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("pelvis"));
				Pouch1->SetRelativeLocation(FVector(15, -12, 0));

				UStaticMeshComponent* Pouch2 = NewObject<UStaticMeshComponent>(MyChar);
				Pouch2->RegisterComponent();
				Pouch2->SetStaticMesh(CubeSM);
				Pouch2->SetWorldScale3D(FVector(0.04f, 0.06f, 0.05f));
				Pouch2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Pouch2->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("pelvis"));
				Pouch2->SetRelativeLocation(FVector(15, 12, 0));
			}

			if (SphereSM)
			{
				// Helmet (sphere on head)
				UStaticMeshComponent* Helmet = NewObject<UStaticMeshComponent>(MyChar);
				Helmet->RegisterComponent();
				Helmet->SetStaticMesh(SphereSM);
				Helmet->SetWorldScale3D(FVector(0.18f, 0.17f, 0.19f));
				Helmet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Helmet->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("head"));
				Helmet->SetRelativeLocation(FVector(0, 0, 0));
			}

			if (CylSM)
			{
				// Visor (cylinder across face)
				UStaticMeshComponent* Visor = NewObject<UStaticMeshComponent>(MyChar);
				Visor->RegisterComponent();
				Visor->SetStaticMesh(CylSM);
				Visor->SetWorldScale3D(FVector(0.12f, 0.03f, 0.04f));
				Visor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Visor->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("head"));
				Visor->SetRelativeLocation(FVector(10, 0, 2));
				Visor->SetRelativeRotation(FRotator(0, 0, 90));
			}

			// Color all gear attachments dark green/black tactical
			TArray<UStaticMeshComponent*> GearParts;
			MyChar->GetComponents<UStaticMeshComponent>(GearParts);
			for (UStaticMeshComponent* GP : GearParts)
			{
				if (GP->GetMaterial(0))
				{
					UMaterialInstanceDynamic* GearMat = UMaterialInstanceDynamic::Create(GP->GetMaterial(0), MyChar);
					if (GearMat)
					{
						if (GP->GetName().Contains(TEXT("Visor")))
						{
							// Visor is dark cyan/teal reflective
							GearMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.02f, 0.08f, 0.1f));
							GearMat->SetScalarParameterValue(TEXT("Metallic"), 0.9f);
							GearMat->SetScalarParameterValue(TEXT("Roughness"), 0.1f);
						}
						else if (GP->GetName().Contains(TEXT("Helmet")))
						{
							GearMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.04f, 0.05f, 0.04f));
							GearMat->SetScalarParameterValue(TEXT("Roughness"), 0.7f);
						}
						else
						{
							// Dark olive/tactical green
							GearMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.06f, 0.07f, 0.05f));
							GearMat->SetScalarParameterValue(TEXT("Roughness"), 0.8f);
						}
						GP->SetMaterial(0, GearMat);
					}
				}
			}

			// (Dark tactical materials are now applied in ASPlayerCharacter::BeginPlay)
		}

		// Create flashlight attached to character
		if (!Flashlight && MyChar)
		{
			Flashlight = NewObject<USpotLightComponent>(MyChar);
			if (Flashlight)
			{
				Flashlight->RegisterComponent();
				Flashlight->AttachToComponent(MyChar->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				Flashlight->SetRelativeLocation(FVector(30, 20, 60));
				Flashlight->SetInnerConeAngle(15.f);
				Flashlight->SetOuterConeAngle(35.f);
				Flashlight->SetIntensity(20000.f);
				Flashlight->SetAttenuationRadius(3000.f);
				Flashlight->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
				Flashlight->SetVisibility(false);
			}
		}

		bWeaponSetupDone = true;
	}

	// Over-the-shoulder camera + FOV effects
	if (MyChar)
	{
		TArray<USpringArmComponent*> SAs;
		MyChar->GetComponents<USpringArmComponent>(SAs);
		for (USpringArmComponent* SA : SAs)
		{
			float TargetY = bIsAiming ? 90.f : 70.f;
			float TargetArm = bIsAiming ? 150.f : 220.f;
			float TargetZ = bIsAiming ? 35.f : 25.f;
			SA->SocketOffset = FMath::VInterpTo(SA->SocketOffset, FVector(0, TargetY, TargetZ), DeltaTime, 8.f);
			SA->TargetArmLength = FMath::FInterpTo(SA->TargetArmLength, TargetArm, DeltaTime, 8.f);
		}

		// Dynamic FOV: narrow when aiming, wide when sprinting, scope zoom for sniper
		if (PlayerCameraManager)
		{
			float TargetFOV = 90.f;
			if (bIsAiming && WeaponIndex == 4) TargetFOV = 35.f; // Sniper scope zoom
			else if (bIsAiming) TargetFOV = 70.f;
			else if (bIsSprinting) TargetFOV = 100.f;
			float CurFOV = PlayerCameraManager->GetFOVAngle();
			PlayerCameraManager->SetFOV(FMath::FInterpTo(CurFOV, TargetFOV, DeltaTime, 6.f));
		}
	}

	// Flashlight follows camera direction
	if (Flashlight && bFlashlightOn)
	{
		FRotator CamRot = GetControlRotation();
		Flashlight->SetWorldRotation(CamRot);
	}

	// Flashlight toggle (F key)
	bool bFlashKey = IsInputKeyDown(EKeys::F);
	if (bFlashKey && !bLastFlashlight) ToggleFlashlight();
	bLastFlashlight = bFlashKey;

	// Wall run detection — only trigger when moving fast and airborne
	if (MyChar && MyChar->GetCharacterMovement()->IsFalling() && !bIsWallRunning)
	{
		float HorizSpeed = FVector(MyChar->GetVelocity().X, MyChar->GetVelocity().Y, 0).Size();
		if (HorizSpeed > 200.f) // Must be moving to wall run
		{
			FVector CharLoc = MyChar->GetActorLocation();
			FVector CharRight = MyChar->GetActorRightVector();
			FHitResult WallHit;
			FCollisionQueryParams WP;
			WP.AddIgnoredActor(MyChar);

			for (float Side : {1.f, -1.f})
			{
				if (GetWorld()->LineTraceSingleByChannel(WallHit, CharLoc, CharLoc + CharRight * Side * 80.f, ECC_WorldStatic, WP))
				{
					bIsWallRunning = true;
					WallRunTimer = 1.5f;
					WallNormal = WallHit.ImpactNormal;
					MyChar->GetCharacterMovement()->GravityScale = 0.15f;
					MyChar->GetCharacterMovement()->Velocity.Z = FMath::Max(MyChar->GetCharacterMovement()->Velocity.Z, 0.f);
					break;
				}
			}
		}
	}

	if (bIsWallRunning && MyChar)
	{
		WallRunTimer -= DeltaTime;
		if (WallRunTimer <= 0.f || MyChar->GetCharacterMovement()->IsMovingOnGround())
		{
			bIsWallRunning = false;
			MyChar->GetCharacterMovement()->GravityScale = 1.f;
		}
		else
		{
			FVector WallDir = FVector::CrossProduct(WallNormal, FVector::UpVector).GetSafeNormal();
			if (FVector::DotProduct(WallDir, MyChar->GetVelocity()) < 0) WallDir *= -1.f;
			MyChar->AddMovementInput(WallDir, 1.f);
			MyChar->GetCharacterMovement()->Velocity.Z = FMath::Max(MyChar->GetCharacterMovement()->Velocity.Z, -50.f);

			DrawDebugLine(GetWorld(), MyChar->GetActorLocation(),
				MyChar->GetActorLocation() - WallNormal * 60.f,
				FColor(100, 200, 255, 150), false, 0.1f, 0, 1.f);
		}

		// Wall jump — edge detection so it only fires once
		static bool bLastWallJump = false;
		bool bJump = IsInputKeyDown(EKeys::SpaceBar);
		if (bJump && !bLastWallJump)
		{
			bIsWallRunning = false;
			MyChar->GetCharacterMovement()->GravityScale = 1.f;
			MyChar->LaunchCharacter(WallNormal * 600.f + FVector(0, 0, 500.f), true, true);
		}
		bLastWallJump = bJump;
	}

	// Speed boost timer
	if (SpeedBoostTimer > 0)
	{
		SpeedBoostTimer -= DeltaTime;
		if (MyChar) MyChar->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(MyChar->GetCharacterMovement()->MaxWalkSpeed, 1000.f);
	}

	// Stamina system
	if (bIsSprinting && Stamina > 0)
	{
		Stamina -= StaminaDrainRate * DeltaTime;
		if (Stamina <= 0)
		{
			Stamina = 0;
			bIsSprinting = false;
			if (MyChar) MyChar->GetCharacterMovement()->MaxWalkSpeed = 400.f;
		}
	}
	else if (!bIsSprinting && Stamina < MaxStamina)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenRate * DeltaTime);
	}

	// Sprint speed — don't override during grapple or wall run
	if (!bGrappling && !bIsWallRunning && MyChar)
	{
		if (bIsSprinting && Stamina > 0)
			MyChar->GetCharacterMovement()->MaxWalkSpeed = 800.f;
		else
			MyChar->GetCharacterMovement()->MaxWalkSpeed = bIsCrouching ? 200.f : 400.f;
	}

	// Health regen when out of combat
	TimeSinceLastDamage += DeltaTime;
	if (TimeSinceLastDamage > HealthRegenDelay && Health < MaxHealth && Health > 0.f)
	{
		Health = FMath::Min(MaxHealth, Health + HealthRegenRate * DeltaTime);
	}

	// Weapon animation (bob, recoil recovery, reload)
	if (WeaponMeshActor && MyChar)
	{
		FVector Vel = MyChar->GetVelocity();
		float Speed = Vel.Size();
		bool bMoving = Speed > 10.f;

		// Weapon bob while moving
		if (bMoving)
		{
			float BobSpeed = bIsSprinting ? 12.f : 8.f;
			float BobAmount = bIsSprinting ? 3.f : 1.5f;
			WeaponBobTime += DeltaTime * BobSpeed;
			FVector BobOffset(0, FMath::Sin(WeaponBobTime) * BobAmount, FMath::Cos(WeaponBobTime * 2.f) * BobAmount * 0.5f);
			FVector RestP = (WeaponIndex == 3) ? FVector(5,0,0) : FVector(0,5,-3);
			WeaponMeshActor->SetActorRelativeLocation(RestP + BobOffset);
		}
		else
		{
			WeaponBobTime = 0.f;
			// Smoothly return to rest position
			FVector RestPos = WeaponIndex == 3 ? FVector(5,0,0) : FVector(0,5,-3);
			FVector CurLoc = WeaponMeshActor->GetRootComponent() ? WeaponMeshActor->GetRootComponent()->GetRelativeLocation() : FVector::ZeroVector;
			WeaponMeshActor->SetActorRelativeLocation(FMath::VInterpTo(CurLoc, RestPos, DeltaTime, 10.f));
		}

		// Recoil recovery (weapon kicks up then comes back)
		if (RecoilRecovery > 0.01f)
		{
			RecoilRecovery = FMath::FInterpTo(RecoilRecovery, 0.f, DeltaTime, 8.f);
			FRotator CurRot = WeaponMeshActor->GetActorRotation();
			FRotator BaseRot = WeaponIndex == 3 ? FRotator(0, 90, 0) : FRotator(-10, 90, 0);
			// Add pitch kick based on recoil
			WeaponMeshActor->SetActorRelativeRotation(BaseRot + FRotator(-RecoilRecovery * 8.f, 0, 0));
		}

		// Reload animation - tilt weapon down during reload
		if (bIsReloading)
		{
			const FWeaponConfig& RWC = WeaponConfigs[FMath::Min(WeaponIndex, WeaponConfigs.Num()-1)];
			ReloadAnimProgress += DeltaTime / RWC.ReloadTime;
			ReloadAnimProgress = FMath::Min(ReloadAnimProgress, 1.f);

			// Weapon tilts down and back during reload
			float TiltCurve = FMath::Sin(ReloadAnimProgress * PI);
			FRotator ReloadRot = (WeaponIndex == 3 ? FRotator(0, 90, 0) : FRotator(-10, 90, 0));
			ReloadRot.Pitch -= TiltCurve * 30.f;
			WeaponMeshActor->SetActorRelativeRotation(ReloadRot);

			FVector ReloadOffset = (WeaponIndex == 3 ? FVector(5,0,0) : FVector(0,5,-3));
			ReloadOffset.Z -= TiltCurve * 8.f;
			ReloadOffset.X -= TiltCurve * 5.f;
			WeaponMeshActor->SetActorRelativeLocation(ReloadOffset);
		}
		else
		{
			ReloadAnimProgress = 0.f;
		}
	}

	// AR spin-down when not firing
	if (!IsInputKeyDown(EKeys::LeftMouseButton))
		ARSpinUp = FMath::Max(0.f, ARSpinUp - DeltaTime * 3.f);

	// Spread calculation
	const FWeaponConfig& WC = WeaponConfigs[FMath::Min(WeaponIndex, WeaponConfigs.Num()-1)];
	float TargetSpread = bIsAiming ? WC.AimSpread : WC.BaseSpread;
	if (bIsSprinting) TargetSpread *= SprintSpreadMult;
	if (bIsCrouching) TargetSpread *= 0.6f; // Crouch bonus
	CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetSpread, DeltaTime, 10.f);

	// Kill streak timer
	if (KillStreakTimer > 0) { KillStreakTimer -= DeltaTime; if (KillStreakTimer <= 0) KillStreak = 0; }

	// Timers
	if (MeleeTimer > 0) MeleeTimer -= DeltaTime;
	if (PunchCooldown > 0) { PunchCooldown -= DeltaTime; if (PunchCooldown <= 0) bPunchReady = true; }

	// ── INPUT ───────────────────────────────────────────────────────────
	bool bFire = IsInputKeyDown(EKeys::LeftMouseButton);
	bool bAim = IsInputKeyDown(EKeys::RightMouseButton);
	bool bSprint = IsInputKeyDown(EKeys::LeftShift);
	bool bReload = IsInputKeyDown(EKeys::R);
	bool bGrenade = IsInputKeyDown(EKeys::G);
	bool bPunch = IsInputKeyDown(EKeys::Q);
	bool bW1 = IsInputKeyDown(EKeys::One);
	bool bW2 = IsInputKeyDown(EKeys::Two);
	bool bW3 = IsInputKeyDown(EKeys::Three);
	bool bW4 = IsInputKeyDown(EKeys::Four);
	bool bInteract = IsInputKeyDown(EKeys::E);
	bool bDodge = IsInputKeyDown(EKeys::LeftAlt);

	bIsAiming = bAim;
	bIsSprinting = bSprint && !bIsAiming;

	// Automatic fire with AR spin-up
	if (WC.bIsAutomatic && bFire)
	{
		ARSpinUp = FMath::Min(ARSpinUp + DeltaTime, 2.f);
		if (bCanFire && !WC.bIsMelee) DoFire();
	}
	else if (bFire && !bLastFire)
	{
		if (WC.bIsMelee) DoKnifeAttack();
		else DoFire();
	}
	bLastFire = bFire;

	if (bReload && !bLastReload) DoReload();
	bLastReload = bReload;
	if (bGrenade && !bLastGrenade) DoGrenade();
	bLastGrenade = bGrenade;
	if (bPunch && !bLastPunch) DoPunch();
	bLastPunch = bPunch;
	if (bW1 && !bLastWeapon1) SwitchWeapon(0);
	bLastWeapon1 = bW1;
	if (bW2 && !bLastWeapon2) SwitchWeapon(1);
	bLastWeapon2 = bW2;
	if (bW3 && !bLastWeapon3) SwitchWeapon(2);
	bLastWeapon3 = bW3;
	static bool bLastW4 = false;
	if (bW4 && !bLastW4) SwitchWeapon(3);
	bLastW4 = bW4;
	bool bW5 = IsInputKeyDown(EKeys::Five);
	static bool bLastW5 = false;
	if (bW5 && !bLastW5) SwitchWeapon(4);
	bLastW5 = bW5;
	bool bW6 = IsInputKeyDown(EKeys::Six);
	static bool bLastW6 = false;
	if (bW6 && !bLastW6) SwitchWeapon(5);
	bLastW6 = bW6;
	if (bInteract && !bLastInteract) DoInteract();
	bLastInteract = bInteract;
	// V = quick dash forward (different from Alt dodge)
	bool bDashKey = IsInputKeyDown(EKeys::V);
	static bool bLastDash = false;
	if (bDashKey && !bLastDash && Stamina >= 25.f && MyChar)
	{
		Stamina -= 25.f;
		FVector DashDir = MyChar->GetActorForwardVector();
		if (bIsAiming)
		{
			// Aim-dash goes toward crosshair
			FVector AimTarget;
			GetAimHitLocation(AimTarget);
			DashDir = (AimTarget - MyChar->GetActorLocation()).GetSafeNormal();
			DashDir.Z = 0;
			DashDir.Normalize();
		}
		MyChar->LaunchCharacter(DashDir * 1200.f + FVector(0,0,50), true, true);

		// Dash trail
		FVector Start = MyChar->GetActorLocation();
		for (int32 t = 0; t < 5; t++)
		{
			DrawDebugPoint(GetWorld(), Start - DashDir * t * 80.f, 4.f,
				FColor(100, 200, 255, 150 - t*25), false, 0.2f);
		}
	}
	bLastDash = bDashKey;

	// Deploy turret (X key)
	bool bDeployKey = IsInputKeyDown(EKeys::X);
	if (bDeployKey && !bLastDeployTurret && DeployableTurrets > 0 && MyChar)
	{
		DeployableTurrets--;
		FVector DeployLoc = MyChar->GetActorLocation() + MyChar->GetActorForwardVector() * 200.f;
		DeployLoc.Z = MyChar->GetActorLocation().Z; // Ground level

		FActorSpawnParameters TSP;
		TSP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AASTurret* NewTurret = GetWorld()->SpawnActor<AASTurret>(AASTurret::StaticClass(), DeployLoc, MyChar->GetActorRotation(), TSP);
		if (NewTurret)
		{
			AASHUD* DeployHUD = Cast<AASHUD>(GetHUD());
			if (DeployHUD) DeployHUD->AddKillFeed(
				FString::Printf(TEXT("Turret deployed! (%d remaining)"), DeployableTurrets), FColor(100, 200, 50));
		}
	}
	bLastDeployTurret = bDeployKey;

	if (bDodge && !bLastDodge) DoDodge();
	bLastDodge = bDodge;

	// Pause menu
	bool bPause = IsInputKeyDown(EKeys::Escape);
	if (bPause && !bLastPause)
	{
		AASHUD* PauseHUD = Cast<AASHUD>(GetHUD());
		if (PauseHUD)
		{
			PauseHUD->bPauseMenuActive = !PauseHUD->bPauseMenuActive;
			if (PauseHUD->bPauseMenuActive)
				UGameplayStatics::SetGamePaused(GetWorld(), true);
			else
				UGameplayStatics::SetGamePaused(GetWorld(), false);
		}
	}
	bLastPause = bPause;

	// Ground pound (crouch while airborne = slam down)
	if (MyChar && MyChar->GetCharacterMovement()->IsFalling() && !bIsWallRunning && !bGrappling)
	{
		bool bCrouchAir = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::C);
		if (bCrouchAir && !bIsCrouching)
		{
			// Slam downward
			MyChar->LaunchCharacter(FVector(0, 0, -2000), false, true);

			// AoE damage on landing (check next tick)
			FTimerHandle GroundPoundTimer;
			GetWorldTimerManager().SetTimer(GroundPoundTimer, [this]()
			{
				ACharacter* MC = Cast<ACharacter>(GetPawn());
				if (MC && MC->GetCharacterMovement()->IsMovingOnGround())
				{
					FVector LandPos = MC->GetActorLocation();
					DrawDebugSphere(GetWorld(), LandPos, 300.f, 16, FColor(200, 150, 50), false, 0.5f, 0, 3.f);
					for (int32 r = 0; r < 3; r++)
						DrawDebugCircle(GetWorld(), LandPos + FVector(0,0,5), 100.f + r*100.f, 16,
							FColor(200, 150, 50), false, 0.4f-r*0.1f, 0, 2.f-r*0.5f, FVector(1,0,0), FVector(0,1,0), false);

					TArray<FHitResult> Hits;
					FCollisionQueryParams P; P.AddIgnoredActor(MC);
					if (GetWorld()->SweepMultiByChannel(Hits, LandPos, LandPos, FQuat::Identity, ECC_Pawn,
						FCollisionShape::MakeSphere(300.f), P))
					{
						TSet<AActor*> Hit;
						for (auto& H : Hits)
						{
							if (!H.GetActor() || Hit.Contains(H.GetActor())) continue;
							Hit.Add(H.GetActor());
							float Dist = FVector::Dist(LandPos, H.GetActor()->GetActorLocation());
							float Falloff = 1.f - FMath::Clamp(Dist / 300.f, 0.f, 1.f);
							UGameplayStatics::ApplyDamage(H.GetActor(), 60.f * Falloff, this, MC, UDamageType::StaticClass());
							if (ACharacter* C = Cast<ACharacter>(H.GetActor()))
								C->LaunchCharacter(FVector(0,0,400*Falloff), false, true);
						}
					}
					AASHUD* HUD = Cast<AASHUD>(GetHUD());
					if (HUD) HUD->AddKillFeed(TEXT("GROUND POUND!"), FColor(200, 150, 50));
				}
			}, 0.15f, false);
		}
	}

	// Crouch toggle
	bool bCrouchKey = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::C);
	if (bCrouchKey && !bLastCrouch && MyChar)
	{
		bIsCrouching = !bIsCrouching;
		if (bIsCrouching)
		{
			MyChar->Crouch();
			CurrentSpread *= 0.7f; // Crouching improves accuracy
		}
		else
		{
			MyChar->UnCrouch();
		}
	}
	bLastCrouch = bCrouchKey;

	// Scroll wheel weapon switching
	bool bScrollUp = IsInputKeyDown(EKeys::MouseScrollUp);
	bool bScrollDown = IsInputKeyDown(EKeys::MouseScrollDown);
	if (bScrollUp && !bLastScrollUp)
	{
		int32 Next = (WeaponIndex + 1) % WeaponConfigs.Num();
		SwitchWeapon(Next);
	}
	bLastScrollUp = bScrollUp;
	if (bScrollDown && !bLastScrollDown)
	{
		int32 Prev = (WeaponIndex - 1 + WeaponConfigs.Num()) % WeaponConfigs.Num();
		SwitchWeapon(Prev);
	}
	bLastScrollDown = bScrollDown;

	// Bullet time (B key) — slow motion for 3 seconds
	bool bBTKey = IsInputKeyDown(EKeys::B);
	if (bBTKey && !bLastBulletTime && BulletTimeCooldown <= 0.f && Stamina >= 40.f)
	{
		bBulletTime = true;
		BulletTimeDuration = 3.f;
		BulletTimeCooldown = 15.f;
		Stamina -= 40.f;
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);
		AASHUD* BTHUD = Cast<AASHUD>(GetHUD());
		if (BTHUD) BTHUD->AddKillFeed(TEXT("BULLET TIME!"), FColor(200, 180, 255));
	}
	bLastBulletTime = bBTKey;

	if (bBulletTime)
	{
		BulletTimeDuration -= DeltaTime / 0.3f; // Real-time countdown
		if (BulletTimeDuration <= 0.f)
		{
			bBulletTime = false;
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
		}
	}
	if (BulletTimeCooldown > 0) BulletTimeCooldown -= DeltaTime / (bBulletTime ? 0.3f : 1.f);

	// Grapple hook
	bool bGrappleKey = IsInputKeyDown(EKeys::T);
	if (bGrappleKey && !bLastGrapple && GrappleCooldown <= 0.f) DoGrapple();
	bLastGrapple = bGrappleKey;
	if (GrappleCooldown > 0) GrappleCooldown -= DeltaTime;

	if (bGrappling && MyChar)
	{
		GrappleTimer -= DeltaTime;
		FVector ToTarget = (GrappleTarget - MyChar->GetActorLocation());
		float Dist = ToTarget.Size();

		// Draw grapple line
		DrawDebugLine(GetWorld(), MyChar->GetActorLocation() + FVector(0,0,50), GrappleTarget,
			FColor(200, 200, 50), false, -1.f, 0, 2.f);

		if (Dist < 150.f || GrappleTimer <= 0.f || MyChar->GetCharacterMovement()->IsMovingOnGround())
		{
			bGrappling = false;
			MyChar->GetCharacterMovement()->GravityScale = 1.f;
			MyChar->GetCharacterMovement()->Velocity *= 0.5f; // Slow down on arrival
		}
		else
		{
			// Pull toward target
			FVector PullDir = ToTarget.GetSafeNormal();
			MyChar->GetCharacterMovement()->Velocity = PullDir * 1500.f;
			MyChar->GetCharacterMovement()->GravityScale = 0.1f;
		}
	}

	// Melee combo timer
	if (MeleeComboResetTimer > 0) { MeleeComboResetTimer -= DeltaTime; if (MeleeComboResetTimer <= 0) MeleeCombo = 0; }

	if (Ammo <= 0 && !bIsReloading && bCanFire && !WC.bIsMelee) DoReload();

	// HUD
	AASHUD* HUD = Cast<AASHUD>(GetHUD());
	if (HUD)
	{
		HUD->Health = Health;
		HUD->MaxHealth = MaxHealth;
		HUD->Ammo = WC.bIsMelee ? 0 : Ammo;
		HUD->MaxAmmo = WC.bIsMelee ? 0 : MaxAmmo;
		HUD->Grenades = Grenades;
		HUD->WeaponIndex = WeaponIndex;
		HUD->WeaponName = WeaponName;
		HUD->PunchCooldown = PunchCooldown;
		HUD->bIsReloading = bIsReloading;
		HUD->Kills = Kills;
		HUD->bSpeedLines = bIsSprinting;
		HUD->bIsAiming = bIsAiming;
		HUD->CurrentSpread = CurrentSpread;
		HUD->bIsCrouching = bIsCrouching;
		HUD->bFlashlightOn = bFlashlightOn;
		HUD->bBulletTime = bBulletTime;
		HUD->BulletTimeDuration = BulletTimeDuration;
		// Check if near cover (wall within 100 units behind player)
		if (MyChar)
		{
			FVector Back = -MyChar->GetActorForwardVector();
			FHitResult CoverHit;
			FCollisionQueryParams CP; CP.AddIgnoredActor(MyChar);
			HUD->bNearCover = GetWorld()->LineTraceSingleByChannel(CoverHit, MyChar->GetActorLocation(),
				MyChar->GetActorLocation() + Back * 100.f, ECC_WorldStatic, CP);
		}
		HUD->Shield = Shield;
		HUD->MaxShield = MaxShield;
		HUD->SpeedBoostTimer = SpeedBoostTimer;
		HUD->Stamina = Stamina;
		HUD->MaxStamina = MaxStamina;
		HUD->XP = XP;
		HUD->XPToNextLevel = XPToNextLevel;
		HUD->Level = Level;
		HUD->HeadshotCount = HeadshotCount;
		HUD->TotalShotsFired = TotalShotsFired;
		HUD->TotalHits = TotalHits;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// FIRE - Complete shooting mechanics
// ═══════════════════════════════════════════════════════════════════════════════

void AASPlayerController::DoFire()
{
	if (!bCanFire || bIsReloading) return;
	const FWeaponConfig& WC = WeaponConfigs[FMath::Min(WeaponIndex, WeaponConfigs.Num()-1)];
	if (WC.bIsMelee) return;
	if (Ammo <= 0) { DoReload(); return; }

	bCanFire = false;
	Ammo--;
	TotalShotsFired++;

	APawn* P = GetPawn();
	if (!P) return;

	FVector CamLoc; FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	FVector MuzzlePos = GetMuzzleLocation();

	bool bAnyHit = false;
	bLastShotWasHeadshot = false;
	LastDamageDealt = 0.f;

	for (int32 i = 0; i < FMath::Max(1, WC.Pellets); i++)
	{
		// Trace from CAMERA for accuracy, spread based on weapon + stance
		FVector ShootDir = FMath::VRandCone(CamRot.Vector(), FMath::DegreesToRadians(CurrentSpread));

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(P);
		if (WeaponMeshActor) Params.AddIgnoredActor(WeaponMeshActor);

		FVector TraceEnd = CamLoc + ShootDir * 50000.f;
		bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_Visibility, Params);
		FVector ImpactPt = bHit ? Hit.ImpactPoint : (CamLoc + ShootDir * 5000.f);

		// ── TRACER (bright core + wider glow + tip flash) ──
		DrawDebugLine(GetWorld(), MuzzlePos, ImpactPt, FColor(255, 230, 120), false, 0.05f, 0, 0.8f);
		DrawDebugLine(GetWorld(), MuzzlePos, ImpactPt, FColor(255, 180, 60, 50), false, 0.1f, 0, 2.2f);
		// Tracer tip point
		DrawDebugPoint(GetWorld(), ImpactPt, 3.f, FColor(255, 255, 200), false, 0.03f);

		if (bHit)
		{
			bool bHitCharacter = (Cast<ACharacter>(Hit.GetActor()) != nullptr);

			// Impact effects — sparks + dust
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 5.f, FColor::White, false, 0.05f);
			if (bHitCharacter)
			{
				// Robot hit sparks (orange/red)
				for (int32 s = 0; s < 6; s++)
				{
					FVector Spark = (FMath::VRand() + Hit.ImpactNormal * 0.3f) * FMath::RandRange(20.f, 60.f);
					DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Spark,
						FColor(255, FMath::RandRange(50,150), FMath::RandRange(10,60)), false, 0.12f, 0, 0.4f);
				}
			}
			else
			{
				// Surface sparks + dust puff + bullet hole + ricochet
				for (int32 s = 0; s < 5; s++)
				{
					FVector Spark = (FMath::VRand() + Hit.ImpactNormal * 0.5f) * FMath::RandRange(15.f, 50.f);
					DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Spark,
						FColor(255, FMath::RandRange(180,255), FMath::RandRange(80,180)), false, 0.1f, 0, 0.3f);
				}
				for (int32 d = 0; d < 3; d++)
				{
					FVector Dust = Hit.ImpactNormal * FMath::RandRange(5.f, 20.f) + FMath::VRand() * 10.f;
					DrawDebugPoint(GetWorld(), Hit.ImpactPoint + Dust, FMath::RandRange(2.f, 5.f),
						FColor(180, 170, 150, 100), false, 0.15f);
				}
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint + Hit.ImpactNormal * 0.3f, 3.f, FColor(20,18,15), false, 15.f);
				FVector Ricochet = FMath::GetReflectionVector(ShootDir, Hit.ImpactNormal) * FMath::RandRange(30.f, 80.f);
				DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Ricochet,
					FColor(255, 200, 100, 60), false, 0.06f, 0, 0.3f);
			}

			if (Hit.GetActor())
			{
				// Headshot check (top 20% of actor)
				float Mult = 1.f;
				FVector Local = Hit.GetActor()->GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
				if (Local.Z > 140.f)
				{
					Mult = 3.f;
					bLastShotWasHeadshot = true;
					DrawDebugString(GetWorld(), Hit.ImpactPoint + FVector(0,0,30), TEXT("HEADSHOT"), nullptr, FColor::Red, 0.6f);
				}
				else if (FVector::DotProduct(ShootDir, Hit.GetActor()->GetActorForwardVector()) > 0.5f)
					Mult = 2.f;

				float TotalDmg = WC.BaseDamage * Mult * DamageMultiplier;
				LastDamageDealt += TotalDmg;
				UGameplayStatics::ApplyDamage(Hit.GetActor(), TotalDmg, this, P, UDamageType::StaticClass());

				AASEnemyBase* Enemy = Cast<AASEnemyBase>(Hit.GetActor());
				if (Enemy && Enemy->CurrentHealth - TotalDmg <= 0)
				{
					Kills++;
					KillStreak++;
					KillStreakTimer = 5.f;

					// Weapon upgrade — every 10 kills = +5% damage
					if (Kills > 0 && Kills % 10 == 0)
					{
						DamageMultiplier += 0.05f;
						if (GEngine) GEngine->AddOnScreenDebugMessage(35, 2.f, FColor::Yellow,
							FString::Printf(TEXT("WEAPON UPGRADED! DMG +%.0f%%"), (DamageMultiplier - 1.f) * 100.f));
					}

					// XP from kill
					int32 XPGain = 25 + (bLastShotWasHeadshot ? 15 : 0) + KillStreak * 5;
					XP += XPGain;
					if (XP >= XPToNextLevel)
					{
						Level++;
						XP -= XPToNextLevel;
						XPToNextLevel = 100 + Level * 50;
						MaxHealth += 10.f;
						Health = MaxHealth;
						if (GEngine) GEngine->AddOnScreenDebugMessage(30, 3.f, FColor::Yellow,
							FString::Printf(TEXT("LEVEL UP! Level %d — Max HP: %.0f"), Level, MaxHealth));
					}
					if (KillStreak >= 3)
					{
						if (GEngine)
						{
							FString StreakMsg;
							if (KillStreak == 3) StreakMsg = TEXT("TRIPLE KILL!");
							else if (KillStreak == 5) StreakMsg = TEXT("KILLING SPREE!");
							else if (KillStreak >= 7) StreakMsg = TEXT("UNSTOPPABLE!");
							else StreakMsg = FString::Printf(TEXT("%d KILL STREAK!"), KillStreak);
							GEngine->AddOnScreenDebugMessage(20, 2.f, FColor::Yellow, StreakMsg);
						}
					}
				}
				bAnyHit = true;
					TotalHits++;
					if (bLastShotWasHeadshot) HeadshotCount++;
			}
		}
	}

	// Grenade launcher — AoE explosion on impact
	if (WeaponIndex == 5 && bAnyHit)
	{
		// Find the last hit point
		FVector ExplosionPos = MuzzlePos + CamRot.Vector() * 500.f; // default
		// Use actual impact from the for loop
		FHitResult GLHit;
		FCollisionQueryParams GLP; GLP.AddIgnoredActor(P);
		if (WeaponMeshActor) GLP.AddIgnoredActor(WeaponMeshActor);
		if (GetWorld()->LineTraceSingleByChannel(GLHit, CamLoc, CamLoc + CamRot.Vector() * 50000.f, ECC_Visibility, GLP))
			ExplosionPos = GLHit.ImpactPoint;

		// Explosion visuals
		DrawDebugSphere(GetWorld(), ExplosionPos, 50.f, 8, FColor(255, 255, 200), false, 0.1f, 0, 3.f);
		DrawDebugSphere(GetWorld(), ExplosionPos, 300.f, 16, FColor(255, 100, 30), false, 0.5f, 0, 2.f);
		for (int32 s = 0; s < 12; s++)
		{
			FVector Spark = FMath::VRand() * FMath::RandRange(50.f, 250.f);
			Spark.Z = FMath::Abs(Spark.Z);
			DrawDebugLine(GetWorld(), ExplosionPos, ExplosionPos + Spark,
				FColor(255, FMath::RandRange(100,255), FMath::RandRange(20,100)), false, 0.4f, 0, 1.5f);
		}

		// AoE damage
		TArray<FHitResult> AoEHits;
		FCollisionQueryParams AoEP; AoEP.AddIgnoredActor(P);
		if (GetWorld()->SweepMultiByChannel(AoEHits, ExplosionPos, ExplosionPos, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeSphere(300.f), AoEP))
		{
			TSet<AActor*> HitActors;
			for (auto& AH : AoEHits)
			{
				if (!AH.GetActor() || HitActors.Contains(AH.GetActor())) continue;
				HitActors.Add(AH.GetActor());
				float Dist = FVector::Dist(ExplosionPos, AH.GetActor()->GetActorLocation());
				float Falloff = 1.f - FMath::Clamp(Dist / 300.f, 0.f, 1.f);
				UGameplayStatics::ApplyDamage(AH.GetActor(), WC.BaseDamage * Falloff, this, P, UDamageType::StaticClass());
				if (ACharacter* C = Cast<ACharacter>(AH.GetActor()))
				{
					FVector KD = (C->GetActorLocation() - ExplosionPos).GetSafeNormal();
					C->LaunchCharacter(KD * 800.f * Falloff + FVector(0,0,300*Falloff), true, true);
				}
			}
		}
	}

	// Muzzle flash — larger, more visible
	DrawDebugPoint(GetWorld(), MuzzlePos, 8.f, FColor(255,255,220), false, 0.03f);
	DrawDebugPoint(GetWorld(), MuzzlePos, 14.f, FColor(255,200,100,80), false, 0.05f);
	DrawDebugPoint(GetWorld(), MuzzlePos, 20.f, FColor(255,150,50,30), false, 0.07f);
	// Side flash
	FVector MuzzleRight = GetPawn()->GetActorRightVector();
	DrawDebugLine(GetWorld(), MuzzlePos, MuzzlePos + MuzzleRight * 15.f, FColor(255,200,100,100), false, 0.03f, 0, 1.f);
	DrawDebugLine(GetWorld(), MuzzlePos, MuzzlePos - MuzzleRight * 15.f, FColor(255,200,100,100), false, 0.03f, 0, 1.f);

	// Hit marker + damage popup
	if (bAnyHit)
	{
		AASHUD* HUD = Cast<AASHUD>(GetHUD());
		if (HUD)
		{
			HUD->bHitMarker = true;
			HUD->HitMarkerTime = 0.2f;
			HUD->DamagePopupValue = LastDamageDealt;
			HUD->DamagePopupTimer = 0.8f;
			HUD->bDamagePopupHeadshot = bLastShotWasHeadshot;
			if (bLastShotWasHeadshot) HUD->HeadshotFlashTimer = 0.5f;
		}
	}

	// Recoil (camera + weapon visual)
	AddPitchInput(-WC.RecoilPitch + FMath::RandRange(-WC.RecoilPitch*0.15f, 0.f));
	AddYawInput(FMath::RandRange(-WC.RecoilYaw, WC.RecoilYaw));
	RecoilRecovery = FMath::Min(RecoilRecovery + WC.RecoilPitch * 0.5f, 3.f);

	// Increase spread temporarily from firing
	CurrentSpread = FMath::Min(CurrentSpread + 0.5f, WC.BaseSpread * 2.f);

	// Fire rate — AR spin-up makes it faster
	float ActualFireRate = WC.FireRate;
	if (WC.bIsAutomatic && ARSpinUp > 0.5f)
	{
		float SpinPct = FMath::Clamp((ARSpinUp - 0.5f) / 1.5f, 0.f, 1.f);
		ActualFireRate = FMath::Lerp(WC.FireRate, WC.FireRate * 0.5f, SpinPct);
	}
	GetWorldTimerManager().SetTimer(FireTimer, [this]() { bCanFire = true; }, ActualFireRate, false);
}

// ═══════════════════════════════════════════════════════════════════════════════
// KNIFE ATTACK
// ═══════════════════════════════════════════════════════════════════════════════

void AASPlayerController::DoKnifeAttack()
{
	if (MeleeTimer > 0) return;

	APawn* P = GetPawn();
	if (!P) return;

	// 3-hit combo system: slash right, slash left, heavy thrust
	MeleeCombo = (MeleeCombo + 1);
	if (MeleeCombo > 3) MeleeCombo = 1;
	MeleeComboResetTimer = 1.0f; // Reset combo if no attack within 1s

	float ComboDamage = 65.f;
	float ComboRange = 200.f;
	float ComboRadius = 70.f;

	// Each combo hit is different
	switch (MeleeCombo)
	{
	case 1: MeleeTimer = 0.3f; ComboDamage = 50.f; break;      // Quick slash
	case 2: MeleeTimer = 0.3f; ComboDamage = 55.f; break;      // Cross slash
	case 3: MeleeTimer = 0.5f; ComboDamage = 100.f; ComboRange = 250.f; ComboRadius = 90.f; break; // Heavy thrust
	}

	FVector AimTarget;
	GetAimHitLocation(AimTarget);
	FVector Start = P->GetActorLocation() + FVector(0,0,50);
	FVector Dir = (AimTarget - Start).GetSafeNormal();
	FVector End = Start + Dir * ComboRange;
	FVector Right = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();

	// Combo-specific slash visuals
	if (MeleeCombo == 1)
	{
		// Right slash
		for (int32 a = -10; a <= 50; a += 5)
		{
			FRotator SlashRot = Dir.Rotation() + FRotator(0, a, 0);
			DrawDebugLine(GetWorld(), Start, Start + SlashRot.Vector() * 160.f,
				FColor(200, 220, 255), false, 0.12f, 0, 1.5f);
		}
	}
	else if (MeleeCombo == 2)
	{
		// Left slash (opposite direction)
		for (int32 a = -50; a <= 10; a += 5)
		{
			FRotator SlashRot = Dir.Rotation() + FRotator(0, a, 0);
			DrawDebugLine(GetWorld(), Start, Start + SlashRot.Vector() * 160.f,
				FColor(220, 200, 255), false, 0.12f, 0, 1.5f);
		}
	}
	else
	{
		// Heavy thrust — forward stab with wide impact
		DrawDebugLine(GetWorld(), Start, End, FColor(255, 200, 100), false, 0.2f, 0, 3.f);
		DrawDebugSphere(GetWorld(), End, 40.f, 8, FColor(255, 180, 80), false, 0.2f, 0, 2.f);

		// Step forward
		ACharacter* C = Cast<ACharacter>(P);
		if (C) C->LaunchCharacter(Dir * 400.f, true, false);
	}

	// Animate knife
	if (WeaponMeshActor)
	{
		float KnifeRoll = (MeleeCombo == 1) ? 45.f : (MeleeCombo == 2) ? -45.f : 0.f;
		float KnifePitch = (MeleeCombo == 3) ? 30.f : 0.f;
		WeaponMeshActor->SetActorRelativeRotation(FRotator(KnifePitch, 90, KnifeRoll));
		FTimerHandle KnifeResetTimer;
		GetWorldTimerManager().SetTimer(KnifeResetTimer, [this]()
		{
			if (WeaponMeshActor) WeaponMeshActor->SetActorRelativeRotation(FRotator(0, 90, 0));
		}, 0.25f, false);
	}

	// Damage sweep
	FHitResult Hit;
	FCollisionQueryParams Params; Params.AddIgnoredActor(P);
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(ComboRadius), Params))
	{
		if (Hit.GetActor())
		{
			UGameplayStatics::ApplyDamage(Hit.GetActor(), ComboDamage, this, P, UDamageType::StaticClass());
			AASHUD* HUD = Cast<AASHUD>(GetHUD());
			if (HUD)
			{
				HUD->bHitMarker = true;
				HUD->HitMarkerTime = 0.15f;
				HUD->DamagePopupValue = ComboDamage;
				HUD->DamagePopupTimer = 0.6f;
				HUD->bDamagePopupHeadshot = (MeleeCombo == 3);

				FString ComboText = (MeleeCombo == 1) ? TEXT("SLASH") : (MeleeCombo == 2) ? TEXT("CROSS") : TEXT("THRUST!");
				HUD->AddKillFeed(FString::Printf(TEXT("Combo %d: %s (%.0f dmg)"), MeleeCombo, *ComboText, ComboDamage),
					(MeleeCombo == 3) ? FColor(255, 200, 50) : FColor(200, 200, 255));
			}

			AASEnemyBase* E = Cast<AASEnemyBase>(Hit.GetActor());
			if (E && E->CurrentHealth - ComboDamage <= 0) { Kills++; KillStreak++; KillStreakTimer = 5.f; }
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// OTHER ACTIONS
// ═══════════════════════════════════════════════════════════════════════════════

void AASPlayerController::DoReload()
{
	const FWeaponConfig& WC = WeaponConfigs[FMath::Min(WeaponIndex, WeaponConfigs.Num()-1)];
	if (bIsReloading || Ammo >= MaxAmmo || WC.bIsMelee) return;
	bIsReloading = true;
	GetWorldTimerManager().SetTimer(ReloadTimer, [this]() { Ammo = MaxAmmo; bIsReloading = false; }, WC.ReloadTime, false);
}

void AASPlayerController::DoGrenade()
{
	if (Grenades <= 0) return;
	Grenades--;
	FVector Target; GetAimHitLocation(Target);
	DrawDebugSphere(GetWorld(), Target, 30, 8, FColor::Blue, false, 2.5f, 0, 2.f);
	FTimerHandle Fuse;
	GetWorldTimerManager().SetTimer(Fuse, [this, Target]()
	{
		DrawDebugSphere(GetWorld(), Target, 500, 24, FColor::Cyan, false, 1.5f, 0, 3.f);
		TArray<FHitResult> Hits;
		FCollisionQueryParams FP; if (GetPawn()) FP.AddIgnoredActor(GetPawn());
		if (GetWorld()->SweepMultiByChannel(Hits, Target, Target, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(500.f), FP))
			for (auto& H : Hits)
				if (AASEnemyBase* E = Cast<AASEnemyBase>(H.GetActor())) E->OnEMPHit(5.f);
	}, 2.5f, false);
}

void AASPlayerController::DoPunch()
{
	if (!bPunchReady || !GetPawn()) return;
	bPunchReady = false; PunchCooldown = 8.f;
	FVector Target; GetAimHitLocation(Target);
	APawn* P = GetPawn();
	FVector Start = P->GetActorLocation();
	FVector Dir = (Target - Start).GetSafeNormal();
	FHitResult Hit;
	FCollisionQueryParams Params; Params.AddIgnoredActor(P);
	DrawDebugLine(GetWorld(), Start+FVector(0,0,50), Start+Dir*250+FVector(0,0,50), FColor::Yellow, false, 0.4f, 0, 4.f);
	if (GetWorld()->SweepSingleByChannel(Hit, Start, Start+Dir*250, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(60.f), Params))
		if (Hit.GetActor())
		{
			UGameplayStatics::ApplyDamage(Hit.GetActor(), 80.f, this, P, UDamageType::StaticClass());
			if (ACharacter* C = Cast<ACharacter>(Hit.GetActor()))
				C->LaunchCharacter((Hit.GetActor()->GetActorLocation()-P->GetActorLocation()).GetSafeNormal()*2000+FVector(0,0,400), true, true);
		}
}

void AASPlayerController::DoDodge()
{
	ACharacter* C = Cast<ACharacter>(GetPawn());
	if (!C) return;
	if (Stamina < 20.f) return; // Need stamina to dodge
	Stamina -= 20.f;
	FVector Dir = C->GetLastMovementInputVector();
	if (Dir.IsNearlyZero()) Dir = C->GetActorForwardVector();

	// If sprinting + crouching = SLIDE (faster, longer, lower to ground)
	if (bIsSprinting && bIsCrouching)
	{
		C->LaunchCharacter(Dir.GetSafeNormal() * 1500 + FVector(0, 0, -50), true, true);
		// Draw slide trail
		FVector Start = C->GetActorLocation();
		for (int32 s = 0; s < 5; s++)
		{
			DrawDebugLine(GetWorld(), Start + FVector(FMath::RandRange(-20.f,20.f), FMath::RandRange(-20.f,20.f), -80),
				Start + Dir * (s * 50.f) + FVector(FMath::RandRange(-20.f,20.f), FMath::RandRange(-20.f,20.f), -80),
				FColor(200, 200, 200, 100), false, 0.3f, 0, 2.f);
		}
	}
	else
	{
		C->LaunchCharacter(Dir.GetSafeNormal() * 1000 + FVector(0, 0, 150), true, true);
	}
}

void AASPlayerController::DoInteract()
{
	APawn* P = GetPawn();
	if (!P) return;
	FVector Target; GetAimHitLocation(Target);
	FVector Start = P->GetActorLocation();
	FVector Dir = (Target-Start).GetSafeNormal();
	FHitResult Hit;
	FCollisionQueryParams Params; Params.AddIgnoredActor(P);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start+Dir*350, ECC_Visibility, Params))
	{
		AActor* A = Hit.GetActor(); if (!A) return;
		if (auto* D = Cast<AASControlledDoor>(A)) { D->Interact(); return; }
		if (auto* B = Cast<AASBreakerBox>(A)) { B->Interact(); return; }
		if (auto* V = Cast<AASValve>(A)) { V->Interact(); return; }
		if (auto* N = Cast<AASNPC>(A)) { N->Interact(); return; }
	}
	TArray<FHitResult> Hits;
	if (GetWorld()->SweepMultiByChannel(Hits, Start, Start+FVector(0,0,1), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(250.f), Params))
		for (auto& H : Hits)
		{
			if (!H.GetActor()) continue;
			if (auto* D = Cast<AASControlledDoor>(H.GetActor())) { D->Interact(); return; }
			if (auto* B = Cast<AASBreakerBox>(H.GetActor())) { B->Interact(); return; }
			if (auto* V = Cast<AASValve>(H.GetActor())) { V->Interact(); return; }
			if (auto* N = Cast<AASNPC>(H.GetActor())) { N->Interact(); return; }
		}
}

void AASPlayerController::DoGrapple()
{
	APawn* P = GetPawn();
	if (!P) return;

	FVector CamLoc; FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(P);
	if (WeaponMeshActor) Params.AddIgnoredActor(WeaponMeshActor);

	FVector TraceEnd = CamLoc + CamRot.Vector() * 3000.f;
	if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_WorldStatic, Params))
	{
		GrappleTarget = Hit.ImpactPoint;
		bGrappling = true;
		GrappleTimer = 2.f;
		GrappleCooldown = 5.f;
		Stamina = FMath::Max(0.f, Stamina - 30.f);

		// Visual feedback
		DrawDebugPoint(GetWorld(), GrappleTarget, 10.f, FColor::Yellow, false, 0.5f);

		AASHUD* HUD = Cast<AASHUD>(GetHUD());
		if (HUD) HUD->AddKillFeed(TEXT("Grapple!"), FColor(200, 200, 50));
	}
}

void AASPlayerController::ToggleFlashlight()
{
	bFlashlightOn = !bFlashlightOn;
	if (Flashlight)
		Flashlight->SetVisibility(bFlashlightOn);

	AASHUD* HUD = Cast<AASHUD>(GetHUD());
	if (HUD)
		HUD->AddKillFeed(bFlashlightOn ? TEXT("Flashlight ON") : TEXT("Flashlight OFF"),
			bFlashlightOn ? FColor(255, 240, 200) : FColor(100, 100, 100));
}

void AASPlayerController::OnPawnTakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bIsDead) return;

	// Shield absorbs damage first
	if (Shield > 0.f)
	{
		float Absorbed = FMath::Min(Shield, Damage);
		Shield -= Absorbed;
		Damage -= Absorbed;
		if (Damage <= 0.f) return;
	}

	Health = FMath::Max(0.f, Health - Damage);
	TimeSinceLastDamage = 0.f;

	// Damage flash + directional indicator
	AASHUD* HUD = Cast<AASHUD>(GetHUD());
	if (HUD)
	{
		HUD->DamageFlash = 1.0f;

		// Directional damage indicator
		if (DamageCauser && GetPawn())
		{
			FVector ToAttacker = DamageCauser->GetActorLocation() - GetPawn()->GetActorLocation();
			float Angle = FMath::RadiansToDegrees(FMath::Atan2(ToAttacker.Y, ToAttacker.X));
			float Intensity = FMath::Clamp(Damage / 30.f, 0.3f, 1.f);
			HUD->AddDamageIndicator(Angle, Intensity);
		}
	}

	// Camera shake for damage feedback
	float ShakeIntensity = FMath::Clamp(Damage / 20.f, 0.3f, 1.5f);
	if (PlayerCameraManager)
	{
		AddPitchInput(FMath::RandRange(-ShakeIntensity, ShakeIntensity));
		AddYawInput(FMath::RandRange(-ShakeIntensity * 0.5f, ShakeIntensity * 0.5f));
	}

	if (Health <= 0.f && !bIsDead)
	{
		bIsDead = true;
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("YOU DIED — Respawning..."));

		FTimerHandle RespawnTimer;
		GetWorldTimerManager().SetTimer(RespawnTimer, [this]()
		{
			Health = MaxHealth;
			bIsDead = false;
			APawn* P = GetPawn();
			if (P)
			{
				P->SetActorLocation(FVector(-300, 0, 50));
				SetControlRotation(FRotator(-10.f, 90.f, 0.f));
			}
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("RESPAWNED — Stay sharp."));
		}, 3.f, false);
	}
}

void AASPlayerController::SwitchWeapon(int32 Index)
{
	if (Index == WeaponIndex || Index < 0 || Index >= WeaponConfigs.Num()) return;
	bCanFire = false;
	if (WeaponMeshActor) { WeaponMeshActor->Destroy(); WeaponMeshActor = nullptr; }

	WeaponIndex = Index;
	const FWeaponConfig& WC = WeaponConfigs[Index];
	WeaponName = WC.Name;
	MaxAmmo = WC.MaxAmmo;
	Ammo = MaxAmmo;
	BaseSpread = WC.BaseSpread;
	AimSpread = WC.AimSpread;
	bIsReloading = false;

	FTimerHandle SwTimer;
	GetWorldTimerManager().SetTimer(SwTimer, [this]()
	{
		SpawnWeaponMesh();
		// Weapon raise animation — start low, come up
		if (WeaponMeshActor)
		{
			FVector LowPos = (WeaponIndex == 3 ? FVector(5,0,-15) : FVector(0,5,-18));
			WeaponMeshActor->SetActorRelativeLocation(LowPos);
			// Will interpolate to rest pos in tick
		}
		bCanFire = true;
	}, 0.25f, false);

	if (GEngine) GEngine->AddOnScreenDebugMessage(7, 1.5f, FColor::White, FString::Printf(TEXT(">> %s"), *WeaponName));
}
