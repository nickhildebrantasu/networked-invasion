// Copyright Epic Games, Inc. All Rights Reserved.

#include "CapstoneCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

#include "Weapon.h"
#include "NetworkProjectile.h"

DEFINE_LOG_CATEGORY( LogTemplateCharacter );

//////////////////////////////////////////////////////////////////////////
// ACapstoneCharacter

ACapstoneCharacter::ACapstoneCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize( 42.f, 96.0f );

	// The controller only rotates the yaw
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character does not rotate to movement
	GetCharacterMovement()->RotationRate = FRotator( 0.0f, 500.0f, 0.0f ); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>( TEXT( "CameraBoom" ) );
	CameraBoom->SetupAttachment( RootComponent );
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>( TEXT( "FollowCamera" ) );
	FollowCamera->SetupAttachment( CameraBoom, USpringArmComponent::SocketName ); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = true; // Camera does not rotate relative to arm

	FPSpring = CreateDefaultSubobject<USpringArmComponent>( TEXT( "FPSpring" ) );
	FPSpring->SetupAttachment( GetMesh(), FName( "Head" ) );
	FPSpring->TargetArmLength = 0.0f;
	FPSpring->bUsePawnControlRotation = true;

	FPSCamera = CreateDefaultSubobject<UCameraComponent>( TEXT( "FPSCamera" ) );
	FPSCamera->SetupAttachment( FPSpring );
	FPSCamera->bUsePawnControlRotation = true;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	//Initialize the player's Health
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	//Initialize projectile class
	ProjectileClass = ANetworkProjectile::StaticClass();
	//Initialize fire rate
	FireRate = 0.25f;
	bIsFiringWeapon = false;
}

void ACapstoneCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	bUseControllerRotationYaw = true;

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if ( HasAuthority() )
	{
		for ( const TSubclassOf<AWeapon>& WeaponClass : DefaultWeapons )
		{
			if ( !WeaponClass ) continue;

			FActorSpawnParameters Params;
			Params.Owner = this;

			AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>( WeaponClass, Params );
			const int32 index = Weapons.Add( SpawnedWeapon );
			if ( index == CurrentIndex )
			{
				CurrentWeapon = SpawnedWeapon;
				OnRep_CurrentWeapon( nullptr );
			}
		}
	}
}

// For the character networking
void ACapstoneCharacter::GetLifetimeReplicatedProps( TArray <FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	//Replicate current health.
	DOREPLIFETIME( ACapstoneCharacter, CurrentHealth);
	DOREPLIFETIME_CONDITION( ACapstoneCharacter, Weapons, COND_None );
	DOREPLIFETIME_CONDITION( ACapstoneCharacter, CurrentWeapon, COND_None );
	DOREPLIFETIME( ACapstoneCharacter, bIsRagdoll );
}

void ACapstoneCharacter::OnRep_CurrentWeapon( const AWeapon* OldWeapon )
{
	if ( CurrentWeapon )
	{
		if ( !CurrentWeapon->CurrentOwner )
		{
			const FTransform& PlacementTransform = CurrentWeapon->PlacementTransform * GetMesh()->GetSocketTransform( FName( "weapon_r" ) );
			CurrentWeapon->SetActorTransform( PlacementTransform, false, nullptr, ETeleportType::TeleportPhysics);
			CurrentWeapon->AttachToComponent( GetMesh(), FAttachmentTransformRules::KeepWorldTransform, FName( "weapon_r" ) );
			CurrentWeapon->CurrentOwner = this;
		}

		CurrentWeapon->Mesh->SetVisibility( true );
	}

	if ( OldWeapon )
	{
		OldWeapon->Mesh->SetVisibility( false );
	}

	CurrentWeaponChangeDelegate.Broadcast( CurrentWeapon, OldWeapon );
}

/// Character health and network interactions
void ACapstoneCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void ACapstoneCharacter::SetCurrentHealth( float healthValue )
{
	if ( GetLocalRole() == ROLE_Authority )
	{
		CurrentHealth = FMath::Clamp( healthValue, 0.f, MaxHealth );
		OnHealthUpdate();
	}
}

float ACapstoneCharacter::TakeDamage( float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser )
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth( damageApplied );
	return damageApplied;
}

void ACapstoneCharacter::OnHealthUpdate()
{
	//Client-specific functionality
	if ( IsLocallyControlled() )
	{
		/*FString healthMessage = FString::Printf( TEXT( "You now have %f health remaining." ), CurrentHealth );
		GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Blue, healthMessage );*/

		if ( CurrentHealth <= 0 )
		{
			FString deathMessage = FString::Printf( TEXT( "%s has been killed." ), *GetFName().ToString() );
			GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Red, deathMessage );
			OnRep_Ragdoll();
			GetMesh()->SetSimulatePhysics( bIsRagdoll );
		}
	}

	//Server-specific functionality
	if ( GetLocalRole() == ROLE_Authority )
	{
		/*FString healthMessage = FString::Printf( TEXT( "%s now has %f health remaining." ), *GetFName().ToString(), CurrentHealth );
		GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Blue, healthMessage );*/
	}

	//Functions that occur on all machines.
	/*
		Any special functionality that should occur as a result of damage or death should be placed here.
	*/
}

