// Fill out your copyright notice in the Description page of Project Settings.

// In AAI_Elite.h, include the necessary header files

#include "AI_Elite.h"
#include "AI_Character.h"
#include "Elite_ChainProjectile.h"
#include "Elite_ThrowableAxe.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

class AElite_ChainProjectile;
class AAI_Character;

AAI_Elite::AAI_Elite()
{
	PrimaryActorTick.bCanEverTick = true;
	// Initialize AIController for the Elite character
	AIControllerClass = AAIController::StaticClass();
	
	bIsBlocking = false;
	BlockDuration = 5.0f;
	BlockCooldown = 2.0f;
	BlockDetectionRange = 1200.0f;
	bCanBlock = true;

	bIsStomping = false;
	bCanStomp = true;
	StompDuration = 1.0f;  // Adjust as needed
	StompCooldown = 5.0f; 
	
}

void AAI_Elite::BeginPlay()
{
	Super::BeginPlay();

	AxeMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Axe")));
	ShieldMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Shield")));
	
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (PlayerPawn != nullptr) {
		GetController<AAIController>()->MoveToActor(PlayerPawn, 5.0f, true, true, false, nullptr, true);
	}
	
    UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
    if (CapsuleComp)
    {
        // Set up a custom collision profile for AI characters
        FName CollisionProfileName = TEXT("AICharacter");
        CapsuleComp->SetCollisionProfileName(CollisionProfileName);
        
        // Block other AI pawns but with special handling
        CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        
        // Prevent physics pushing between AI characters
        CapsuleComp->SetSimulatePhysics(false);
        CapsuleComp->SetEnableGravity(true);
        
        // Lock rotations to prevent tipping
        CapsuleComp->BodyInstance.bLockXRotation = true;
        CapsuleComp->BodyInstance.bLockYRotation = true;
        CapsuleComp->BodyInstance.bLockZRotation = true;
        
        // Set collision shape to be slightly narrower at the top
        // This helps prevent climbing on top of each other
        float Height = CapsuleComp->GetUnscaledCapsuleHalfHeight();
        float Radius = CapsuleComp->GetUnscaledCapsuleRadius();
        CapsuleComp->SetCapsuleSize(Radius * 0.9f, Height, true);
        
        // Add a small vertical offset to ensure characters don't spawn inside each other
        FVector CurrentLocation = GetActorLocation();
        SetActorLocation(FVector(CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z + 5.0f));
    }
    
    // Configure character movement to prevent pushing and climbing
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        // Enhanced avoidance settings
        MovementComp->bUseRVOAvoidance = true;
        MovementComp->AvoidanceConsiderationRadius = 500.0f;
        MovementComp->AvoidanceWeight = 2.0f;
        
        // Completely prevent pushing
        MovementComp->PushForceFactor = 0.0f;
        MovementComp->bPushForceUsingZOffset = false;
        MovementComp->bPushForceScaledToMass = false;
        MovementComp->bEnablePhysicsInteraction = false;
        
        // Improve movement control
        MovementComp->bUseSeparateBrakingFriction = true;
        MovementComp->BrakingFriction = 10.0f;
        MovementComp->MaxAcceleration = 1500.0f;
        MovementComp->BrakingDecelerationWalking = 2000.0f;
        
        // Prevent climbing slopes that are too steep
        MovementComp->SetWalkableFloorAngle(40.0f); // Reduce from default 45 degrees
        MovementComp->bMaintainHorizontalGroundVelocity = true;
        
        // Prevent stepping up onto other characters
        MovementComp->MaxStepHeight = 65.0f; // Reduce from default 45.0
        MovementComp->SetGroundMovementMode(EMovementMode::MOVE_Walking);
    }
    
    // Assign a unique avoidance group to each AI with stronger separation
    static uint8 AvoidanceGroupCounter = 0;
    uint8 AvoidanceGroup = (++AvoidanceGroupCounter) % 32; // Cycle through available groups (0-31)
    GetCharacterMovement()->SetAvoidanceGroup(AvoidanceGroup);
    GetCharacterMovement()->SetGroupsToAvoid(~0); // Avoid all groups
    
    // // Add a small random offset to initial position to prevent exact overlaps
    // FVector RandomOffset(FMath::RandRange(-10.0f, 10.0f), FMath::RandRange(-10.0f, 10.0f), 0.0f);
    // AddActorWorldOffset(RandomOffset);

	//UCameraComponent* CameraRef = FindComponentByClass<UCameraComponent>();
	CameraRef = FindComponentByClass<UCameraComponent>();

	RingMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Ring")));
	DomeMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Dome")));
	// Move AI to a specific location when the game starts (exathe mple)
}

