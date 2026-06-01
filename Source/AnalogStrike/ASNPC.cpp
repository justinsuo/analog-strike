#include "ASNPC.h"
#include "ASHUD.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AASNPC::AASNPC()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::Disabled;

    GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);
    GetCapsuleComponent()->SetCapsuleRadius(42.f);

    IndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndicatorLight"));
    IndicatorLight->SetupAttachment(RootComponent);
    IndicatorLight->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
    IndicatorLight->SetIntensity(3000.f);
    IndicatorLight->SetAttenuationRadius(300.f);
    IndicatorLight->SetLightColor(FLinearColor(0.2f, 0.5f, 1.f));

    Tags.Add(TEXT("NPC"));
}

void AASNPC::BeginPlay()
{
    Super::BeginPlay();
    PlayerRef = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

    // Set dialogue based on NPC role
    DialogueLines.Empty();
    switch (NPCRole)
    {
    case ENPCRole::Commander:
        NPCName = TEXT("Commander Vasquez");
        DialogueLines.Add(TEXT("Operative. Listen carefully."));
        DialogueLines.Add(TEXT("The machines have taken control of this entire sector."));
        DialogueLines.Add(TEXT("Every digital system is compromised - comms, nav, weapons guidance."));
        DialogueLines.Add(TEXT("That's why you're here. No tech. No AI. Just your hands and your training."));
        DialogueLines.Add(TEXT("Your mission: locate the Relay Node deep in the facility."));
        DialogueLines.Add(TEXT("It's the source of their control. Destroy it manually."));
        DialogueLines.Add(TEXT("Watch out for Security Frames - they shoot on sight."));
        DialogueLines.Add(TEXT("The heavy Builder Units guard key areas. Use your Hydraulic Punch to stagger them."));
        DialogueLines.Add(TEXT("And the Warden... it's the deadliest one. It guards the Relay Core."));
        DialogueLines.Add(TEXT("Use breaker panels to open locked doors. EMP grenades disable robots temporarily."));
        DialogueLines.Add(TEXT("Once the Relay Node is destroyed, get to the extraction point."));
        DialogueLines.Add(TEXT("Good luck, operative. Analog beats machine."));
        break;
    case ENPCRole::Technician:
        NPCName = TEXT("Tech Specialist Reeves");
        DialogueLines.Add(TEXT("Hey, you must be the new operative."));
        DialogueLines.Add(TEXT("I've been studying these machines. They have weak points."));
        DialogueLines.Add(TEXT("Aim for the head - triple damage on the optics cluster."));
        DialogueLines.Add(TEXT("Shoot them from behind for double damage on the power cell."));
        DialogueLines.Add(TEXT("The Scout Drones are fast but fragile. Take them out first."));
        DialogueLines.Add(TEXT("They'll alert every enemy in the area if they spot you."));
        DialogueLines.Add(TEXT("I rigged some breaker panels around the facility."));
        DialogueLines.Add(TEXT("Pull them to temporarily restore power to doors and lights."));
        DialogueLines.Add(TEXT("But the effect only lasts 30 seconds before the machines regain control."));
        DialogueLines.Add(TEXT("Your weapons: Shotgun for close range, Revolver for precision, Knife for stealth."));
        DialogueLines.Add(TEXT("Press 1, 2, or 3 to switch. Each has its use."));
        break;
    case ENPCRole::Medic:
        NPCName = TEXT("Field Medic Torres");
        DialogueLines.Add(TEXT("I'm Torres. I patch people up."));
        DialogueLines.Add(TEXT("Your augmentations include health regeneration when not in combat."));
        DialogueLines.Add(TEXT("Stay out of fire for 5 seconds and you'll start healing."));
        DialogueLines.Add(TEXT("The Reflex Conditioning modification improves your dodge window."));
        DialogueLines.Add(TEXT("But it comes with a tradeoff - reduced stamina regeneration."));
        DialogueLines.Add(TEXT("Use Alt to dodge-roll. You're invulnerable during the roll."));
        DialogueLines.Add(TEXT("Green pickups restore health. Yellow pickups give ammo."));
        DialogueLines.Add(TEXT("Blue pickups restock your EMP grenades."));
        DialogueLines.Add(TEXT("Be careful out there. I've seen what these machines do to people."));
        break;
    case ENPCRole::Scout:
        NPCName = TEXT("Scout Pilot Jensen");
        DialogueLines.Add(TEXT("I fly the extraction shuttle."));
        DialogueLines.Add(TEXT("Once you destroy the Relay Node, the extraction point activates."));
        DialogueLines.Add(TEXT("Look for the green beacon - that's where I'll pick you up."));
        DialogueLines.Add(TEXT("The helipad is east of here, past the research station."));
        DialogueLines.Add(TEXT("But don't come to extraction until the Relay is down."));
        DialogueLines.Add(TEXT("The machines control the landing zone when the Relay is active."));
        DialogueLines.Add(TEXT("Watch the skies for Scout Drones near the extraction point."));
        DialogueLines.Add(TEXT("I'll be monitoring your progress from up here."));
        DialogueLines.Add(TEXT("Good hunting, operative."));
        break;
    }
}

void AASNPC::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!PlayerRef) return;

    float Dist = FVector::Dist(GetActorLocation(), PlayerRef->GetActorLocation());
    bPlayerNearby = (Dist < InteractRadius);

    if (bPlayerNearby)
    {
        FVector ToPlayer = (PlayerRef->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        FRotator LookRot = ToPlayer.Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, LookRot.Yaw, 0), DeltaTime, 3.f));
    }
}

void AASNPC::Interact()
{
    if (!bPlayerNearby) return;

    if (DialogueIndex < DialogueLines.Num())
    {
        SayLine(DialogueLines[DialogueIndex]);
        DialogueIndex++;
    }
    else
    {
        DialogueIndex = 0;
        SayLine(TEXT("Stay sharp out there, operative."));
    }
}

void AASNPC::SayLine(const FString& Line)
{
    // Use HUD dialogue box if available, fall back to debug messages
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        AASHUD* HUD = Cast<AASHUD>(PC->GetHUD());
        if (HUD)
        {
            HUD->ShowDialogue(NPCName, Line, 6.f);
            return;
        }
    }
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
            FString::Printf(TEXT("[%s]: \"%s\""), *NPCName, *Line));
    }
}
