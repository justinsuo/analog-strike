#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ASHUD.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // Written by ASPlayerController
    float Health = 100.f;
    float MaxHealth = 100.f;
    int32 Ammo = 6;
    int32 MaxAmmo = 6;
    int32 Grenades = 3;
    int32 WeaponIndex = 0;
    FString WeaponName = TEXT("Pump Shotgun");
    float PunchCooldown = 0.f;
    bool bIsReloading = false;
    bool bHitMarker = false;
    float HitMarkerTime = 0.f;
    int32 Kills = 0;

    float DamageFlash = 0.f;
    float MissionTime = 0.f;
    float CompassYaw = 0.f;
    float TutorialTimer = 8.f;

    // Dialogue display
    FString DialogueSpeaker;
    FString DialogueLine;
    float DialogueTimer = 0.f;
    void ShowDialogue(const FString& Speaker, const FString& Line, float Duration = 5.f);

    // Kill feed
    struct FKillFeedEntry { FString Text; float Timer; FColor Color; };
    TArray<FKillFeedEntry> KillFeed;
    void AddKillFeed(const FString& Text, FColor Color = FColor::White);

    // Kill streak display
    FString KillStreakText;
    float KillStreakDisplayTimer = 0.f;

    // Sprint/aim state
    bool bSpeedLines = false;
    bool bIsAiming = false;
    float CurrentSpread = 3.f;

    // Headshot indicator
    float HeadshotFlashTimer = 0.f;

    // Player state
    bool bIsCrouching = false;
    bool bFlashlightOn = false;
    float Stamina = 100.f;
    float MaxStamina = 100.f;
    float Shield = 0.f;
    float MaxShield = 50.f;
    float SpeedBoostTimer = 0.f;
    int32 XP = 0;
    int32 XPToNextLevel = 100;
    int32 Level = 1;
    int32 HeadshotCount = 0;
    int32 TotalShotsFired = 0;
    int32 TotalHits = 0;

    // Damage dealt popup
    float DamagePopupValue = 0.f;
    float DamagePopupTimer = 0.f;
    bool bDamagePopupHeadshot = false;

    // Directional damage indicators
    struct FDamageIndicator { float Angle; float Timer; float Intensity; };
    TArray<FDamageIndicator> DamageIndicators;
    void AddDamageIndicator(float WorldAngle, float Intensity = 1.f);

    // Pause menu
    bool bPauseMenuActive = false;
    int32 PauseSelection = 0;

    // Objective waypoint
    FVector ObjectiveWorldPos = FVector::ZeroVector;
    bool bShowObjectiveWaypoint = false;

    // Enemy proximity warning
    float ProximityWarningAlpha = 0.f;

    // Cover indicator
    bool bNearCover = false;
    bool bBulletTime = false;
    float BulletTimeDuration = 0.f;

private:
    UPROPERTY() ACharacter* PlayerChar = nullptr;
};