void AAI_Elite::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DomeMesh->SetVisibility(bIsBlocking);
	
	if(!isThrowingAxe)
	{
		AxeMesh->SetVisibility(true);
	} 
	// Don't process AI behavior if in damage state (stunned)
	if (bIsInDamageState)
	{
		ClearAllAttackTimers();
		return;
	}

	if (!bIsBlocking && bCanBlock && !bIsInDamageState && !bIsDead && IsPlayerAttacking() && !isThrowingAxe && !bIsExecutingSummon)
	{
		if (!bShieldDetached)
		{
			TriggerShieldBlock();
		}
	}
	if (!IsPlayerAttacking())
	{
		if (bIsBlocking)
		{
			GetWorldTimerManager().SetTimer(BlockDropTimerHandle, this, &AAI_Elite::EndShieldBlock, 1.5f, false);
		}
	}

	// Get the distance to the player
	float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
	RingMesh->SetVisibility((DistanceToPlayer >= 1000.f));

	if (IsAttacking || bIsKicking || bIsExecutingSummon || isThrowingAxe || bUsingChain)
	{
		GetController<AAIController>()->StopMovement();
	}

	

	AAIController* AIController = Cast<AAIController>(GetController());
        if (!AIController)
        {
            UE_LOG(LogTemp, Warning, TEXT("AIController not found!"));
            return;
        }

        // Get the distance to the player
	//DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());

        // Define the detection and stopping distances
        constexpr float DetectionRadius = 2000.0f; // AI notices player within this range
        constexpr float StopRadius = 150.0f; // AI stops moving if within this range
		constexpr float RunStartDistance = 150.0f; // AI starts running if closer than this
		float RunStopDistance = 400.0f;
		if (!bShieldDetached)
		{
			RunStopDistance = 400.0f;
		}
		else
		{
			RunStopDistance = 900.0f;
			RunningSpeed = 400.0f;
		}
		

		bool bShouldRun = (DistanceToPlayer <= RunStartDistance && DistanceToPlayer > RunStopDistance);

	if (DistanceToPlayer <= 1200.f && DistanceToPlayer > AttackRange && bCanUseChain && !bIsExecutingSummon && !isThrowingAxe)
	{
		//ThrowChain();
	}

	// Get the character movement component
		UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
		if (MovementComponent)
		{
			// Set the movement speed based on whether the AI should run or walk
			float TargetSpeed = bShouldRun ? RunningSpeed : WalkingSpeed;
			MovementComponent->MaxWalkSpeed = TargetSpeed;
		}

    	// Get camera location and rotation
    	FVector CameraLocation = CameraRef->GetComponentLocation();
    	FRotator CameraRotation = CameraRef->GetComponentRotation();

    	// Define the FOV parameters
    	float FOVAngle = CameraRef->FieldOfView; // In degrees
    	float HalfFOVAngle = FOVAngle * 0.7f; // Half of the FOV angle

    	// Calculate the number of rays to cast within the FOV
    	int NumRays = 500; // Adjust this value to control the density of rays

    	// Calculate the angle between each ray
    	float AngleBetweenRays = FOVAngle / (NumRays - 1);

    	// Perform ray tracing for each ray within the FOV
    	for (int i = 0; i < NumRays; i++)
    	{
    		// Calculate the angle for the current ray
    		float CurrentAngle = -HalfFOVAngle + (i * AngleBetweenRays);

    		// Convert the angle to a rotation
    		FRotator RayRotation = CameraRotation;
    		RayRotation = FRotator(CameraRotation.Pitch, CameraRotation.Yaw + CurrentAngle, CameraRotation.Roll);


    		// Calculate the end location of the ray
    		FVector RayEndLocation = CameraLocation + (RayRotation.Vector() * 5000.0f); // Adjust this value to control the ray length

    		// Perform the line trace
    		FCollisionQueryParams QueryParams;
    		FHitResult HitResult;
    		if (GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, RayEndLocation, ECC_Pawn, QueryParams))
    		{
    			// If the line trace hits the player's pawn, the AI can see the player
    			if (HitResult.GetActor() == PlayerPawn)
    			{
    				bHasFoundPlayer = true;
    			}
    		}
    	}
	
        if (DistanceToPlayer <= DetectionRadius)
        {
        	bHasFoundPlayer = true;
        }

	if (DistanceToPlayer > 350.0f && bHasFoundPlayer && !bIsDead && !bIsBlocking)
	{
		int32 RandomNumber = FMath::RandRange(0, 100);
		if (RandomNumber < 50  && bCanThrowAxe && !bIsExecutingSummon)
		{
			EndShieldBlock();
			PerformAxeThrow();
		}
		else
		{
			if (bCanSummonGrunts && !isThrowingAxe && DistanceToPlayer > 650.0f)
			{
				EndShieldBlock();
				SummonGrunts();
			}
		}
		
	}
	else
	{
		GetController<AAIController>()->MoveToActor(PlayerPawn, 5.0f, true, true, false, nullptr, true);
	}

	if (DistanceToPlayer < 600.0f)
	{
		isThrowingAxe = false;
	}

	if (bHasFoundPlayer && DistanceToPlayer > StopRadius) {
		GetController<AAIController>()->MoveToActor(PlayerPawn, 5.0f, true, true, false, nullptr, true);
	}
	else
	{
		GetController<AAIController>()->StopMovement();

		// Check if within attack range
		if (DistanceToPlayer <= AttackRange)
		{
			// Only start a new attack if not already attacking and cooldown has expired
			if (bCanAttack && !bIsExecutingAttack)
			{
				if (!bIsDead)
				{
					// Perform the attack
					if (DistanceToPlayer <= AttackRange - 20.f && bCanKick)
					{
						EndShieldBlock();
						KickPlayer(PlayerPawn, AIController);
						bIsKicking = true;
					}
					else
					{
						if (!bIsKicking)
						{
							EndShieldBlock();
							AttackPlayer(PlayerPawn, AIController);
							IsAttacking = true;
						}
					}
				}
			}
		}
		else
		{
			// Only reset attack state if we're not in the middle of an attack
			if (!bIsExecutingAttack)
			{
				IsAttacking = false;
			}
			if (!bIsInDamageState)
			{
				AIController->MoveToActor(PlayerPawn, 5.0f, true, true, true, nullptr, true);
			}
		}
	}
}


