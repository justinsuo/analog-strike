#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASWeaponBase.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8 { Shotgun, Revolver, Melee };

UCLASS()
class ANALOGSTRIKE_API AASWeaponBase : public AActor
{
	GENERATED_BODY()
public:
	AASWeaponBase();

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* WeaponMesh;
	UPROPERTY(VisibleAnywhere) USceneComponent* MuzzlePoint;

	UPROPERTY(EditAnywhere, Category="Weapon") FString WeaponName = TEXT("Weapon");
	UPROPERTY(EditAnywhere, Category="Weapon") EWeaponType WeaponType = EWeaponType::Shotgun;
	UPROPERTY(EditAnywhere, Category="Weapon") float Damage = 15.f;
	UPROPERTY(EditAnywhere, Category="Weapon") float Range = 5000.f;
	UPROPERTY(EditAnywhere, Category="Weapon") float FireRate = 0.8f;
	UPROPERTY(EditAnywhere, Category="Weapon") int32 MaxAmmo = 6;
	UPROPERTY(EditAnywhere, Category="Weapon") int32 PelletCount = 1;
	UPROPERTY(EditAnywhere, Category="Weapon") float SpreadAngle = 0.f;
	UPROPERTY(EditAnywhere, Category="Weapon") float ReloadTime = 2.0f;
	UPROPERTY(EditAnywhere, Category="Weapon") float RecoilPitch = 1.f;
	UPROPERTY(EditAnywhere, Category="Weapon") float RecoilYaw = 0.3f;
	UPROPERTY(EditAnywhere, Category="Weapon") bool bIsAutomatic = false;

	UPROPERTY(BlueprintReadWrite) int32 CurrentAmmo;
	UPROPERTY(BlueprintReadOnly) bool bCanFire = true;
	UPROPERTY(BlueprintReadOnly) bool bIsReloading = false;

	UFUNCTION(BlueprintCallable) virtual void FireWeapon(AController* WeaponInstigator, FVector CamLoc, FRotator CamRot);
	UFUNCTION(BlueprintCallable) void Reload();
	UFUNCTION(BlueprintCallable) void StopReload();

	virtual void BeginPlay() override;

protected:
	FTimerHandle FireCooldownHandle;
	FTimerHandle ReloadHandle;
	void ApplyRecoil(AController* C);
};
