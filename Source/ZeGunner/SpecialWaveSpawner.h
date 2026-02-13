// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UFOAI.h"
#include "SpecialWaveSpawner.generated.h"

UCLASS()
class ZEGUNNER_API ASpecialWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	ASpecialWaveSpawner();

	/** Manually trigger the next wave (called by FighterPawn) */
	void TriggerNextWave(int32 WaveNumber);

	/** Returns number of active special enemies */
	int32 GetActiveEnemyCount() const { return ActiveEnemyCount; }

	/** Returns how many special enemies will spawn in the given wave (0 if wave <= 5) */
	int32 GetNextWaveEnemyCount(int32 WaveNumber) const;

	/** Returns how many UFOs will spawn in the given wave */
	int32 GetNextWaveUFOCount(int32 WaveNumber) const;

protected:
	virtual void BeginPlay() override;

	// ==================== UFO Settings ====================

	/** The class to spawn for UFOs (should be a Blueprint based on UFOAI) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning")
	TSubclassOf<class AUFOAI> UFOClass;

	/** Number of UFOs to spawn in the first special wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning", meta = (ClampMin = "1"))
	int32 UFOsPerWave = 1;

	/** Additional UFOs added per subsequent wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning", meta = (ClampMin = "0"))
	int32 UFOsAddedPerWave = 1;

	/** Spawn radius for UFOs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning", meta = (ClampMin = "100.0"))
	float UFOSpawnRadius = 3000.0f;

	/** Minimum spawn height for UFOs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning", meta = (ClampMin = "0.0"))
	float UFOMinSpawnHeight = 400.0f;

	/** Maximum spawn height for UFOs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning", meta = (ClampMin = "0.0"))
	float UFOMaxSpawnHeight = 800.0f;

	/** UFO movement speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning", meta = (ClampMin = "0.0"))
	float UFOSpeed = 400.0f;

	/** Rotation offset to fix UFO model orientation (in degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning")
	float UFOMeshRotationOffset = 0.0f;

	/** Minimum distance the UFO gets closer to base per waypoint (units) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Approach Movement", meta = (ClampMin = "0.0"))
	float UFOMinApproachIncrement = 200.0f;

	/** Maximum distance the UFO gets closer to base per waypoint (units) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Approach Movement", meta = (ClampMin = "0.0"))
	float UFOMaxApproachIncrement = 500.0f;

	/** Minimum lateral spread when picking next waypoint (units, perpendicular offset) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Approach Movement", meta = (ClampMin = "0.0"))
	float UFOMinLateralSpread = 100.0f;

	/** Maximum lateral spread when picking next waypoint (units, perpendicular offset) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Approach Movement", meta = (ClampMin = "0.0"))
	float UFOMaxLateralSpread = 800.0f;

	/** Distance from base where UFO stops approaching and starts firing (line of fire) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Approach Movement", meta = (ClampMin = "0.0"))
	float UFOLineOfFireDistance = 500.0f;

	/** Minimum time UFO hovers at a waypoint before moving to next (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Approach Movement", meta = (ClampMin = "0.1"))
	float UFOMinHoverTime = 1.0f;

	/** Maximum time UFO hovers at a waypoint before moving to next (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Approach Movement", meta = (ClampMin = "0.1"))
	float UFOMaxHoverTime = 3.0f;

	/** Rate of fire for UFOs - seconds between shots at the base */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning", meta = (ClampMin = "0.1"))
	float UFORateOfFire = 4.0f;

	/** The wave number after which special enemies start spawning */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Spawning", meta = (ClampMin = "1"))
	int32 StartAfterWave = 5;

private:
	/** Number of active UFO enemies currently alive */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UFO Spawning", meta = (AllowPrivateAccess = "true"))
	int32 ActiveEnemyCount = 0;

	/** Calculate how many UFOs to spawn for a given wave */
	int32 GetUFOCountForWave(int32 WaveNumber) const;

	/** Spawn UFOs for the wave */
	void SpawnUFOs(int32 WaveNumber);

	/** Called when a special enemy is destroyed */
	UFUNCTION()
	void OnEnemyDestroyed(AActor* DestroyedActor);
};