void AAI_Elite::AttackPlayer(APawn* Pawn, AController* AIController)
{
	UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			AxeSwingSound,// Sound to play
			GetActorLocation(), // Location to play sound
			0.4f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
	CurrentAttackNumber = FMath::RandRange(0, 1);
	GetController<AAIController>()->StopMovement();
	if (!bCanAttack || bIsDead || bIsInDamageState || bIsExecutingAttack)
	{
		return;
	}

	// Prevent multiple attack calls before cooldown starts
	bCanAttack = false;
	bIsExecutingAttack = true;

	// Set the current target
	CurrentTarget = Pawn;
    
	// Reset damage flag
	bHasAppliedDamageInCurrentAttack = false;
    
	// Schedule the actual damage application
	GetWorldTimerManager().SetTimer(
		AttackExecutionTimerHandle,
		this,
		&AAI_Elite::ExecuteAttackDamage,
		AttackWindupTime,
		false
	);

	// Schedule attack cooldown
	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&AAI_Elite::ResetAttackCooldown,
		AttackCooldown,
		false
	);

	// Schedule attack animation completion
	GetWorldTimerManager().SetTimer(
		AttackFinishTimerHandle,
		[this]()
		{
			IsAttacking = false;
			bIsExecutingAttack = false;
		},
		AttackAnimationTime,
		false
	);
}

