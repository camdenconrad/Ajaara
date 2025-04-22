// Fill out your copyright notice in the Description page of Project Settings.

#include "AI_Character.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "Components/WidgetComponent.h"
#include "Engine/DamageEvents.h"
#include "PhysicsEngine/PhysicsAsset.h" // Add this header for physics
#include "GameFramework/Character.h" // Make sure to include this
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/CameraComponent.h"
#include "MyProjectTest2/CPP_Repo/MyProjectTest2Character.h"
class AMyProjectTest2Character;
// Sets default values
AAI_Character::AAI_Character()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AttackCooldown = 2.0f; // 2 seconds between attacks by default
	bCanAttack = true;

	// Set up AI avoidance with stronger settings
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 500.0f; // Increased radius
	GetCharacterMovement()->AvoidanceWeight = 2.0f; // Increased weight
	//GetCharacterMovement()->AvoidanceLockTimeAfterAvoid = 0.5f; // Lock direction after avoiding
	//GetCharacterMovement()->AvoidanceLockTimeAfterClean = 0.2f; // Lock direction after clean
	//GetCharacterMovement()->AvoidanceVelocityBias = 0.9f; // Bias toward velocity changes
	GetCharacterMovement()->AvoidanceConsiderationRadius = 500.0f; // Larger consideration radius
    
	// Disable physics interactions completely
	GetCharacterMovement()->bEnablePhysicsInteraction = false;
	GetCharacterMovement()->PushForceFactor = 0.0f;
	GetCharacterMovement()->bPushForceScaledToMass = false;
	GetCharacterMovement()->bPushForceUsingZOffset = false;
    
	// Create a custom collision channel for AI characters if needed
	// This would require engine modifications, but the concept is:
	// ECollisionChannel::ECC_GameTraceChannel1 = AI_Character
}

// Called when the game starts or when spawned
void AAI_Character::BeginPlay()
{
    Super::BeginPlay();
    TargetHealth = 100.f;
    
    // Find the health bar widget component
    HealthBarWidget = Cast<UWidgetComponent>(GetComponentByClass(UWidgetComponent::StaticClass()));
    if (HealthBarWidget)
    {
        HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
        HealthBarWidget->SetDrawSize(FVector2D(100.0f, 15.0f));
        HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f)); // Position above the character
        HealthBarWidget->SetVisibility(false); // Initially hidden
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HealthBarWidget not found!"));
    }

    // Set up collision for AI avoidance with stronger settings
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
    
    // Add a small random offset to initial position to prevent exact overlaps
    FVector RandomOffset(FMath::RandRange(-10.0f, 10.0f), FMath::RandRange(-10.0f, 10.0f), 0.0f);
    AddActorWorldOffset(RandomOffset);

	//UCameraComponent* CameraRef = FindComponentByClass<UCameraComponent>();
	CameraRef = FindComponentByClass<UCameraComponent>();
}

void AAI_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind jump action
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AAI_Character::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AAI_Character::StopJump);
}

void AAI_Character::StartJump()
{
	if (!bIsJumping && GetCharacterMovement()->IsWalking())
	{
		bIsJumping = true;
		FVector JumpVelocity = GetActorForwardVector() * JumpHeight;
		JumpVelocity.Z = JumpHeight;
		GetCharacterMovement()->Velocity = JumpVelocity;
	}
}

void AAI_Character::StopJump()
{
	bIsJumping = false;
}

bool AAI_Character::IsJumpInputPressed() const
{
	return GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::SpaceBar);
}

void AAI_Character::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	//Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (bIsJumping)
	{
		// Stop jumping if we're no longer pressing the jump button
		if (!IsJumpInputPressed())
		{
			bIsJumping = false;
		}
	}
}

