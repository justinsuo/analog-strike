#include "ASRelayNode.h"
#include "ASControlledDoor.h"
#include "ASExtractionZone.h"
#include "ASGameMode.h"
#include "ASHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

AASRelayNode::AASRelayNode()
{
    NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
    RootComponent = NodeMesh;

    // Load sphere mesh
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        NodeMesh->SetStaticMesh(SphereMesh.Object);
    }
    NodeMesh->SetWorldScale3D(FVector(3.f));
    NodeMesh->SetCollisionProfileName(TEXT("BlockAll"));

    // Red pulsing light
    NodeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("NodeLight"));
    NodeLight->SetupAttachment(RootComponent);
    NodeLight->SetLightColor(FLinearColor(1.f, 0.1f, 0.1f));
    NodeLight->SetIntensity(10000.f);
    NodeLight->SetAttenuationRadius(2000.f);
}

void AASRelayNode::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

float AASRelayNode::TakeDamage(float Damage, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (!bIsActive) return 0.f;

    float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
    CurrentHealth -= ActualDamage;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());
        if (HUD)
            HUD->AddKillFeed(FString::Printf(TEXT("Relay Node: %.0f/%.0f HP"), CurrentHealth, MaxHealth), FColor(255, 100, 50));
    }

    if (CurrentHealth <= 0.f)
    {
        DestroyRelay();
    }

    return ActualDamage;
}

void AASRelayNode::DestroyRelay()
{
    bIsActive = false;
    NodeLight->SetLightColor(FLinearColor(0.1f, 1.f, 0.1f));
    NodeLight->SetIntensity(15000.f);

    // Show via HUD dialogue + kill feed
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());
        if (HUD)
        {
            HUD->ShowDialogue(TEXT("SYSTEM"), TEXT("RELAY NODE DESTROYED - Proceed to extraction!"), 8.f);
            HUD->AddKillFeed(TEXT("*** RELAY NODE DESTROYED ***"), FColor::Green);
        }
    }

    // Unlock ALL doors
    TArray<AActor*> AllDoors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASControlledDoor::StaticClass(), AllDoors);
    for (AActor* A : AllDoors)
    {
        AASControlledDoor* Door = Cast<AASControlledDoor>(A);
        if (Door) Door->UnlockDoor();
    }

    // Activate extraction zone
    TArray<AActor*> AllExtracts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASExtractionZone::StaticClass(), AllExtracts);
    for (AActor* A : AllExtracts)
    {
        AASExtractionZone* EZ = Cast<AASExtractionZone>(A);
        if (EZ) EZ->ActivateExtraction();
    }

    // Advance objective
    AASGameMode* GM = Cast<AASGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM)
    {
        GM->CurrentObjective = TEXT("Extract - Follow the green light");
    }

    // Boost all point lights in the level (simulating power restoration)
    TArray<AActor*> AllLights;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APointLight::StaticClass(), AllLights);
    for (AActor* Light : AllLights)
    {
        APointLight* PL = Cast<APointLight>(Light);
        if (PL)
        {
            PL->GetLightComponent()->SetIntensity(12000.f);
            PL->GetLightComponent()->SetLightColor(FLinearColor(1.f, 0.95f, 0.8f));
        }
    }
}
