// Fill out your copyright notice in the Description page of Project Settings.


#include "M_StaticMeshVehicle.h"
#include "StaticMeshDescription.h"


// Sets default values
AM_StaticMeshVehicle::AM_StaticMeshVehicle()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//BodyMesh
	StaticMeshC = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleStaticMeshC"));
	RootComponent = StaticMeshC;

	//PawnSetup
	FRepMovement RepMovementData;
	RepMovementData.RotationQuantizationLevel = ERotatorQuantization::ShortComponents;
	SetReplicatedMovement(RepMovementData);

	StaticMeshC->SetCollisionObjectType(ECC_Pawn);
	StaticMeshC->SetAngularDamping(0.7);

	//Wheels
	ArrowC_FR = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_FR"));
	ArrowC_FR->SetupAttachment(StaticMeshC, FName("WheelFR"));
	ArrowC_FR->SetRelativeRotation(FRotator(-90, 0, 0));

	ArrowC_FL = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_FL"));
	ArrowC_FL->SetupAttachment(StaticMeshC, FName("WheelFL"));
	ArrowC_FL->SetRelativeRotation(FRotator(-90, 0, 0));

	ArrowC_RR = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_RR"));
	ArrowC_RR->SetupAttachment(StaticMeshC, FName("WheelRR"));
	ArrowC_RR->SetRelativeRotation(FRotator(-90, 0, 0));

	ArrowC_RL = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowC_RL"));
	ArrowC_RL->SetupAttachment(StaticMeshC, FName("WheelRL"));
	ArrowC_RL->SetRelativeRotation(FRotator(-90, 0, 0));

	//Here we setup replication for wheel angle, rotation is fine without it.
	WheelSceneFL = CreateDefaultSubobject<USceneComponent>(TEXT("WheelSceneFL"));
	WheelSceneFL->SetupAttachment(ArrowC_FL);
	WheelSceneFL->SetIsReplicated(true);
	WheelSceneFR = CreateDefaultSubobject<USceneComponent>(TEXT("WheelSceneFR"));
	WheelSceneFR->SetupAttachment(ArrowC_FR);
	WheelSceneFR->SetIsReplicated(true);

	WheelSceneRR = CreateDefaultSubobject<USceneComponent>(TEXT("WheelSceneRR"));
	WheelSceneRR->SetupAttachment(ArrowC_RR);
	WheelSceneRL = CreateDefaultSubobject<USceneComponent>(TEXT("WheelSceneRL"));
	WheelSceneRL->SetupAttachment(ArrowC_RL);

	WheelMeshFR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMeshFR"));
	WheelMeshFR->SetupAttachment(WheelSceneFR);

	WheelMeshFL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMeshFL"));
	WheelMeshFL->SetupAttachment(WheelSceneFL);

	WheelMeshRR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMeshRR"));
	WheelMeshRR->SetupAttachment(WheelSceneRR);

	WheelMeshRL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMeshRL"));
	WheelMeshRL->SetupAttachment(WheelSceneRL);

	//SpringArm
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(StaticMeshC);
	SpringArmComponent->TargetArmLength = 600;
	SpringArmComponent->SetRelativeRotation(FRotator(-10, 0, 0));
	SpringArmComponent->SetRelativeLocation(FVector(0, 0, 130));
	//ArmCamera
	CameraC = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraC"));
	CameraC->SetupAttachment(SpringArmComponent);



}

// Called when the game starts or when spawned
void AM_StaticMeshVehicle::BeginPlay()
{
	Super::BeginPlay();
	StaticMeshC->SetSimulatePhysics(true);
	StaticMeshC->SetMassOverrideInKg(NAME_None, VehicleWeight);

}