// Called every frame
void AAI_Character::Tick(float DeltaTime)
{
	UpdateHealthLerp();

	if (bIsJumping)
	{
		// Apply gravity while jumping
		FVector JumpVelocity = GetCharacterMovement()->Velocity;
		JumpVelocity.Z -= GetWorld()->GetGravityZ() * DeltaTime;
		GetCharacterMovement()->Velocity = JumpVelocity;

		// Stop jumping if we're no longer pressing the jump button
		if (!IsJumpInputPressed())
		{
			bIsJumping = false;
		}
	}
	
	if (!bIsHealthLerping && bIsDead)
	{
		HealthBarWidget->SetVisibility(false);
	}

	// Check if the AI is dead
	if (bIsDead)
	{
		// Don't hide health bar here - we'll hide it after the lerp is complete in UpdateHealthLerp
		// Only return if we're not lerping health anymore
		if (!bIsHealthLerping)
		{
			return;
		}
	}

	// Show health bar if health is not full
	if (CurrentHealth < 100.0f)
	{
		HealthBarWidget->SetVisibility(true);
	}
	
	Super::Tick(DeltaTime);

	// UCameraComponent* Camera;
	// AAIController* AIController = Cast<AAIController>(GetController());
	// if (AIController)
	// {
	// 	Camera = AIController->GetPawn()->FindComponentByClass<UCameraComponent>();
	// }
	
    if (TargetHealth > 0.0f || bIsHealthLerping)
    {
        // Get the player's pawn (assuming player index 0)
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (!PlayerPawn)
        {
            return;
        }
    
    		// Get direction from health bar to player
    		FVector PlayerLocation = PlayerPawn->GetActorLocation();
    		FVector WidgetLocation = HealthBarWidget->GetComponentLocation();
    		FVector Direction = PlayerLocation - WidgetLocation;
    		Direction.Normalize();
            
    		// Convert to rotation
    		FRotator LookAtRotation = Direction.Rotation();
    	
    		// We only want to rotate on the yaw axis (around Z)
    		// This keeps the health bar upright while facing the player
    		HealthBarWidget->SetWorldRotation(FRotator(0, LookAtRotation.Yaw + 180.0f, 0));
    	

        // Get AI Controller
        AAIController* AIController = Cast<AAIController>(GetController());
        if (!AIController)
        {
            UE_LOG(LogTemp, Warning, TEXT("AIController not found!"));
            return;
        }

        // Get the distance to the player
        float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());

        // Define the detection and stopping distances
        constexpr float DetectionRadius = 1600.0f; // AI notices player within this range
        constexpr float StopRadius = 100.0f; // AI stops moving if within this range

    	// Get camera location and rotation
    	FVector CameraLocation = CameraRef->GetComponentLocation();
    	FRotator CameraRotation = CameraRef->GetComponentRotation();

    	// Define the FOV parameters
    	float FOVAngle = CameraRef->FieldOfView; // In degrees
    	float HalfFOVAngle = FOVAngle * 1.5f; // Half of the FOV angle

    	// Calculate the number of rays to cast within the FOV
    	int NumRays = 1000; // Adjust this value to control the density of rays

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
        	AMyProjectTest2Character* MyPlayerCharacter = Cast<AMyProjectTest2Character>(PlayerPawn);
        	if (MyPlayerCharacter->CachedSpeed > 300.f)
        	{
        		bHasFoundPlayer = true;
        	}
        		
            
        }
    	
        // Don't process AI behavior if in damage state (stunned)
        if (bIsInDamageState)
        {
            return;
        }
    	
    	if (HealthBarWidget && PlayerPawn)
    	{
    		//float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
            
    		// Define min and max distances for scaling
    		const float MinDistance = 200.0f;  // Distance at which health bar is at max size
    		const float MaxDistance = 2000.0f; // Distance at which health bar is at min size
            
    		// Define min and max scale factors
    		const float MinScale = 0.5f;  // Minimum scale when far away
    		const float MaxScale = 1.0f;  // Maximum scale when close
            
    		// Calculate scale factor based on distance (clamped between min and max scale)
    		float ScaleFactor = FMath::GetMappedRangeValueClamped(
				FVector2D(MinDistance, MaxDistance),
				FVector2D(MaxScale, MinScale),
				DistanceToPlayer
			);
            
    		// Apply scale to health bar widget
    		FVector2D BaseSize(100.0f, 15.0f);  // Your original size
    		HealthBarWidget->SetDrawSize(BaseSize * ScaleFactor);
    	}


    	if (bHasFoundPlayer)
    	{
    		if (DistanceToPlayer > StopRadius)
    		{
    			// Only move if not currently executing an attack
    			if (!bIsExecutingAttack && !bIsInDamageState)
    			{
    				if (!bIsDead && TargetHealth > 0.0f)
    				{
    					// Use AI avoidance when moving to player
    					FPathFindingQuery PathFindingQuery;
    					FAIMoveRequest MoveRequest;
    					MoveRequest.SetGoalActor(PlayerPawn);
    					MoveRequest.SetAcceptanceRadius(5.0f);
    					MoveRequest.SetUsePathfinding(true);
                
    					AIController->MoveToActor(PlayerPawn, 5.0f, true, true, false, nullptr, true);
    					IsAttacking = false; // Ensure attack state is reset when moving
    				}
    			}
    		}
            else
            {
                AIController->StopMovement();

                // Check if within attack range
                if (DistanceToPlayer <= AttackRange)
                {
                    // Only start a new attack if not already attacking and cooldown has expired
                    if (bCanAttack && !bIsExecutingAttack)
                    {
                    	if (!bIsDead)
                    	{
                    		// Perform the attack
                    		AttackPlayer(PlayerPawn, AIController);
                    		IsAttacking = true;
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
    	
    }

	if (bIsInDamageState)
	{
		// Stop movement if currently moving
		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController)
		{
			AIController->StopMovement();
		}
		return;
	}

}


void AAI_Character::AttackPlayer(APawn* PlayerPawn, AAIController* AIController)
{
	if (!bCanAttack || bIsDead || bIsInDamageState || bIsExecutingAttack)
	{
		return;
	}

	// Prevent multiple attack calls before cooldown starts
	bCanAttack = false;
	bIsExecutingAttack = true;

	// Set the current target
	CurrentTarget = PlayerPawn;
    
	// Reset damage flag
	bHasAppliedDamageInCurrentAttack = false;
    
	// Schedule the actual damage application
	GetWorldTimerManager().SetTimer(
		AttackExecutionTimerHandle,
		this,
		&AAI_Character::ExecuteAttackDamage,
		AttackWindupTime,
		false
	);

	// Schedule attack cooldown
	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&AAI_Character::ResetAttackCooldown,
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




void AAI_Character::ExecuteAttackDamage()
{
	// If we've already applied damage in this attack, don't apply it again
	if (bHasAppliedDamageInCurrentAttack)
	{
		return;
	}
    
	// Check if we have a valid target
	if (CurrentTarget && !bIsDead && TargetHealth > 0.0f)
	{
		// Cast to your player character class
		AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(CurrentTarget);
		if (PlayerCharacter)
		{
			// Apply damage to the player
			UGameplayStatics::ApplyDamage(
				PlayerCharacter,
				Damage,
				GetController(),
				this,
				UDamageType::StaticClass()
			);
            
			// Mark that we've applied damage in this attack
			bHasAppliedDamageInCurrentAttack = true;
		}
	}
}

void AAI_Character::ResetAttackCooldown()
{
	bCanAttack = true;
	IsAttacking = false;
	bIsExecutingAttack = false;
	ClearAttackTimers();
}


// Called to bind functionality to input
void AAI_Character::ClearAttackTimers()
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackExecutionTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackFinishTimerHandle);
	IsAttacking = false;
}

float AAI_Character::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!bIsDead)
	{
		AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (PlayerCharacter)
		{
			// Give player 30 health
			PlayerCharacter->AddHealth(1.0f);
		}
	}
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->StopMovement();
	}
	bIsInDamageState = true;
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
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
        // Update health
        Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
        TargetHealth = Health;
        bIsHealthLerping = true;
        
        // Check for death
        if (Health <= 0.0f)
        {
            // Handle death
            bIsDead = true;
            IsAttacking = false;
            bIsExecutingAttack = false;
            
            // Clear all attack timers
            ClearAttackTimers();
            
            // Disable collision
            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            
            // Enable ragdoll after a delay
            GetWorldTimerManager().SetTimer(
                RagdollTimerHandle,
                this,
                &AAI_Character::EnableRagdoll,
                0.5f,
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
                ClearAttackTimers();
                IsAttacking = false;
                bIsExecutingAttack = false;
            }
            
            // Set timer to exit damage state
            GetWorldTimerManager().SetTimer(
                DamageStateTimerHandle,
                this,
                &AAI_Character::ExitDamageState,
                DamageStateStunDuration,
                false
            );
        }
    }
    
    return ActualDamage;
}

