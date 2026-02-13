// Copyright Epic Games, Inc. All Rights Reserved.

#include "UFOAI.h"
#include "FighterPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AUFOAI::AUFOAI()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create explosion component for death effects
	ExplosionComp = CreateDefaultSubobject<UExplosionComponent>(TEXT("ExplosionComp"));

	// Create scene root component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = SceneRoot;

	// Create box collision component for reliable hit detection
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(SceneRoot);
	CollisionBox->SetBoxExtent(FVector(150.0f, 150.0f, 75.0f));
	CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionBox->SetGenerateOverlapEvents(true);
	CollisionBox->SetSimulatePhysics(false);
	CollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	// Create static mesh component
	UFOMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFOMesh"));
	UFOMesh->SetupAttachment(SceneRoot);

	// Disable collision on mesh - CollisionBox handles it
	UFOMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UFOMesh->SetSimulatePhysics(false);
}

void AUFOAI::BeginPlay()
{
	Super::BeginPlay();

	// Ensure root component is Movable
	if (SceneRoot && SceneRoot->Mobility != EComponentMobility::Movable)
	{
		SceneRoot->SetMobility(EComponentMobility::Movable);
	}

	// Calculate initial distance to base from spawn position
	FVector SpawnPos = GetActorLocation();
	CurrentDistanceToBase = FVector::Dist2D(SpawnPos, TargetLocation);

	UE_LOG(LogTemp, Log, TEXT("UFOAI: BeginPlay - Location=%s, DistToBase=%.0f"), *SpawnPos.ToString(), CurrentDistanceToBase);

	// Apply the rotation offset and scale to the mesh (visual only)
	if (UFOMesh)
	{
		FRotator RelativeRotation = UFOMesh->GetRelativeRotation();
		RelativeRotation.Yaw = MeshRotationOffset;
		UFOMesh->SetRelativeRotation(RelativeRotation);
		
		// Apply mesh scale
		UFOMesh->SetRelativeScale3D(MeshScale);
		UE_LOG(LogTemp, Log, TEXT("UFOAI: Applied mesh scale %s"), *MeshScale.ToString());
	}

	// Pick initial waypoint
	if (bTargetSet)
	{
		PickNewWaypoint();
	}
}

void AUFOAI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Spawn explosion effect when UFO is destroyed
	if (ExplosionComp && EndPlayReason == EEndPlayReason::Destroyed)
	{
		UE_LOG(LogTemp, Log, TEXT("UFOAI: Spawning explosion at owner location"));
		ExplosionComp->SpawnExplosionAtOwner();
	}

	Super::EndPlay(EndPlayReason);
}

void AUFOAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Don't update AI when game is paused
	if (IsGamePaused())
	{
		return;
	}

	if (!bTargetSet)
	{
		return;
	}

	if (bIsHovering)
	{
		// Hovering at waypoint - count down timer
		HoverTimer -= DeltaTime;
		if (HoverTimer <= 0.0f)
		{
			bIsHovering = false;
			PickNewWaypoint();
			UE_LOG(LogTemp, Log, TEXT("UFOAI: Hover complete, moving to new waypoint %s"), *CurrentWaypoint.ToString());
		}
	}
	else
	{
		// Flying toward waypoint
		MoveTowardWaypoint(DeltaTime);
		RotateTowardWaypoint(DeltaTime);

		// Check if we reached the waypoint
		float DistToWaypoint = FVector::Dist(GetActorLocation(), CurrentWaypoint);
		if (DistToWaypoint <= WaypointReachedThreshold)
		{
			bIsHovering = true;
			HoverTimer = FMath::FRandRange(MinHoverTime, MaxHoverTime);
			UE_LOG(LogTemp, Log, TEXT("UFOAI: Reached waypoint, hovering for %.1f seconds"), HoverTimer);
		}
	}

	// Fire at base only when at stopping distance (line of fire)
	if (bReachedLineOfFire && !bIsFiring)
	{
		bIsFiring = true;
		FireTimer = RateOfFire;
		UE_LOG(LogTemp, Log, TEXT("UFOAI: Reached line of fire! Starting to fire at base."));
	}

	if (bIsFiring)
	{
		FireTimer -= DeltaTime;
		if (FireTimer <= 0.0f)
		{
			FireAtBase();
			FireTimer = RateOfFire;
		}
	}
}

void AUFOAI::SetTargetLocation(const FVector& NewTarget)
{
	TargetLocation = NewTarget;
	bTargetSet = true;

	// Recalculate distance to base now that we have the correct target
	CurrentDistanceToBase = FVector::Dist2D(GetActorLocation(), TargetLocation);

	UE_LOG(LogTemp, Log, TEXT("UFOAI: Target set to %s, CurrentPos=%s, DistToBase=%.0f"), 
		*TargetLocation.ToString(), *GetActorLocation().ToString(), CurrentDistanceToBase);

	// Pick first waypoint now that we know where the base is
	PickNewWaypoint();
}

void AUFOAI::SetMoveSpeed(float NewSpeed)
{
	MoveSpeed = FMath::Max(0.0f, NewSpeed);
}

void AUFOAI::SetMeshRotation(float YawRotation)
{
	MeshRotationOffset = YawRotation;

	if (UFOMesh)
	{
		FRotator RelativeRotation = UFOMesh->GetRelativeRotation();
		RelativeRotation.Yaw = MeshRotationOffset;
		UFOMesh->SetRelativeRotation(RelativeRotation);
	}
}