void AM_StaticMeshVehicle::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	WheelArrowComponentHolder = { ArrowC_FL, ArrowC_FR, ArrowC_RL, ArrowC_RR };
	WheelSceneComponentHolder = { WheelSceneFL, WheelSceneFR, WheelSceneRL, WheelSceneRR };
	MinLength = RestLength - SpringTravelLength;
	MaxLength = RestLength + SpringTravelLength;

	//const FName TraceTag("MyTraceTag");
	//LineTraceCollisionQuery.TraceTag = TraceTag;
	//LineTraceCollisionQuery.bDebugQuery = true;
	LineTraceCollisionQuery.AddIgnoredActor(this);

}
// Called every frame
void AM_StaticMeshVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//UE_LOG(LogTemp, Warning, TEXT("Forward Axis Value: %f"), ForwardAxisValue);
	//UE_LOG(LogTemp, Warning, TEXT("Right Axis Value: %f"), SteerAxisValue);

	bIsForwardInput = !FMath::IsNearlyZero(ForwardAxisValue, KINDA_SMALL_NUMBER);
	bIsRightInput = !FMath::IsNearlyZero(SteerAxisValue, KINDA_SMALL_NUMBER);


	worldLinearVelocity = StaticMeshC->GetPhysicsLinearVelocity();
	localLinearVelocity = UKismetMathLibrary::InverseTransformDirection(StaticMeshC->GetComponentTransform(), worldLinearVelocity);
	/*GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Velocity Y: ") + localLinearVelocity.ToString());*/

	for (int WheelArrowIndex = 0; WheelArrowIndex < 4; WheelArrowIndex++)
	{
		if (HasAuthority())
			UpdateVehicleForce(WheelArrowIndex, DeltaTime);
		AnimateWheels(WheelArrowIndex, DeltaTime);
	}
	HandleSteeringReturnToCenter(DeltaTime);
}



void AM_StaticMeshVehicle::UpdateVehicleForce(int WheelArrowIndex, float DeltaTime)
{


	if (!WheelArrowComponentHolder.IsValidIndex(WheelArrowIndex))
		return;

	float CurrentSteeringAngle = UKismetMathLibrary::MapRangeClamped(SteerAxisValue, -1, 1, MaxSteeringAngle * -1, MaxSteeringAngle);
	if (WheelArrowIndex < 2)
	{
		WheelSceneComponentHolder[WheelArrowIndex]->SetRelativeRotation(FRotator(0, 0, CurrentSteeringAngle));
	}


	if (!bIsForwardInput && !bIsRightInput && (localLinearVelocity.SizeSquared() < 120))
	{
		StaticMeshC->SetLinearDamping(15);
	}
	else {
		StaticMeshC->SetLinearDamping(0.01);
	}




	FVector frictionVector = FVector::ZeroVector;
	if (UKismetMathLibrary::Abs(localLinearVelocity.Y) > 2)
		frictionVector = StaticMeshC->GetRightVector() * localLinearVelocity.Y * FrictionConst * -1;


	FVector netForce = GetActorForwardVector() * ForwardAxisValue * ForwardForceConst + frictionVector;

	if (bBrakeApplied)
	{
		netForce = worldLinearVelocity * -1 * BrakeConst;

	}

	if (localLinearVelocity.SizeSquared() > 120)
	{
		if (localLinearVelocity.X > 0)
		{
			FVector LocalPosition = FVector(-140, 0, 0); // Replace with your values
			FVector WorldPosition = StaticMeshC->GetComponentLocation() + StaticMeshC->GetUpVector() * LocalPosition.Z + StaticMeshC->GetRightVector() * LocalPosition.Y + StaticMeshC->GetForwardVector() * LocalPosition.X;

			StaticMeshC->AddTorqueInDegrees(FVector(0, 0, CurrentSteeringAngle), NAME_None, true);
			StaticMeshC->AddForceAtLocation(FVector(CurrentSteeringAngle * 50000, 0, 0), WorldPosition);

		}
		else
			StaticMeshC->AddTorqueInDegrees(FVector(0, 0, CurrentSteeringAngle * -1), NAME_None, true);
	}


	StaticMeshC->AddForce(netForce);




}