void AAI_Elite::ExecuteKickDamage()
{
	bCanAttack = false;
	bCanKick = false;
	// If we've already applied damage in this attack, don't apply it again
	if (bHasAppliedDamageInCurrentAttack)
	{
		return;
	}
    
	AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(CurrentTarget);
	if (PlayerCharacter)
	{
		// Get the direction to the player
		FVector DirectionToPlayer = (PlayerCharacter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

		// Get the forward direction of the enemy
		FVector ForwardDirection = GetActorForwardVector();

		// Check if the player is in front of the enemy
		float DotProduct = FVector::DotProduct(DirectionToPlayer, ForwardDirection);
		if (DotProduct > 0.0f)
		{
			// Check if the player is still in attack range
			float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
			if (DistanceToPlayer <= AttackRange)
			{
				FDamageEvent DamageEvent;
				PlayerCharacter->KickDamage(5.f, DamageEvent, GetController(), this);

				// Mark that we've applied damage in this attack
				bHasAppliedDamageInCurrentAttack = true;
			}
		}
	}
}

void AAI_Elite::ClearKickTimers()
{

	GetWorldTimerManager().ClearTimer(KickTimerHandle);
	GetWorldTimerManager().ClearTimer(KickExecutionTimerHandle);
	GetWorldTimerManager().ClearTimer(KickFinishTimerHandle);
	bIsKicking = false;
}

void AAI_Elite::ResetKickCooldown()
{
	bCanKick = true;
	bIsKicking = false;
	bIsExecutingKick = false;
	ClearKickTimers();
}

void AAI_Elite::KickPlayer(APawn* Pawn, AController* AIController)
{
	GetController<AAIController>()->StopMovement();
	if (!bCanAttack || bIsDead || bIsInDamageState || bIsExecutingAttack)
	{
		return;
	}

	// Prevent multiple attack calls before cooldown starts
	bIsKicking = true;
	// Set the current target
	CurrentTarget = Pawn;
    
	// Reset damage flag
	bHasAppliedDamageInCurrentAttack = false;
	
	
	
	AAI_Elite::ExecuteKickDamage();
		
	// Schedule attack cooldown
	GetWorldTimerManager().SetTimer(
		KickTimerHandle,
		this,
		&AAI_Elite::ResetKickCooldown,
		KickCooldown,
		false
	);

	// Schedule attack animation completion
	GetWorldTimerManager().SetTimer(
		KickFinishTimerHandle,
		[this]()
		{
			bIsKicking = false;
			bIsExecutingKick = false;
			bCanAttack = true;
		},
		KickAnimationTime,
		false
	);
}




void AAI_Elite::ExecuteAttackDamage()
{
	
	bCanAttack = false;
	// If we've already applied damage in this attack, don't apply it again
	if (bHasAppliedDamageInCurrentAttack)
	{
		return;
	}
    
	AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(CurrentTarget);
	if (PlayerCharacter)
	{
		// Get the direction to the player
		FVector DirectionToPlayer = (PlayerCharacter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

		// Get the forward direction of the enemy
		FVector ForwardDirection = GetActorForwardVector();

		// Check if the player is in front of the enemy
		float DotProduct = FVector::DotProduct(DirectionToPlayer, ForwardDirection);
		if (DotProduct > 0.0f)
		{
			// Check if the player is still in attack range
			float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
			if (DistanceToPlayer <= AttackRange)
			{
				// Apply damage to the player
				UGameplayStatics::ApplyDamage(
					PlayerCharacter,
					90,
					GetController(),
					this,
					UDamageType::StaticClass()
				);

				// Mark that we've applied damage in this attack
				bHasAppliedDamageInCurrentAttack = true;
			}
		}
	}
}


void AAI_Elite::ResetAttackCooldown()
{
	bCanAttack = true;
	IsAttacking = false;
	bIsExecutingAttack = false;
	ClearAttackTimers();
}


// Called to bind functionality to input
void AAI_Elite::ClearAttackTimers()
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackExecutionTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackFinishTimerHandle);
	IsAttacking = false;
}

void AAI_Elite::ClearAllAttackTimers()
{
	ClearAttackTimers();
	ClearKickTimers();
	GetWorldTimerManager().ClearTimer(SummonExecutionTimerHandle);
	GetWorldTimerManager().ClearTimer(SummonFinishTimerHandle);
	IsAttacking = false;
	bIsExecutingAttack = false;
	bIsKicking = false;
	bIsExecutingKick = false;
	bIsExecutingSummon = false;
}

