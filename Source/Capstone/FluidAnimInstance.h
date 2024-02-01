// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Weapon.h"
#include "FluidAnimInstance.generated.h"

UCLASS()
class CAPSTONE_API UFluidAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFluidAnimInstance();

protected:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation( float DeltaTime ) override;

	UFUNCTION()
	virtual void CurrentWeaponChanged( class AWeapon* NewWeapon, const class AWeapon* OldWeapon );

	virtual void SetVariables( const float deltaTime );
	virtual void CalculateWeaponSway( const float deltaTime );

	virtual void SetIKTransforms();

public:
	/// *****************************
	/// Character References
	/// *****************************
	UPROPERTY(BlueprintReadWrite, Category="Animation")
	class ACapstoneCharacter* Character;

	UPROPERTY(BlueprintReadWrite, Category="Animation")
	class USkeletalMeshComponent* Mesh;

	UPROPERTY( BlueprintReadWrite, Category = "Animation" )
	class AWeapon* Weapon;

	/// *****************************
	/// IK Variables
	/// *****************************
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	FIKProperties IKProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	FTransform CameraTransform;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Animation" )
	FTransform RelativeCameraTransform;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Animation" )
	FTransform HandToSightsTransform;
};
