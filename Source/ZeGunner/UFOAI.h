// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExplosionComponent.h"
#include "FighterPawn.h"
#include "UFOAI.generated.h"

UCLASS()
class ZEGUNNER_API AUFOAI : public APawn
{
	GENERATED_BODY()

public:
	/** Explosion component for death effects */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO")
	UExplosionComponent* ExplosionComp;

	AUFOAI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The static mesh component for the UFO */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UFO")
	class UStaticMeshComponent* UFOMesh;

	/** Box collision component for reliable hit detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO")
	class UBoxComponent* CollisionBox;

	/** The root scene component (for actor rotation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO")
	class USceneComponent* SceneRoot;

	/** The base/target location (world origin) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UFO Movement")
	FVector TargetLocation;

	/** Movement speed in units per second */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UFO Movement")
	float MoveSpeed = 400.0f;

	/** Rotation offset to fix UFO model orientation (in degrees) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UFO Movement")
	float MeshRotationOffset = 0.0f;

	/** Scale for the UFO mesh (default 1.0 = original size) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UFO Appearance", meta = (ClampMin = "0.1"))
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

	/** Rotation speed when turning toward waypoint */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UFO Movement", meta = (ClampMin = "0.0"))
	float RotationSpeed = 3.0f;

	/** Flying height for the UFO */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UFO Movement")
	float FlyHeight = 500.0f;

public:
	/** Set the base/target location */
	UFUNCTION(BlueprintCallable, Category = "UFO Movement")
	void SetTargetLocation(const FVector& NewTarget);

	/** Set the movement speed */
	UFUNCTION(BlueprintCallable, Category = "UFO Movement")
	void SetMoveSpeed(float NewSpeed);

	/** Set the mesh rotation offset */
	UFUNCTION(BlueprintCallable, Category = "UFO Movement")
	void SetMeshRotation(float YawRotation);

	/** Set the mesh scale */
	UFUNCTION(BlueprintCallable, Category = "UFO Appearance")
	void SetMeshScale(const FVector& NewScale);

	/** Set the flying height */
	UFUNCTION(BlueprintCallable, Category = "UFO Movement")
	void SetFlyHeight(float NewHeight);

	/** Set the rate of fire (seconds between shots) */
	void SetRateOfFire(float Rate);

	/** Set approach movement parameters */
	void SetApproachSettings(float InMinApproachIncrement, float InMaxApproachIncrement, float InMinLateralSpread, float InMaxLateralSpread, float InMinHoverTime, float InMaxHoverTime, float InLineOfFireDistance);

	/** Get current move speed */
	UFUNCTION(BlueprintPure, Category = "UFO Movement")
	float GetMoveSpeed() const { return MoveSpeed; }

private:
	/** Has a target been explicitly set? */
	bool bTargetSet = false;

	/** Whether the UFO is currently hovering at a waypoint */
	bool bIsHovering = false;

	/** Whether the UFO has reached its current waypoint */
	bool bReachedWaypoint = false;

	/** Whether this UFO is firing at the base */
	bool bIsFiring = false;

	/** Seconds between shots at the base */
	float RateOfFire = 3.0f;

	/** Timer counting down to next shot */
	float FireTimer = 0.0f;

	/** Current waypoint the UFO is flying toward */
	FVector CurrentWaypoint;

	/** Timer for hovering at waypoint */
	float HoverTimer = 0.0f;

	/** Current distance from the base (decreases each waypoint) */
	float CurrentDistanceToBase = 0.0f;

	/** Minimum distance to get closer to base per waypoint (units) */
	float MinApproachIncrement = 200.0f;

	/** Maximum distance to get closer to base per waypoint (units) */
	float MaxApproachIncrement = 500.0f;

	/** Minimum lateral spread when picking next waypoint (units, perpendicular offset) */
	float MinLateralSpread = 100.0f;

	/** Maximum lateral spread when picking next waypoint (units, perpendicular offset) */
	float MaxLateralSpread = 800.0f;

	/** Distance from base where UFO stops approaching and starts firing (line of fire) */
	float LineOfFireDistance = 500.0f;

	/** Whether the UFO has reached its line of fire */
	bool bReachedLineOfFire = false;

	/** Minimum time to hover at a waypoint (seconds) */
	float MinHoverTime = 1.0f;

	/** Maximum time to hover at a waypoint (seconds) */
	float MaxHoverTime = 3.0f;

	/** Distance threshold to consider waypoint reached */
	float WaypointReachedThreshold = 50.0f;

	/** Pick a new random waypoint near the base */
	void PickNewWaypoint();

	/** Move toward the current waypoint */
	void MoveTowardWaypoint(float DeltaTime);

	/** Smoothly rotate toward the current waypoint */
	void RotateTowardWaypoint(float DeltaTime);

	/** Fire at the base (damage it) */
	void FireAtBase();

	/** Check if the game is currently paused */
	bool IsGamePaused() const;
};
