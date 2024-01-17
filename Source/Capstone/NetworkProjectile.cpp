// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ANetworkProjectile::ANetworkProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	//Definition for the SphereComponent that will serve as the Root component for the projectile and its collision.
	SphereComponent = CreateDefaultSubobject<USphereComponent>( TEXT( "RootComponent" ) );
	SphereComponent->InitSphereRadius( 37.5f );
	SphereComponent->SetCollisionProfileName( TEXT( "BlockAllDynamic" ) );
	RootComponent = SphereComponent;

	//Registering the Projectile Impact function on a Hit event.
	if ( GetLocalRole() == ROLE_Authority )
	{
		SphereComponent->OnComponentHit.AddDynamic( this, &ANetworkProjectile::OnProjectileImpact );
	}

	//Definition for the Mesh that will serve as your visual representation.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh( TEXT( "/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere" ) );
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "Mesh" ) );
	StaticMesh->SetupAttachment( RootComponent );

	//Set the Static Mesh and its position/scale if you successfully found a mesh asset to use.
	if ( DefaultMesh.Succeeded() )
	{
		StaticMesh->SetStaticMesh( DefaultMesh.Object );
		StaticMesh->SetRelativeLocation( FVector( 0.0f, 0.0f, -37.5f ) );
		StaticMesh->SetRelativeScale3D( FVector( 0.75f, 0.75f, 0.75f ) );
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultExplosionEffect( TEXT( "/Game/StarterContent/Particles/P_Explosion.P_Explosion" ) );
	if ( DefaultExplosionEffect.Succeeded() )
	{
		ExplosionEffect = DefaultExplosionEffect.Object;
	}

	//Definition for the Projectile Movement Component.
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>( TEXT( "ProjectileMovement" ) );
	ProjectileMovementComponent->SetUpdatedComponent( SphereComponent );
	ProjectileMovementComponent->InitialSpeed = 1500.0f;
	ProjectileMovementComponent->MaxSpeed = 1500.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

	DamageType = UDamageType::StaticClass();
	Damage = 10.0f;
}

// Called when the game starts or when spawned
void ANetworkProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANetworkProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ANetworkProjectile::Destroyed()
{
	FVector spawnLocation = GetActorLocation();
	UGameplayStatics::SpawnEmitterAtLocation( this, ExplosionEffect, spawnLocation, FRotator::ZeroRotator, true, EPSCPoolMethod::AutoRelease );
}

void ANetworkProjectile::OnProjectileImpact( UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit )
{
	if ( OtherActor )
	{
		UGameplayStatics::ApplyPointDamage( OtherActor, Damage, NormalImpulse, Hit, GetInstigator()->Controller, this, DamageType );
	}

	Destroy();
}