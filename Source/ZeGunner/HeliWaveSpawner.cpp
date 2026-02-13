// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeliWaveSpawner.h"
#include "HeliAI.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AHeliWaveSpawner::AHeliWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void AHeliWaveSpawner::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Base target is world origin (0,0,0). Waiting for command to spawn."));
}

void AHeliWaveSpawner::TriggerNextWave()
{
	CurrentWave++;
	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: TriggerNextWave -> Wave %d"), CurrentWave);
	SpawnWave();
}

void AHeliWaveSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHeliWaveSpawner::ScheduleNextWave()
{
	CurrentWave++;
	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Wave %d scheduled, spawning in %.1f seconds"), CurrentWave, WaveDelay);

	GetWorld()->GetTimerManager().SetTimer(WaveTimerHandle, this, &AHeliWaveSpawner::SpawnWave, WaveDelay, false);
}

void AHeliWaveSpawner::SpawnWave()
{
	if (!HeliClass)
	{
		UE_LOG(LogTemp, Error, TEXT("HeliWaveSpawner: No HeliClass set! Cannot spawn wave."));
		return;
	}

	// Calculate helicopters for this wave
	int32 HelisToSpawn = HelisPerWave + (CurrentWave - 1) * HelisAddedPerWave;

	// Calculate wave-scaled spawn radius
	float WaveSpawnRadius = FMath::Min(InitialSpawnRadius + (CurrentWave - 1) * SpawnRadiusWaveIncrement, MaxSpawnRadius);

	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Spawning wave %d with %d helicopters at radius %.0f"), CurrentWave, HelisToSpawn, WaveSpawnRadius);

	UsedSpawnAngles.Empty();

	// Spawn all helicopters for this wave simultaneously
	for (int32 i = 0; i < HelisToSpawn; i++)
	{
		FVector SpawnLocation = GetRandomSpawnPosition(WaveSpawnRadius);

		if (SpawnLocation.IsNearlyZero())
		{
			UE_LOG(LogTemp, Warning, TEXT("HeliWaveSpawner: Could not find valid spawn position for helicopter %d"), i);
			continue;
		}

		// Calculate rotation to face the base at world origin
		FVector TargetLocation = FVector::ZeroVector;
		FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
		SpawnRotation.Pitch = 0.0f;
		SpawnRotation.Roll = 0.0f;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APawn* SpawnedHeli = GetWorld()->SpawnActor<APawn>(HeliClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (SpawnedHeli)
		{
			// Calculate wave-scaled speed range
			float WaveMinSpeed = FMath::Min(InitialMinSpeed + (CurrentWave - 1) * MinSpeedIncrementPerWave, MaxPossibleMinSpeed);
			float WaveMaxSpeed = FMath::Min(InitialMaxSpeed + (CurrentWave - 1) * MaxSpeedIncrementPerWave, MaxPossibleMaxSpeed);
			float RandomSpeed = FMath::FRandRange(WaveMinSpeed, WaveMaxSpeed);

			// If it's our HeliAI class, set the target, speed, stopping distance, mesh rotation, fly height, and lateral dancing
			if (AHeliAI* HeliAI = Cast<AHeliAI>(SpawnedHeli))
			{
				HeliAI->SetMoveSpeed(RandomSpeed);
				HeliAI->SetStoppingDistance(LineOfFireDistance);
				HeliAI->SetMeshRotation(MeshRotationOffset);
				HeliAI->SetFlyHeight(SpawnLocation.Z); // Use the spawned height
				HeliAI->SetRateOfFire(RateOfFire);
				HeliAI->SetLateralDanceSettings(LateralDanceDistance, MinLateralSpeed, MaxLateralSpeed, MinLateralTime, MaxLateralTime);
				HeliAI->SetTargetLocation(TargetLocation);
			}

			// Bind to destruction event
			SpawnedHeli->OnDestroyed.AddDynamic(this, &AHeliWaveSpawner::OnHeliDestroyed);

			ActiveHeliCount++;

			UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Spawned helicopter %d/%d at %s with speed %.1f"),
				i + 1, HelisToSpawn, *SpawnLocation.ToString(), RandomSpeed);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Wave %d complete. Active helicopters: %d"), CurrentWave, ActiveHeliCount);
}

FVector AHeliWaveSpawner::GetRandomSpawnPosition(float Radius)
{
	// Maximum attempts to find a valid position
	const int32 MaxAttempts = 50;
	const float AngleSeparationRad = FMath::DegreesToRadians(MinSpawnSeparation / Radius);

	for (int32 Attempt = 0; Attempt < MaxAttempts; Attempt++)
	{
		// Random angle between 0 and 360 degrees
		float RandomAngleDegrees = FMath::FRandRange(0.0f, 360.0f);
		float RandomAngleRad = FMath::DegreesToRadians(RandomAngleDegrees);

		// Check if this angle is far enough from all used angles
		bool bAngleValid = true;
		for (float UsedAngle : UsedSpawnAngles)
		{
			float AngleDiff = FMath::Abs(RandomAngleDegrees - UsedAngle);
			// Handle wrap-around at 360 degrees
			if (AngleDiff > 180.0f)
				AngleDiff = 360.0f - AngleDiff;

			float UsedAngleRad = FMath::DegreesToRadians(AngleDiff);
			if (UsedAngleRad < AngleSeparationRad)
			{
				bAngleValid = false;
				break;
			}
		}

		if (bAngleValid)
		{
			UsedSpawnAngles.Add(RandomAngleDegrees);

			// Convert polar coordinates to Cartesian
			float X = FMath::Cos(RandomAngleRad) * Radius;
			float Y = FMath::Sin(RandomAngleRad) * Radius;

			// Random height between MinSpawnHeight and MaxSpawnHeight
			float RandomHeight = FMath::FRandRange(MinSpawnHeight, MaxSpawnHeight);

			return FVector(X, Y, RandomHeight);
		}
	}

	// Could not find valid position
	UE_LOG(LogTemp, Warning, TEXT("HeliWaveSpawner: Could not find valid spawn position after %d attempts"), MaxAttempts);
	return FVector::ZeroVector;
}

void AHeliWaveSpawner::OnHeliDestroyed(AActor* DestroyedActor)
{
	ActiveHeliCount--;
	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Helicopter destroyed. Active helicopters remaining: %d"), ActiveHeliCount);

	CheckWaveComplete();
}

void AHeliWaveSpawner::CheckWaveComplete()
{
	if (ActiveHeliCount <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Wave %d complete! Waiting for next command."), CurrentWave);
	}
}
