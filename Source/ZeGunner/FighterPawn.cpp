// Copyright Epic Games, Inc. All Rights Reserved.

#include "FighterPawn.h"
#include "RocketProjectile.h"
#include "TankAI.h"
#include "HeliAI.h"
#include "TankWaveSpawner.h"
#include "HeliWaveSpawner.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerInput.h"
#include "Framework/Application/SlateApplication.h"
#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LandscapeProxy.h"
#include "WorldPartition/WorldPartition.h"
#include "Engine/World.h"

AFighterPawn::AFighterPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create scene root
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create first-person camera attached directly to root
	NoseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("NoseCamera"));
	NoseCamera->SetupAttachment(SceneRoot);
	NoseCamera->bUsePawnControlRotation = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Create game state input actions
	PauseAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Pause_Auto"));
	PauseAction->ValueType = EInputActionValueType::Boolean;

	// Debug action for testing (Delete key)
	DebugTestWaveAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_DebugTestWave_Auto"));
	DebugTestWaveAction->ValueType = EInputActionValueType::Boolean;

	ContinueAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Continue_Auto"));
	ContinueAction->ValueType = EInputActionValueType::Boolean;

	QuitAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Quit_Auto"));
	QuitAction->ValueType = EInputActionValueType::Boolean;

	// Create height input actions (Q/E keys)
	HeightDownAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_HeightDown_Auto"));
	HeightDownAction->ValueType = EInputActionValueType::Boolean;

	HeightUpAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_HeightUp_Auto"));
	HeightUpAction->ValueType = EInputActionValueType::Boolean;

	// Create radar zoom input actions ([ ] keys)
	RadarZoomInAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_RadarZoomIn_Auto"));
	RadarZoomInAction->ValueType = EInputActionValueType::Boolean;

	RadarZoomOutAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_RadarZoomOut_Auto"));
	RadarZoomOutAction->ValueType = EInputActionValueType::Boolean;

	// Create FPS toggle input action (F key)
	FpsToggleAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_FpsToggle_Auto"));
	FpsToggleAction->ValueType = EInputActionValueType::Boolean;

	// Create mouse wheel zoom input actions
	MouseWheelZoomInAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_MouseWheelZoomIn_Auto"));
	MouseWheelZoomInAction->ValueType = EInputActionValueType::Boolean;

	MouseWheelZoomOutAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_MouseWheelZoomOut_Auto"));
	MouseWheelZoomOutAction->ValueType = EInputActionValueType::Boolean;
}

void AFighterPawn::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: BeginPlay started (Turret Mode)"));

	TurretYaw = 0.0f;
	TurretPitch = 0.0f;
	bTurretPositioned = false;

	// Configure landscape streaming
	ConfigureLandscapeStreaming();

	// Create a mapping context for turret controls (always works, no Blueprint needed)
	UInputMappingContext* TurretMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Turret_Auto"));
	TurretMappingContext->MapKey(PauseAction, EKeys::Escape);
	TurretMappingContext->MapKey(ContinueAction, EKeys::C);
	TurretMappingContext->MapKey(QuitAction, EKeys::X);
	TurretMappingContext->MapKey(HeightDownAction, EKeys::Q);
	TurretMappingContext->MapKey(HeightUpAction, EKeys::E);
	TurretMappingContext->MapKey(RadarZoomInAction, EKeys::LeftBracket);
	TurretMappingContext->MapKey(RadarZoomOutAction, EKeys::RightBracket);
	TurretMappingContext->MapKey(FpsToggleAction, EKeys::F);
	TurretMappingContext->MapKey(DebugTestWaveAction, EKeys::Delete);
	TurretMappingContext->MapKey(MouseWheelZoomInAction, EKeys::MouseScrollUp);
	TurretMappingContext->MapKey(MouseWheelZoomOutAction, EKeys::MouseScrollDown);
	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Created Turret mapping context with Q/E height, [ ] zoom, F FPS toggle, Delete debug, mouse wheel zoom"));

	// Add input mapping contexts
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (FighterMappingContext)
			{
				Subsystem->AddMappingContext(FighterMappingContext, 0);
				UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Blueprint mapping context added"));
			}

			// Add the turret mapping context at higher priority
			Subsystem->AddMappingContext(TurretMappingContext, 1);
			UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Turret mapping context added"));
		}
	}

	// Hide OS mouse cursor and use Game-only input mode for zero-lag mouse
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->bShowMouseCursor = false;
		PC->bEnableClickEvents = false;
		PC->bEnableMouseOverEvents = false;

		// Game-only mode: raw mouse input, no Slate cursor processing = zero lag
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);

		// Set raw mouse sensitivity to 1:1 for immediate response
		if (PC->PlayerInput)
		{
			PC->PlayerInput->SetMouseSensitivity(1.0f);
		}
	}

	// Bind to existing enemy destruction events for score tracking
	BindEnemyDestroyedEvents();

	// Start in Instructions state
	CurrentGameState = EGameState::Instructions;

	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Turret initialized at position (0, 0, %.0f)"), StartAltitude);
}

void AFighterPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Force turret to correct position on first frames (after GameMode spawn completes)
	if (!bTurretPositioned)
	{
		SetActorLocationAndRotation(FVector(0.0f, 0.0f, StartAltitude), FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		if (Controller)
		{
			Controller->SetControlRotation(FRotator::ZeroRotator);
		}

		// Find and configure whatever camera component exists (C++ or Blueprint)
		TArray<UCameraComponent*> Cameras;
		GetComponents<UCameraComponent>(Cameras);
		for (UCameraComponent* Cam : Cameras)
		{
			// Disable pawn control rotation — we set actor rotation directly
			Cam->bUsePawnControlRotation = false;
			// Reset camera to pawn origin (no offset)
			Cam->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
			UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Configured camera '%s' - disabled PawnControlRotation, reset transform"), *Cam->GetName());
		}

		bTurretPositioned = true;
		UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Turret positioned at (0, 0, %.0f). Actual: %s. Found %d cameras."), StartAltitude, *GetActorLocation().ToString(), Cameras.Num());
	}

	// Warmup period - prevent any actions on first frames
	if (!bWarmupComplete)
	{
		WarmupTimer -= DeltaTime;
		if (WarmupTimer <= 0.0f)
		{
			bWarmupComplete = true;
			UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Warmup complete, input enabled"));
		}
		return;
	}

	// Read raw mouse delta ONCE per frame (consumed on read, so only call once)
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC)
	{
		PC->GetInputMouseDelta(FrameMouseDeltaX, FrameMouseDeltaY);
	}
	else
	{
		FrameMouseDeltaX = 0.0f;
		FrameMouseDeltaY = 0.0f;
	}

	// Decay damage flash
	if (DamageFlashAlpha > 0.0f)
	{
		DamageFlashAlpha = FMath::Max(0.0f, DamageFlashAlpha - DamageFlashDecayRate * DeltaTime);
	}

	// Update FPS display timer (runs even when paused)
	if (bShowFps)
	{
		FpsUpdateTimer -= DeltaTime;
		if (FpsUpdateTimer <= 0.0f)
		{
			CurrentFps = 1.0f / DeltaTime;
			FpsUpdateTimer = FpsUpdateInterval;
		}
	}

	// Only run gameplay when Playing
	if (CurrentGameState != EGameState::Playing) return;

	UpdateTurretAim(DeltaTime);
	UpdateTurretHeight(DeltaTime);
	UpdateMouseAim();

	// Periodically re-bind to newly spawned enemies (every ~1 second)
	EnemyScanTimer -= DeltaTime;
	if (EnemyScanTimer <= 0.0f)
	{
		BindEnemyDestroyedEvents();
		EnemyScanTimer = 1.0f;
	}

	// Auto-fire rockets while button is held
	if (bFireRocketHeld)
	{
		FireRocket();
	}
}

void AFighterPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind mouse wheel zoom actions
		EnhancedInputComponent->BindAction(MouseWheelZoomInAction, ETriggerEvent::Started, this, &AFighterPawn::OnMouseWheelZoomIn);
		EnhancedInputComponent->BindAction(MouseWheelZoomOutAction, ETriggerEvent::Started, this, &AFighterPawn::OnMouseWheelZoomOut);
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Q = Height Down
		if (HeightDownAction)
		{
			EIC->BindAction(HeightDownAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnHeightDown);
			EIC->BindAction(HeightDownAction, ETriggerEvent::Completed, this, &AFighterPawn::OnHeightDownReleased);
		}

		// E = Height Up
		if (HeightUpAction)
		{
			EIC->BindAction(HeightUpAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnHeightUp);
			EIC->BindAction(HeightUpAction, ETriggerEvent::Completed, this, &AFighterPawn::OnHeightUpReleased);
		}

		// [ = Radar Zoom In
		if (RadarZoomInAction)
		{
			EIC->BindAction(RadarZoomInAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnRadarZoomIn);
		}

		// ] = Radar Zoom Out
		if (RadarZoomOutAction)
		{
			EIC->BindAction(RadarZoomOutAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnRadarZoomOut);
		}

		// Left Mouse = Fire Rocket
		if (FireRocketAction)
		{
			EIC->BindAction(FireRocketAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnFireRocket);
			EIC->BindAction(FireRocketAction, ETriggerEvent::Completed, this, &AFighterPawn::OnFireRocketReleased);
		}

		// ESC = Pause
		if (PauseAction)
		{
			EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AFighterPawn::OnPausePressed);
		}

		// C = Continue / Start / Next Wave / Restart
		if (ContinueAction)
		{
			EIC->BindAction(ContinueAction, ETriggerEvent::Started, this, &AFighterPawn::OnContinuePressed);
		}

		// X = Quit
		if (QuitAction)
		{
			EIC->BindAction(QuitAction, ETriggerEvent::Started, this, &AFighterPawn::OnQuitGame);
		}

		// Volume controls
		if (VolumeUpAction)
		{
			EIC->BindAction(VolumeUpAction, ETriggerEvent::Started, this, &AFighterPawn::OnVolumeUp);
		}
		if (VolumeDownAction)
		{
			EIC->BindAction(VolumeDownAction, ETriggerEvent::Started, this, &AFighterPawn::OnVolumeDown);
		}

		// Sensitivity controls
		if (SensitivityUpAction)
		{
			EIC->BindAction(SensitivityUpAction, ETriggerEvent::Started, this, &AFighterPawn::OnSensitivityUp);
		}
		if (SensitivityDownAction)
		{
			EIC->BindAction(SensitivityDownAction, ETriggerEvent::Started, this, &AFighterPawn::OnSensitivityDown);
		}

		// Debug: Delete = Test high-level wave
		if (DebugTestWaveAction)
		{
			EIC->BindAction(DebugTestWaveAction, ETriggerEvent::Started, this, &AFighterPawn::OnDebugTestWave);
		}

		// F key = Toggle FPS display
		if (FpsToggleAction)
		{
			EIC->BindAction(FpsToggleAction, ETriggerEvent::Started, this, &AFighterPawn::OnFpsToggle);
		}
	}
}

// ==================== Input Handlers ====================

void AFighterPawn::OnHeightDown(const FInputActionValue& Value)
{
	HeightInput = -1.0f;
}

void AFighterPawn::OnHeightDownReleased(const FInputActionValue& Value)
{
	HeightInput = 0.0f;
}

void AFighterPawn::OnHeightUp(const FInputActionValue& Value)
{
	HeightInput = 1.0f;
}

void AFighterPawn::OnHeightUpReleased(const FInputActionValue& Value)
{
	HeightInput = 0.0f;
}

void AFighterPawn::OnRadarZoomIn(const FInputActionValue& Value)
{
	RadarZoom = FMath::Clamp(RadarZoom - RadarZoomStep, RadarZoomMin, RadarZoomMax);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Radar Zoom IN -> %.2f"), RadarZoom);
}

void AFighterPawn::OnRadarZoomOut(const FInputActionValue& Value)
{
	RadarZoom = FMath::Clamp(RadarZoom + RadarZoomStep, RadarZoomMin, RadarZoomMax);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Radar Zoom OUT -> %.2f"), RadarZoom);
}

void AFighterPawn::OnFireRocket(const FInputActionValue& Value)
{
	if (!bWarmupComplete) return;
	bFireRocketHeld = true;
}

void AFighterPawn::OnFireRocketReleased(const FInputActionValue& Value)
{
	bFireRocketHeld = false;
}

