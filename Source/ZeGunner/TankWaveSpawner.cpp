// Copyright Epic Games, Inc. All Rights Reserved.

#include "TankWaveSpawner.h"
#include "TankAI.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

ATankWaveSpawner::ATankWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void ATankWaveSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("TankWaveSpawner: Base target is world origin (0,0,0). Waiting for command to spawn."));
}

void ATankWaveSpawner::TriggerNextWave()
{
	CurrentWave++;
	UE_LOG(LogTemp, Log, TEXT("TankWaveSpawner: TriggerNextWave -> Wave %d"), CurrentWave);
	SpawnWave();
}

void ATankWaveSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATankWaveSpawner::ScheduleNextWave()
{
	CurrentWave++;
	UE_LOG(LogTemp, Log, TEXT("TankWaveSpawner: Wave %d scheduled, spawning in %.1f seconds"), CurrentWave, WaveDelay);
	
	GetWorld()->GetTimerManager().SetTimer(WaveTimerHandle, this, &ATankWaveSpawner::SpawnWave, WaveDelay, false);
}

void ATankWaveSpawner::SpawnWave()
{
	if (!TankClass)
	{
		UE_LOG(LogTemp, Error, TEXT("TankWaveSpawner: No TankClass set! Cannot spawn wave."));
		return;
	}
	
	// Calculate tanks for this wave
	int32 TanksToSpawn = TanksPerWave + (CurrentWave - 1) * TanksAddedPerWave;
	
	// Calculate wave-scaled spawn radius
	float WaveSpawnRadius = FMath::Min(InitialSpawnRadius + (CurrentWave - 1) * SpawnRadiusWaveIncrement, MaxSpawnRadius);
	
	UE_LOG(LogTemp, Log, TEXT("TankWaveSpawner: Spawning wave %d with %d tanks at radius %.0f"), CurrentWave, TanksToSpawn, WaveSpawnRadius);
	
	UsedSpawnAngles.Empty();
	
	// Spawn all tanks for this wave simultaneously
	for (int32 i = 0; i < TanksToSpawn; i++)
	{
		FVector SpawnLocation = GetRandomSpawnPosition(WaveSpawnRadius);
		
		if (SpawnLocation.IsNearlyZero())
		{
			UE_LOG(LogTemp, Warning, TEXT("TankWaveSpawner: Could not find valid spawn position for tank %d"), i);
			continue;
		}
		
		// Calculate rotation to face the base at world origin
		FVector TargetLocation = FVector::ZeroVector;
		FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
		SpawnRotation.Pitch = 0.0f;
		SpawnRotation.Roll = 0.0f;
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		
		APawn* SpawnedTank = GetWorld()->SpawnActor<APawn>(TankClass, SpawnLocation, SpawnRotation, SpawnParams);
		
		if (SpawnedTank)
		{
			// Calculate wave-scaled speed range
			float WaveMinSpeed = FMath::Min(InitialMinSpeed + (CurrentWave - 1) * MinSpeedIncrementPerWave, MaxPossibleMinSpeed);
			float WaveMaxSpeed = FMath::Min(InitialMaxSpeed + (CurrentWave - 1) * MaxSpeedIncrementPerWave, MaxPossibleMaxSpeed);
			float RandomSpeed = FMath::FRandRange(WaveMinSpeed, WaveMaxSpeed);
			
			// If it's our TankAI class, set the target, speed, stopping distance, mesh rotation, and zigzag settings
			if (ATankAI* TankAI = Cast<ATankAI>(SpawnedTank))
			{
				TankAI->SetMoveSpeed(RandomSpeed);
				TankAI->SetStoppingDistance(LineOfFireDistance);
				TankAI->SetMeshRotation(MeshRotationOffset);
				TankAI->SetZigzagSettings(bUseZigzagMovement, ZigzagMinDistance, ZigzagMaxDistance, StraightLineDistance);
				TankAI->SetRateOfFire(RateOfFire);
				TankAI->SetTargetLocation(TargetLocation);
			}
			
			// Bind to destruction event
			SpawnedTank->OnDestroyed.AddDynamic(this, &ATankWaveSpawner::OnTankDestroyed);
			
			ActiveTankCount++;
			
			UE_LOG(LogTemp, Log, TEXT("TankWaveSpawner: Spawned tank %d/%d at %s with speed %.1f"), 
				i + 1, TanksToSpawn, *SpawnLocation.ToString(), RandomSpeed);
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("TankWaveSpawner: Wave %d complete. Active tanks: %d"), CurrentWave, ActiveTankCount);
}

FVector ATankWaveSpawner::GetRandomSpawnPosition(float Radius)
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
			
			return FVector(X, Y, SpawnHeightOffset);
		}
	}
	
	// Could not find valid position
	UE_LOG(LogTemp, Warning, TEXT("TankWaveSpawner: Could not find valid spawn position after %d attempts"), MaxAttempts);
	return FVector::ZeroVector;
}

void ATankWaveSpawner::OnTankDestroyed(AActor* DestroyedActor)
{
	ActiveTankCount--;
	UE_LOG(LogTemp, Log, TEXT("TankWaveSpawner: Tank destroyed. Active tanks remaining: %d"), ActiveTankCount);
	
	CheckWaveComplete();
}

void ATankWaveSpawner::CheckWaveComplete()
{
	if (ActiveTankCount <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("TankWaveSpawner: Wave %d complete! Waiting for next command."), CurrentWave);
	}
}