void AAI_Character::UpdateHealthLerp()
{

	if (FMath::Abs(CurrentHealth - TargetHealth) > 75.0f)
	{
		ErrorTolerance = 0.1f; // Increase tolerance for large health changes
	}
	else
	{
		ErrorTolerance = 5.f; // Default tolerance for small health changes
	}

	float lerpSpeed = 0.02f; // Default lerp speed
    
	// If the health difference is large, use a slower lerp speed
	if (FMath::Abs(CurrentHealth - TargetHealth) > 50.0f)
	{
		lerpSpeed = 0.01f; // Slower lerp for big health changes
	}

	// Directly lerp between CurrentHealth and TargetHealth without using time-based calculation
	CurrentHealth = FMath::Lerp(CurrentHealth, TargetHealth, lerpSpeed);
	

	// You can adjust the 0.1f value to control the lerp speed. The closer to 1.0, the faster it interpolates.
	
	// Stop lerping if we're "close enough"
	if (FMath::IsNearlyEqual(CurrentHealth, TargetHealth, ErrorTolerance)) 
	{
		CurrentHealth = TargetHealth;
		bIsHealthLerping = false;
		//GetWorldTimerManager().ClearTimer(HealthLerpTimerHandle);
	}
	else
	{
		bIsHealthLerping = true;
	}
}

