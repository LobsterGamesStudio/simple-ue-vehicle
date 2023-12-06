// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "M_StaticMeshVehicle.generated.h"



UCLASS()
class LOBSTERVH_API AM_StaticMeshVehicle : public APawn
{
	GENERATED_BODY()
public:
	// Mesh
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MainMesh")
	UStaticMeshComponent* StaticMeshC;
	// Wheels

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	UArrowComponent* ArrowC_FR;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	UArrowComponent* ArrowC_FL;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	UArrowComponent* ArrowC_RR;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	UArrowComponent* ArrowC_RL;

	//Wheels Attachment
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	USceneComponent* WheelSceneFR;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	USceneComponent* WheelSceneFL;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	USceneComponent* WheelSceneRR;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	USceneComponent* WheelSceneRL;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	UStaticMeshComponent* WheelMeshFR;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	UStaticMeshComponent* WheelMeshFL;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	UStaticMeshComponent* WheelMeshRR;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	UStaticMeshComponent* WheelMeshRL;

	//Camera & SpringArm
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArmComponent;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraC;



	//Exposed Variables
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float RestLength = 70;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float SpringTravelLength = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float WheelRadius = 34;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float SpringForceConst = 50000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float DamperForceConst = 5000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float ForwardForceConst = 100000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float MaxSteeringAngle = 30;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float FrictionConst = 500;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float BrakeConst = 1000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
	float VehicleWeight = 1100;

	// Sets default values for this pawn's properties
	AM_StaticMeshVehicle();



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;


protected:
	bool bBrakeApplied;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	//UFUNCTION(BlueprintCallable, Category = "Arcade System")
	//void MoveForward(float Value);
	//UFUNCTION(BlueprintCallable, Category = "Arcade System")
	//void Steer(float Value);
	UFUNCTION(BlueprintCallable, Category = "Static Mesh Vehicle")
	void BrakePressed();
	UFUNCTION(BlueprintCallable, Category = "Static Mesh Vehicle")
	void BrakeReleased();

	UFUNCTION(BlueprintCallable, Category = "Static Mesh Vehicle")
	void ClientTryMoveForward(float Value);


	UFUNCTION(BlueprintCallable, Category = "Static Mesh Vehicle")
	void ClientTrySteer(float Value);
	// Server RPC function declaration
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMoveForward(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSteer(float Value);

private:
	TArray<UArrowComponent*> WheelArrowComponentHolder;
	TArray<USceneComponent*> WheelSceneComponentHolder;
	float MinLength;
	float MaxLength;
	FCollisionQueryParams LineTraceCollisionQuery;
	float SpringLength[4] = { 0,0,0,0 };
	float ForwardAxisValue;
	float SteerAxisValue;
	float LeftAxisValue;
	bool bIsForwardInput;
	bool bIsRightInput;
	FVector worldLinearVelocity;
	FVector localLinearVelocity;


private:
	void UpdateVehicleForce(int WheelArrowIndex, float DeltaTime);
	void HandleSteeringReturnToCenter(float DeltaTime);
	void MoveForward(float Value);
	void Steer(float Value);
	void AnimateWheels(int WheelArrowIndex, float DeltaTime);
	//void BrakePressed();
	//void BrakeReleased();
};
