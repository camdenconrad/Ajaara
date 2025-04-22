#include "Projectile_Arrow_Base.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

AProjectile_Arrow_Base::AProjectile_Arrow_Base()
{
	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Create the arrow mesh component
	ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
	RootComponent = ArrowMesh;

	// Set up physics and collision
	ArrowMesh->SetSimulatePhysics(true);
	ArrowMesh->SetNotifyRigidBodyCollision(true); // Enable hit notifications
	ArrowMesh->SetCollisionProfileName(TEXT("Projectile"));

	// Set up the collision response
	ArrowMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ArrowMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // Make sure it hits characters

	// Bind the OnHit event
	ArrowMesh->OnComponentHit.AddDynamic(this, &AProjectile_Arrow_Base::OnHit);

	// Initialize velocity and speed
	Velocity = FVector::ZeroVector;
	Speed = 5000.0f;
}

void AProjectile_Arrow_Base::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectile_Arrow_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If the arrow is not using physics, update its position manually
	if (ArrowMesh && !ArrowMesh->IsSimulatingPhysics() && !Velocity.IsZero())
	{
		FVector NewLocation = GetActorLocation() + Velocity * DeltaTime;
		SetActorLocation(NewLocation);
	}

	if (TrailEffect && ArrowMesh && !Velocity.IsZero())
	{
		FVector ArrowEndLocation;
		FRotator ArrowEndRotation;
        
		// Calculate the end of the arrow based on its transform
		FVector ArrowDirection = GetActorForwardVector();
		float ArrowLength =40.0f; // Adjust this value based on your arrow's actual length
        
		// Calculate the position at the back of the arrow (opposite to forward vector)
		ArrowEndLocation = GetActorLocation() - (ArrowDirection * ArrowLength);
		ArrowEndRotation = GetActorRotation();
        
		// Spawn the trail effect
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			TrailEffect,
			ArrowEndLocation,
			ArrowEndRotation
		);
	}
	
}

void AProjectile_Arrow_Base::SetVelocity(const FVector& Vector)
{
	Velocity = Vector;

	// Set the arrow's rotation to match its velocity direction
	if (!Velocity.IsZero())
	{
		FRotator NewRotation = Velocity.Rotation();
		SetActorRotation(NewRotation);

		// Apply the velocity to the physics object
		if (ArrowMesh && ArrowMesh->IsSimulatingPhysics())
		{
			ArrowMesh->SetPhysicsLinearVelocity(Velocity);
		}
	}
}

void AProjectile_Arrow_Base::SetDamage(float Damage)
{
	DamageAmount = Damage;
}

void AProjectile_Arrow_Base::Scale(double X)
{
	ArrowMesh->SetWorldScale3D(FVector(X, X, X));
}

void AProjectile_Arrow_Base::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                   FVector NormalImpulse, const FHitResult& Hit)
{
	// Early return if invalid hit
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner() || !ArrowMesh || !OtherComp)
	{
		return;
	}

	hasCollided = true;
	// Apply damage
	FDamageEvent DamageEvent;
	OtherActor->TakeDamage(DamageAmount, DamageEvent, GetInstigatorController(), this);
	DamageAmount = 0.0f;

	UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			ArrowHitSound,// Sound to play
			GetActorLocation(), // Location to play sound
			0.2f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);

	if (HitEffect) // Ensure effect is assigned
	{
		// Get hit location from damage event if possible
		FVector HitLocation = GetActorLocation(); // Default to actor location
		FRotator HitRotation = GetActorRotation(); // Default rotation
        
		// Try to get more accurate hit information if available
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			// If it's point damage, we can get the exact hit location
			const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
			HitLocation = PointDamageEvent->HitInfo.ImpactPoint;
			HitRotation = PointDamageEvent->HitInfo.ImpactNormal.Rotation();
		}
		else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
		{
			// For radial damage, use the origin of the explosion
			const FRadialDamageEvent* RadialDamageEvent = static_cast<const FRadialDamageEvent*>(&DamageEvent);
			HitLocation = RadialDamageEvent->Origin;
		}
        
		// Spawn the effect at the hit location
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),    // Get world context
			HitEffect,     // Niagara system to spawn
			HitLocation,   // Location of the hit
			HitRotation    // Rotation to align with surface
		);
	}

	// Stop arrow physics and movement
	StopArrowMovement();

	// Handle arrow attachment based on what was hit
	AttachArrowToTarget(OtherActor, OtherComp, Hit);
}

void AProjectile_Arrow_Base::StopArrowMovement()
{
	if (ArrowMesh)
	{
		ArrowMesh->SetSimulatePhysics(false);
		ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ArrowMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Velocity = FVector::ZeroVector;
		Speed = 0.0f;
	}
}

void AProjectile_Arrow_Base::AttachArrowToTarget(AActor* HitActor, UPrimitiveComponent* HitComponent,
                                                 const FHitResult& Hit)
{
	if (!ArrowMesh || !HitComponent)
	{
		return;
	}

	// Try to find skeletal mesh for character hits
	USkeletalMeshComponent* SkeletalMesh = HitActor->FindComponentByClass<USkeletalMeshComponent>();

	if (SkeletalMesh)
	{
		// Try socket attachment first
		FName SocketName = TEXT("ArrowSocket");
		if (SkeletalMesh->DoesSocketExist(SocketName))
		{
			// Attach to socket
			ArrowMesh->AttachToComponent(SkeletalMesh, FAttachmentTransformRules::KeepWorldTransform, SocketName);
		}
		else
		{
			// Fall back to bone attachment
			FName BoneName = Hit.BoneName;
			if (BoneName == NAME_None)
			{
				BoneName = SkeletalMesh->GetBoneName(0);
			}

			ArrowMesh->AttachToComponent(SkeletalMesh, FAttachmentTransformRules::KeepWorldTransform, BoneName);
		}

		// Set correct position and rotation
		ArrowMesh->SetWorldLocation(Hit.ImpactPoint);
		ArrowMesh->SetWorldRotation(Velocity.Rotation());
	}
	else
	{
		// For static objects (walls, etc.)
		ArrowMesh->AttachToComponent(HitComponent, FAttachmentTransformRules::KeepWorldTransform);

		// Adjust arrow position to be slightly offset from the surface
		FVector SurfaceNormal = Hit.ImpactNormal;
		FVector AdjustedLocation = Hit.ImpactPoint - (SurfaceNormal * 5.0f); // Small offset to prevent clipping

		ArrowMesh->SetWorldLocation(AdjustedLocation);

		// Calculate rotation to align with surface
		FRotator ArrowRotation = Velocity.Rotation();
		ArrowMesh->SetWorldRotation(ArrowRotation);
	}

	// Optional: Play impact effects
	//PlayImpactEffects(Hit);
}
