// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeliAI.h"
#include "HeliWaveSpawner.generated.h"

UCLASS()
class ZEGUNNER_API AHeliWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	AHeliWaveSpawner();

	/** Manually trigger the next wave (called by FighterPawn) */
	void TriggerNextWave();

	/** Returns number of active helis */
	int32 GetActiveHeliCount() const { return ActiveHeliCount; }

	/** Returns current wave number */
	int32 GetCurrentWave() const { return CurrentWave; }

	/** Returns how many helis will spawn in the next wave */
	int32 GetNextWaveHeliCount() const { return HelisPerWave + CurrentWave * HelisAddedPerWave; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** The class to spawn for helicopters (should be a Blueprint based on HeliAI) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning")
	TSubclassOf<class AHeliAI> HeliClass;

	/** The static mesh actor representing the base/center target */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Helicopter Spawning")
	AActor* BaseTarget;

	/** Initial distance from center (0,0,0) where helicopters spawn on wave 1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "100.0"))
	float InitialSpawnRadius = 2000.0f;

	/** Maximum spawn radius across all waves */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "100.0"))
	float MaxSpawnRadius = 5000.0f;

	/** How much the spawn radius increases per wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float SpawnRadiusWaveIncrement = 200.0f;

	// ==================== Wave Speed Scaling ====================

	/** Minimum helicopter speed on wave 1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Speed", meta = (ClampMin = "0.0"))
	float InitialMinSpeed = 200.0f;

	/** Maximum helicopter speed on wave 1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Speed", meta = (ClampMin = "0.0"))
	float InitialMaxSpeed = 500.0f;

	/** Absolute cap for minimum speed (min speed cannot exceed this across waves) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Speed", meta = (ClampMin = "0.0"))
	float MaxPossibleMinSpeed = 600.0f;

	/** Absolute cap for maximum speed (max speed cannot exceed this across waves) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Speed", meta = (ClampMin = "0.0"))
	float MaxPossibleMaxSpeed = 1200.0f;

	/** How much the minimum speed increases per wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Speed", meta = (ClampMin = "0.0"))
	float MinSpeedIncrementPerWave = 20.0f;

	/** How much the maximum speed increases per wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Speed", meta = (ClampMin = "0.0"))
	float MaxSpeedIncrementPerWave = 40.0f;

	/** Number of helicopters to spawn in the first wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "1"))
	int32 HelisPerWave = 3;

	/** Additional helicopters added for each subsequent wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0"))
	int32 HelisAddedPerWave = 1;

	/** Time delay between waves in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float WaveDelay = 5.0f;

	/** Minimum distance between spawned helicopters */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "10.0"))
	float MinSpawnSeparation = 100.0f;

	/** Minimum height above ground to spawn helicopters */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float MinSpawnHeight = 400.0f;

	/** Maximum height above ground to spawn helicopters */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float MaxSpawnHeight = 800.0f;

	/** Rotation offset to fix helicopter model orientation (in degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning")
	float MeshRotationOffset = 90.0f;

	/** Distance from base where helicopters stop and start firing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float LineOfFireDistance = 500.0f;

	/** Rate of fire - seconds between shots at the base */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.1"))
	float RateOfFire = 3.0f;

	// ==================== Lateral Dancing ====================

	/** Distance from base where helicopters start lateral dancing (units) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Lateral Dancing", meta = (ClampMin = "0.0"))
	float LateralDanceDistance = 1000.0f;

	/** Minimum lateral movement speed (units/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Lateral Dancing", meta = (ClampMin = "0.0"))
	float MinLateralSpeed = 100.0f;

	/** Maximum lateral movement speed (units/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Lateral Dancing", meta = (ClampMin = "0.0"))
	float MaxLateralSpeed = 400.0f;

	/** Minimum time moving in one lateral direction before switching (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Lateral Dancing", meta = (ClampMin = "0.1"))
	float MinLateralTime = 0.5f;

	/** Maximum time moving in one lateral direction before switching (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Lateral Dancing", meta = (ClampMin = "0.1"))
	float MaxLateralTime = 2.0f;

private:
	/** Current wave number */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWave = 0;

	/** Number of active helicopters currently alive */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (AllowPrivateAccess = "true"))
	int32 ActiveHeliCount = 0;

	/** Timer handle for wave spawning */
	FTimerHandle WaveTimerHandle;

	/** Set of used spawn angles to prevent overlapping */
	TArray<float> UsedSpawnAngles;

	/** Spawn a single wave of helicopters */
	UFUNCTION()
	void SpawnWave();

	/** Calculate a random spawn position on the circle that doesn't overlap with others */
	FVector GetRandomSpawnPosition(float Radius);

	/** Called when a helicopter is destroyed */
	UFUNCTION()
	void OnHeliDestroyed(AActor* DestroyedActor);

	/** Start the next wave timer */
	void ScheduleNextWave();

	/** Check if all helicopters from current wave are destroyed */
	void CheckWaveComplete();

	/** Whether spawner is waiting for external command to spawn */
	bool bWaitingForCommand = true;
};
