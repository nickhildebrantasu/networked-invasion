// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "CapstoneCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FCurrentWeaponChangeDelegate, class AWeapon*, CurrentWeapon, const class AWeapon*, OldWeapon );

UCLASS(config=Game)
class ACapstoneCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY( VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = ( AllowPrivateAccess = "true" ) )
	USpringArmComponent* FPSpring;

	///** FPS camera */
	UPROPERTY( VisibleAnywhere, BlueprintReadWrite, Category = Camera, meta = ( AllowPrivateAccess = "true" ) )
	UCameraComponent* FPSCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = Input, meta = ( AllowPrivateAccess = "true" ) )
	UInputAction* NextToolAction;

	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = Input, meta = ( AllowPrivateAccess = "true" ) )
	UInputAction* PrevToolAction;

	/** Fire Input Action */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = Input, meta = ( AllowPrivateAccess = "true" ) )
	UInputAction* FireAction;

	/** Select Input Action */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = Input, meta = ( AllowPrivateAccess = "true" ) )
	UInputAction* SelectAction;

public:

	ACapstoneCharacter();

	void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;
	
	/** Getter for Max Health.*/
	UFUNCTION( BlueprintPure, Category = "Health" )
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** Getter for Current Health.*/
	UFUNCTION( BlueprintPure, Category = "Health" )
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	/** Setter for Current Health. Clamps the value between 0 and MaxHealth and calls OnHealthUpdate. Should only be called on the server.*/
	UFUNCTION( BlueprintCallable, Category = "Health" )
	void SetCurrentHealth( float healthValue );

	/** Event for taking damage. Overridden from APawn.*/
	UFUNCTION( BlueprintCallable, Category = "Health" )
	float TakeDamage( float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser ) override;

	UFUNCTION( BlueprintCallable, Category = "Character" )
	void Equip( const int32 index );

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void NextTool();
	void PrevTool();

protected:
	// Weapons the character spawns with
	UPROPERTY(EditDefaultsOnly, Category = "Configurations")
	TArray<TSubclassOf<class AWeapon>> DefaultWeapons;

	UFUNCTION()
	void OnRep_CurrentWeapon( const class AWeapon* OldWeapon );

	UFUNCTION(Server, Reliable)
	void Server_SetWeapon( class AWeapon* newWeapon );
	void Server_SetWeapon_Implementation( class AWeapon* newWeapon );

	/** The player's maximum health. This is the highest value of their health can be. This value is a value of the player's health, which starts at when spawned.*/
	UPROPERTY( EditDefaultsOnly, Category = "Health" )
	float MaxHealth;

	/** The player's current health. When reduced to 0, they are considered dead.*/
	UPROPERTY( ReplicatedUsing = OnRep_CurrentHealth )
	float CurrentHealth;

	/** RepNotify for changes made to current health.*/
	UFUNCTION()
	void OnRep_CurrentHealth();

	/** Response to health being updated. Called on the server immediately after modification, and on clients in response to a RepNotify*/
	void OnHealthUpdate();

	UPROPERTY( ReplicatedUsing = OnRep_Ragdoll )
	bool bIsRagdoll;

	UFUNCTION()
	void OnRep_Ragdoll();

	UPROPERTY( EditDefaultsOnly, Category = "Gameplay|Projectile" )
	TSubclassOf<class ANetworkProjectile > ProjectileClass;

	/** Delay between shots in seconds. Used to control fire rate for your test projectile, but also to prevent an overflow of server functions from binding SpawnProjectile directly to input.*/
	UPROPERTY( EditDefaultsOnly, Category = "Gameplay" )
	float FireRate;

	/** If true, you are in the process of firing projectiles. */
	bool bIsFiringWeapon;

	/** Function for beginning weapon fire.*/
	UFUNCTION( BlueprintCallable, Category = "Gameplay" )
	void StartFire( const FInputActionValue& value );

	/** Function for ending weapon fire. Once this is called, the player can use StartFire again.*/
	UFUNCTION( BlueprintCallable, Category = "Gameplay" )
	void StopFire();

	/** Server function for spawning projectiles.*/
	UFUNCTION( Server, Reliable )
	void HandleFire();

	/** A timer handle used for providing the fire rate delay in-between spawns.*/
	FTimerHandle FiringTimer;
			
	UFUNCTION( BlueprintCallable, Category = "Camera")
	void SwitchCameras();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE class UCameraComponent* GetFPSCamera() const { return FPSCamera; }
	virtual UCameraComponent* GetCamera();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Replicated, Category = "State")
	TArray<class AWeapon*> Weapons;

	UPROPERTY( VisibleInstanceOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentWeapon, Category = "State" )
	class AWeapon* CurrentWeapon;

	UPROPERTY( BlueprintAssignable, Category="Delegates")
	FCurrentWeaponChangeDelegate CurrentWeaponChangeDelegate; // when weapon is changed

	UPROPERTY( VisibleInstanceOnly, BlueprintReadWrite, Category = "State" )
	int32 CurrentIndex = 0;
};

