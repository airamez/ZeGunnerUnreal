// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExplosionComponent.h"
#include "FighterPawn.h"
#include "HeliAI.generated.h"

UCLASS()
class ZEGUNNER_API AHeliAI : public APawn
{
	GENERATED_BODY()

public:
	/** Explosion component for death effects */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Helicopter")
	UExplosionComponent* ExplosionComp;

	AHeliAI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The static mesh component for the helicopter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Helicopter")
	class UStaticMeshComponent* HeliMesh;

	/** The root scene component (for actor rotation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Helicopter")
	class USceneComponent* SceneRoot;

	/** The target location to move toward (base/church position) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Movement")
	FVector TargetLocation;

	/** Movement speed in units per second */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Movement")
	float MoveSpeed = 300.0f;

	/** Distance to stop from target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Movement", meta = (ClampMin = "0.0"))
	float StoppingDistance = 100.0f;

	/** Rotation offset to fix helicopter model orientation (in degrees) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Movement")
	float MeshRotationOffset = 0.0f;

	/** Rotation speed when turning toward target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Movement", meta = (ClampMin = "0.0"))
	float RotationSpeed = 3.0f;

	/** Flying height for the helicopter */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Movement")
	float FlyHeight = 500.0f;

public:
	/** Set the target location for the helicopter to move toward */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetTargetLocation(const FVector& NewTarget);

	/** Set the movement speed */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetMoveSpeed(float NewSpeed);

	/** Set the stopping distance */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetStoppingDistance(float NewDistance);

	/** Set the mesh rotation offset */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetMeshRotation(float YawRotation);

	/** Set the flying height */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetFlyHeight(float NewHeight);

	/** Set the rate of fire (seconds between shots) */
	void SetRateOfFire(float Rate);

	/** Set lateral dancing parameters */
	void SetLateralDanceSettings(float DanceDistance, float MinSpeed, float MaxSpeed, float MinTime, float MaxTime);

	/** Get current move speed */
	UFUNCTION(BlueprintPure, Category = "Helicopter Movement")
	float GetMoveSpeed() const { return MoveSpeed; }

	/** Check if helicopter has reached the target */
	UFUNCTION(BlueprintPure, Category = "Helicopter Movement")
	bool HasReachedTarget() const;

private:
	/** Initial spawn position for reference */
	FVector InitialSpawnLocation;

	/** Has a target been explicitly set? */
	bool bTargetSet = false;

	/** Whether this heli has reached the line of fire and is shooting */
	bool bIsFiring = false;

	/** Seconds between shots at the base */
	float RateOfFire = 3.0f;

	/** Timer counting down to next shot */
	float FireTimer = 0.0f;

	// ==================== Lateral Dancing State ====================

	/** Distance from base where lateral dancing begins */
	float LateralDanceDistance = 1000.0f;

	/** Minimum lateral speed (units/sec) */
	float LateralMinSpeed = 100.0f;

	/** Maximum lateral speed (units/sec) */
	float LateralMaxSpeed = 400.0f;

	/** Minimum time moving in one lateral direction (seconds) */
	float LateralMinTime = 0.5f;

	/** Maximum time moving in one lateral direction (seconds) */
	float LateralMaxTime = 2.0f;

	/** Whether lateral dancing is currently active */
	bool bIsDancing = false;

	/** Current lateral direction: 1 = left, -1 = right */
	int32 LateralDirection = 1;

	/** Current lateral speed for this dance leg */
	float CurrentLateralSpeed = 0.0f;

	/** Timer counting down until next direction change */
	float LateralTimer = 0.0f;

	/** The lateral axis (perpendicular to approach direction) */
	FVector LateralAxis = FVector::ZeroVector;

	/** Move the helicopter toward target */
	void MoveTowardTarget(float DeltaTime);

	/** Perform lateral dancing movement */
	void UpdateLateralDance(float DeltaTime);

	/** Pick a new random lateral direction, speed, and duration */
	void PickNewLateralLeg();

	/** Smoothly rotate toward target */
	void RotateTowardTarget(float DeltaTime);

	/** Fire at the base (damage it) */
	void FireAtBase();

	/** Check if the game is currently paused */
	bool IsGamePaused() const;
};