float AAI_Elite::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsStomping)
	{
		return 0.0f;
	}
	if (!bIsDead)
	{
		AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (PlayerCharacter)
		{
			// Give player 30 health
			if (!bIsBlocking)
			{
				PlayerCharacter->AddHealth(5.0f);
			}
		}
	}

	isThrowingAxe = false;
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->StopMovement();
	}
	bIsInDamageState = true;

	if (bIsBlocking)
	{
		// Reduce damage when blocking, for example by 50%
		DamageAmount *= 0.1f;
		UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			ShieldHitSound,// Sound to play
			GetActorLocation(), // Location to play sound
			0.4f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);

		AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(CurrentTarget);
		if (PlayerCharacter)
		{
			// Get the direction to the player
			FVector DirectionToPlayer = (PlayerCharacter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

			// Get the forward direction of the enemy
			FVector ForwardDirection = GetActorForwardVector();

			// Check if the player is in front of the enemy
			float DotProduct = FVector::DotProduct(DirectionToPlayer, ForwardDirection);
			if (DotProduct > 0.0f)
			{
					// Apply damage to the player
					// UGameplayStatics::ApplyDamage(
					// 	PlayerCharacter,
					// 	10,
					// 	GetController(),
					// 	this,
					// 	UDamageType::StaticClass()
					// );

				KickPlayer(PlayerPawn, AIController);
				
			}
		}
		
		
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(
		this,           // World context object
		TakeDamageSound,// Sound to play
		GetActorLocation(), // Location to play sound
		0.4f,           // Volume multiplier
		1.0f,           // Pitch multiplier
		0.0f,           // Start time
		nullptr,        // Attenuation settings
		nullptr         // Concurrency settings
	);
	}
	
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
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
    
    if (ActualDamage > 0.0f && !bIsDead)
    {
    	Health -= ActualDamage;
    	
    	if (Health <= MaxHealth * 0.5f && !bShieldDetached)
    	{
    		DetachShield();
    	}
        
        // Check for death
        if (Health <= 0.0f)
        {
            // Handle death
            bIsDead = true;
            IsAttacking = false;
            bIsExecutingAttack = false;

        	DetachAxe();
            
            // Clear all attack timers
            ClearAttackTimers();
            
            // Disable collision
            //GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            
            // Enable ragdoll after a delay
            GetWorldTimerManager().SetTimer(
                RagdollTimerHandle,
                this,
                &AAI_Elite::EnableRagdoll,
                3.5f,
                false
            );
            
            // If this is a boss or special enemy that should trigger game restart when defeated
            // Uncomment the following code:
            /*
            // Set timer to restart the game after a delay
            GetWorldTimerManager().SetTimer(
                RestartTimerHandle,
                this,
                &AAI_Character::RestartGame,
                RestartDelay,
                false
            );
            */
        }
        else
        {
            // Enter damage state (stun) if not dead
            bIsInDamageState = true;
            
            // Clear attack timers if we're in the middle of an attack
            if (IsAttacking || bIsExecutingAttack)
            {
                ClearAllAttackTimers();
                IsAttacking = false;
                bIsExecutingAttack = false;
            }
            
            // Set timer to exit damage state
            GetWorldTimerManager().SetTimer(
                DamageStateTimerHandle,
                this,
                &AAI_Elite::ExitDamageState,
                DamageStateStunDuration,
                false
            );


        	GetWorldTimerManager().SetTimer(
				StompTimerHandle,
				this,
				&AAI_Elite::PerformStomp,
				DamageStateStunDuration/2.f,
				false
			);
        	
        }
    }
    
    return ActualDamage;
}

void AAI_Elite::DetachShield()
{
	if (ShieldMesh)
	{
		// Detach the shield from the character
		ShieldMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        
		// Enable physics on the shield
		ShieldMesh->SetSimulatePhysics(true);
		ShieldMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        
		// Apply an impulse to make the shield fall away
		FVector ImpulseDirection = FVector::UpVector + GetActorRightVector();
		float ImpulseStrength = 1000.0f; // Adjust this value as needed
		ShieldMesh->AddImpulse(ImpulseDirection * ImpulseStrength);
        
		// Mark the shield as detached
		bShieldDetached = true;
        
		// Disable blocking ability
		bCanBlock = false;
		bIsBlocking = false;
	}
}

