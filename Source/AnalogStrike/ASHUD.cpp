#include "ASHUD.h"
#include "ASGameMode.h"
#include "ASPlayerController.h"
#include "ASEnemyBase.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void AASHUD::BeginPlay()
{
    Super::BeginPlay();
    PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void AASHUD::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!PlayerChar) PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    MissionTime += DeltaTime;
    if (DamageFlash > 0) DamageFlash -= DeltaTime * 3.f;
    if (DialogueTimer > 0) DialogueTimer -= DeltaTime;
    if (bHitMarker) { HitMarkerTime -= DeltaTime; if (HitMarkerTime <= 0) bHitMarker = false; }
    if (TutorialTimer > 0) TutorialTimer -= DeltaTime;
    if (HeadshotFlashTimer > 0) HeadshotFlashTimer -= DeltaTime;
    if (DamagePopupTimer > 0) DamagePopupTimer -= DeltaTime;
    // Damage indicators fade
    for (int32 di = DamageIndicators.Num()-1; di >= 0; di--)
    {
        DamageIndicators[di].Timer -= DeltaTime;
        if (DamageIndicators[di].Timer <= 0) DamageIndicators.RemoveAt(di);
    }
    // Proximity warning fade
    if (ProximityWarningAlpha > 0) ProximityWarningAlpha = FMath::Max(0.f, ProximityWarningAlpha - DeltaTime * 2.f);
    if (KillStreakDisplayTimer > 0) KillStreakDisplayTimer -= DeltaTime;
    // Update kill feed timers
    for (int32 i = KillFeed.Num()-1; i >= 0; i--)
    {
        KillFeed[i].Timer -= DeltaTime;
        if (KillFeed[i].Timer <= 0) KillFeed.RemoveAt(i);
    }
    if (APlayerController* PC = GetOwningPlayerController())
        CompassYaw = PC->GetControlRotation().Yaw;
}

void AASHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!PlayerChar) return;

    float W = Canvas->SizeX, H = Canvas->SizeY;
    // Crosshair at true screen center - this is where the camera ray goes
    // The third-person camera is offset behind/right of character
    // so crosshair at center correctly shows where bullets will hit
    float CX = W * 0.5f, CY = H * 0.5f;

    // ── DAMAGE VIGNETTE ─────────────────────────────────────────────────
    if (DamageFlash > 0.01f)
    {
        FLinearColor R(0.7f, 0, 0, DamageFlash * 0.4f);
        DrawRect(R, 0, 0, W, H * 0.08f);
        DrawRect(R, 0, H * 0.92f, W, H * 0.08f);
        DrawRect(R, 0, 0, W * 0.05f, H);
        DrawRect(R, W * 0.95f, 0, W * 0.05f, H);
    }

    // ── DIRECTIONAL DAMAGE INDICATORS ─────────────────────────────────
    for (const FDamageIndicator& DI : DamageIndicators)
    {
        float DIA = FMath::Min(1.f, DI.Timer / 0.3f) * DI.Intensity;
        // Calculate screen angle relative to player facing
        float RelAngle = FMath::DegreesToRadians(DI.Angle - CompassYaw);
        float IX = FMath::Sin(RelAngle);
        float IY = -FMath::Cos(RelAngle);

        // Draw chevron at screen edge pointing toward damage source
        float EdgeDist = 0.38f;
        float PX = CX + IX * W * EdgeDist;
        float PY = CY + IY * H * EdgeDist;

        // Red wedge shape
        float WedgeSize = 20.f * DIA;
        FLinearColor WedgeColor(1.f, 0.1f, 0.05f, DIA * 0.8f);
        // Draw as 3 overlapping rects approximating a triangle
        for (int32 wi = 0; wi < 3; wi++)
        {
            float WS = WedgeSize * (1.f - wi * 0.3f);
            float WO = wi * 3.f;
            DrawRect(WedgeColor, PX - WS/2 + IX*WO, PY - WS/2 + IY*WO, WS, WS * 0.3f);
        }
    }

    // ── PROXIMITY WARNING (screen edge glow when enemies very close) ─
    if (ProximityWarningAlpha > 0.01f)
    {
        FLinearColor PW(1.f, 0.3f, 0.1f, ProximityWarningAlpha * 0.15f);
        DrawRect(PW, 0, 0, W * 0.03f, H);
        DrawRect(PW, W * 0.97f, 0, W * 0.03f, H);
        DrawRect(PW, 0, 0, W, H * 0.03f);
        DrawRect(PW, 0, H * 0.97f, W, H * 0.03f);
    }

    // ── BULLET TIME OVERLAY ───────────────────────────────────────────
    if (bBulletTime)
    {
        // Purple-tinted screen edges
        FLinearColor BTC(0.3f, 0.15f, 0.5f, 0.2f);
        DrawRect(BTC, 0, 0, W * 0.04f, H);
        DrawRect(BTC, W * 0.96f, 0, W * 0.04f, H);
        DrawRect(BTC, 0, 0, W, H * 0.04f);
        DrawRect(BTC, 0, H * 0.96f, W, H * 0.04f);

        // Timer
        DrawText(FString::Printf(TEXT("BULLET TIME %.1fs"), BulletTimeDuration),
            FLinearColor(0.7f, 0.5f, 1.f), CX - 60, H * 0.12f, nullptr, 0.8f);
    }

    // ── SPRINT SPEED LINES (screen edges) ─────────────────────────────
    if (bSpeedLines)
    {
        float SA = 0.15f + FMath::Sin(MissionTime * 15.f) * 0.05f;
        // Radial lines from edges toward center
        for (int32 sl = 0; sl < 12; sl++)
        {
            float Angle = sl * 30.f + MissionTime * 60.f;
            float Rad = FMath::DegreesToRadians(Angle);
            float EdgeDist = (sl % 2 == 0) ? 0.48f : 0.45f;
            float InnerDist = 0.35f;
            float SX1 = CX + FMath::Cos(Rad) * W * EdgeDist;
            float SY1 = CY + FMath::Sin(Rad) * H * EdgeDist;
            float SX2 = CX + FMath::Cos(Rad) * W * InnerDist;
            float SY2 = CY + FMath::Sin(Rad) * H * InnerDist;
            DrawRect(FLinearColor(1,1,1, SA), FMath::Min(SX1,SX2), FMath::Min(SY1,SY2),
                FMath::Abs(SX2-SX1)+1, FMath::Abs(SY2-SY1)+1);
        }
    }

    // ── SNIPER SCOPE (when aiming with sniper rifle) ──────────────────
    if (bIsAiming && WeaponIndex == 4)
    {
        // Dark vignette around scope
        float ScopeR = H * 0.4f;
        // Top/bottom black bars
        DrawRect(FLinearColor(0,0,0,0.9f), 0, 0, W, CY - ScopeR);
        DrawRect(FLinearColor(0,0,0,0.9f), 0, CY + ScopeR, W, H - CY - ScopeR);
        // Left/right black bars
        DrawRect(FLinearColor(0,0,0,0.9f), 0, CY-ScopeR, CX - ScopeR, ScopeR*2);
        DrawRect(FLinearColor(0,0,0,0.9f), CX + ScopeR, CY-ScopeR, W - CX - ScopeR, ScopeR*2);

        // Scope crosshair (fine lines)
        DrawRect(FLinearColor(0,0,0,0.6f), CX - ScopeR, CY - 0.5f, ScopeR*2, 1);
        DrawRect(FLinearColor(0,0,0,0.6f), CX - 0.5f, CY - ScopeR, 1, ScopeR*2);

        // Mil-dots
        for (int32 md = 1; md <= 4; md++)
        {
            float MDist = md * ScopeR * 0.2f;
            DrawRect(FLinearColor(0,0,0,0.8f), CX + MDist - 1, CY - 3, 2, 6);
            DrawRect(FLinearColor(0,0,0,0.8f), CX - MDist - 1, CY - 3, 2, 6);
            DrawRect(FLinearColor(0,0,0,0.8f), CX - 3, CY + MDist - 1, 6, 2);
            DrawRect(FLinearColor(0,0,0,0.8f), CX - 3, CY - MDist - 1, 6, 2);
        }

        // Range indicator
        DrawText(TEXT("12x"), FLinearColor(0.3f,0.3f,0.3f,0.5f), CX + ScopeR*0.6f, CY + ScopeR*0.6f, nullptr, 0.6f);
    }

    // ── CROSSHAIR (modern tactical style) ──────────────────────────────
    FLinearColor CW(1, 1, 1, 0.9f);      // White
    FLinearColor CO(0, 0, 0, 0.5f);       // Outline/shadow
    // Dynamic crosshair — gap increases with spread, tightens when aiming
    float SpreadFactor = FMath::Clamp(CurrentSpread, 0.5f, 8.f);
    float CS = 10.f, CG = 2.f + SpreadFactor * 1.5f, CT = 2.f;

    // Shadow/outline (1px offset for readability against any background)
    DrawRect(CO, CX-CT/2+1, CY-CS-CG+1, CT, CS);   // top shadow
    DrawRect(CO, CX-CT/2+1, CY+CG+1, CT, CS);       // bottom shadow
    DrawRect(CO, CX-CS-CG+1, CY-CT/2+1, CS, CT);   // left shadow
    DrawRect(CO, CX+CG+1, CY-CT/2+1, CS, CT);       // right shadow

    // Main crosshair lines
    DrawRect(CW, CX-CT/2, CY-CS-CG, CT, CS);   // top
    DrawRect(CW, CX-CT/2, CY+CG, CT, CS);       // bottom
    DrawRect(CW, CX-CS-CG, CY-CT/2, CS, CT);   // left
    DrawRect(CW, CX+CG, CY-CT/2, CS, CT);       // right

    // Center dot (small, bright)
    DrawRect(FLinearColor(1, 0.3f, 0.3f, 0.9f), CX-1, CY-1, 3, 3); // Red center dot

    // Hit marker (X shape)
    if (bHitMarker)
    {
        FLinearColor HM(1, 1, 1, 0.9f);
        float M = 10.f;
        DrawRect(HM, CX-M-2, CY-1, M*0.5f, 2);
        DrawRect(HM, CX+M*0.5f+1, CY-1, M*0.5f, 2);
        DrawRect(HM, CX-1, CY-M-2, 2, M*0.5f);
        DrawRect(HM, CX-1, CY+M*0.5f+1, 2, M*0.5f);
    }

    // ── HEADSHOT FLASH ────────────────────────────────────────────────
    if (HeadshotFlashTimer > 0.f)
    {
        float HSA = FMath::Min(1.f, HeadshotFlashTimer / 0.2f);
        // Red X flash on headshot
        float HSS = 15.f;
        DrawRect(FLinearColor(1, 0.2f, 0.1f, HSA), CX-HSS, CY-1, HSS*2, 2);
        DrawRect(FLinearColor(1, 0.2f, 0.1f, HSA), CX-1, CY-HSS, 2, HSS*2);
        DrawText(TEXT("HEADSHOT"), FLinearColor(1, 0.3f, 0.1f, HSA), CX-30, CY+20, nullptr, 0.8f);
    }

    // ── DAMAGE POPUP (below crosshair) ──────────────────────────────
    if (DamagePopupTimer > 0.f)
    {
        float DPA = FMath::Min(1.f, DamagePopupTimer / 0.3f);
        float DPY = CY + 35 - (1.f - DPA) * 15.f; // Float upward
        FLinearColor DPC = bDamagePopupHeadshot ? FLinearColor(1, 0.3f, 0.1f, DPA) : FLinearColor(1, 1, 1, DPA);
        DrawText(FString::Printf(TEXT("%.0f"), DamagePopupValue), DPC, CX-10, DPY, nullptr, bDamagePopupHeadshot ? 1.2f : 0.9f);
    }

    // ── COMPASS ─────────────────────────────────────────────────────────
    float CompW = 280.f;
    DrawRect(FLinearColor(0,0,0,0.35f), CX-CompW/2, 12, CompW, 16);
    FString Dirs[] = {TEXT("N"),TEXT("NE"),TEXT("E"),TEXT("SE"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
    float DA[] = {0,45,90,135,180,225,270,315};
    for (int i = 0; i < 8; i++)
    {
        float Diff = FMath::FindDeltaAngleDegrees(CompassYaw, DA[i]);
        if (FMath::Abs(Diff) < 55)
        {
            float PX = CX + Diff * (CompW/110.f);
            FLinearColor DC = (i==0) ? FLinearColor(1,0.2f,0.2f) : ((i%2==0)?FLinearColor(0.8f,0.8f,0.8f):FLinearColor(0.5f,0.5f,0.5f,0.6f));
            DrawText(Dirs[i], DC, PX-((i%2==0)?3:5), 14, nullptr, (i%2==0)?0.9f:0.7f);
        }
    }
    DrawRect(FLinearColor::White, CX-1, 10, 2, 5);

    // Mission timer (top left)
    int32 MMin = (int32)(MissionTime / 60);
    int32 MSec = (int32)FMath::Fmod(MissionTime, 60.f);
    DrawText(FString::Printf(TEXT("%d:%02d"), MMin, MSec),
        FLinearColor(0.6f, 0.6f, 0.6f, 0.5f), 15, 14, nullptr, 0.7f);

    // ── ZONE NAME ───────────────────────────────────────────────────────
    FVector PLoc = PlayerChar->GetActorLocation();
    float PXP = PLoc.X, PYP = PLoc.Y, Dist = FMath::Sqrt(PXP*PXP+PYP*PYP);
    FString Zone = TEXT("CENTRAL VALLEY");
    if (PYP < -5000) Zone = TEXT("BUNKER ENTRANCE");
    else if (PXP > 5000 && PYP > 3000) Zone = TEXT("CAVE SYSTEM");
    else if (PXP > 6000) Zone = TEXT("HELIPAD");
    else if (PXP > 4000 && FMath::Abs(PYP) < 1500) Zone = TEXT("RESEARCH STATION");
    else if (PXP > 3000 && PYP < -3000) Zone = TEXT("CHECKPOINT");
    else if (PXP < -4000) Zone = TEXT("PINE FOREST");
    else if (PYP > 7000) Zone = TEXT("MOUNTAIN RIDGE");
    else if (Dist < 3000) Zone = TEXT("CENTRAL VALLEY");
    else Zone = TEXT("VALLEY OUTSKIRTS");

    DrawRect(FLinearColor(0,0,0,0.5f), 15, 34, Zone.Len()*8.f+16, 22);
    DrawText(Zone, FLinearColor(0.8f,0.8f,0.8f), 23, 36, nullptr, 0.95f);

    AASGameMode* GM = Cast<AASGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM && !GM->bMissionComplete) GM->CheckZoneObjective(Zone);

    // ── OBJECTIVE ───────────────────────────────────────────────────────
    if (GM)
    {
        FString Obj = GM->CurrentObjective;
        float OW = Obj.Len()*6.f;
        DrawRect(FLinearColor(0,0,0,0.5f), CX-OW/2-8, 32, OW+16, 20);
        DrawText(Obj, FLinearColor(0.85f,0.85f,0.7f), CX-OW/2, 34, nullptr, 0.85f);
    }

    // ── AMMO (bottom right) ─────────────────────────────────────────────
    float AX=W-170, AY=H-85;
    DrawRect(FLinearColor(0,0,0,0.4f), AX-10, AY-22, 170, 100);

    DrawText(WeaponName.ToUpper(), FLinearColor(0.55f,0.55f,0.55f), AX, AY-18, nullptr, 0.7f);
    if (bIsReloading)
    {
        // Animated dots for reload
        int32 Dots = ((int32)(MissionTime * 3.f)) % 4;
        FString DotStr = TEXT("RELOADING");
        for (int32 d = 0; d < Dots; d++) DotStr += TEXT(".");
        DrawText(DotStr, FLinearColor(1,0.7f,0.2f), AX, AY, nullptr, 1.5f);
    }
    else
    {
        FLinearColor AC = Ammo > 0 ? FLinearColor::White : FLinearColor(1,0.15f,0.15f);
        DrawText(FString::Printf(TEXT("%d"), Ammo), AC, AX, AY-3, nullptr, 2.0f);
        DrawText(FString::Printf(TEXT("/ %d"), MaxAmmo), FLinearColor(0.45f,0.45f,0.45f), AX+35, AY+5, nullptr, 0.9f);
    }
    DrawText(FString::Printf(TEXT("EMP x%d"), Grenades),
        Grenades>0?FLinearColor(0.2f,0.65f,1):FLinearColor(0.3f,0.3f,0.3f), AX, AY+22, nullptr, 0.75f);
    DrawText(PunchCooldown>0 ? FString::Printf(TEXT("Punch %.0fs"), PunchCooldown) : TEXT("Punch RDY"),
        PunchCooldown>0?FLinearColor(0.4f,0.4f,0.4f):FLinearColor(0.9f,0.8f,0.2f), AX, AY+36, nullptr, 0.75f);

    // Show deployable turrets
    AASPlayerController* TPC = Cast<AASPlayerController>(GetOwningPlayerController());
    if (TPC)
        DrawText(FString::Printf(TEXT("[X]Turret x%d"), TPC->DeployableTurrets),
            TPC->DeployableTurrets>0?FLinearColor(0.4f,0.8f,0.3f):FLinearColor(0.3f,0.3f,0.3f), AX+80, AY+36, nullptr, 0.65f);
    // Low ammo warning
    if (Ammo > 0 && Ammo <= 3 && MaxAmmo > 0 && !bIsReloading)
    {
        float LowAmmoFlash = FMath::Sin(MissionTime * 6.f) * 0.3f + 0.7f;
        DrawText(TEXT("LOW AMMO"), FLinearColor(1.f, 0.3f, 0.1f, LowAmmoFlash), AX, AY-35, nullptr, 0.7f);
    }

    // Status indicators
    float IndX = AX - 5, IndY = AY + 62;
    if (bIsCrouching) DrawText(TEXT("CROUCHED"), FLinearColor(0.5f, 0.8f, 1.f), IndX, IndY, nullptr, 0.55f);
    if (bFlashlightOn) DrawText(TEXT("LIGHT"), FLinearColor(1.f, 0.95f, 0.7f), IndX + 70, IndY, nullptr, 0.55f);
    if (bNearCover) DrawText(TEXT("IN COVER"), FLinearColor(0.3f, 0.9f, 0.4f), IndX + 120, IndY, nullptr, 0.55f);

    // Weapon selector with active highlight (6 weapons)
    FString WNames[] = {TEXT("[1]AR"), TEXT("[2]Rev"), TEXT("[3]Shot"), TEXT("[4]Knife"), TEXT("[5]Snpr"), TEXT("[6]GL")};
    for (int32 w = 0; w < 6; w++)
    {
        FLinearColor WCol = (WeaponIndex==w) ? FLinearColor(1,0.9f,0.3f) : FLinearColor(0.4f,0.4f,0.4f,0.4f);
        DrawText(WNames[w], WCol, AX-5+w*42, AY+50, nullptr, 0.6f);
    }

    // ── HEALTH (bottom left) ────────────────────────────────────────────
    float BX=25, BY=H-75, BW=200, BH=18;
    float HPct = (MaxHealth > 0) ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
    DrawRect(FLinearColor(0,0,0,0.4f), BX-5, BY-18, BW+10, BH+30);
    DrawText(TEXT("HEALTH"), FLinearColor(0.5f,0.65f,0.5f), BX, BY-16, nullptr, 0.65f);
    // Health number
    DrawText(FString::Printf(TEXT("%.0f"), Health), FLinearColor::White, BX+50, BY-16, nullptr, 0.65f);
    // Background bar
    DrawRect(FLinearColor(0.05f,0.02f,0.02f,0.8f), BX, BY, BW, BH);
    // Health fill - color changes with health level
    FLinearColor BarColor;
    if (HPct > 0.6f) BarColor = FLinearColor(0.15f, 0.6f, 0.25f);       // Green
    else if (HPct > 0.3f) BarColor = FLinearColor(0.8f, 0.6f, 0.1f);     // Yellow
    else BarColor = FLinearColor(0.8f, 0.15f, 0.1f);                      // Red
    DrawRect(BarColor, BX, BY, BW * HPct, BH);

    // Shield bar (above health, blue)
    if (Shield > 0.f)
    {
        float ShPct = FMath::Clamp(Shield / MaxShield, 0.f, 1.f);
        DrawRect(FLinearColor(0.03f, 0.03f, 0.08f, 0.8f), BX, BY - 8, BW, 5);
        DrawRect(FLinearColor(0.3f, 0.5f, 1.f), BX, BY - 8, BW * ShPct, 5);
        DrawText(FString::Printf(TEXT("SHIELD %.0f"), Shield), FLinearColor(0.4f,0.6f,1.f), BX+60, BY-24, nullptr, 0.55f);
    }

    // Speed boost indicator
    if (SpeedBoostTimer > 0.f)
    {
        DrawText(FString::Printf(TEXT("SPEED %.0fs"), SpeedBoostTimer), FLinearColor(1.f,0.5f,0.2f), BX, BY-24, nullptr, 0.55f);
    }

    // Stamina bar (below health)
    float SPct = (MaxStamina > 0) ? FMath::Clamp(Stamina / MaxStamina, 0.f, 1.f) : 0.f;
    DrawRect(FLinearColor(0.03f, 0.03f, 0.05f, 0.8f), BX, BY + BH + 4, BW, 6);
    DrawRect(FLinearColor(0.2f, 0.5f, 0.8f), BX, BY + BH + 4, BW * SPct, 6);
    if (SPct < 0.25f)
        DrawText(TEXT("EXHAUSTED"), FLinearColor(0.5f, 0.5f, 0.8f, 0.7f), BX + 60, BY + BH + 2, nullptr, 0.5f);

    // XP bar (below stamina)
    float XPPct = (XPToNextLevel > 0) ? FMath::Clamp((float)XP / (float)XPToNextLevel, 0.f, 1.f) : 0.f;
    DrawRect(FLinearColor(0.03f, 0.03f, 0.03f, 0.6f), BX, BY + BH + 14, BW, 4);
    DrawRect(FLinearColor(0.8f, 0.7f, 0.2f), BX, BY + BH + 14, BW * XPPct, 4);
    DrawText(FString::Printf(TEXT("LV %d"), Level), FLinearColor(0.8f, 0.7f, 0.2f), BX, BY + BH + 20, nullptr, 0.6f);

    // Low health warning pulse
    if (HPct > 0.f && HPct <= 0.25f)
    {
        float Pulse = (FMath::Sin(MissionTime * 6.f) + 1.f) * 0.5f;
        DrawText(TEXT("LOW HEALTH"), FLinearColor(1, 0.1f, 0.1f, 0.5f + Pulse * 0.5f), BX + 60, BY-16, nullptr, 0.65f);
    }

    // Kill counter
    if (Kills > 0)
        DrawText(FString::Printf(TEXT("KILLS: %d"), Kills),
            FLinearColor(0.9f, 0.3f, 0.2f), BX, BY+BH+6, nullptr, 0.75f);

    // ── TUTORIAL (RIGHT SIDE, small, non-blocking) ──────────────────────
    if (TutorialTimer > 0.f)
    {
        float A = FMath::Min(1.f, TutorialTimer / 1.5f);
        FLinearColor BG2(0,0,0, 0.6f*A);
        FLinearColor T2(1,1,1, A);
        FLinearColor HL2(1,0.85f,0.3f, A);

        float TX = W - 260, TY = H * 0.15f;
        DrawRect(BG2, TX, TY, 245, 280);
        DrawRect(FLinearColor(0.3f,0.7f,1,0.7f*A), TX, TY, 245, 2);

        float Y = TY + 8, LX = TX + 10;
        DrawText(TEXT("CONTROLS"), HL2, LX+70, Y, nullptr, 1.1f); Y += 20;

        DrawText(TEXT("WASD      Move"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("Mouse     Look"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("Space     Jump"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("Shift     Sprint"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("Alt       Dodge"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("C/Ctrl    Crouch"), T2, LX, Y, nullptr, 0.7f); Y += 18;

        DrawText(TEXT("LClick    Shoot"), HL2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("RClick    Aim"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("R         Reload"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("1-4       Weapons"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("V         Melee"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("G         EMP"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("Q         Punch"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("E         Interact"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("Scroll    Switch Wpn"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("ESC       Pause"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("F         Flashlight"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("T         Grapple"), T2, LX, Y, nullptr, 0.7f); Y += 18;

        DrawText(TEXT("5 / 6     More Weapons"), T2, LX, Y, nullptr, 0.7f); Y += 13;
        DrawText(TEXT("T         Grapple Hook"), T2, LX, Y, nullptr, 0.7f); Y += 18;
        DrawText(TEXT("Destroy the Relay Node."), FLinearColor(0.3f,1,0.4f,A), LX+20, Y, nullptr, 0.75f);
    }

    // ── KILL FEED (right side) ────────────────────────────────────────
    for (int32 ki = 0; ki < FMath::Min(KillFeed.Num(), 5); ki++)
    {
        float KFA = FMath::Min(1.f, KillFeed[ki].Timer / 0.5f);
        FLinearColor KFC(KillFeed[ki].Color.R/255.f, KillFeed[ki].Color.G/255.f, KillFeed[ki].Color.B/255.f, KFA);
        float KFY = H * 0.25f + ki * 18.f;
        DrawRect(FLinearColor(0,0,0,0.4f*KFA), W-250, KFY-2, 240, 16);
        DrawText(KillFeed[ki].Text, KFC, W-245, KFY, nullptr, 0.7f);
    }

    // ── KILL STREAK (center) ────────────────────────────────────────
    if (KillStreakDisplayTimer > 0.f)
    {
        float KSA = FMath::Min(1.f, KillStreakDisplayTimer / 0.3f);
        float KSScale = 1.5f + (1.f - KSA) * 0.5f; // Slight scale-down as it fades
        DrawText(KillStreakText, FLinearColor(1.f, 0.85f, 0.2f, KSA), CX - KillStreakText.Len()*5, H*0.3f, nullptr, KSScale);
    }

    // ── MINIMAP (top right) ───────────────────────────────────────────
    {
        float MMS = 100.f; // Minimap size
        float MMX = W - MMS - 15, MMY = 35;
        float MMRange = 5000.f; // World units visible on minimap

        // Background circle approximation (square with dark bg)
        DrawRect(FLinearColor(0,0,0,0.5f), MMX, MMY, MMS, MMS);
        DrawRect(FLinearColor(0.15f,0.2f,0.15f,0.3f), MMX+1, MMY+1, MMS-2, MMS-2);

        // Border
        DrawRect(FLinearColor(0.4f,0.5f,0.4f,0.6f), MMX, MMY, MMS, 1);
        DrawRect(FLinearColor(0.4f,0.5f,0.4f,0.6f), MMX, MMY+MMS, MMS, 1);
        DrawRect(FLinearColor(0.4f,0.5f,0.4f,0.6f), MMX, MMY, 1, MMS);
        DrawRect(FLinearColor(0.4f,0.5f,0.4f,0.6f), MMX+MMS, MMY, 1, MMS);

        // Player dot (center, white)
        float PCX = MMX + MMS/2, PCY = MMY + MMS/2;
        DrawRect(FLinearColor(1,1,1,0.9f), PCX-2, PCY-2, 4, 4);

        // Player facing direction indicator
        float FwdRad = FMath::DegreesToRadians(CompassYaw);
        float FX = FMath::Sin(FwdRad) * 8.f;
        float FY = -FMath::Cos(FwdRad) * 8.f;
        DrawRect(FLinearColor(1,1,1,0.6f), PCX+FX-1, PCY+FY-1, 2, 2);

        // Objective dot on minimap (green diamond)
        if (bShowObjectiveWaypoint)
        {
            FVector ObjDiff = ObjectiveWorldPos - PLoc;
            float ODX = ObjDiff.X / MMRange * (MMS/2);
            float ODY = -ObjDiff.Y / MMRange * (MMS/2);
            float OCYR = FMath::DegreesToRadians(-CompassYaw);
            float ORDX = ODX * FMath::Cos(OCYR) - ODY * FMath::Sin(OCYR);
            float ORDY = ODX * FMath::Sin(OCYR) + ODY * FMath::Cos(OCYR);
            float OEX = FMath::Clamp(PCX + ORDX, MMX+3.f, MMX+MMS-3.f);
            float OEY = FMath::Clamp(PCY + ORDY, MMY+3.f, MMY+MMS-3.f);
            float OPulse = FMath::Sin(MissionTime * 4.f) * 0.3f + 0.7f;
            DrawRect(FLinearColor(0.2f, 1.f, 0.4f, OPulse), OEX-3, OEY-3, 6, 6);
        }

        // Enemy dots (red)
        TArray<AActor*> MMEnemies;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASEnemyBase::StaticClass(), MMEnemies);
        for (AActor* EA : MMEnemies)
        {
            AASEnemyBase* Enemy = Cast<AASEnemyBase>(EA);
            if (!Enemy || Enemy->CurrentHealth <= 0) continue;

            FVector Diff = Enemy->GetActorLocation() - PLoc;
            float DX = Diff.X / MMRange * (MMS/2);
            float DY = -Diff.Y / MMRange * (MMS/2); // flip Y

            // Rotate by player yaw so map rotates with player
            float CYR = FMath::DegreesToRadians(-CompassYaw);
            float RDX = DX * FMath::Cos(CYR) - DY * FMath::Sin(CYR);
            float RDY = DX * FMath::Sin(CYR) + DY * FMath::Cos(CYR);

            float EX = PCX + RDX, EY = PCY + RDY;
            // Clamp to minimap bounds
            if (EX > MMX+2 && EX < MMX+MMS-2 && EY > MMY+2 && EY < MMY+MMS-2)
            {
                FLinearColor EC = Enemy->bPlayerDetected ? FLinearColor(1,0.2f,0.1f) : FLinearColor(0.8f,0.4f,0.1f,0.5f);
                DrawRect(EC, EX-1.5f, EY-1.5f, 3, 3);
            }
        }
    }

    // ── ENEMY HEALTH BARS + PROXIMITY CHECK ────────────────────────────
    AASPlayerController* ASPC = Cast<AASPlayerController>(GetOwningPlayerController());
    if (ASPC)
    {
        TArray<AActor*> AllEnemies;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AASEnemyBase::StaticClass(), AllEnemies);
        float ClosestDist = 99999.f;
        for (AActor* EA : AllEnemies)
        {
            AASEnemyBase* Enemy = Cast<AASEnemyBase>(EA);
            if (!Enemy || Enemy->CurrentHealth <= 0) continue;
            float EDist = FVector::Dist(PlayerChar->GetActorLocation(), Enemy->GetActorLocation());

            // Track closest enemy for proximity warning
            if (EDist < ClosestDist) ClosestDist = EDist;

            if (EDist > 2000.f) continue;

            // Enemy health bar
            FVector ELoc = Enemy->GetActorLocation() + FVector(0, 0, 120);
            FVector2D ScreenPos;
            if (UGameplayStatics::ProjectWorldToScreen(GetOwningPlayerController(), ELoc, ScreenPos))
            {
                float EHPct = FMath::Clamp(Enemy->CurrentHealth / Enemy->MaxHealth, 0.f, 1.f);
                float EBW = 50.f, EBH = 4.f;
                DrawRect(FLinearColor(0,0,0,0.6f), ScreenPos.X-EBW/2, ScreenPos.Y, EBW, EBH);
                FLinearColor EColor = EHPct > 0.5f ? FLinearColor(0.8f,0.15f,0.1f) : FLinearColor(1,0.3f,0.1f);
                DrawRect(EColor, ScreenPos.X-EBW/2, ScreenPos.Y, EBW*EHPct, EBH);

                // Show enemy type name if close enough
                if (EDist < 800.f)
                {
                    FString EType = Enemy->GetClass()->GetName().Replace(TEXT("AAS"), TEXT("")).Replace(TEXT("AS"), TEXT(""));
                    DrawText(EType, FLinearColor(0.9f,0.3f,0.2f,0.7f), ScreenPos.X-EType.Len()*3, ScreenPos.Y-12, nullptr, 0.55f);
                }
            }
        }

        // Set proximity warning if enemies within 500 units
        if (ClosestDist < 500.f)
            ProximityWarningAlpha = FMath::Max(ProximityWarningAlpha, 1.f - ClosestDist / 500.f);

        // Count aware enemies for threat display
        int32 AwareCount = 0;
        for (AActor* EA2 : AllEnemies)
        {
            AASEnemyBase* E2 = Cast<AASEnemyBase>(EA2);
            if (E2 && E2->CurrentHealth > 0 && E2->bPlayerDetected) AwareCount++;
        }
        if (AwareCount > 0)
        {
            FLinearColor ThreatColor = AwareCount > 5 ? FLinearColor(1,0.2f,0.1f) :
                (AwareCount > 2 ? FLinearColor(1,0.6f,0.2f) : FLinearColor(0.8f,0.8f,0.3f));
            DrawText(FString::Printf(TEXT("THREAT: %d"), AwareCount), ThreatColor, 25, 58, nullptr, 0.7f);
        }
        else
        {
            DrawText(TEXT("CLEAR"), FLinearColor(0.3f,0.7f,0.3f,0.5f), 25, 58, nullptr, 0.6f);
        }
    }

    // ── OBJECTIVE WAYPOINT (on-screen marker) ───────────────────────
    if (bShowObjectiveWaypoint && PlayerChar)
    {
        FVector2D ObjScreen;
        if (UGameplayStatics::ProjectWorldToScreen(GetOwningPlayerController(), ObjectiveWorldPos, ObjScreen))
        {
            // Diamond marker
            float OMS = 8.f;
            float Pulse = FMath::Sin(MissionTime * 3.f) * 2.f;
            DrawRect(FLinearColor(0.2f, 1.f, 0.4f, 0.8f), ObjScreen.X-OMS/2, ObjScreen.Y-OMS-Pulse, OMS, OMS);
            // Distance text
            float ODist = FVector::Dist(PlayerChar->GetActorLocation(), ObjectiveWorldPos) / 100.f;
            DrawText(FString::Printf(TEXT("%.0fm"), ODist), FLinearColor(0.2f,1,0.4f,0.6f),
                ObjScreen.X-10, ObjScreen.Y+OMS+2, nullptr, 0.6f);
        }
        else
        {
            // Off-screen: show arrow at edge pointing toward objective
            FVector ToObj = ObjectiveWorldPos - PlayerChar->GetActorLocation();
            float ObjAngle = FMath::Atan2(ToObj.Y, ToObj.X);
            float RelAngle = ObjAngle - FMath::DegreesToRadians(CompassYaw) + PI/2;
            float ArrowX = CX + FMath::Cos(RelAngle) * (W * 0.4f);
            float ArrowY = CY + FMath::Sin(RelAngle) * (H * 0.4f);
            ArrowX = FMath::Clamp(ArrowX, 30.f, W-30.f);
            ArrowY = FMath::Clamp(ArrowY, 30.f, H-30.f);
            DrawRect(FLinearColor(0.2f, 1.f, 0.4f, 0.6f), ArrowX-4, ArrowY-4, 8, 8);
        }
    }

    // ── DIALOGUE BOX (bottom center) ──────────────────────────────────
    if (DialogueTimer > 0.f)
    {
        float DlgAlpha = FMath::Min(1.f, DialogueTimer / 0.5f); // Fade out last 0.5s
        float DW = 500.f, DHt = 60.f;
        float DX = CX - DW/2, DY = H - 140;

        // Dark box with accent bar
        DrawRect(FLinearColor(0, 0, 0, 0.75f * DlgAlpha), DX, DY, DW, DHt);
        DrawRect(FLinearColor(0.2f, 0.6f, 1.f, 0.8f * DlgAlpha), DX, DY, DW, 2);

        // Speaker name
        DrawText(DialogueSpeaker, FLinearColor(0.3f, 0.8f, 1.f, DlgAlpha), DX + 12, DY + 6, nullptr, 0.8f);
        // Dialogue text
        DrawText(DialogueLine, FLinearColor(0.9f, 0.9f, 0.9f, DlgAlpha), DX + 12, DY + 24, nullptr, 0.75f);
        // [E] continue hint
        DrawText(TEXT("[E] Continue"), FLinearColor(0.5f, 0.5f, 0.5f, DlgAlpha * 0.6f), DX + DW - 90, DY + DHt - 16, nullptr, 0.55f);
    }

    // ── PAUSE MENU ──────────────────────────────────────────────────────
    if (bPauseMenuActive)
    {
        DrawRect(FLinearColor(0, 0, 0, 0.7f), 0, 0, W, H);
        DrawRect(FLinearColor(0, 0, 0, 0.9f), W*0.3f, H*0.2f, W*0.4f, H*0.55f);
        DrawRect(FLinearColor(0.3f, 0.7f, 1.f, 0.8f), W*0.3f, H*0.2f, W*0.4f, 2);

        DrawText(TEXT("ANALOG STRIKE"), FLinearColor(0.3f, 0.8f, 1.f), W*0.38f, H*0.23f, nullptr, 1.5f);
        DrawText(TEXT("PAUSED"), FLinearColor(0.6f,0.6f,0.6f), W*0.44f, H*0.3f, nullptr, 1.0f);

        // Stats
        float SY = H*0.38f, SX = W*0.35f;
        int32 Mi=(int32)(MissionTime/60), Se=(int32)FMath::Fmod(MissionTime,60);
        DrawText(FString::Printf(TEXT("Mission Time:  %d:%02d"), Mi, Se), FLinearColor::White, SX, SY, nullptr, 0.8f); SY += 22;
        DrawText(FString::Printf(TEXT("Kills:         %d"), Kills), FLinearColor(0.9f,0.3f,0.2f), SX, SY, nullptr, 0.8f); SY += 22;
        DrawText(FString::Printf(TEXT("Health:        %.0f / %.0f"), Health, MaxHealth), FLinearColor(0.3f,0.9f,0.3f), SX, SY, nullptr, 0.8f); SY += 22;
        DrawText(FString::Printf(TEXT("Weapon:        %s"), *WeaponName), FLinearColor::White, SX, SY, nullptr, 0.8f); SY += 22;
        DrawText(FString::Printf(TEXT("Ammo:          %d / %d"), Ammo, MaxAmmo), FLinearColor::White, SX, SY, nullptr, 0.8f); SY += 22;
        DrawText(FString::Printf(TEXT("EMP Grenades:  %d"), Grenades), FLinearColor(0.2f,0.7f,1.f), SX, SY, nullptr, 0.8f); SY += 22;
        DrawText(FString::Printf(TEXT("Level:         %d  (XP: %d/%d)"), Level, XP, XPToNextLevel), FLinearColor(0.8f,0.7f,0.2f), SX, SY, nullptr, 0.8f); SY += 22;
        DrawText(FString::Printf(TEXT("Stamina:       %.0f / %.0f"), Stamina, MaxStamina), FLinearColor(0.3f,0.6f,0.9f), SX, SY, nullptr, 0.8f); SY += 30;

        DrawText(TEXT("[ESC] Resume"), FLinearColor(0.5f,0.5f,0.5f), SX+30, SY, nullptr, 0.75f);
    }

    // ── DEATH SCREEN ────────────────────────────────────────────────────
    if (Health <= 0.f)
    {
        DrawRect(FLinearColor(0.3f, 0, 0, 0.7f), 0, 0, W, H);
        DrawText(TEXT("YOU DIED"), FLinearColor(0.9f, 0.1f, 0.1f), W*0.38f, H*0.4f, nullptr, 2.5f);
        DrawText(TEXT("Respawning..."), FLinearColor(0.6f, 0.6f, 0.6f), W*0.42f, H*0.5f, nullptr, 1.0f);
    }

    // ── MISSION COMPLETE ────────────────────────────────────────────────
    if (GM && GM->bMissionComplete)
    {
        float MCW = W * 0.5f, MCH = H * 0.5f;
        float MCX = W * 0.25f, MCY = H * 0.2f;
        DrawRect(FLinearColor(0, 0, 0, 0.85f), MCX, MCY, MCW, MCH);
        DrawRect(FLinearColor(0.1f, 0.8f, 0.2f, 0.9f), MCX, MCY, MCW, 3);

        float TY = MCY + 15;
        DrawText(TEXT("MISSION COMPLETE"), FLinearColor(0.2f, 1.f, 0.3f), MCX + MCW*0.2f, TY, nullptr, 2.0f); TY += 40;
        DrawText(TEXT("Sector Reclaimed. Analog beats machine."), FLinearColor(0.7f, 0.7f, 0.7f), MCX + 30, TY, nullptr, 0.8f); TY += 30;

        // Stats
        int32 Mi=(int32)(MissionTime/60), Se=(int32)FMath::Fmod(MissionTime,60);
        DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f), MCX+20, TY, MCW-40, 120);
        float SX = MCX + 40;
        DrawText(FString::Printf(TEXT("Mission Time:    %d:%02d"), Mi, Se), FLinearColor::White, SX, TY+8, nullptr, 0.85f);
        DrawText(FString::Printf(TEXT("Enemies Killed:  %d"), Kills), FLinearColor(0.9f, 0.3f, 0.2f), SX, TY+30, nullptr, 0.85f);

        float AccuracyPct = TotalShotsFired > 0 ? (float)TotalHits / (float)TotalShotsFired * 100.f : 0.f;
        DrawText(FString::Printf(TEXT("Accuracy:        %.1f%% (%d/%d)"), AccuracyPct, TotalHits, TotalShotsFired), FLinearColor(0.3f, 0.8f, 1.f), SX, TY+52, nullptr, 0.85f);
        DrawText(FString::Printf(TEXT("Headshots:       %d"), HeadshotCount), FLinearColor(1.f, 0.3f, 0.2f), SX, TY+74, nullptr, 0.85f);
        DrawText(FString::Printf(TEXT("Level:           %d"), Level), FLinearColor(0.8f, 0.7f, 0.2f), SX, TY+96, nullptr, 0.85f);

        // Grade
        FString Grade;
        FLinearColor GradeColor;
        float Score = Kills * 100.f + HeadshotCount * 50.f + AccuracyPct * 10.f - MissionTime * 0.5f + Level * 200.f;
        if (Score > 3000) { Grade = TEXT("S"); GradeColor = FLinearColor(1.f, 0.85f, 0.2f); }
        else if (Score > 2000) { Grade = TEXT("A"); GradeColor = FLinearColor(0.2f, 1.f, 0.3f); }
        else if (Score > 1000) { Grade = TEXT("B"); GradeColor = FLinearColor(0.3f, 0.7f, 1.f); }
        else if (Score > 500) { Grade = TEXT("C"); GradeColor = FLinearColor(0.8f, 0.8f, 0.8f); }
        else { Grade = TEXT("D"); GradeColor = FLinearColor(0.6f, 0.4f, 0.4f); }

        DrawText(TEXT("GRADE"), FLinearColor(0.5f,0.5f,0.5f), SX + MCW*0.5f, TY+8, nullptr, 0.75f);
        DrawText(Grade, GradeColor, SX + MCW*0.5f + 10, TY+28, nullptr, 3.5f);

        TY += 130;
        DrawText(FString::Printf(TEXT("Score: %.0f"), FMath::Max(0.f, Score)), FLinearColor(1.f, 0.9f, 0.3f), SX, TY, nullptr, 1.0f);
    }
}

void AASHUD::ShowDialogue(const FString& Speaker, const FString& Line, float Duration)
{
    DialogueSpeaker = Speaker;
    DialogueLine = Line;
    DialogueTimer = Duration;
}

void AASHUD::AddDamageIndicator(float WorldAngle, float Intensity)
{
    FDamageIndicator DI;
    DI.Angle = WorldAngle;
    DI.Timer = 1.5f;
    DI.Intensity = FMath::Clamp(Intensity, 0.3f, 1.f);
    DamageIndicators.Add(DI);
    if (DamageIndicators.Num() > 6) DamageIndicators.RemoveAt(0);
}

void AASHUD::AddKillFeed(const FString& Text, FColor Color)
{
    FKillFeedEntry Entry;
    Entry.Text = Text;
    Entry.Timer = 4.f;
    Entry.Color = Color;
    KillFeed.Insert(Entry, 0);
    if (KillFeed.Num() > 8) KillFeed.SetNum(8);
}
