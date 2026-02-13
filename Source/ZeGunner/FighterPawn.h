// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "FighterPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class UInputMappingContext;
class UInputAction;
class USoundBase;

/** Game state for managing screens and flow */
UENUM(BlueprintType)
enum class EGameState : uint8
{
	Instructions,
	Playing,
	Paused,
	WaveEnd,
	GameOver
};

/**
 * Turret defense pawn. Fixed at position (0,0,TurretHeight).
 * Mouse movement rotates the turret 360 degrees (yaw) and up/down (pitch).
 * Q/E keys move the turret down/up within configurable height limits.
 * Left-click fires rockets that destroy both tanks and helicopters.
 * Crosshair is always centered on screen.
 */
UCLASS()
class ZEGUNNER_API AFighterPawn : public APawn
{
	GENERATED_BODY()

public:
	AFighterPawn();

	/** Returns the mouse-aim world target (used by HUD) */
	UFUNCTION(BlueprintCallable, Category = "Turret")
	FVector GetRocketAimPoint() const { return RocketAimWorldTarget; }

	/** Returns current altitude (Z) */
	UFUNCTION(BlueprintCallable, Category = "Turret")
	float GetCurrentAltitude() const { return GetActorLocation().Z; }

	/** Returns number of tanks destroyed in current wave */
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetTanksDestroyed() const { return WaveTanksDestroyed; }

	/** Returns number of helicopters destroyed in current wave */
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetHelisDestroyed() const { return WaveHelisDestroyed; }

	/** Returns total tanks in current wave */
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetWaveTotalTanks() const { return WaveTotalTanks; }

	/** Returns total helis in current wave */
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetWaveTotalHelis() const { return WaveTotalHelis; }

	/** Called by projectiles when they destroy an enemy */
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddTankKill() { WaveTanksDestroyed++; TotalTanksDestroyed++; CheckWaveCleared(); }

	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddHeliKill() { WaveHelisDestroyed++; TotalHelisDestroyed++; CheckWaveCleared(); }

	/** Returns current base HP */
	UFUNCTION(BlueprintCallable, Category = "Game")
	int32 GetBaseHP() const { return BaseHP; }

	/** Returns max base HP */
	UFUNCTION(BlueprintCallable, Category = "Game")
	int32 GetBaseMaxHP() const { return BaseMaxHP; }

	/** Returns current game state */
	UFUNCTION(BlueprintCallable, Category = "Game")
	EGameState GetGameState() const { return CurrentGameState; }

	/** Returns current wave number */
	UFUNCTION(BlueprintCallable, Category = "Game")
	int32 GetCurrentWave() const { return CurrentWave; }

	/** Returns red flash alpha (0-1, for screen damage flash) */
	UFUNCTION(BlueprintCallable, Category = "Game")
	float GetDamageFlashAlpha() const { return DamageFlashAlpha; }

	/** Returns the instructions text loaded from file */
	UFUNCTION(BlueprintCallable, Category = "Game")
	const FString& GetInstructionsText() const { return InstructionsText; }

	/** Returns wave duration in seconds */
	UFUNCTION(BlueprintCallable, Category = "Game")
	float GetWaveDuration() const { return WaveDuration; }

	/** Called by enemies when they shoot the base */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void DamageBase(int32 Damage = 1);

	/** Called by spawners to register wave enemy counts */
	void RegisterWaveEnemies(int32 Tanks, int32 Helis);

	/** Returns current sound volume (0.0 - 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetSoundVolume() const { return SoundVolume; }

	/** Returns current aim sensitivity */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetAimSensitivity() const { return AimSensitivity; }

	/** Returns aim sensitivity scaled for UI display */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetAimSensitivityDisplay() const { return AimSensitivity * SensitivityDisplayScale; }

	/** Returns current radar zoom level (multiplier applied to radar world range) */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetRadarZoom() const { return RadarZoom; }

	/** Returns whether FPS display is enabled */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	bool IsFpsDisplayEnabled() const { return bShowFps; }

	/** Returns current smoothed FPS value for display */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetCurrentFps() const { return CurrentFps; }


protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ==================== Components ====================

	/** Root scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	class USceneComponent* SceneRoot;

	/** First-person camera on the turret */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* NoseCamera;

	// ==================== Turret Parameters ====================

