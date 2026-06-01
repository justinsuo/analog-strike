#include "ASWeatherSystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

AASWeatherSystem::AASWeatherSystem()
{
	PrimaryActorTick.bCanEverTick = true;

	// Directional sun light
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	RootComponent = SunLight;
	SunLight->SetIntensity(8.f);
	SunLight->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
	SunLight->SetAtmosphereSunLight(true);
	SunLight->SetCastShadows(true);

	// Sky atmosphere for proper sky rendering
	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);

	// Fog for atmosphere
	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	Fog->SetFogDensity(0.005f);
	Fog->SetFogMaxOpacity(0.8f);
	Fog->SetFogHeightFalloff(0.2f);
	Fog->SetDirectionalInscatteringColor(FLinearColor(1.f, 0.85f, 0.6f));
}

void AASWeatherSystem::BeginPlay()
{
	Super::BeginPlay();
	UpdateLighting();
}

void AASWeatherSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Advance time of day
	TimeOfDay += DeltaTime * TimeSpeed;
	if (TimeOfDay >= 24.f) TimeOfDay -= 24.f;

	UpdateLighting();

	// Rain effects (random particles via debug draw when raining)
	if (bRaining)
	{
		ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		if (Player)
		{
			FVector PLoc = Player->GetActorLocation();
			for (int32 r = 0; r < 30; r++)
			{
				FVector RainStart = PLoc + FVector(
					FMath::RandRange(-800.f, 800.f),
					FMath::RandRange(-800.f, 800.f),
					FMath::RandRange(300.f, 600.f));
				FVector RainEnd = RainStart - FVector(0, 0, FMath::RandRange(100.f, 200.f));
				DrawDebugLine(GetWorld(), RainStart, RainEnd,
					FColor(150, 180, 220, 80), false, -1.f, 0, 0.5f);
			}
		}

		// Increase fog during rain
		Fog->SetFogDensity(FMath::FInterpTo(Fog->FogDensity, 0.02f, DeltaTime, 1.f));
	}
	else
	{
		Fog->SetFogDensity(FMath::FInterpTo(Fog->FogDensity, 0.005f + FogDensity, DeltaTime, 1.f));
	}

	// Random weather changes
	RainTimer -= DeltaTime;
	if (RainTimer <= 0.f)
	{
		RainTimer = FMath::RandRange(120.f, 300.f); // Change weather every 2-5 minutes
		bRaining = FMath::FRand() < 0.3f; // 30% chance of rain
	}
}

void AASWeatherSystem::UpdateLighting()
{
	// Sun angle based on time of day
	// 0h = midnight (below horizon), 6h = sunrise, 12h = noon, 18h = sunset
	float SunAngle = (TimeOfDay - 6.f) / 12.f * 180.f; // -90 to 270
	SunLight->SetWorldRotation(FRotator(-SunAngle, 180.f, 0.f));

	// Sun color shifts through the day
	float NormalizedTime = TimeOfDay / 24.f;
	FLinearColor SunColor;
	float SunIntensity;

	if (TimeOfDay < 5.f || TimeOfDay > 20.f)
	{
		// Night — moonlight
		SunColor = FLinearColor(0.3f, 0.35f, 0.5f);
		SunIntensity = 1.f;
	}
	else if (TimeOfDay < 7.f)
	{
		// Sunrise — warm orange
		float T = (TimeOfDay - 5.f) / 2.f;
		SunColor = FMath::Lerp(FLinearColor(0.3f, 0.35f, 0.5f), FLinearColor(1.f, 0.6f, 0.3f), T);
		SunIntensity = FMath::Lerp(1.f, 6.f, T);
	}
	else if (TimeOfDay < 10.f)
	{
		// Morning — warming up
		float T = (TimeOfDay - 7.f) / 3.f;
		SunColor = FMath::Lerp(FLinearColor(1.f, 0.6f, 0.3f), FLinearColor(1.f, 0.95f, 0.85f), T);
		SunIntensity = FMath::Lerp(6.f, 10.f, T);
	}
	else if (TimeOfDay < 16.f)
	{
		// Daytime
		SunColor = FLinearColor(1.f, 0.98f, 0.9f);
		SunIntensity = 10.f;
	}
	else if (TimeOfDay < 18.f)
	{
		// Afternoon — golden hour
		float T = (TimeOfDay - 16.f) / 2.f;
		SunColor = FMath::Lerp(FLinearColor(1.f, 0.98f, 0.9f), FLinearColor(1.f, 0.5f, 0.2f), T);
		SunIntensity = FMath::Lerp(10.f, 5.f, T);
	}
	else
	{
		// Sunset to night
		float T = (TimeOfDay - 18.f) / 2.f;
		SunColor = FMath::Lerp(FLinearColor(1.f, 0.5f, 0.2f), FLinearColor(0.3f, 0.35f, 0.5f), T);
		SunIntensity = FMath::Lerp(5.f, 1.f, T);
	}

	SunLight->SetLightColor(SunColor);
	SunLight->SetIntensity(SunIntensity);

	// Fog color follows sun
	Fog->SetDirectionalInscatteringColor(SunColor);
}