void AFighterPawn::OnVolumeUp(const FInputActionValue& Value)
{
	SoundVolume = FMath::Clamp(SoundVolume + VolumeStep, 0.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Volume UP -> %.0f%%"), SoundVolume * 100.0f);
}

void AFighterPawn::OnVolumeDown(const FInputActionValue& Value)
{
	SoundVolume = FMath::Clamp(SoundVolume - VolumeStep, 0.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Volume DOWN -> %.0f%%"), SoundVolume * 100.0f);
}

void AFighterPawn::OnSensitivityUp(const FInputActionValue& Value)
{
	AimSensitivity = FMath::Clamp(AimSensitivity + SensitivityStep, MinSensitivity, MaxSensitivity);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Sensitivity UP -> %.1f"), AimSensitivity);
}

void AFighterPawn::OnSensitivityDown(const FInputActionValue& Value)
{
	AimSensitivity = FMath::Clamp(AimSensitivity - SensitivityStep, MinSensitivity, MaxSensitivity);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Sensitivity DOWN -> %.1f"), AimSensitivity);
}

void AFighterPawn::OnPausePressed(const FInputActionValue& Value)
{
	if (CurrentGameState == EGameState::Playing)
	{
		CurrentGameState = EGameState::Paused;
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Game PAUSED"));
	}
}

void AFighterPawn::OnDebugTestWave(const FInputActionValue& Value)
{
	if (CurrentGameState != EGameState::Playing) return;

	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: DEBUG - Destroying all enemies except one of each type"));

	TArray<ATankAI*> AllTanks;
	for (TActorIterator<ATankAI> It(GetWorld()); It; ++It)
	{
		AllTanks.Add(*It);
	}

	for (int32 i = 1; i < AllTanks.Num(); i++)
	{
		if (AllTanks[i] && AllTanks[i]->IsValidLowLevel())
		{
			AllTanks[i]->Destroy();
		}
	}

	TArray<AHeliAI*> AllHelis;
	for (TActorIterator<AHeliAI> It(GetWorld()); It; ++It)
	{
		AllHelis.Add(*It);
	}

	for (int32 i = 1; i < AllHelis.Num(); i++)
	{
		if (AllHelis[i] && AllHelis[i]->IsValidLowLevel())
		{
			AllHelis[i]->Destroy();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: DEBUG - Kept 1 tank and 1 heli for testing. Destroyed %d tanks and %d helis."), 
		FMath::Max(0, AllTanks.Num() - 1), FMath::Max(0, AllHelis.Num() - 1));
}

void AFighterPawn::OnContinuePressed(const FInputActionValue& Value)
{
	if (CurrentGameState == EGameState::Instructions || CurrentGameState == EGameState::WaveEnd)
	{
		StartNextWave();
	}
	else if (CurrentGameState == EGameState::Paused)
	{
		CurrentGameState = EGameState::Playing;
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Game RESUMED"));
	}
	else if (CurrentGameState == EGameState::GameOver)
	{
		UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()));
	}
}

void AFighterPawn::OnQuitGame(const FInputActionValue& Value)
{
	if (CurrentGameState == EGameState::Paused)
	{
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Quitting game"));
		UKismetSystemLibrary::QuitGame(GetWorld(), Cast<APlayerController>(Controller), EQuitPreference::Quit, false);
	}
}

void AFighterPawn::OnFpsToggle(const FInputActionValue& Value)
{
	bShowFps = !bShowFps;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: FPS display %s"), bShowFps ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void AFighterPawn::DamageBase(int32 Damage)
{
	if (CurrentGameState != EGameState::Playing) return;

	BaseHP = FMath::Max(0, BaseHP - Damage);
	DamageFlashAlpha = 0.6f;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Base hit! HP: %d/%d"), BaseHP, BaseMaxHP);

	if (BaseHP <= 0)
	{
		CurrentGameState = EGameState::GameOver;
		UE_LOG(LogTemp, Warning, TEXT("FighterPawn: GAME OVER - Base destroyed!"));
	}
}

void AFighterPawn::RegisterWaveEnemies(int32 Tanks, int32 Helis)
{
	WaveTotalTanks += Tanks;
	WaveTotalHelis += Helis;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Wave enemies registered - Tanks: %d, Helis: %d"), WaveTotalTanks, WaveTotalHelis);
}

void AFighterPawn::CheckWaveCleared()
{
	if (CurrentGameState != EGameState::Playing) return;

	int32 TotalKilled = WaveTanksDestroyed + WaveHelisDestroyed;
	int32 TotalEnemies = WaveTotalTanks + WaveTotalHelis;

	if (TotalEnemies > 0 && TotalKilled >= TotalEnemies)
	{
		WaveDuration = GetWorld()->GetTimeSeconds() - WaveStartTime;
		CurrentGameState = EGameState::WaveEnd;
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Wave %d cleared in %.1f seconds!"), CurrentWave, WaveDuration);
	}
}

void AFighterPawn::StartNextWave()
{
	CurrentWave++;
	WaveTanksDestroyed = 0;
	WaveHelisDestroyed = 0;
	WaveTotalTanks = 0;
	WaveTotalHelis = 0;
	WaveStartTime = GetWorld()->GetTimeSeconds();
	
	// Reset base HP at the start of each wave
	BaseHP = BaseMaxHP;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Base HP reset to %d for wave %d"), BaseHP, CurrentWave);

	CurrentGameState = EGameState::Playing;

	// Find spawners and trigger them
	for (TActorIterator<ATankWaveSpawner> It(GetWorld()); It; ++It)
	{
		int32 TankCount = It->GetNextWaveTankCount();
		It->TriggerNextWave();
		RegisterWaveEnemies(TankCount, 0);
	}

	for (TActorIterator<AHeliWaveSpawner> It(GetWorld()); It; ++It)
	{
		int32 HeliCount = It->GetNextWaveHeliCount();
		It->TriggerNextWave();
		RegisterWaveEnemies(0, HeliCount);
	}

	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Wave %d started! Tanks: %d, Helis: %d"), CurrentWave, WaveTotalTanks, WaveTotalHelis);
}

// ==================== Turret Aim (Mouse Rotation) ====================

void AFighterPawn::UpdateTurretAim(float DeltaTime)
{
	// Apply raw mouse delta directly to turret rotation for maximum responsiveness
	// No frame scaling - raw mouse delta is already frame-independent
	TurretYaw += FrameMouseDeltaX * AimSensitivity;
	TurretPitch += FrameMouseDeltaY * AimSensitivity;

	// Clamp pitch (negative = look down, positive = look up)
	TurretPitch = FMath::Clamp(TurretPitch, -TurretMinPitch, TurretMaxPitch);

	FRotator NewRotation(TurretPitch, TurretYaw, 0.0f);

	// Apply rotation to the pawn (360 degree yaw, clamped pitch, no roll)
	SetActorRotation(NewRotation);

	// Also update controller rotation so cameras with bUsePawnControlRotation follow
	if (Controller)
	{
		Controller->SetControlRotation(NewRotation);
	}
}

// ==================== Turret Height (Q/E Keys) ====================

void AFighterPawn::UpdateTurretHeight(float DeltaTime)
{
	if (FMath::Abs(HeightInput) < 0.01f) return;

	FVector CurrentLocation = GetActorLocation();
	float NewZ = CurrentLocation.Z + HeightInput * HeightChangeSpeed * DeltaTime;
	NewZ = FMath::Clamp(NewZ, MinTurretHeight, MaxTurretHeight);

	// Keep X and Y fixed at 0
	SetActorLocation(FVector(0.0f, 0.0f, NewZ));
}

// ==================== Mouse Aim (Center Screen Raycast) ====================

void AFighterPawn::UpdateMouseAim()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;

	// Deproject from screen center (crosshair is always centered)
	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	float CenterX = SizeX * 0.5f;
	float CenterY = SizeY * 0.5f;

	FVector WorldLocation, WorldDirection;
	PC->DeprojectScreenPositionToWorld(CenterX, CenterY, WorldLocation, WorldDirection);

	FVector TraceStart = WorldLocation;
	FVector TraceEnd = WorldLocation + (WorldDirection * CrosshairMaxDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		RocketAimWorldTarget = HitResult.ImpactPoint;
	}
	else
	{
		RocketAimWorldTarget = TraceEnd;
	}
}

// ==================== Weapons ====================

void AFighterPawn::FireRocket()
{
	if (!RocketClass) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastRocketFireTime < RocketCooldown) return;
	LastRocketFireTime = CurrentTime;

	FVector SpawnLocation = GetActorLocation() + GetActorTransform().TransformVector(RocketSpawnOffset);

	// Direction from spawn point to mouse-aim target
	FVector Direction = (RocketAimWorldTarget - SpawnLocation).GetSafeNormal();

	if (RocketAimWorldTarget.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	FRotator SpawnRotation = Direction.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AActor* Rocket = GetWorld()->SpawnActor<AActor>(RocketClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Rocket)
	{
		if (ARocketProjectile* RocketProj = Cast<ARocketProjectile>(Rocket))
		{
			RocketProj->SetFlightDirection(Direction);
		}

		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Rocket fired toward %s"), *RocketAimWorldTarget.ToString());
	}
}

// ==================== Score Tracking ====================

void AFighterPawn::BindEnemyDestroyedEvents()
{
	if (!GetWorld()) return;

	// Bind to all TankAI actors
	for (TActorIterator<ATankAI> It(GetWorld()); It; ++It)
	{
		AActor* Tank = *It;
		if (!BoundEnemies.Contains(Tank))
		{
			Tank->OnDestroyed.AddDynamic(this, &AFighterPawn::OnEnemyDestroyed);
			BoundEnemies.Add(Tank);
		}
	}

	// Bind to all HeliAI actors
	for (TActorIterator<AHeliAI> It(GetWorld()); It; ++It)
	{
		AActor* Heli = *It;
		if (!BoundEnemies.Contains(Heli))
		{
			Heli->OnDestroyed.AddDynamic(this, &AFighterPawn::OnEnemyDestroyed);
			BoundEnemies.Add(Heli);
		}
	}
}

void AFighterPawn::OnEnemyDestroyed(AActor* DestroyedActor)
{
	BoundEnemies.Remove(DestroyedActor);

	if (DestroyedActor->IsA<ATankAI>())
	{
		AddTankKill();
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Tank destroyed! Wave: %d/%d"), WaveTanksDestroyed, WaveTotalTanks);
	}
	else if (DestroyedActor->IsA<AHeliAI>())
	{
		AddHeliKill();
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Helicopter destroyed! Wave: %d/%d"), WaveHelisDestroyed, WaveTotalHelis);
	}
}

// ==================== Landscape Streaming ====================

void AFighterPawn::ConfigureLandscapeStreaming()
{
	// Disabled for turret mode — not needed at low altitude and caused crashes
	// (EXCEPTION_ACCESS_VIOLATION in UnrealEditor_Landscape during BeginPlay)
}

void AFighterPawn::UpdateLandscapeStreaming()
{
	// This can be called from Tick if needed for dynamic streaming updates
	// For now, the initial configuration should be sufficient for most cases
}

// ==================== Mouse Wheel Zoom ====================

void AFighterPawn::OnMouseWheelZoomIn(const FInputActionValue& Value)
{
	CurrentZoomLevel = FMath::Clamp(CurrentZoomLevel + MouseWheelZoomSpeed, MinZoomLevel, MaxZoomLevel);
	ApplyZoomToCamera();
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Zoom IN -> %.1fx"), CurrentZoomLevel);
}

void AFighterPawn::OnMouseWheelZoomOut(const FInputActionValue& Value)
{
	CurrentZoomLevel = FMath::Clamp(CurrentZoomLevel - MouseWheelZoomSpeed, MinZoomLevel, MaxZoomLevel);
	ApplyZoomToCamera();
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Zoom OUT -> %.1fx"), CurrentZoomLevel);
}

void AFighterPawn::ApplyZoomToCamera()
{
	// Apply zoom to all camera components by adjusting FOV
	TArray<UCameraComponent*> Cameras;
	GetComponents<UCameraComponent>(Cameras);
	
	for (UCameraComponent* Cam : Cameras)
	{
		// Default FOV is typically 90 degrees
		float DefaultFOV = 90.0f;
		float NewFOV = DefaultFOV / CurrentZoomLevel;
		
		// Clamp FOV to reasonable values (10-120 degrees)
		NewFOV = FMath::Clamp(NewFOV, 10.0f, 120.0f);
		
		Cam->SetFieldOfView(NewFOV);
	}
}