	/** Mouse sensitivity for turret aiming (degrees per raw mouse unit) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "0.01"))
	float AimSensitivity = 0.05f;

	/** Maximum pitch angle the turret can look up (degrees, positive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float TurretMaxPitch = 80.0f;

	/** Maximum pitch angle the turret can look down (degrees, positive value = how far down) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float TurretMinPitch = 45.0f;

	/** Starting height of the turret (Z coordinate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float StartAltitude = 3000.0f;

	/** Minimum height the turret can go (Z coordinate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "0.0"))
	float MinTurretHeight = 100.0f;

	/** Maximum height the turret can go (Z coordinate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "0.0"))
	float MaxTurretHeight = 5000.0f;

	/** Speed at which the turret moves up/down with Q/E keys (units/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "0.0"))
	float HeightChangeSpeed = 300.0f;

	// ==================== Mouse Wheel Zoom ====================

	/** Mouse wheel zoom speed (how much zoom changes per wheel tick) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "0.1"))
	float MouseWheelZoomSpeed = 0.5f;

	/** Maximum zoom level (multiplier, 1.0 = normal, higher = zoomed in) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float MaxZoomLevel = 10.0f;

	/** Minimum zoom level (multiplier, 1.0 = normal, lower = zoomed out) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinZoomLevel = 1.0f;

	// ==================== Landscape Streaming ====================

	/** Streaming distance multiplier for landscape loading around the fighter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (ClampMin = "1.0"))
	float LandscapeStreamingDistance = 300000.0f;

	/** Whether to force load all landscape at start (for small maps) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	bool bLoadAllLandscapeAtStart = true;

	// ==================== Rocket / Mouse Crosshair ====================

	/** Blueprint class for the rocket to fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket")
	TSubclassOf<AActor> RocketClass;

	/** Rocket fire rate cooldown (seconds between shots) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket", meta = (ClampMin = "0.01"))
	float RocketCooldown = 0.15f;

	/** Offset from pawn origin where rockets spawn (local space).
	 *  Positive X = forward, negative Z = below camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket")
	FVector RocketSpawnOffset = FVector(300.0f, 0.0f, -50.0f);

	/** Maximum distance for mouse-aim raycast (units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket", meta = (ClampMin = "1000.0"))
	float CrosshairMaxDistance = 50000.0f;

	// ==================== Enhanced Input ====================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* FighterMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* FireRocketAction;

	/** Q = Move turret down */
	UInputAction* HeightDownAction;

	/** E = Move turret up */
	UInputAction* HeightUpAction;

	UInputAction* DebugTestWaveAction;

	UInputAction* PauseAction;
	UInputAction* ContinueAction;
	UInputAction* QuitAction;

	UInputAction* RadarZoomInAction;
	UInputAction* RadarZoomOutAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* VolumeUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* VolumeDownAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SensitivityUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SensitivityDownAction;

	UInputAction* FpsToggleAction;

	/** Mouse wheel zoom input actions */
	UInputAction* MouseWheelZoomInAction;
	UInputAction* MouseWheelZoomOutAction;

private:
	// ==================== Internal State ====================

	/** Current turret yaw (degrees, unlimited 360) */
	float TurretYaw = 0.0f;

	/** Current turret pitch (degrees, clamped) */
	float TurretPitch = 0.0f;

	/** Current height input: -1 = Q (down), +1 = E (up), 0 = none */
	float HeightInput = 0.0f;

	/** Time when last rocket was fired */
	float LastRocketFireTime = -999.0f;

	/** Whether fire button is held */
	bool bFireRocketHeld = false;

	/** Whether turret has been positioned at correct location (done on first Tick) */
	bool bTurretPositioned = false;

	/** Warmup complete flag - prevents input on first frames */
	bool bWarmupComplete = false;

	/** Warmup countdown timer */
	float WarmupTimer = 1.0f;

	/** Current mouse-aim world target location (for rockets / crosshair) */
	FVector RocketAimWorldTarget = FVector::ZeroVector;

	/** Current zoom level (1.0 = normal, >1.0 = zoomed in) */
	float CurrentZoomLevel = 1.0f;

	/** Score tracking (per wave) */
	int32 WaveTanksDestroyed = 0;
	int32 WaveHelisDestroyed = 0;
	int32 WaveTotalTanks = 0;
	int32 WaveTotalHelis = 0;

	/** Score tracking (total) */
	int32 TotalTanksDestroyed = 0;
	int32 TotalHelisDestroyed = 0;

	/** Game state */
	EGameState CurrentGameState = EGameState::Instructions;
	int32 CurrentWave = 0;