void AAI_Elite::DetachAxe()
{
	if (AxeMesh)
	{
		// Detach the shield from the character
		AxeMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        
		// Enable physics on the shield
		AxeMesh->SetSimulatePhysics(true);
		AxeMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        
		// Apply an impulse to make the shield fall away
		FVector ImpulseDirection = -FVector::UpVector.GetSafeNormal() + -GetActorRightVector().GetSafeNormal();
		float ImpulseStrength = .05f; // Adjust this value as needed
		AxeMesh->AddImpulse(ImpulseDirection * ImpulseStrength);
	}
}

void AAI_Elite::ExitDamageState()
{
	bIsInDamageState = false;
	bCanAttack = true; // Allow attacks again after stun
}


void AAI_Elite::EnableRagdoll()
{
	AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerCharacter)
	{
		PlayerCharacter->AddHealth(15.0f);
	}
    
	UGameplayStatics::PlaySoundAtLocation(
		this,
		DeathSound,
		GetActorLocation(),
		0.4f,
		1.0f,
		0.0f,
		nullptr,
		nullptr
	);

	// Disable capsule collision
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	// Disable movement component
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->DisableMovement();
		MovementComp->StopMovementImmediately();
	}

	// Set the mesh to not simulate physics and disable collision
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp)
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	// You might want to play a death animation here instead of enabling ragdoll
	// For example:
	// PlayAnimMontage(DeathAnimMontage);

	UE_LOG(LogTemp, Display, TEXT("AI death handled without ragdoll"));
}

float  AAI_Elite::GetHealth()
{
	return Health;
}

float  AAI_Elite::GetMaxHealth()
{
	return MaxHealth;
}

void AAI_Elite::ResetSummonCooldown()
{
    bCanSummonGrunts = true;
	GetWorldTimerManager().ClearTimer(SummonExecutionTimerHandle);
	GetWorldTimerManager().ClearTimer(SummonTimerHandle);
	GetWorldTimerManager().ClearTimer(SummonFinishTimerHandle);
}

void AAI_Elite::SummonGrunts()
{
	if (!bIsDead && !bIsInDamageState && !bIsExecutingAttack)
	{
		// Prevent multiple summon calls before cooldown starts
		if (bIsExecutingSummon)
		{
			return;
		}

		UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			SummonSound,// Sound to play
			GetActorLocation(), // Location to play sound
			0.4f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
		
		bCanSummonGrunts = false;
		bIsExecutingSummon = true;

		// Stop AI movement before summoning grunts
		GetController<AAIController>()->StopMovement();

		// Schedule the actual summoning
		GetWorldTimerManager().SetTimer(
			SummonExecutionTimerHandle,
			this,
			&AAI_Elite::ExecuteSummon,
			SummonWindupTime,
			false
		);

		// Schedule summon cooldown
		GetWorldTimerManager().SetTimer(
			SummonTimerHandle,
			this,
			&AAI_Elite::ResetSummonCooldown,
			SummonCooldown,
			false
		);

		// Schedule summon animation completion
		GetWorldTimerManager().SetTimer(
			SummonFinishTimerHandle,
			[this]
			{
				bIsExecutingSummon = false;
			},
			SummonAnimationTime,
			false
		);
	}
}

void AAI_Elite::ExecuteSummon()
{
	for (int i = 0; i < 3; i++)
	{
		// Calculate a random offset around the elite AI in the X and Y plane
		float Radius = 500.0f; // Adjust this value to change the radius of the spawn area
		FVector Offset = FVector(FMath::RandRange(-Radius, Radius), FMath::RandRange(-Radius, Radius), 0.0f);

		// Spawn a new grunt at the calculated location
		FVector SpawnLocation = FVector::ZeroVector;
		if (IsValid(this))
		{
			SpawnLocation = GetActorLocation() + Offset;
			FRotator SpawnRotation = GetActorRotation();
			FTimerHandle DelayTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, [this, i, SpawnLocation, SpawnRotation]()
			{
				// Spawn logic here
				SpawnGrunt(SpawnLocation, SpawnRotation);
			}, i * 0.01f, false);  // Small delay between spawns
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AAI_Elite actor is invalid during SummonGrunts."));
			return;
		}
	}
}

