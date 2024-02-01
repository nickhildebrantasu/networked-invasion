// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/CameraComponent.h"

#include "FluidAnimInstance.h"
#include "CapstoneCharacter.h"

UFluidAnimInstance::UFluidAnimInstance()
{

}

void UFluidAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

void UFluidAnimInstance::NativeUpdateAnimation( float deltaTime )
{
	Super::NativeUpdateAnimation( deltaTime );

	if ( !Character )
	{
		Character = Cast<ACapstoneCharacter>( TryGetPawnOwner() );
		if ( Character )
		{
			Mesh = Character->GetMesh();
			Character->CurrentWeaponChangeDelegate.AddDynamic( this, &UFluidAnimInstance::CurrentWeaponChanged );
			CurrentWeaponChanged( Character->CurrentWeapon, nullptr );
		}
		else return;
	}

	SetVariables( deltaTime );
	CalculateWeaponSway( deltaTime );
}

void UFluidAnimInstance::CurrentWeaponChanged( AWeapon* NewWeapon, const AWeapon* OldWeapon )
{
	Weapon = NewWeapon;
	if ( Weapon )
	{
		IKProperties = Weapon->IKProperties;
		GetWorld()->GetTimerManager().SetTimerForNextTick( this, &UFluidAnimInstance::SetIKTransforms );
	}
	else
	{
		
	}
}

void UFluidAnimInstance::SetVariables( const float deltaTime )
{
	CameraTransform = FTransform(Character->GetBaseAimRotation(), Character->GetCamera()->GetComponentLocation() );

	const FTransform& RootOffset = Mesh->GetSocketTransform( FName( "root" ), RTS_Component ).Inverse() * Mesh->GetSocketTransform( FName( "ik_hand_root" ) );
	RelativeCameraTransform = CameraTransform.GetRelativeTransform( RootOffset );
}

void UFluidAnimInstance::CalculateWeaponSway( const float deltaTime )
{

}

void UFluidAnimInstance::SetIKTransforms( )
{
	HandToSightsTransform = Weapon->GetSightsWorldTransform().GetRelativeTransform( Mesh->GetSocketTransform( FName( "weapon_r" ) ) );
}