	/** Base HP */
	int32 BaseHP = 100;
	int32 BaseMaxHP = 100;

	/** Damage flash */
	float DamageFlashAlpha = 0.0f;
	float DamageFlashDecayRate = 3.0f;

	/** Wave timing */
	float WaveStartTime = 0.0f;
	float WaveDuration = 0.0f;

	/** Instructions text (editable in Blueprint or C++) */
	UPROPERTY(EditAnywhere, Category = "Game")
	FString InstructionsText =
		TEXT("INSTRUCTIONS\n")
		TEXT("  | Aim: Mouse (360 rotation)\n")
		TEXT("  | Fire rockets: Left Mouse (hold for auto-fire)\n")
		TEXT("  | Turret Up: E || Turret Down: Q\n")
		TEXT("  | Zoom radar: [ ] keys || Toggle FPS: F key\n")
		TEXT("  | Volume: Arrows Up/Down || Sensitivity: Arrows Left/Right\n")
		TEXT("  | Pause game: ESC\n")
		TEXT("  | Rockets destroy tanks and helicopters!\n")
		TEXT("  | If the base HP reaches zero, you lose the game!\n");

	/** Sound volume (0.0 - 1.0) */
	float SoundVolume = 0.5f;

	/** Volume/sensitivity step per key press */
	float VolumeStep = 0.05f;
	float SensitivityStep = 0.1f;
	float MinSensitivity = 0.1f;
	float MaxSensitivity = 5.0f;

	/** Display scaling for sensitivity (multiplier for UI display only) */
	float SensitivityDisplayScale = 50.0f;

	/** Radar zoom level (1.0 = default, lower = zoomed in, higher = zoomed out) */
	float RadarZoom = 1.0f;
	float RadarZoomStep = 0.15f;
	float RadarZoomMin = 0.2f;
	float RadarZoomMax = 3.0f;

	/** Timer for periodic enemy scanning */
	float EnemyScanTimer = 0.0f;

	/** Set of enemies we've already bound OnDestroyed to (avoid double-binding) */
	TSet<AActor*> BoundEnemies;

	/** Cached mouse delta for current frame */
	float FrameMouseDeltaX = 0.0f;
	float FrameMouseDeltaY = 0.0f;

	/** Whether FPS display is enabled */
	bool bShowFps = false;

	/** FPS display update timer */
	float FpsUpdateTimer = 0.0f;

	/** FPS update interval in seconds (how often to refresh the display) */
	float FpsUpdateInterval = 5.0f;

	/** Current smoothed FPS value for display */
	float CurrentFps = 0.0f;

	// ==================== Input Handlers ====================

	void OnHeightDown(const FInputActionValue& Value);
	void OnHeightDownReleased(const FInputActionValue& Value);
	void OnHeightUp(const FInputActionValue& Value);
	void OnHeightUpReleased(const FInputActionValue& Value);
	void OnFireRocket(const FInputActionValue& Value);
	void OnFireRocketReleased(const FInputActionValue& Value);
	void OnVolumeUp(const FInputActionValue& Value);
	void OnVolumeDown(const FInputActionValue& Value);
	void OnSensitivityUp(const FInputActionValue& Value);
	void OnSensitivityDown(const FInputActionValue& Value);
	void OnDebugTestWave(const FInputActionValue& Value);
	void OnPausePressed(const FInputActionValue& Value);
	void OnContinuePressed(const FInputActionValue& Value);
	void OnQuitGame(const FInputActionValue& Value);
	void OnRadarZoomIn(const FInputActionValue& Value);
	void OnRadarZoomOut(const FInputActionValue& Value);
	void OnFpsToggle(const FInputActionValue& Value);
	void OnMouseWheelZoomIn(const FInputActionValue& Value);
	void OnMouseWheelZoomOut(const FInputActionValue& Value);

	// ==================== Core Logic ====================

	void UpdateTurretAim(float DeltaTime);
	void UpdateTurretHeight(float DeltaTime);
	void UpdateMouseAim();
	void FireRocket();
	void BindEnemyDestroyedEvents();
	void CheckWaveCleared();
	void StartNextWave();
	void ApplyZoomToCamera();

	UFUNCTION()
	void OnEnemyDestroyed(AActor* DestroyedActor);

	// ==================== Landscape Streaming ====================

	/** Configure landscape streaming settings for optimal aerial view */
	void ConfigureLandscapeStreaming();

	/** Update landscape streaming based on turret position */
	void UpdateLandscapeStreaming();
};
