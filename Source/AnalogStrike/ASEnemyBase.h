#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ASEnemyBase.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASEnemyBase : public ACharacter
{
    GENERATED_BODY()

public:
    AASEnemyBase();

    UPROPERTY(VisibleAnywhere) class UPointLightComponent* EnemyGlow;
    UPROPERTY(VisibleAnywhere) class UStaticMeshComponent* BodyMesh; // Visible robot body

    virtual void Tick(float DeltaTime) override;
    virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats") float MaxHealth = 100.f;
    UPROPERTY(BlueprintReadWrite, Category="Stats") float CurrentHealth = 100.f;
    UPROPERTY(EditAnywhere, Category="Stats") float MoveSpeed = 400.f;
    UPROPERTY(EditAnywhere, Category="Combat") float AttackDamage = 10.f;
    UPROPERTY(EditAnywhere, Category="Combat") float AttackRange = 200.f;
    UPROPERTY(EditAnywhere, Category="Combat") float DetectionRange = 2000.f;
    UPROPERTY(EditAnywhere, Category="Combat") float AttackCooldown = 1.5f;

    UPROPERTY(BlueprintReadOnly) bool bIsStunned = false;
    UPROPERTY(EditAnywhere) bool bHasShield = false;
    UPROPERTY(EditAnywhere) float ShieldHP = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bIsEMPDisabled = false;
    UPROPERTY(BlueprintReadOnly) bool bPlayerDetected = false;

    UFUNCTION(BlueprintCallable) void OnEMPHit(float Duration);
    UFUNCTION(BlueprintCallable) virtual void OnDeath();
    void AlertNearbyEnemies(float Radius = 2000.f);

protected:
    virtual void BeginPlay() override;
    virtual void ChasePlayer(float DeltaTime);
    virtual void AttackPlayer();
    void StrafeAroundPlayer(float DeltaTime);
    void TakeCover(float DeltaTime);

    UPROPERTY() ACharacter* PlayerRef = nullptr;
    bool bSeekingCover = false;
    float CoverCheckTimer = 0.f;
    bool bCanAttack = true;
    FTimerHandle AttackTimerHandle;
    FTimerHandle EMPTimerHandle;
};