void AAI_Elite::SpawnGrunt(FVector Location, FRotator Rotation)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AAI_Character* Grunt = World->SpawnActor<AAI_Character>(
			GruntClass,
			Location,
			Rotation,
			SpawnParams
		);

	}
}


void AAI_Elite::PerformAxeThrow()
{
	isThrowingAxe = true;
	bCanThrowAxe = false;

	// Add a timer to reset bCanThrowAxe after a cooldown period
	GetWorldTimerManager().SetTimer(
		AxeThrowCooldownTimerHandle,
		[this]()
		{
			bCanThrowAxe = true;
		},
		AxeThrowCooldown,
		false
	);

	GetWorldTimerManager().SetTimer(
		AxeThrowCooldownExecutionHandle,
		[this]()
		{
			ThrowAxe();
			UGameplayStatics::PlaySoundAtLocation(
				this,           // World context object
				AxeThrowSound,// Sound to play
				GetActorLocation(), // Location to play sound
				0.4f,           // Volume multiplier
				1.0f,           // Pitch multiplier
				0.0f,           // Start time
				nullptr,        // Attenuation settings
				nullptr         // Concurrency settings
			);
		},
		1.f,
		false
	);

	GetWorldTimerManager().SetTimer(
		AxeThrowTimerHandle,
		[this]()
		{
			isThrowingAxe = false;
		},
		2.f,
		false
	);
	
}

void AAI_Elite::ThrowAxe()
{
	AxeMesh->SetVisibility(false);
	if (!isThrowingAxe)
	{
		return;
	}
	if (EliteThrowableAxeClass && PlayerPawn)
	{
		// Get the skeletal mesh component
		USkeletalMeshComponent* MeshComponent = GetMesh();
		if (MeshComponent)
		{
			// Get the socket transform
			FTransform SocketTransform = MeshComponent->GetSocketTransform("RightHandSocket");

			// Use the socket location and rotation for spawning
			FVector SpawnLocation = SocketTransform.GetLocation();
			FRotator SpawnRotation = SocketTransform.Rotator();

			AElite_ThrowableAxe* Axe = GetWorld()->SpawnActor<AElite_ThrowableAxe>(EliteThrowableAxeClass, SpawnLocation, FRotator(0,0,0));
			if (Axe)
			{
				Axe->ThrowAxe(PlayerPawn->GetActorLocation(), this, PlayerPawn);
			}
		}
		else
		{
			// Fallback to using actor location and rotation if mesh component is not found
			FVector SpawnLocation = GetActorLocation();
			FRotator SpawnRotation = GetActorRotation();

			AElite_ThrowableAxe* Axe = GetWorld()->SpawnActor<AElite_ThrowableAxe>(EliteThrowableAxeClass, SpawnLocation, SpawnRotation);
			if (Axe)
			{
				Axe->ThrowAxe(PlayerPawn->GetActorLocation(), this, PlayerPawn);
			}
		}
	}
}

bool AAI_Elite::AreAllGruntsDead()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	// Iterate through all actors in the world
	for (TActorIterator<AAI_Character> It(World); It; ++It)
	{
		AAI_Character* Character = *It;
		// Check if the character is a grunt (not an elite) and is alive
		if (Character && !Character->IsA(AAI_Elite::StaticClass()) && !Character->bIsDead)
		{
			return false;
		}
	}

	return true;
}

