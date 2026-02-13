// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpecialWaveSpawner.h"
#include "UFOAI.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

ASpecialWaveSpawner::ASpecialWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void ASpecialWaveSpawner::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("SpecialWaveSpawner: Initialized. Special enemies start after wave %d."), StartAfterWave);
}

void ASpecialWaveSpawner::TriggerNextWave(int32 WaveNumber)
{
	if (WaveNumber <= StartAfterWave)
	{
		UE_LOG(LogTemp, Log, TEXT("SpecialWaveSpawner: Wave %d <= %d, no special enemies."), WaveNumber, StartAfterWave);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("SpecialWaveSpawner: Spawning UFOs for wave %d"), WaveNumber);

	SpawnUFOs(WaveNumber);
}

int32 ASpecialWaveSpawner::GetUFOCountForWave(int32 WaveNumber) const
{
	if (WaveNumber <= StartAfterWave)
	{
		return 0;
	}
	int32 WavesSinceStart = WaveNumber - StartAfterWave;
	return UFOsPerWave + (WavesSinceStart - 1) * UFOsAddedPerWave;
}

int32 ASpecialWaveSpawner::GetNextWaveEnemyCount(int32 WaveNumber) const
{
	return GetUFOCountForWave(WaveNumber);
}

int32 ASpecialWaveSpawner::GetNextWaveUFOCount(int32 WaveNumber) const
{
	return GetUFOCountForWave(WaveNumber);
}

void ASpecialWaveSpawner::SpawnUFOs(int32 WaveNumber)
{
	if (!UFOClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpecialWaveSpawner: No UFOClass set! Cannot spawn UFOs."));
		return;
	}

	int32 UFOCount = GetUFOCountForWave(WaveNumber);
	for (int32 i = 0; i < UFOCount; i++)
	{
		// Random angle for spawn position
		float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
		float AngleRad = FMath::DegreesToRadians(RandomAngle);

		float RandomHeight = FMath::FRandRange(UFOMinSpawnHeight, UFOMaxSpawnHeight);

		float X = FMath::Cos(AngleRad) * UFOSpawnRadius;
		float Y = FMath::Sin(AngleRad) * UFOSpawnRadius;
		FVector SpawnLocation(X, Y, RandomHeight);

		// Face toward the base
		FVector TargetLocation = FVector::ZeroVector;
		FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
		SpawnRotation.Pitch = 0.0f;
		SpawnRotation.Roll = 0.0f;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APawn* SpawnedUFO = GetWorld()->SpawnActor<APawn>(UFOClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (SpawnedUFO)
		{
			if (AUFOAI* UFO = Cast<AUFOAI>(SpawnedUFO))
			{
				UFO->SetMoveSpeed(UFOSpeed);
				UFO->SetMeshRotation(UFOMeshRotationOffset);
				UFO->SetFlyHeight(RandomHeight);
				UFO->SetRateOfFire(UFORateOfFire);
				UFO->SetApproachSettings(UFOMinApproachIncrement, UFOMaxApproachIncrement, UFOMinLateralSpread, UFOMaxLateralSpread, UFOMinHoverTime, UFOMaxHoverTime, UFOLineOfFireDistance);
				UFO->SetTargetLocation(TargetLocation);
			}

			SpawnedUFO->OnDestroyed.AddDynamic(this, &ASpecialWaveSpawner::OnEnemyDestroyed);
			ActiveEnemyCount++;

			UE_LOG(LogTemp, Log, TEXT("SpecialWaveSpawner: Spawned UFO %d/%d at %s"),
				i + 1, UFOCount, *SpawnLocation.ToString());
		}
	}
}

void ASpecialWaveSpawner::OnEnemyDestroyed(AActor* DestroyedActor)
{
	ActiveEnemyCount--;
	UE_LOG(LogTemp, Log, TEXT("SpecialWaveSpawner: Special enemy destroyed. Active remaining: %d"), ActiveEnemyCount);
}
