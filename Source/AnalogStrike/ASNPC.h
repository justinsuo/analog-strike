#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ASNPC.generated.h"

UENUM(BlueprintType)
enum class ENPCRole : uint8 { Commander, Technician, Medic, Scout };

UCLASS()
class ANALOGSTRIKE_API AASNPC : public ACharacter
{
	GENERATED_BODY()
public:
	AASNPC();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) class UPointLightComponent* IndicatorLight;

	UPROPERTY(EditAnywhere, Category="NPC") FString NPCName = TEXT("Operative");
	UPROPERTY(EditAnywhere, Category="NPC") ENPCRole NPCRole = ENPCRole::Commander;
	UPROPERTY(EditAnywhere, Category="NPC") TArray<FString> DialogueLines;
	UPROPERTY(EditAnywhere, Category="NPC") float InteractRadius = 300.f;

	UPROPERTY(BlueprintReadOnly) int32 DialogueIndex = 0;
	UPROPERTY(BlueprintReadOnly) bool bPlayerNearby = false;

	UFUNCTION(BlueprintCallable) void Interact();
	UFUNCTION(BlueprintCallable) void SayLine(const FString& Line);

protected:
	virtual void BeginPlay() override;
	UPROPERTY() ACharacter* PlayerRef;
};