void AUFOAI::SetMeshScale(const FVector& NewScale)
{
	MeshScale = NewScale;

	if (UFOMesh)
	{
		UFOMesh->SetRelativeScale3D(MeshScale);
	}
}

void AUFOAI::SetFlyHeight(float NewHeight)
{
	FlyHeight = NewHeight;

	FVector CurrentLocation = GetActorLocation();
	CurrentLocation.Z = FlyHeight;
	SetActorLocation(CurrentLocation);
}

void AUFOAI::SetRateOfFire(float Rate)
{
	RateOfFire = FMath::Max(0.1f, Rate);
}

void AUFOAI::SetApproachSettings(float InMinApproachIncrement, float InMaxApproachIncrement, float InMinLateralSpread, float InMaxLateralSpread, float InMinHoverTime, float InMaxHoverTime, float InLineOfFireDistance)
{
	MinApproachIncrement = InMinApproachIncrement;
	MaxApproachIncrement = InMaxApproachIncrement;
	MinLateralSpread = InMinLateralSpread;
	MaxLateralSpread = InMaxLateralSpread;
	MinHoverTime = InMinHoverTime;
	MaxHoverTime = InMaxHoverTime;
	LineOfFireDistance = InLineOfFireDistance;
}

void AUFOAI::PickNewWaypoint()
{
	// If already at stopping distance, stay there (pick nearby hover points)
	if (bReachedLineOfFire)
	{
		// Hover around the stopping distance with some lateral variation
		float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
		float AngleRad = FMath::DegreesToRadians(RandomAngle);
		float Radius = LineOfFireDistance + FMath::FRandRange(-100.0f, 100.0f);
		Radius = FMath::Max(Radius, 50.0f);

		CurrentWaypoint = FVector(
			TargetLocation.X + FMath::Cos(AngleRad) * Radius,
			TargetLocation.Y + FMath::Sin(AngleRad) * Radius,
			FlyHeight);

		UE_LOG(LogTemp, Log, TEXT("UFOAI: Hovering near base, waypoint at %s (radius=%.0f)"), *CurrentWaypoint.ToString(), Radius);
		return;
	}

	// Decrease distance to base by a random increment
	float ApproachAmount = FMath::FRandRange(MinApproachIncrement, MaxApproachIncrement);
	CurrentDistanceToBase = FMath::Max(CurrentDistanceToBase - ApproachAmount, LineOfFireDistance);

	// Check if we've reached the stopping distance
	if (CurrentDistanceToBase <= LineOfFireDistance)
	{
		bReachedLineOfFire = true;
		CurrentDistanceToBase = LineOfFireDistance;
		UE_LOG(LogTemp, Log, TEXT("UFOAI: Reached stopping distance (%.0f)"), LineOfFireDistance);
	}

	// Pick a random angle from the base, then apply lateral spread
	// Direction from base to current position
	FVector CurrentPos = GetActorLocation();
	FVector ToUFO = CurrentPos - TargetLocation;
	ToUFO.Z = 0.0f;
	float CurrentAngle = FMath::Atan2(ToUFO.Y, ToUFO.X);

	// Add random lateral offset angle based on spread
	float LateralOffset = FMath::FRandRange(MinLateralSpread, MaxLateralSpread);
	if (FMath::RandBool()) LateralOffset = -LateralOffset;
	float LateralAngleOffset = FMath::Atan2(LateralOffset, CurrentDistanceToBase);
	float NewAngle = CurrentAngle + LateralAngleOffset;

	float X = TargetLocation.X + FMath::Cos(NewAngle) * CurrentDistanceToBase;
	float Y = TargetLocation.Y + FMath::Sin(NewAngle) * CurrentDistanceToBase;

	CurrentWaypoint = FVector(X, Y, FlyHeight);

	UE_LOG(LogTemp, Log, TEXT("UFOAI: New waypoint at %s (distToBase=%.0f, lateralOffset=%.0f)"), *CurrentWaypoint.ToString(), CurrentDistanceToBase, LateralOffset);
}

void AUFOAI::MoveTowardWaypoint(float DeltaTime)
{
	FVector CurrentLocation = GetActorLocation();
	FVector Direction = (CurrentWaypoint - CurrentLocation).GetSafeNormal();

	FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
	NewLocation.Z = FlyHeight; // Maintain fly height

	SetActorLocation(NewLocation);
}

void AUFOAI::RotateTowardWaypoint(float DeltaTime)
{
	FVector CurrentLocation = GetActorLocation();
	FVector Direction = (CurrentWaypoint - CurrentLocation).GetSafeNormal();
	Direction.Z = 0.0f;

	if (Direction.IsNearlyZero())
	{
		return;
	}

	FRotator TargetRotation = Direction.Rotation();
	FRotator CurrentRotation = GetActorRotation();

	float NewYaw = FMath::FInterpTo(CurrentRotation.Yaw, TargetRotation.Yaw, DeltaTime, RotationSpeed);
	SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
}

void AUFOAI::FireAtBase()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (AFighterPawn* Fighter = Cast<AFighterPawn>(PlayerPawn))
	{
		Fighter->DamageBase(1);
	}
}

bool AUFOAI::IsGamePaused() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (AFighterPawn* Fighter = Cast<AFighterPawn>(PC->GetPawn()))
			{
				return Fighter->GetGameState() == EGameState::Paused;
			}
		}
	}
	return false;
}