void ACapstoneCharacter::OnRep_Ragdoll()
{
	bIsRagdoll = true;
	GetMesh()->SetSimulatePhysics( true );
}

//////////////////////////////////////////////////////////////////////////
// Input

void ACapstoneCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Changing the camera view
		EnhancedInputComponent->BindAction( SelectAction, ETriggerEvent::Triggered, this, &ACapstoneCharacter::SwitchCameras );
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACapstoneCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACapstoneCharacter::Look);

		// Weapon swapping
		EnhancedInputComponent->BindAction( NextToolAction, ETriggerEvent::Triggered, this, &ACapstoneCharacter::NextTool );
		EnhancedInputComponent->BindAction( PrevToolAction, ETriggerEvent::Triggered, this, &ACapstoneCharacter::PrevTool );

		// Shooting
		EnhancedInputComponent->BindAction( FireAction, ETriggerEvent::Started, this, &ACapstoneCharacter::StartFire );
		EnhancedInputComponent->BindAction( FireAction, ETriggerEvent::Completed, this, &ACapstoneCharacter::StopFire );
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACapstoneCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr )
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput( ForwardDirection, MovementVector.Y );
		//if( FollowCamera->IsActive()) AddMovementInput( RightDirection, MovementVector.X );
		AddMovementInput( RightDirection, MovementVector.X );
	}
}

void ACapstoneCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// enables head rotation animation
	yaw += LookAxisVector.X;
	pitch += LookAxisVector.Y;

	yaw = FMath::Clamp( yaw, -10, 10 ); // left to right head movement
	pitch = FMath::Clamp( pitch, -8, 10 ); // up and down head movement

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ACapstoneCharacter::NextTool()
{
	const int32 index = Weapons.IsValidIndex( CurrentIndex + 1 ) ? CurrentIndex + 1 : 0;
	Equip( index );
}

void ACapstoneCharacter::PrevTool()
{
	const int32 index = Weapons.IsValidIndex( CurrentIndex - 1 ) ? CurrentIndex - 1 : Weapons.Num() - 1;
	Equip( index );
}

void ACapstoneCharacter::Equip( const int32 index )
{
	if ( !Weapons.IsValidIndex( index ) || CurrentWeapon == Weapons[index] ) return;

	if ( IsLocallyControlled() )
	{
		CurrentIndex = index;
		const AWeapon* OldWeapon = CurrentWeapon;
		CurrentWeapon = Weapons[index];
		OnRep_CurrentWeapon( OldWeapon );
	}
	else if ( !HasAuthority() )
	{
		Server_SetWeapon( Weapons[index] );
	}
}

void ACapstoneCharacter::Server_SetWeapon_Implementation( AWeapon* newWeapon )
{
	const AWeapon* OldWeapon = CurrentWeapon;
	CurrentWeapon = newWeapon;
	OnRep_CurrentWeapon( OldWeapon );
}

void ACapstoneCharacter::StartFire( const FInputActionValue& Value )
{
	const bool CurrentValue = Value.Get<bool>();

	if ( CurrentValue ) { UE_LOG( LogTemp, Warning, TEXT( "Left Mouse Clicked" ) ); }

	if ( !bIsFiringWeapon )
	{
		bIsFiringWeapon = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().SetTimer( FiringTimer, this, &ACapstoneCharacter::StopFire, FireRate, false );
		HandleFire();
	}
}

void ACapstoneCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void ACapstoneCharacter::HandleFire_Implementation()
{
	FVector spawnLocation = GetActorLocation() + ( GetActorRotation().Vector() * 100.0f ) + ( GetActorUpVector() * 50.0f );
	FRotator spawnRotation = GetActorRotation();

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	ANetworkProjectile* spawnedProjectile = GetWorld()->SpawnActor<ANetworkProjectile>( spawnLocation, spawnRotation, spawnParameters );
}

UCameraComponent* ACapstoneCharacter::GetCamera()
{
	if ( FollowCamera->IsActive() ) return FollowCamera;
	else return FPSCamera;
}

void ACapstoneCharacter::SwitchCameras()
{
	FollowCamera->SetActive( !FollowCamera->IsActive() );
	FPSCamera->SetActive( !FPSCamera->IsActive() );
}