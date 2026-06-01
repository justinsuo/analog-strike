#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASWeatherSystem.generated.h"

UCLASS()
class ANALOGSTRIKE_API AASWeatherSystem : public AActor
{
	GENERATED_BODY()
public:
	AASWeatherSystem();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere) float TimeOfDay = 14.f; // 24h format, start at 2pm
	UPROPERTY(EditAnywhere) float TimeSpeed = 0.01f; // How fast time passes (1.0 = real-time)
	UPROPERTY(EditAnywhere) bool bRaining = false;
	UPROPERTY(EditAnywhere) float FogDensity = 0.f;

	UPROPERTY(VisibleAnywhere) class UDirectionalLightComponent* SunLight;
	UPROPERTY(VisibleAnywhere) class USkyAtmosphereComponent* SkyAtmosphere;
	UPROPERTY(VisibleAnywhere) class UExponentialHeightFogComponent* Fog;

protected:
	virtual void BeginPlay() override;
	void UpdateLighting();

	float RainTimer = 0.f;
};
