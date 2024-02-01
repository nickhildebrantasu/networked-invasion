// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

USTRUCT(BlueprintType)
struct FIKProperties {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UAnimSequence* AnimationPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AimOffset = 15.0f;

	UPROPERTY( EditAnywhere, BlueprintReadWrite )
	FTransform CustomOffsetTransform;
};

UCLASS(Abstract)
class CAPSTONE_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Components")
	class USceneComponent* Root;

	UPROPERTY( VisibleAnywhere, BlueprintReadWrite, Category = "Components" )
	class USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="State")
	class ACapstoneCharacter* CurrentOwner;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Configurations" )
	FIKProperties IKProperties;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Configurations" )
	FTransform PlacementTransform;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="IK")
	FTransform GetSightsWorldTransform() const;
	virtual FORCEINLINE	FTransform GetSightsWorldTransform_Implementation() const { return Mesh->GetSocketTransform( FName( "Aim" ) ); }
};