void AAI_Character::ExitDamageState()
{
	bIsInDamageState = false;
	bCanAttack = true; // Allow attacks again after stun
}

bool AAI_Character::IsPendingKill()
{
	return (Health  < 15.f);
}

void AAI_Character::EnableRagdoll()
{

	AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerCharacter)
	{
		// Give player 30 health
		PlayerCharacter->AddHealth(15.0f);
	}

	UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			DeathSound,// Sound to play
			GetActorLocation(), // Location to play sound
			0.4f,          // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
    // Get the mesh component
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (MeshComp)
    {
        // Enable physics and set to ragdoll
        MeshComp->SetSimulatePhysics(true);
        MeshComp->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        
        // Set collision responses for the ragdoll
        MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore); // Start by ignoring everything
        MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Block the world/floor
        MeshComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // Block dynamic objects
        
        // Explicitly ignore all pawns (both player and AI)
        MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
        MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        
        // Set physics parameters to reduce bouncing and sliding
        MeshComp->SetAngularDamping(10.0f);
        MeshComp->SetLinearDamping(1.0f);
        
        // Create a custom collision profile for dead AI
        FName CollisionProfileName = TEXT("DeadAI");
        MeshComp->SetCollisionProfileName(CollisionProfileName);
        
        // Ensure capsule is still disabled
        UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
        if (CapsuleComp)
        {
            CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        
        // Disable movement component to prevent any residual movement
        UCharacterMovementComponent* MovementComp = GetCharacterMovement();
        if (MovementComp)
        {
            MovementComp->DisableMovement();
            MovementComp->StopMovementImmediately();
        }
        
        UE_LOG(LogTemp, Display, TEXT("AI ragdoll enabled with improved collision settings"));
    }
}

float AAI_Character::GetKicked(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	KickStun = true;
	if (!bIsDead)
	{
		AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if (PlayerCharacter)
		{
			// Give player 30 health
			PlayerCharacter->AddHealth(1.0f);
		}
	}
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->StopMovement();
	}
	bIsInDamageState = true;
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
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
        // Update health
        Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
        TargetHealth = Health;
        bIsHealthLerping = true;
        
        // Check for death
        if (Health <= 0.0f)
        {
            // Handle death
            bIsDead = true;
            IsAttacking = false;
            bIsExecutingAttack = false;
            
            // Clear all attack timers
            ClearAttackTimers();
            
            // Disable collision
            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            
            // Enable ragdoll after a delay
            GetWorldTimerManager().SetTimer(
                RagdollTimerHandle,
                this,
                &AAI_Character::EnableRagdoll,
                0.5f,
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
                ClearAttackTimers();
                IsAttacking = false;
                bIsExecutingAttack = false;
            }
            
            // Set timer to exit damage state
            GetWorldTimerManager().SetTimer(
                DamageStateTimerHandle,
                this,
                &AAI_Character::ExitDamageState,
                2.f,
                false
            );
        }
    }
    
    return ActualDamage;
}
