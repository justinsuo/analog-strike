#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ASGameMode.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AASGameMode();

    UPROPERTY(BlueprintReadOnly) FString CurrentObjective = TEXT("Reach the Maintenance Wing");
    UPROPERTY(BlueprintReadOnly) int32 ObjectiveIndex = 0;
    UPROPERTY(BlueprintReadOnly) bool bMissionComplete = false;

    UFUNCTION(BlueprintCallable) void AdvanceObjective();
    UFUNCTION(BlueprintCallable) void CheckZoneObjective(const FString& ZoneName);

    virtual void Tick(float DeltaTime) override;

    UPROPERTY() TSet<FString> VisitedZones;

    // Wave system — respawn enemies when too few are alive
    UPROPERTY(EditAnywhere) int32 MinActiveEnemies = 6;
    UPROPERTY(EditAnywhere) float WaveCheckInterval = 12.f;
    float WaveTimer = 3.f;
    int32 TotalWaveEnemiesSpawned = 0;
    float DifficultyMultiplier = 1.f; // Increases over time
    float GameTime = 0.f;

    virtual void BeginPlay() override;

protected:
    TArray<FString> Objectives;
    void SpawnWaveEnemies(int32 Count);
    void SpawnInitialEnemies();
};