void AM_StaticMeshVehicle::AnimateWheels(int WheelArrowIndex, float DeltaTime)
{
	if (!WheelArrowComponentHolder.IsValidIndex(WheelArrowIndex))
		return;



	auto wheelArrowC = WheelArrowComponentHolder[WheelArrowIndex];
	FHitResult outHit;
	FVector startTraceLoc = wheelArrowC->GetComponentLocation();
	FVector endTraceLoc = wheelArrowC->GetForwardVector() * (MaxLength + WheelRadius) + startTraceLoc;

	GetWorld()->LineTraceSingleByChannel(outHit, startTraceLoc, endTraceLoc, ECC_Camera, LineTraceCollisionQuery, FCollisionResponseParams());
	float previousSpringLength = SpringLength[WheelArrowIndex];

	if (outHit.IsValidBlockingHit())
	{
		float currentSpringLength = outHit.Distance - WheelRadius;
		SpringLength[WheelArrowIndex] = UKismetMathLibrary::FClamp(currentSpringLength, MinLength, MaxLength);
		float springVelocity = (previousSpringLength - SpringLength[WheelArrowIndex]) / DeltaTime;
		float springForce = (RestLength - SpringLength[WheelArrowIndex]) * SpringForceConst;
		float damperForce = springVelocity * DamperForceConst;
		FVector upwardForce = GetActorUpVector() * (springForce + damperForce);
		StaticMeshC->AddForceAtLocation(upwardForce, wheelArrowC->GetComponentLocation());
	}
	else
	{
		SpringLength[WheelArrowIndex] = MaxLength;
	}

	WheelSceneComponentHolder[WheelArrowIndex]->SetRelativeLocation(FVector(SpringLength[WheelArrowIndex], 0, 0));
	if (WheelArrowIndex % 2 == 0)
	{
		WheelSceneComponentHolder[WheelArrowIndex]->GetChildComponent(0)->AddLocalRotation(FRotator((360 * localLinearVelocity.X * DeltaTime) / (2 * 3.14 * WheelRadius), 0, 0));
	}
	else {
		WheelSceneComponentHolder[WheelArrowIndex]->GetChildComponent(0)->AddLocalRotation(FRotator((-1 * 360 * localLinearVelocity.X * DeltaTime) / (2 * 3.14 * WheelRadius), 0, 0));
	}
}
void AM_StaticMeshVehicle::MoveForward(float Value)
{
	ForwardAxisValue = Value;
}

void AM_StaticMeshVehicle::HandleSteeringReturnToCenter(float DeltaTime)
{
	float ReturnToCenterSpeed = 5.0f; // Speed at which the wheel returns to center
	float MinSpeedToSteer = 3.0f; // Minimum speed at which the steering is active
	float VelocityKMH = StaticMeshC->GetPhysicsLinearVelocity().Size() * 0.036;

	// If there's no input (SteerAxisValue is near zero) and the vehicle is moving
	if (FMath::IsNearlyZero(SteerAxisValue, KINDA_SMALL_NUMBER) && VelocityKMH > MinSpeedToSteer)
	{
		// Do nothing, maintain steering direction at high speeds with no input
	}
	else
	{
		// If there's steering input or the vehicle is moving slowly, interpolate SteerAxisValue towards zero
		SteerAxisValue = UKismetMathLibrary::FInterpTo(SteerAxisValue, 0.0f, DeltaTime, ReturnToCenterSpeed);
	}
}

void AM_StaticMeshVehicle::Steer(float Value)
{
	if (StaticMeshC->GetPhysicsLinearVelocity().Size() * 0.036 > 3)
		SteerAxisValue = UKismetMathLibrary::FInterpTo(SteerAxisValue, Value, GetWorld()->GetDeltaSeconds(), 5);
}

void AM_StaticMeshVehicle::BrakePressed()
{
	bBrakeApplied = true;
}

void AM_StaticMeshVehicle::BrakeReleased()
{
	bBrakeApplied = false;
}
// Multiplayer
// Throttle
void AM_StaticMeshVehicle::ClientTryMoveForward(float Value)
{
	if (IsLocallyControlled())
	{
		// Call the Server RPC function
		ServerMoveForward(Value);
	}
}

void AM_StaticMeshVehicle::ServerMoveForward_Implementation(float Value)
{
	if (HasAuthority())
	{
		MoveForward(Value);
	}
}

bool AM_StaticMeshVehicle::ServerMoveForward_Validate(float Value)
{
	// Here you can validate the input value if necessary
	return true; // The value is always valid
}

//Steering

void AM_StaticMeshVehicle::ClientTrySteer(float Value)
{
	if (IsLocallyControlled())
		//Steer(Value);
		ServerSteer(Value);
}

void AM_StaticMeshVehicle::ServerSteer_Implementation(float Value)
{
	if (HasAuthority())
	{
		Steer(Value);
	}
}

bool AM_StaticMeshVehicle::ServerSteer_Validate(float Value)
{
	return true;
}