void AAI_Elite::ThrowChain()
{
	bUsingChain = true;
	bCanUseChain = false;
	UE_LOG(LogTemp, Warning, TEXT("ThrowChain function called"));

	

	GetWorldTimerManager().SetTimer(
		ChainExecuteTimerHandle,
		[this]()
		{
			if (ChainProjectileClass && PlayerPawn)
			{
				USkeletalMeshComponent* MeshComponent = GetMesh();
				FTransform SocketTransform = MeshComponent->GetSocketTransform("LeftHandSocket");

				// Use the socket location and rotation for spawning
				FVector SpawnLocation = SocketTransform.GetLocation();
				FRotator SpawnRotation = MeshComponent->GetSocketRotation("LeftHandSocket");  // Get the rotation from the socket

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
				AElite_ChainProjectile* Chain = GetWorld()->SpawnActor<AElite_ChainProjectile>(ChainProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
				if (Chain)
				{

					Chain->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			
					Chain->LaunchChain(PlayerPawn, this);
					UE_LOG(LogTemp, Warning, TEXT("Chain spawned successfully"))
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to spawn Chain"));
				}
			}
		},
		.5f,
		false
	);

	GetWorldTimerManager().SetTimer(
		ChainTimerHandle,
		[this]()
		{
			bUsingChain = false;
		},
		2.5f,
		false
	);

	GetWorldTimerManager().SetTimer(
		ChainCooldownTimerHandle,
		[this]()
		{
			bCanUseChain = true;
		},
		8.f,
		false
	);
}

bool AAI_Elite::IsPlayerAttacking()
{
	if (!PlayerPawn)
	{
		return false;
	}

	// Cast the player pawn to your player character class
	AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(PlayerPawn);
	if (!PlayerCharacter)
	{
		return false;
	}

	// Check if the player is within range
	float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
	if (DistanceToPlayer > BlockDetectionRange)
	{
		return false;
	}

	// Check if the player is attacking
	// You'll need to implement this function in your player character class
	return PlayerCharacter->IsAttacking || PlayerCharacter->bIsAiming || PlayerCharacter->bIsQuickAttackInProgress;
}

void AAI_Elite::TriggerShieldBlock()
{
	if (!bCanBlock || bIsBlocking)
	{
		return;
	}
	float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
	if (DistanceToPlayer <= AttackRange)
	{
		AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(PlayerPawn);
		if (PlayerCharacter)
		{
			// Calculate direction from AI to player
			FVector LaunchDirection = PlayerCharacter->GetActorLocation() - GetActorLocation();
			LaunchDirection.Z = 0.5f; // Add some upward force
			LaunchDirection.Normalize();
			PlayerCharacter->LaunchCharacter(LaunchDirection * StompForce, true, true);
		}
	}

	bIsBlocking = true;
	bCanBlock = false;

	// Play shield block animation or visual effect here

	// Set timer to end blocking
	GetWorldTimerManager().SetTimer(BlockTimerHandle, this, &AAI_Elite::EndShieldBlock, BlockDuration, false);
}

void AAI_Elite::EndShieldBlock()
{
	bIsBlocking = false;

	// Set timer for block cooldown
	GetWorldTimerManager().SetTimer(BlockTimerHandle, [this]() { bCanBlock = true; }, BlockCooldown, false);
}

void AAI_Elite::PerformStomp()
{
	bHasDoneStompDamage = true;
	if (bIsStomping || !bCanStomp)
	{
		return;
	}
	bCanStomp = false;

	// Play stomp sound
	if (StompSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, StompSound, GetActorLocation());
	}

	// Spawn stomp effect
	if (StompEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), StompEffect, GetActorLocation());
	}

	// Check if PlayerPawn is valid and within the stomp radius
	if (PlayerPawn)
	{
		float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
		if (DistanceToPlayer <= StompRadius)
		{
			bIsStomping = true;
			AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(PlayerPawn);
			if (PlayerCharacter)
			{
				// Apply kick damage
				FDamageEvent DamageEvent;
				if (!bHasDoneStompDamage)
				{
					PlayerCharacter->KickDamage(StompDamage, DamageEvent, GetController(), this);
				}
			}
		}
	}

	// Set timer to end stomping state
	GetWorldTimerManager().SetTimer(
		StompEndTimerHandle,
		[this]()
		{
			bIsStomping = false;
			bHasDoneStompDamage = false;
			// Launch the player
			AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(PlayerPawn);
			if (PlayerCharacter)
			{
				// Calculate direction from AI to player
				FVector LaunchDirection = PlayerCharacter->GetActorLocation() - GetActorLocation();
				LaunchDirection.Z = 0.5f; // Add some upward force
				LaunchDirection.Normalize();
				//PlayerCharacter->LaunchCharacter(LaunchDirection * StompForce, true, true);
			}
		},
		StompDuration,
		false
	);

	// Set timer for stomp cooldown
	GetWorldTimerManager().SetTimer(
		StompCooldownTimerHandle,
		[this]()
		{
			bCanStomp = true;
		},
		StompCooldown,
		false
	);
}
