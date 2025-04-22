// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectTest2Character.h"

#include "AI_Character.h"
#include "AI_Elite.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Projectile_Arrow_Base.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProfilingDebugging/CookStats.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMyProjectTest2Character

AMyProjectTest2Character::AMyProjectTest2Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
 
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 800.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// static ConstructorHelpers::FClassFinder<AProjectile_Arrow_Base> ProjectileBP(
	// 	TEXT("/Game/Blueprints/BP_Projectile_Arrow_Base_"));
	// if (ProjectileBP.Succeeded())
	// {
	// 	ProjectileClass = ProjectileBP.Class;
	// }

	DefaultCameraPosition = FollowCamera->GetRelativeLocation();
	DefaultCameraRotation = FollowCamera->GetRelativeRotation();

	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AMyProjectTest2Character::OnCapsuleOverlap);

	//BowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BowMesh"));
	//BowMesh->SetupAttachment(RootComponent);

	//SwordRightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordRightMesh"));
	//SwordRightMesh->SetupAttachment(RootComponent);

	//SwordLeftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordLeftMesh"));
	//SwordLeftMesh->SetupAttachment(RootComponent);

	//CrossbowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrossbowMesh"));
	//CrossbowMesh->SetupAttachment(RootComponent);

	// Initialize weapon state
	//bIsBowEquipped = false;
	//UpdateWeaponVisibility();

	CurrentDisplayHealth = 100.0f;
	TargetHealth = 100.0f;
	HealthLerpSpeed = 5.0f;
	bIsHealthLerping = false;
}
	


//////////////////////////////////////////////////////////////////////////
// Input

void AMyProjectTest2Character::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AMyProjectTest2Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyProjectTest2Character::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Rolling (Double-tap to trigger roll)
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this,
		                                   &AMyProjectTest2Character::OnSpaceBarPressed);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyProjectTest2Character::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyProjectTest2Character::Look);

		// Sprinting (Held input)
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this,
		                                   &AMyProjectTest2Character::StartSprinting);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this,
		                                   &AMyProjectTest2Character::StopSprinting);

		// Sprinting (Held input)
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this,
		                                   &AMyProjectTest2Character::StartCrouching);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this,
		                                   &AMyProjectTest2Character::StopCrouching);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this,
		                                   &AMyProjectTest2Character::StartAttack);

		EnhancedInputComponent->BindAction(ToggleWeaponAction, ETriggerEvent::Triggered, this,
		                                   &AMyProjectTest2Character::ToggleWeapon);

		EnhancedInputComponent->BindAction(HealthAction, ETriggerEvent::Triggered, this,
		                                   &AMyProjectTest2Character::AddHealthInput);

		EnhancedInputComponent->BindAction(QuickAttackAction, ETriggerEvent::Triggered, this,
		                                   &AMyProjectTest2Character::QuickAttack);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this,
		                                   &AMyProjectTest2Character::StartAiming);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this,
		                                   &AMyProjectTest2Character::StopAiming);

		// Movement Direction Tracking
		EnhancedInputComponent->BindAction(IA_Forward, ETriggerEvent::Started, this,
		                                   &AMyProjectTest2Character::SetMovingForward);
		EnhancedInputComponent->BindAction(IA_Forward, ETriggerEvent::Completed, this,
		                                   &AMyProjectTest2Character::StopMovingForward);

		EnhancedInputComponent->BindAction(IA_Backward, ETriggerEvent::Started, this,
		                                   &AMyProjectTest2Character::SetMovingBackward);
		EnhancedInputComponent->BindAction(IA_Backward, ETriggerEvent::Completed, this,
		                                   &AMyProjectTest2Character::StopMovingBackward);

		EnhancedInputComponent->BindAction(IA_Right, ETriggerEvent::Started, this,
		                                   &AMyProjectTest2Character::SetMovingRight);
		EnhancedInputComponent->BindAction(IA_Right, ETriggerEvent::Completed, this,
		                                   &AMyProjectTest2Character::StopMovingRight);

		EnhancedInputComponent->BindAction(IA_Left, ETriggerEvent::Started, this,
		                                   &AMyProjectTest2Character::SetMovingLeft);
		EnhancedInputComponent->BindAction(IA_Left, ETriggerEvent::Completed, this,
		                                   &AMyProjectTest2Character::StopMovingLeft);

		EnhancedInputComponent->BindAction(RestartAction, ETriggerEvent::Triggered, this,
										   &AMyProjectTest2Character::RestartGame);

		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this,
										   &AMyProjectTest2Character::PauseGame);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error,
		       TEXT(
			       "'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."
		       ), *GetNameSafe(this));
	}
}

void AMyProjectTest2Character::Move(const FInputActionValue& Value)
{
	bIsStrafing = false;
	if (!IsRolling && !IsAttacking)
	{
		FVector2D MovementVector = Value.Get<FVector2D>();

		if (Controller != nullptr)
		{
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// Get forward vector (X) and right vector (Y)
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			// Combine them based on the input vector
			FVector Direction = ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X;

			// Normalize the direction vector
			MoveDirection = Direction.GetSafeNormal(); // Make sure it's a unit vector

			// Apply movement input
			AddMovementInput(ForwardDirection * GetCharacterMovement()->MaxWalkSpeed, MovementVector.Y);
			AddMovementInput(RightDirection * GetCharacterMovement()->MaxWalkSpeed, MovementVector.X);

			// Check if player is strafing (left or right)
			if (MovementVector.X > 0.5f)
			{
				if (!(MovementVector.Y > 0.5f))
				{
					bIsStrafing = true;
				}
			}
			else if (MovementVector.X < -0.5f)
			{
				if (!(MovementVector.Y > 0.5f))
				{
					bIsStrafing = true;
				}
			}
			
		}
	}
}

void AMyProjectTest2Character::BeginPlay()
{
	Super::BeginPlay();
	CurrentAttackNumber = 0;
	BowMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("bow")));
	SwordRightMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("mini_sword_right")));
	SwordLeftMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("mini_sword_left")));
	CrossbowMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("crossbow")));
	BowArrowMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("arrow_1")));
	HealingVialMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("HealingVial")));
	HealingAmbientLight = Cast<UPointLightComponent>(GetDefaultSubobjectByName(TEXT("VialAmbience")));

	HealingGlyphMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("HealingAura")));
	HealingGlyphLight = Cast<USpotLightComponent>(GetDefaultSubobjectByName(TEXT("HealingSpotLight")));
	
	SwordRightMeshInner = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("mini_sword_right_inner")));
	SwordLeftMeshInner = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("mini_sword_left_inner")));

	BowOnBackRef = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("BowOnBack")));
	ArrowInQuiver0 = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("arrow_in_quiver_0")));
	ArrowInQuiver1 = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("arrow_in_quiver_1")));
	ArrowInQuiver2 = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("arrow_in_quiver_2")));
	ArrowInQuiver3 = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("arrow_in_quiver_3")));

	CrossBowOnBeltRef = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("CrossBowBelt")));

	if (!BowMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("BowMesh is not assigned in Blueprint!"));
	}

	if (!SwordRightMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwordRightMesh is not assigned in Blueprint!"));
	}

	if (!SwordLeftMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwordLeftMesh is not assigned in Blueprint!"));
	}

	if (!CrossbowMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("CrossbowMesh is not assigned in Blueprint!"));
	}

	if (HealingGlyphMesh && HealingGlyphLight)
	{
		HealingGlyphMesh->SetVisibility(false);
		HealingGlyphLight->SetVisibility(false);
	}
    

	SwordRightMesh->SetVisibility(true);
	SwordLeftMesh->SetVisibility(true);
	CrossbowMesh->SetVisibility(false);
	BowMesh->SetVisibility(false);
	BowArrowMesh->SetVisibility(false);
	HealingVialMesh->SetVisibility(false);
	HealingAmbientLight->SetVisibility(false);

	// Initialize visibility for bow on back and quiver arrows
	if (BowOnBackRef) BowOnBackRef->SetVisibility(true);
	UpdateQuiverArrowsVisibility();

	// Get the CharacterMovementComponent and cast it to UCharacterMovementComponent*
	// UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(GetMovementComponent());
	//
	// if (CharacterMovement)
	// {
	// 	// Access the CapsuleComponent property
	// 	UCapsuleComponent* CapsuleComponent = CharacterMovement->GetPawnCapsuleComponent();
	//
	// 	if (CapsuleComponent)
	// 	{
	// 		// Modify the HalfHeight property to half the desired height
	// 		float NewHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight() / 2.0f;
	// 		CapsuleComponent->SetCapsuleHalfHeight(NewHalfHeight);
	// 	}
	// }
}

void AMyProjectTest2Character::UpdateQuiverArrowsVisibility()
{
	// Make sure all arrow components are valid
	if (!ArrowInQuiver0 || !ArrowInQuiver1 || !ArrowInQuiver2)
		return;

	// Update visibility based on arrow count
	if (Arrows >= 3)
	{
		ArrowInQuiver0->SetVisibility(true);
		ArrowInQuiver1->SetVisibility(true);
		ArrowInQuiver2->SetVisibility(true);
		ArrowInQuiver3->SetVisibility(true);
	}
	else if (Arrows == 2)
	{
		ArrowInQuiver0->SetVisibility(true);
		ArrowInQuiver1->SetVisibility(true);
		ArrowInQuiver2->SetVisibility(true);
		ArrowInQuiver3->SetVisibility(false);
	}
	else if (Arrows == 1)
	{
		ArrowInQuiver0->SetVisibility(true);
		ArrowInQuiver1->SetVisibility(false);
		ArrowInQuiver2->SetVisibility(false);
		ArrowInQuiver3->SetVisibility(false);
	}
	else
	{
		ArrowInQuiver0->SetVisibility(false);
		ArrowInQuiver1->SetVisibility(false);
		ArrowInQuiver2->SetVisibility(false);
		ArrowInQuiver3->SetVisibility(false);
	}
}

void AMyProjectTest2Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMyProjectTest2Character::Jump()
{
	if (Stamina > 0)
	{
		if (isCrouching)
		{
			WasCrouching = true;
			isCrouching = false;
		}
		// Only allow jumping if not already rolling
		if (GetCharacterMovement()->IsMovingOnGround() && !IsRolling && !IsAttacking)
		{
			Stamina -= JumpCost;
			JumpCooldownTimer = 0.0f;
			isJumping = true;
			DeactivateHealingGlyph();
			// Get the direction the character is facing
			FVector ForwardDirection = GetActorForwardVector();

			// Define the jump force (upward force + direction force)
			FVector JumpForce = (ForwardDirection * (CachedSpeed/4.f)) + FVector(0.f, 0.f, 650.f);
			// 700 is the upward force, adjust as needed

			// Apply the force to the character
			GetCharacterMovement()->Launch(FVector(JumpForce));
			PauseStaminaRegeneration();
		}
	}
}

void AMyProjectTest2Character::StartSprinting()
{
	if (!bIsStrafing && !IsRolling && !isCrouching && !IsAttacking)
	{
		WasSprinting = true;
		// Increase the walk speed to a sprinting value
		GetCharacterMovement()->MaxWalkSpeed = 800.f; // Adjust the value as needed
		PauseStaminaRegeneration();
	}
}

void AMyProjectTest2Character::StopSprinting()
{
	// Reset the walk speed back to the normal walking speed
	WasSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = 500.f; // Adjust the value as needed
	PauseStaminaRegeneration();
}

void AMyProjectTest2Character::StartCrouching()
{
	if (!IsAttacking)
	{
		StopSprinting();
		isCrouching = true;
		GetCharacterMovement()->MaxWalkSpeed = 200.f;
		
	}
}

void AMyProjectTest2Character::StopCrouching()
{
	isCrouching = false;
	WasCrouching = false;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
}

void AMyProjectTest2Character::StartAiming()
{
	// Prevent aiming if falling
	if (bIsFalling)
	{
		return;
	}
	if (!IsAttacking && !IsRolling && Arrows > 0)
	{
		bIsAiming = true;
		AimTime = 0.0f;
		// When starting to aim, always switch to bow
		bIsBowEquipped = true;

		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->MaxWalkSpeed = 0.f;
	}
}

void AMyProjectTest2Character::StopAiming()
{
	bIsAiming = false;
	AimTime = 0.0f;
	FollowCamera->SetRelativeLocation(DefaultCameraPosition);
	FollowCamera->SetRelativeRotation(DefaultCameraRotation);
	bIsBowEquipped = false;
}


void AMyProjectTest2Character::Roll()
{
	if (Stamina > 0)
	{
		if (isCrouching)
		{
			WasCrouching = true;
			isCrouching = false;
		}
		RollFinished = false;
		bIsAiming = false;
		bCanAim = false;
		Stamina -= RollCost;
		// Start the rolling process
		IsRolling = true;
		isJumping = false;
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
		RollDuration = 0.0f; // Reset the frame counter when the roll starts
		// You can add more logic for roll behavior here (movement, animations, etc.)
		GetCharacterMovement()->MaxWalkSpeed = 700.f; // Set your desired roll speed
		bIsWaitingForDoubleTap = false;
		PauseStaminaRegeneration();
	}
}

void AMyProjectTest2Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if (GetCharacterMovement()->IsMovingOnGround())
	// {
	// 	bCanJump = true;
	// }

	if (FindNearestEliteBoss())
	{
		ReachOfJudgement(FVector::Dist(GetActorLocation(), EliteBoss->GetActorLocation()), DeltaTime);
	}
	// Check if the elite boss is within the reach of judgement
	
	// Regenerate arrows
	RegenerateInventory(DeltaTime);
	
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		isJumping = false;
		bCanJump = true;
		//bIsWaitingForDoubleTap = false;
		JumpFrameCounter = 0; // Reset the jump frame counter when the character stops jumping
		//JumpTimeCounter = 0.0f;
		bIsWaitingForDoubleTap = true;
	}

	if (bIsDead)
	{
		DeathTime += DeathTime;
	}

	if(!bIsAiming)
	{
		bHasPlayedBowAimSound = false;
		if (BowPullbackAudio && BowPullbackAudio->IsPlaying())
		{
			BowPullbackAudio->Stop();
			BowPullbackAudio = nullptr; // Reset for the next use
			bHasPlayedBowAimSound = false; // Reset flag so it can play again
		}
	}
	
	if (CachedSpeed > 0.f && !IsAttacking && !IsRolling && !isJumping)
	{

		// Calculate the time interval between footsteps based on the character's speed
		float FootstepInterval = FMath::Clamp(100.f / CachedSpeed, 0.f, 1.0f); // Adjust the range as needed
		FootstepInterval += .2f;
		
		FootstepTimer += DeltaTime;

		if (FootstepTimer >= FootstepInterval)
		{
			//UGameplayStatics::PlaySound2D(this, FootstepSound, 1.0f, 1.0f);
			UGameplayStatics::PlaySoundAtLocation(
				this,           // World context object
				WalkingSound,// Sound to play
				GetActorLocation(), // Location to play sound
				1.0f,           // Volume multiplier
				1.0f,           // Pitch multiplier
				0.0f,           // Start time
				nullptr,        // Attenuation settings
				nullptr         // Concurrency settings
			);
		
			FootstepTimer = 0.0f;
		 }
	}
	else
	{
		FootstepTimer = 0.0f;
	}

	if (CurrentDisplayHealth <= (MaxHealth * 0.8f))
	{

		// Calculate the time interval between footsteps based on the character's speed
		float HeartInterval = FMath::Clamp(MaxHealth / CurrentDisplayHealth, 0.f, 1.0f); // Adjust the range as needed
		HeartInterval -= .3f;
		
		HeartTimer += DeltaTime;

		if (HeartTimer >= HeartInterval)
		{
			//UGameplayStatics::PlaySound2D(this, FootstepSound, 1.0f, 1.0f);
			float HealthPercentage = CurrentDisplayHealth / MaxHealth;
			float VolumeMultiplier = FMath::Lerp(2.0f, 0.5f, HealthPercentage);

			UGameplayStatics::PlaySoundAtLocation(
				this,           // World context object
				HeartSound,     // Sound to play
				GetActorLocation(), // Location to play sound
				VolumeMultiplier,   // Volume multiplier (will be louder at lower health)
				1.0f,           // Pitch multiplier
				0.0f,           // Start time
				nullptr,        // Attenuation settings
				nullptr         // Concurrency settings
			);
		
			HeartTimer = 0.0f;
		}
	}
	else
	{
		HeartTimer = 0.0f;
	}

	CrossBowOnBeltRef->SetVisibility(!bInQuickAttack);
	
	if (bIsStaminaRegenPaused)
	{
		StaminaRegenPauseTimer += DeltaTime;
		if (StaminaRegenPauseTimer >= StaminaRegenCooldown)
		{
			bIsStaminaRegenPaused = false;
		}
	}
	//WasJumping = isJumping;
	
	if (FMath::IsNearlyEqual(CurrentDisplayHealth, 0.f, 0.5f))
	{
		HandleDeath();
	}
	if (bInQuickAttack)
	{
		QuickAttackCurrentTime += DeltaTime;
        
		// Cap the progress at 1.0 (100%)
		if (QuickAttackCurrentTime >= QuickAttackTotalDuration)
		{
			QuickAttackCurrentTime = QuickAttackTotalDuration;
		}
	}
	
	if (bIsHealthLerping)
	{
		CurrentDisplayHealth = FMath::FInterpTo(CurrentDisplayHealth, TargetHealth, DeltaTime, HealthLerpSpeed);
        
		// If we're close enough to the target, snap to it and stop lerping
		if (FMath::IsNearlyEqual(CurrentDisplayHealth, TargetHealth, 0.1f))
		{
			CurrentDisplayHealth = TargetHealth;
			bIsHealthLerping = false;
		}
	}
	
	if (bIsHealingGlyphActive && bIsHealingGlyphFading)
	{
		HealingGlyphTimer += DeltaTime;
		float FadeProgress = FMath::Clamp(HealingGlyphTimer / HealingGlyphFadeOutDuration, 0.0f, 1.0f);
        
		if (HealingGlyphLight)
		{
			// Gradually reduce light intensity
			float NewIntensity = FMath::Lerp(HealingGlyphInitialIntensity, 0.0f, FadeProgress);
			HealingGlyphLight->SetIntensity(NewIntensity);
		}
        
		// You can also fade the mesh opacity if needed
		if (HealingGlyphMesh)
		{
			// Create a dynamic material instance if needed
			UMaterialInterface* Material = HealingGlyphMesh->GetMaterial(0);
			if (Material)
			{
				UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);
				if (!DynamicMaterial)
				{
					DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
					HealingGlyphMesh->SetMaterial(0, DynamicMaterial);
				}
                
				if (DynamicMaterial)
				{
					// Assuming the material has an opacity parameter
					// You might need to adjust this based on your material setup
					DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f - FadeProgress);
				}
			}
		}
	}
	UpdateHealingGlyph(DeltaTime);
	UpdateWeaponVisibility();
	
	if (bIsAiming)
	{
		if (AimTime < 5.0f)
		{
			GetCharacterMovement()->MaxWalkSpeed = 0.f;
		}
		else
		{
			GetCharacterMovement()->MaxWalkSpeed = 250.f;
		}
	}
	else
	{
		// Check if sprinting
		if (WasSprinting && !IsRolling && !isCrouching && !IsAttacking)
		{
			GetCharacterMovement()->MaxWalkSpeed = 800.f;  // Use sprint speed
		}
		else if (isCrouching)
		{
			GetCharacterMovement()->MaxWalkSpeed = 250.f;
		}
		else
		{
			GetCharacterMovement()->MaxWalkSpeed = 500.f;  // Use normal speed
		}
	}


	// Don't process movement or actions if in damage state
	if (bIsInDamageState)
	{
		return;
	}

	// Check if the character is falling
	if (GetCharacterMovement())
	{
		bIsFalling = GetCharacterMovement()->IsFalling();
	}

	// Arrow shooting cooldown
	if (!bCanShootArrow)
	{
		ArrowCooldownTimer += DeltaTime;
		if (ArrowCooldownTimer >= ArrowCooldownDuration)
		{
			bCanShootArrow = true;
		}
	}


	if (GetCharacterMovement())
	{
		CachedVelocity = GetCharacterMovement()->Velocity;
		CachedSpeed = CachedVelocity.Size();
		bIsFalling = GetCharacterMovement()->IsFalling();
	}

	// Arrow shooting cooldown
	if (!bCanShootArrow)
	{
		ArrowCooldownTimer += DeltaTime;
		if (ArrowCooldownTimer >= ArrowCooldownDuration)
		{
			bCanShootArrow = true;
		}
	}

	if (bIsStaminaRegenPaused)
	{
		StaminaRegenPauseTimer += DeltaTime;
		if (StaminaRegenPauseTimer >= StaminaRegenCooldown)
		{
			bIsStaminaRegenPaused = false;
		}
	}

	if (Stamina <= 0 && !StaminaDepletion)
	{
		PauseStaminaRegeneration();
		Stamina = 0;
		StaminaZeroTimer = 0.0f;
		StaminaDepletion = true;
		StaminaRegenRate = BaseStaminaRegenRate; // Reset regen rate when stamina is depleted
    
		// Reduce movement speed when stamina is depleted
		if (!IsRolling && !bIsAiming) // Don't override special movement states
		{
			GetCharacterMovement()->MaxWalkSpeed = 125.f; // Reduced speed when stamina is drained
		}
	}
	// Stamina regeneration
	if (Stamina > 0 && !StaminaDepletion && !bIsStaminaRegenPaused)
	{
		// Increase regeneration rate over time
		StaminaRegenRate = FMath::Min(StaminaRegenRate + (DeltaTime * StaminaRegenAcceleration), MaxStaminaRegenRate);
    
		// Apply the current regeneration rate
		Stamina += DeltaTime * StaminaRegenRate;
		Stamina = FMath::Clamp(Stamina, 0.f, MaxStamina);
    
		// Restore normal movement speed if it was reduced due to stamina depletion
		if (GetCharacterMovement()->MaxWalkSpeed == 125.f && !IsRolling && !bIsAiming && !WasSprinting && !isCrouching)
		{
			GetCharacterMovement()->MaxWalkSpeed = 500.f; // Restore to normal speed
		}
	}
	else if (StaminaDepletion)
	{
		StaminaZeroTimer += DeltaTime;
		if (StaminaZeroTimer >= StaminaRecoveryDelay)
		{
			Stamina = 1.f;
			StaminaDepletion = false;
			StaminaRegenRate = BaseStaminaRegenRate; // Reset to base rate when recovery begins
        
			// Restore normal movement speed when stamina starts regenerating
			if (GetCharacterMovement()->MaxWalkSpeed == 125.f && !IsRolling && !bIsAiming && !WasSprinting && !isCrouching)
			{
				GetCharacterMovement()->MaxWalkSpeed = 500.f; // Restore to normal speed
			}
		}
	}

	if (WasSprinting && !IsRolling)
	{
		Stamina -= SprintCost;
		if (Stamina < SprintCost)
		{
			StopSprinting();
		}
	}

	FramesSinceLastJump++;

	JumpCooldownTimer += DeltaTime;
	AttackCooldownTimer += DeltaTime;

	if (isJumping)
	{
		JumpFrameCounter++;
	}

	if (IsAttacking)
	{
		if (AttackTimeCounter < AttackDuration * 0.3f)
		{
			FVector AttackDirection = GetActorForwardVector();
			AttackDirection.Normalize();
			FVector AttackMovement = AttackDirection * 100.f * DeltaTime;
			GetCharacterMovement()->MoveUpdatedComponent(AttackMovement, GetActorRotation(), true);
		}


		AttackTimeCounter += DeltaTime;
		if (AttackTimeCounter >= AttackDuration)
		{
			StopAttack();
			AttackFinished = true;
		}
	}

	// If the character is rolling, increment the frame counter
	if (IsRolling)
	{
		RollDuration += DeltaTime;

		// Ensure movement direction is valid, defaulting to forward if no input
		FVector RollDirection = MoveDirection.IsNearlyZero() ? GetActorForwardVector() : MoveDirection;

		// Normalize before modifying it
		RollDirection.Normalize();

		// Rotate the player to face the roll direction
		FRotator RollRotation = RollDirection.Rotation();
		//SetActorRotation(FRotator(0.f, RollRotation.Yaw, 0.f)); // Keep only yaw rotation to avoid tilting

		// Define roll speed and lift (adjust as needed)
		RollDirection = RollDirection * 1100.f + FVector(0.f, 0.f, 200.f);

		// Normalize again to maintain consistent speed
		RollDirection.Normalize();

		// Calculate roll movement
		FVector RollMovement = RollDirection * GetCharacterMovement()->MaxWalkSpeed * DeltaTime;

		// Apply movement
		GetCharacterMovement()->MoveUpdatedComponent(RollMovement, GetActorRotation(), true);

		// Stop rolling after reaching duration
		if (RollDuration >= MaxRollDuration)
		{
			IsRolling = false;
			StopRolling();
			RollFinished = true;
		}
	}

	if (bIsAiming)
	{
		AimTime += 0.15;
		AimBow();
	}
	else
	{
		StopAiming();
	}
	if (!bIsAiming) // Ensure this only runs when not aiming
	{
		// Get the neck socket world location
		FVector NeckSocketLocation = GetMesh()->GetSocketLocation("NeckSocket");

		// Calculate direction to the neck socket (from the camera)
		FVector DirectionToNeck = NeckSocketLocation - FollowCamera->GetComponentLocation();
		DirectionToNeck.Normalize();

		// Calculate the rotation to face the neck socket
		FRotator TargetRotation = DirectionToNeck.Rotation();

		// Smoothly interpolate the camera's rotation to face the neck
		FollowCamera->SetWorldRotation(TargetRotation); // Adjust interpolation speed
	}
	

} 

void AMyProjectTest2Character::StopRolling()
{
	// Reset the velocity to normal movement after the roll stops
	//GetCharacterMovement()->Velocity = FVector::ZeroVector;

	// Optionally, reset the character's speed or state here
	// Adjust the walk speed as needed
	if (WasCrouching)
	{
		WasCrouching = false;
		StartCrouching();
	}

	if (WasSprinting)
	{
		WasSprinting = false;
		StartSprinting();
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = 500.f;
	}

	if (IsAttacking)
	{
		AttackCooldownTimer = 0.0f;
		AttackTimeCounter = 0.0f;
		IsAttacking = true;
	}

	bCanAim = true;
}

void AMyProjectTest2Character::OnSpaceBarPressed()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeSinceLastPress = CurrentTime - LastSpaceBarPressTime;
	float TimeSinceLastRoll = CurrentTime - LastRollTime;

	// Very short cooldown between rolls (e.g., 0.2 seconds)
	const float RollCooldown = 0.2f;

	// Check if we're in a state where we can initiate a jump or roll
	bool canPerformAction = !IsRolling && !IsAttacking && !bIsInDamageState;

	if (canPerformAction)
	{
		if (GetCharacterMovement()->IsMovingOnGround())
		{
			// On ground: Jump on first press, roll on quick double-tap
			if (TimeSinceLastPress <= DoubleTapThreshold && TimeSinceLastRoll >= RollCooldown)
			{
				Roll();
				LastRollTime = CurrentTime;
			}
			else
			{
				Jump();
				isJumping = true;
			}
		}
		else if (GetCharacterMovement()->IsFalling() && TimeSinceLastRoll >= RollCooldown)
		{
			// In air: Always roll if cooldown has passed
			Roll();
			LastRollTime = CurrentTime;
		}
	}

	LastSpaceBarPressTime = CurrentTime;
}

void AMyProjectTest2Character::StartAttack()
{
	// Prevent attacking if falling
	if (bIsFalling && !isJumping)
	{
		return;
	}
	if (IsAttacking || AttackCooldownTimer < AttackCooldownDuration + 0.1f || bIsInDamageState || IsRolling || !GotUp)
	{
		return; // Prevent attacking if already attacking or on cooldown
	}
	if (AttackFinished)
	{
		if (bIsAiming && AimTime >= 5.0f)
		{
			ShootBow();
			BowArrowMesh->SetVisibility(false);
		}
		else if (AttackCooldownTimer > AttackCooldownDuration && !IsAttacking && !bIsAiming)
		{
			AttackFinished = false;
			IsAttacking = true;
			StopJumping();
			//AttackCooldownTimer = 0.0f;
			//AttackTimeCounter = 0.0f;

			CheckForMeleeHits();
			IsAttacking = true;
			PauseStaminaRegeneration();

			AAI_Character* NearestEnemy = nullptr;
			float NearestDistanceSquared = FLT_MAX;  // Start with the largest possible distance

			// Find the nearest enemy AI character
			for (TActorIterator<AAI_Character> It(GetWorld()); It; ++It)
			{
				AAI_Character* Enemy = *It;

				if (Enemy && !Enemy->bIsDead)
				{
					// Calculate the squared distance to the enemy once
					float DistanceSquared = (GetActorLocation() - Enemy->GetActorLocation()).SizeSquared();

					if (DistanceSquared < NearestDistanceSquared)
					{
						NearestEnemy = Enemy;
						NearestDistanceSquared = DistanceSquared;
					}
				}
			}

			if (NearestEnemy)
			{
				// Calculate direction to the nearest enemy
				FVector DirectionToEnemy = NearestEnemy->GetActorLocation() - GetActorLocation();
				DirectionToEnemy.Normalize();

				// Calculate the rotation to face the enemy
				FRotator TargetRotation = DirectionToEnemy.Rotation();

				// Set the character's rotation to face the enemy directly (no interpolation)
				SetActorRotation(TargetRotation);
			}

			CurrentAttackNumber = FMath::RandRange(0, 2);
		}
	}
}

void AMyProjectTest2Character::StopAttack()
{
	AttackCooldownTimer = 0.0f;
	AttackTimeCounter = 0.0f;
	IsAttacking = false;
	AttackFinished = true;
}

void AMyProjectTest2Character::ShootBow()
{
    if (!ProjectileClass || !bCanShootArrow)
    {
        return;
    }

    // Set cooldown to prevent rapid firing
    bCanShootArrow = false;
    ArrowCooldownTimer = 0.0f;

    // Get the socket transform from the left hand (where the bow is held)
    FTransform SocketTransform = GetMesh()->GetSocketTransform("LeftHandSocketAim", RTS_World);

    // Get the camera's location and forward vector
    FVector CameraLocation = FollowCamera->GetComponentLocation();
    // Get the player controller to access screen dimensions
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
    {
        return;
    }

	FVector2D CrosshairScreenPos = FVector2D(0.5f, 0.5f); // Normalized screen position of the crosshair
	float SearchAreaWidth = 50.5f;  
	float SearchAreaHeight = 50.5f;

	// Calculate search bounds relative to crosshair position
	FVector2D SearchAreaMin = CrosshairScreenPos - FVector2D(SearchAreaWidth, SearchAreaHeight);
	FVector2D SearchAreaMax = CrosshairScreenPos + FVector2D(SearchAreaWidth, SearchAreaHeight);

	AAI_Character* NearestEnemy = nullptr;
	float NearestDistanceSquared = FLT_MAX; 

	// Find the nearest enemy within the defined screen-space area
	for (TActorIterator<AAI_Character> It(GetWorld()); It; ++It)
	{
		AAI_Character* Enemy = *It;

		if (Enemy && !Enemy->bIsDead)
		{
			FVector2D EnemyScreenPos;
			FVector EnemyWorldPos = Enemy->GetActorLocation();
			if (UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), EnemyWorldPos, EnemyScreenPos))
			{
				if (EnemyScreenPos.X >= SearchAreaMin.X && EnemyScreenPos.X <= SearchAreaMax.X &&
					EnemyScreenPos.Y >= SearchAreaMin.Y && EnemyScreenPos.Y <= SearchAreaMax.Y)
				{
					float DistanceSquared = FVector2D::DistSquared(EnemyScreenPos, CrosshairScreenPos);

					if (DistanceSquared < NearestDistanceSquared)
					{
						NearestEnemy = Enemy;
						NearestDistanceSquared = DistanceSquared;
					}
				}
			}
		}
	}


	if (NearestEnemy)
	{
		// Convert crosshair screen position (offset included) to world position and direction
		FVector WorldLocation, WorldDirection;
		int32 ViewportSizeX, ViewportSizeY;
		FVector2D ScreenCenter((ViewportSizeX * 0.5f) + 50.f, (ViewportSizeY * 0.5f) - 120.f);
		PlayerController->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection);

		// Get the enemy's location
		FVector EnemyLocation = NearestEnemy->GetActorLocation();

		// Adjusted direction from the crosshair world location to the enemy
		FVector AdjustedDirection = (EnemyLocation - WorldLocation).GetSafeNormal();

		// Compute the new camera rotation to align the crosshair with the enemy
		FRotator TargetRotation = AdjustedDirection.Rotation();

		// Smoothly interpolate to the new rotation (prevents abrupt snapping)
		FRotator NewRotation = FMath::RInterpTo(FollowCamera->GetComponentRotation(), TargetRotation, GetWorld()->GetDeltaSeconds(), 30.0f);

		FollowCamera->SetWorldRotation(NewRotation);
	}
    // Get the viewport size
	int32 ViewportSizeX, ViewportSizeY;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

	// Apply the crosshair offset (as per your setup)
	FVector2D ScreenCenter((ViewportSizeX * 0.5f) + 0.f, (ViewportSizeY * 0.5f) - 118.f);

	// Convert screen position to world location and direction
	FVector WorldLocation, WorldDirection;
	PlayerController->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection);

	// Perform a line trace to find an enemy at the crosshair position
	FHitResult HitResult;
	FVector TraceEnd = WorldLocation + (WorldDirection * 50000.0f); // Extend trace far ahead
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignore self

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams);

	// Determine shooting direction
	FVector ShootDirection;
	if (bHit && HitResult.GetActor()) 
	{
		// If the line trace hits an enemy, aim directly at them
		FVector EnemyLocation = HitResult.ImpactPoint;
		ShootDirection = (EnemyLocation - SocketTransform.GetLocation()).GetSafeNormal();
	}
	else 
	{
		// If no enemy is hit, shoot in the default crosshair direction
		ShootDirection = WorldDirection;
	}

	// Spawn the projectile at the left-hand socket location
	AProjectile_Arrow_Base* Projectile = GetWorld()->SpawnActor<AProjectile_Arrow_Base>(
		ProjectileClass,
		SocketTransform.GetLocation(),
		ShootDirection.Rotation()
	);

    if (Projectile)
    {
    	LastArrowTime = 0.f;
    	UGameplayStatics::PlaySoundAtLocation(
					this,           // World context object
					BowReleaseSound,// Sound to play
					GetActorLocation(), // Location to play sound
					1.0f,           // Volume multiplier
					1.0f,           // Pitch multiplier
					0.0f,           // Start time
					nullptr,        // Attenuation settings
					nullptr         // Concurrency settings
				);
        Arrows--;
    	UpdateQuiverArrowsVisibility();
    	Projectile->SetDamage(100.f);
        // Set the velocity for the arrow
        float ArrowSpeed = 2500.f;
        FVector LaunchVelocity = ShootDirection * ArrowSpeed;

        // Apply velocity to the projectile
        Projectile->SetVelocity(LaunchVelocity);

        // Set this character as the instigator (for damage attribution)
        Projectile->SetInstigator(this);

        // If the projectile has a mesh with physics enabled, apply impulse
        if (Projectile->GetArrowMesh() && Projectile->GetArrowMesh()->IsSimulatingPhysics())
        {
            Projectile->GetArrowMesh()->AddImpulse(LaunchVelocity, NAME_None, true);
        }

    	// Set the shoot animation flag and start the timer
    	bDidShoot = true;
    	float ShootAnimationDuration = 0.15f;
    	GetWorldTimerManager().SetTimer(ShootAnimationTimerHandle, this, &AMyProjectTest2Character::ResetShootAnimation, ShootAnimationDuration, false);
    }

    bIsAiming = false;
    IsAttacking = false;
    AttackFinished = true;

    AttackCooldownTimer = 0.0f; // Reset the timer
    AttackCooldownDuration = 0.5f; // Set a short cooldown duration
}

void AMyProjectTest2Character::ResetShootAnimation()
{
    bDidShoot = false;
}


void AMyProjectTest2Character::AimBow()
{
    if (bIsAiming)
    {
        // Play the bow pullback sound only once and loop until the bow has been shot
        if (!bHasPlayedBowAimSound)
        {
            // Ensure we only play it once
            bHasPlayedBowAimSound = true;

            // Play the looping sound
        	BowPullbackAudio = UGameplayStatics::SpawnSoundAttached(
				BowpullbackSound,
				GetRootComponent(), // Attach it to the character
				NAME_None,
				FVector::ZeroVector,
				EAttachLocation::KeepRelativeOffset,
				true // Looping enabled
			);
        }

        FollowCamera->SetRelativeLocation(DefaultCameraPosition);
        FollowCamera->SetRelativeRotation(DefaultCameraRotation);

        FRotator NewRotation = FollowCamera->GetComponentRotation();
        NewRotation.Pitch = 0.0f; // Keep the character level on the ground
        SetActorRotation(NewRotation);

        FollowCamera->SetRelativeLocation(FVector(50.f, 70.f, 0.f));

        // Get target locations (where the character is aiming)
        FVector TargetLocation = GetMesh()->GetSocketLocation("LeftHandSocketAim");
        FVector ArrowLocation = GetMesh()->GetSocketLocation("ArrowSocket");

        // Calculate direction to the target
        FVector DirectionToTarget = TargetLocation - ArrowLocation;

        // Clamp the camera rotation
        float MinPitch = -89.0f; // Minimum angle (looking down)
        float MaxPitch = 89.0f; // Maximum angle (looking up)

        FRotator TargetRotation = DirectionToTarget.Rotation();
        TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, MinPitch, MaxPitch);
        TargetRotation.Roll = 0.0f; // Lock Roll to prevent unwanted tilting

        // Smoothly interpolate to the new rotation
        FRotator SmoothRotation = FMath::RInterpTo(FollowCamera->GetComponentRotation(), TargetRotation,
                                                   GetWorld()->GetDeltaSeconds(), 0.5f);
        FollowCamera->SetWorldRotation(SmoothRotation);

        // Aim blend calculation using the **clamped** Pitch
        float ClampedPitch = FMath::Clamp(SmoothRotation.Pitch, MinPitch, MaxPitch);
        ClampedPitch += 90;
        ClampedPitch /= 180;

        AimBlend = FMath::Clamp(ClampedPitch, 0.05f, 0.95f) * 100.0f;

        if (BowArrowMesh && BowMesh && AimTime >= 5.0f) // Ensure both are valid
        {
            // Apply the rotation to the arrow mesh
            BowArrowMesh->SetWorldRotation(DirectionToTarget.Rotation());
        }
    }
}

void AMyProjectTest2Character::RegenerateInventory(float DeltaTime)
{

	if (MaxArrows > Arrows)
	{
		LastArrowTime += DeltaTime;
	}
	if (MaxCrossbowArrows > Crossbow_arrows)
	{
		LastCrossbowTime += DeltaTime;
	}
	if (MaxHealVials > Health_vials)
	{
		LastVialTime += DeltaTime;
	}
	
	if (MaxArrows > Arrows && LastArrowTime > LastArrowTimeGoal)
	{
		Arrows = FMath::Clamp(Arrows + 1, 0.0, MaxArrows);
		LastArrowTime = 0.f;

		UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			BowRestockSound,// Sound to play
			GetActorLocation(), // Location to play sound
			4.5f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
	}

	// Regenerate crossbow arrows
	if (MaxCrossbowArrows > Crossbow_arrows && LastCrossbowTime > LastCrossbowTimeGoal)
	{
		Crossbow_arrows = FMath::Clamp(Crossbow_arrows + 1, 0.0f, MaxCrossbowArrows);
		LastCrossbowTime = 0.f;
		UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			CrossbowRestockSound,// Sound to play
			GetActorLocation(), // Location to play sound
			2.5f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
	}

	// Regenerate heal vials
	if (MaxHealVials > Health_vials && LastVialTime > LastVialTimeGoal)
	{
		Health_vials = FMath::Clamp(Health_vials + 1, 0.0f, MaxHealVials);
		LastVialTime = 0.f;
		UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			VialRestockSound,// Sound to play
			GetActorLocation(), // Location to play sound
			0.4f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
	}
}

bool AMyProjectTest2Character::FindNearestEliteBoss()
{
	AAI_Elite* FoundBoss = Cast<AAI_Elite>(UGameplayStatics::GetActorOfClass(GetWorld(), AAI_Elite::StaticClass()));

	if (FoundBoss)
	{
		EliteBoss = FoundBoss;
		if (EliteBoss->Health > 0.0f)
		{
			return true;
		}
	}
	return false;
}


void AMyProjectTest2Character::SetMovingForward() { IsMovingForwad = true; }
void AMyProjectTest2Character::StopMovingForward() { IsMovingForwad = false; }

void AMyProjectTest2Character::SetMovingBackward() { IsMovingBackward = true; }
void AMyProjectTest2Character::StopMovingBackward() { IsMovingBackward = false; }

void AMyProjectTest2Character::SetMovingRight() { IsMovingRight = true; }
void AMyProjectTest2Character::StopMovingRight() { IsMovingRight = false; }

void AMyProjectTest2Character::SetMovingLeft() { IsMovingLeft = true; }
void AMyProjectTest2Character::StopMovingLeft() { IsMovingLeft = false; }

void AMyProjectTest2Character::OnCapsuleOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// Ensure that the overlapping actor is valid and is not ourself.
	if (OtherActor != this && Health > 0.0f)
	{
		// Set the damage amount (adjust as needed)
		float Damage = 10.0f;

		// Subtract the damage from Health
		Health -= Damage;

		UE_LOG(LogTemplateCharacter, Log, TEXT("Collision with %s: Damage = %f, New Health = %f"),
		       *OtherActor->GetName(), Damage, Health);

		// If Health drops to zero or below, handle death
		if (Health <= 0.0f)
		{
			//HandleDeath();
		}
	}
}

void AMyProjectTest2Character::ToggleWeapon()
{
	// Only allow weapon switching when not in special states
	if (!IsAttacking && !IsRolling && !bIsAiming && !bInQuickAttack)
	{
		// Toggle between bow and swords
		bIsBowEquipped = !bIsBowEquipped;
        
		// Update weapon visibility based on new state
		UpdateWeaponVisibility();
	}
}
// Update the weapon visibility function
void AMyProjectTest2Character::UpdateWeaponVisibility()
{
    // Make sure all weapon meshes are valid before accessing them
    //if (BowMesh && SwordRightMesh && SwordLeftMesh && CrossbowMesh)
    {
        if (bIsSmashingVial)
        {
            CrossbowMesh->SetVisibility(false);
            BowMesh->SetVisibility(false);
            SwordRightMesh->SetVisibility(false);
            SwordLeftMesh->SetVisibility(false);
            BowArrowMesh->SetVisibility(false);
            if (BowOnBackRef) BowOnBackRef->SetVisibility(false);

            // Also hide inner sword meshes
            if (SwordRightMeshInner) SwordRightMeshInner->SetVisibility(false);
            if (SwordLeftMeshInner) SwordLeftMeshInner->SetVisibility(false);

            HealingVialMesh->SetVisibility(true);
            HealingAmbientLight->SetVisibility(true);
            return;
        }
        else
        {
            HealingVialMesh->SetVisibility(false);
            HealingAmbientLight->SetVisibility(false);
        }

        // Show the appropriate weapon based on state
        if (bInQuickAttack)
        {
            // In quick attack mode, only show crossbow
            CrossbowMesh->SetVisibility(true);
            
            BowMesh->SetVisibility(false);
            SwordRightMesh->SetVisibility(false);
            SwordLeftMesh->SetVisibility(false);
            BowArrowMesh->SetVisibility(false);
            if (BowOnBackRef) BowOnBackRef->SetVisibility(true);
            
            // Hide inner sword meshes
            if (SwordRightMeshInner) SwordRightMeshInner->SetVisibility(false);
            if (SwordLeftMeshInner) SwordLeftMeshInner->SetVisibility(false);
            return;
        }
        if (bIsAiming || bIsBowEquipped)	
        {
            BowMesh->SetVisibility(true);
            if (Arrows > 0)
            {
                BowArrowMesh->SetVisibility(true);
            }
            //bIsBowEquipped = true;

            CrossbowMesh->SetVisibility(false);
            SwordRightMesh->SetVisibility(false);
            SwordLeftMesh->SetVisibility(false);
            if (BowOnBackRef) BowOnBackRef->SetVisibility(false);
            
            // Hide inner sword meshes
            if (SwordRightMeshInner) SwordRightMeshInner->SetVisibility(false);
            if (SwordLeftMeshInner) SwordLeftMeshInner->SetVisibility(false);
            return;
        }
        else
        {
            // Normal state: show either bow or swords based on equipped stat
            SwordRightMesh->SetVisibility(true);
            SwordLeftMesh->SetVisibility(true);
            
            // Set inner sword meshes to match their counterparts
            if (SwordRightMeshInner) SwordRightMeshInner->SetVisibility(SwordRightMesh->IsVisible());
            if (SwordLeftMeshInner) SwordLeftMeshInner->SetVisibility(SwordLeftMesh->IsVisible());
            
            CrossbowMesh->SetVisibility(false);
            BowMesh->SetVisibility(false);
            BowArrowMesh->SetVisibility(false);
            if (BowOnBackRef) BowOnBackRef->SetVisibility(true);
        }
        
        // Update quiver arrows visibility
        UpdateQuiverArrowsVisibility();
    }
}

float AMyProjectTest2Character::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (isJumping || IsRolling) return -1.0f;
	
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!bIsDead)
	{
		UGameplayStatics::PlaySoundAtLocation(
				this,           // World context object
				TakeDamageSound,// Sound to play
				GetActorLocation(), // Location to play sound
				1.0f,           // Volume multiplier
				1.0f,           // Pitch multiplier
				0.0f,           // Start time
				nullptr,        // Attenuation settings
				nullptr         // Concurrency settings
			);
	}
	
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
    
	if (ActualDamage > 0.0f)
	{
		// Update the actual health value
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
        
		// Set the target for lerping
		TargetHealth = Health;
		bIsHealthLerping = true;

		//if (Health > 0.0f)
		{
			// Calculate stun duration based on damage amount
			// More damage = longer stun, but capped at a maximum
			float StunDuration = FMath::Clamp(ActualDamage / 20.0f, 0.2f, 1.0f);
			if (!bIsInDamageState)
			{
				EnterDamageState(StunDuration);
			}
		}
        
        
		// Handle death if needed
		if (Health <= 0.0f)
		{
			//HandleDeath();
		}
	}
    
	return ActualDamage;
}

// Add a getter for the display health that UI can use
float AMyProjectTest2Character::GetDisplayHealth() const
{
	return CurrentDisplayHealth;
}

void AMyProjectTest2Character::HandleDeath()
{
	if (bIsDead) return;
	UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			DeathSound, // Sound to play
			GetActorLocation(), // Location to play sound
			1.0f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
	// Prevent further input and movement
	bIsDead = true;
	IsAttacking = false;
	IsRolling = false;
	bIsAiming = false;

	// Disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	// Disable input for the player
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		//DisableInput(PC);
	}

	UE_LOG(LogTemplateCharacter, Warning, TEXT("%s is dead!"), *GetName());

	// Set timer to enable ragdoll after delay (allows death animation to play first)
	GetWorldTimerManager().SetTimer(
		RagdollTimerHandle,
		this,
		&AMyProjectTest2Character::EnableRagdoll,
		2.0f,
		false);
        
	// Set timer to restart the game after 5 seconds
	// GetWorldTimerManager().SetTimer(
	// 	RestartGameTimerHandle,
	// 	this,
	// 	&AMyProjectTest2Character::RestartGame,
	// 	5.0f,
	// 	false);
}

void AMyProjectTest2Character::CheckForMeleeHits()
{
	// Define the attack range
	float AttackRange = 140.0f;

	// Get the player's location
	FVector PlayerLocation = GetActorLocation();

	// Get the player's forward direction
	FVector ForwardDirection = GetActorForwardVector();

	// Define a sphere to check for overlapping actors
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignore self


	// Perform the overlap check in front of the player
	FVector SphereCenter = PlayerLocation + (ForwardDirection * (AttackRange / 2.0f));
	bool bHasOverlaps = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		SphereCenter,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(AttackRange),
		QueryParams
	);

	// Track actors that have been hit in this attack to avoid hitting them multiple times
	static TArray<AActor*> AlreadyHitActors;

	// Clear the hit actors array when starting a new attack
	if (AttackTimeCounter == 0.0f)
	{
		AlreadyHitActors.Empty();
	}

	// If we found any overlapping actors
	if (bHasOverlaps)
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AActor* HitActor = Overlap.GetActor())
			{
				// Check if the hit actor is an AI character
				// and hasn't been hit yet in this attack
				if (HitActor != this && !AlreadyHitActors.Contains(HitActor))
				{
					// Add to the list of already hit actors
					AlreadyHitActors.Add(HitActor);

					// Calculate damage based on your game's rules
					float DamageAmount = 80.0f; // Example damage value
					LastArrowTime += LastArrowTimeGoal / 4.0f;
					LastCrossbowTime += LastCrossbowTimeGoal / 4.0f;
					LastVialTime += LastVialTimeGoal / 8.0f;

					// Apply damage to the hit actor
					HitActor->TakeDamage(DamageAmount, FDamageEvent(), GetController(), this);
					UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			MeleeHitSound,// Sound to play
			GetActorLocation(), // Location to play sound
			0.5f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
				}
			}
		}
	}

	// Play swing sound regardless of whether we hit anything
	if (MeleeSwingSound && AttackTimeCounter == 0.0f)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,           // World context object
			MeleeSwingSound,// Sound to play
			GetActorLocation(), // Location to play sound
			1.0f,           // Volume multiplier
			1.0f,           // Pitch multiplier
			0.0f,           // Start time
			nullptr,        // Attenuation settings
			nullptr         // Concurrency settings
		);
	}
	
}

void AMyProjectTest2Character::EnableRagdoll()
{
	// No need to check health again since this is only called after death
	// Disable capsule collision
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	// Get the mesh component
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp)
	{
		// Detach from capsule to allow the mesh to fall freely
		MeshComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

		// Enable physics and set to ragdoll
		MeshComp->SetSimulatePhysics(true);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // Let players walk through
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Don't block camera

		// Make sure all bones can be affected by physics
		MeshComp->SetAllBodiesBelowSimulatePhysics("root", true, true);

		// Apply a small impulse to make the ragdoll fall more naturally
		MeshComp->AddImpulse(GetActorForwardVector() * 1000.0f, NAME_None, true);
	}
	
	UE_LOG(LogTemplateCharacter, Warning, TEXT("Player ragdoll enabled"));
}

void AMyProjectTest2Character::SetHealth(float NewHealth)
{
	// Clamp the health value between 0 and max health
	NewHealth = FMath::Clamp(NewHealth, 0.0f, 100.0f);
    
	// Set the target health
	TargetHealth = NewHealth;
	Health = NewHealth;
    
	// Start the health lerping
	bIsHealthLerping = true;
}

void AMyProjectTest2Character::AddHealth(float HealthAmount)
{
	if (Health <= 0)
	{
		//HandleDeath();
		return;
	}
	// Calculate new health value
	float NewHealth = Health + HealthAmount;
    
	// Set the health with lerping
	SetHealth(NewHealth);
}

void AMyProjectTest2Character::AddHealthInput(const FInputActionValue& Value)
{
	if (Health_vials <= 0)
	{
		return;
	}
	// Check if player is in any state that should prevent vial use
	if (IsAttacking || IsRolling || bInQuickAttack || bInQuickAttackFall || bIsFalling || bIsInDamageState || !bCanThrowVial)
	{
		// Player is in an action state that prevents vial use
		return;
	}

	// If player is aiming, stop aiming first
	if (bIsAiming)
	{
		StopAiming();
	}

	if (GetWorld()->GetTimeSeconds() - LastHealthAddTime >= HealthAddCooldown)
	{ 
		// Add health and update the timestamp
		LastHealthAddTime = GetWorld()->GetTimeSeconds();
	}
	bCanThrowVial = false;
	StartVialSmashAnimation();
}

void AMyProjectTest2Character::StartVialSmashAnimation()
{
	
    // Set the animation blueprint variable to true
    bIsSmashingVial = true;
    
    // Store original movement capability to restore later
    StoredMovementMode = GetCharacterMovement()->MovementMode;
    StoredMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
    
    // Disable movement during vial smash
    GetCharacterMovement()->DisableMovement();
    
    // Hide all weapons during vial smash
    if (BowMesh) BowMesh->SetVisibility(false);
    if (SwordRightMesh) SwordRightMesh->SetVisibility(false);
    if (SwordLeftMesh) SwordLeftMesh->SetVisibility(false);
    if (CrossbowMesh) CrossbowMesh->SetVisibility(false);
    if (BowArrowMesh) BowArrowMesh->SetVisibility(false);
    
    // Show the vial mesh
    if (HealingVialMesh) HealingVialMesh->SetVisibility(true);
    if (HealingAmbientLight) HealingAmbientLight->SetVisibility(true);
    
    // Set a timer to throw the vial after a short delay (during animation)
    GetWorldTimerManager().SetTimer(
        VialThrowTimerHandle,
        this,
        &AMyProjectTest2Character::ThrowVial,
        0.2f,  // Throw vial 0.2 seconds into the animation
        false
    );
    
    // Set a timer to end the animation after the full duration
    float VialSmashAnimationDuration = 0.3f;
    GetWorldTimerManager().SetTimer(
        VialSmashAnimationTimerHandle,
        this,
        &AMyProjectTest2Character::EndVialSmashAnimation,
        VialSmashAnimationDuration,
        false
    );
}

void AMyProjectTest2Character::ThrowVial()
{
	UGameplayStatics::PlaySoundAtLocation(
				this,           // World context object
				VialThrowSound,// Sound to play
				GetActorLocation(), // Location to play sound
				1.0f,           // Volume multiplier
				1.0f,           // Pitch multiplier
				0.0f,           // Start time
				nullptr,        // Attenuation settings
				nullptr         // Concurrency settings
			);
	Health_vials--;
	HealingGlyphTimer = 0.0f;
	HealingGlyphInitialIntensity = 0.0f;
    // Spawn a simple vial static mesh that falls to the ground
    UStaticMesh* VialMesh = nullptr;
    
    // Try to get the same mesh that's used for the HealingVialMesh component
    if (HealingVialMesh && HealingVialMesh->GetStaticMesh())
    {
        VialMesh = HealingVialMesh->GetStaticMesh();
    }
    else
    {
        // Fallback to a default mesh if available
        static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultVialMeshFinder(TEXT("/Game/Meshes/SM_HealingVial"));
        if (DefaultVialMeshFinder.Succeeded())
        {
            VialMesh = DefaultVialMeshFinder.Object;
        }
    }
    
    if (VialMesh)
    {
        // Get the hand socket location
        FVector SpawnLocation = GetMesh()->GetSocketLocation("RightHandSocket");
        
        // Create a new static mesh component
        UStaticMeshComponent* FallingVial = NewObject<UStaticMeshComponent>(this);
        FallingVial->RegisterComponent();
        FallingVial->SetStaticMesh(VialMesh);
        FallingVial->SetWorldLocation(SpawnLocation);
        FallingVial->SetWorldRotation(GetActorRotation());
        
        // Set up physics properties for better bouncing
        FallingVial->SetSimulatePhysics(true);
        FallingVial->SetCollisionProfileName(TEXT("PhysicsActor"));
        //FallingVial->SetEnableGravity(true);
        FallingVial->SetNotifyRigidBodyCollision(true); // Enable hit notifications

        FallingVial->SetPhysicsMaxAngularVelocityInDegrees(1000.0f);
        FallingVial->SetPhysicsLinearVelocity(FVector::ZeroVector);
        FallingVial->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        FallingVial->SetLinearDamping(0.1f);
        FallingVial->SetAngularDamping(0.1f);
        
        // Match the scale of the original vial if possible
        if (HealingVialMesh)
        {
            FallingVial->SetWorldScale3D(HealingVialMesh->GetComponentScale());
        }
        
        // Apply a small impulse to make it fall naturally with a bit of forward momentum
        FVector ThrowDirection = -GetActorUpVector() + (GetMesh()->GetForwardVector() * 0.2) + (GetMesh()->GetRightVector().Normalize() * 0.2);
        ThrowDirection.Normalize();
        FallingVial->AddImpulse(ThrowDirection * 800.0f, NAME_None, true);
        
        // Add a small random rotation
        FVector RandomRotation(FMath::RandRange(-1.0f, 1.0f), 
                              FMath::RandRange(-1.0f, 1.0f), 
                              FMath::RandRange(-1.0f, 1.0f));
        FallingVial->AddTorqueInRadians(RandomRotation * 5000.0f, NAME_None, true);

    	FallingVial->SetCollisionObjectType(ECC_PhysicsBody);
    	FallingVial->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    	//FallingVial->SetCollisionResponseToAllChannels(ECR_Block);
    	FallingVial->SetEnableGravity(true);
    	FallingVial->SetUseCCD(true);  // Enable Continuous Collision Detection
    	//FallingVial->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    	FallingVial->OnComponentHit.AddDynamic(this, &AMyProjectTest2Character::OnVialHit);
    	//FallingVial->OnComponentBeginOverlap.AddDynamic(this, &AMyProjectTest2Character::OnVialOverlap);
        
        // Set up hit notification
        //FallingVial->OnComponentHit.AddDynamic(this, &AMyProjectTest2Character::OnVialHit);
        
        // Store the component pointer for hit detection
        TempFallingVial = FallingVial;
        
        // Set up a timer to destroy the component after a delay
        FTimerHandle DestroyTimerHandle;
        FTimerDelegate DestroyDelegate;
        DestroyDelegate.BindLambda([this, FallingVial]()
        {
            if (FallingVial && FallingVial->IsValidLowLevel())
            {
                // Play break effects at the vial's current location before destroying it
                //PlayVialBreakEffects(FallingVial->GetComponentLocation());
                FallingVial->DestroyComponent();
                
                // Clear the temporary reference
                if (TempFallingVial == FallingVial)
                {
                    TempFallingVial = nullptr;
                }
            }
        });
        GetWorldTimerManager().SetTimer(DestroyTimerHandle, DestroyDelegate, 5.0f, false); // Increased to 5 seconds
    }
    
    // Hide the attached vial mesh now that we've thrown the physics one
    if (HealingVialMesh) HealingVialMesh->SetVisibility(false);
    if (HealingAmbientLight) HealingAmbientLight->SetVisibility(false);
}

void AMyProjectTest2Character::OnVialOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Call OnVialHit with default parameters for consistency
	OnVialHit(OverlappedComponent, OtherActor, OtherComp, FVector::ZeroVector, SweepResult);
}

void AMyProjectTest2Character::EndVialSmashAnimation()
{
	// Re-enable movement
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	bIsSmashingVial = false;
	AddHealth(30.0f);
	// Restore weapon visibility based on previous state
	UpdateWeaponVisibility();
}

// Add this function to handle vial hits
void AMyProjectTest2Character::OnVialHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
                                        UPrimitiveComponent* OtherComp, FVector NormalImpulse, 
                                        const FHitResult& Hit)
{
	ActivateHealingGlyph();
	bCanThrowVial = true;
	LastVialTime = 0.f;
	Stamina += 30.0f;
    // Only process if the hit was significant enough and not hitting the player
    if (NormalImpulse.Size() > 50.0f && OtherActor != this)
    {
        UE_LOG(LogTemp, Warning, TEXT("Vial hit detected with impulse: %f"), NormalImpulse.Size());
        
        // Create a temporary light effect at the hit location
        UPointLightComponent* HitLight = NewObject<UPointLightComponent>(this);
        if (HitLight)
        {
            HitLight->SetWorldLocation(Hit.Location);
            HitLight->SetLightColor(FColor(106.0f, 0.0f, 2.f));
            HitLight->SetIntensity(1000.0f); // Flash on impact
            HitLight->SetAttenuationRadius(150.0f);
            HitLight->RegisterComponent();
            
            // Set up a timer to fade out the light
            FTimerHandle FadeTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(
                FadeTimerHandle,
                [HitLight]()
                {
                    HitLight->DestroyComponent();
                },
                0.3f, // Short flash duration
                false
            );
        }

    	if (GlassShatter)
    	{
    		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),    // Get world context
				GlassShatter,  // Niagara system to spawn
				Hit.Location,  // Location of the hit
				Hit.ImpactNormal.Rotation(), // Rotation to align with surface
				FVector(0.3f) 
			);
    	}
    	
    	if (TempFallingVial)
    	{
    		TempFallingVial->DestroyComponent();
    		TempFallingVial = nullptr;
    	}
    }

	
}

void AMyProjectTest2Character::ActivateHealingGlyph()
{
	if (HealingGlyphMesh && HealingGlyphLight)
	{
		// Make sure both are visible
		HealingGlyphMesh->SetVisibility(true);
		HealingGlyphLight->SetVisibility(true);
        
		// Set the light intensity
		HealingGlyphLight->SetIntensity(5000.0f);
        
		// Store the initial light intensity for fading
		HealingGlyphInitialIntensity = 5000.0f;
        
		// Set up the glyph state
		bIsHealingGlyphActive = true;
		bIsHealingGlyphFading = false;
		HealingGlyphTimer = 0.0f;
		//HealingGlyphDuration = 0.5f;
		//HealingGlyphFadeOutDuration = 1.0f;
        
		// Set up the timer for fade out
		GetWorld()->GetTimerManager().ClearTimer(HealingGlyphTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(
			HealingGlyphTimerHandle, 
			this, 
			&AMyProjectTest2Character::StartHealingGlyphFadeOut, 
			HealingGlyphDuration, 
			false);
	}
}

void AMyProjectTest2Character::StartHealingGlyphFadeOut()
{
	if (bIsHealingGlyphActive && HealingGlyphLight && HealingGlyphMesh)
	{
		bIsHealingGlyphFading = true;
        
		// Set up a timer that will call a function to update the fade every frame
		GetWorld()->GetTimerManager().ClearTimer(HealingGlyphTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(
			HealingGlyphTimerHandle, 
			this, 
			&AMyProjectTest2Character::DeactivateHealingGlyph, 
			HealingGlyphFadeOutDuration, 
			false);
            
		// Start the fade out process
		HealingGlyphTimer = 0.0f; // Reset for fade timing
	}
}

void AMyProjectTest2Character::DeactivateHealingGlyph()
{
    if (HealingGlyphMesh && HealingGlyphLight)
    {
        HealingGlyphMesh->SetVisibility(false);
        HealingGlyphLight->SetVisibility(false);
        bIsHealingGlyphActive = false;
        bIsHealingGlyphFading = false;
    }
}

void AMyProjectTest2Character::UpdateHealingGlyph(float DeltaTime)
{
    if (!bIsHealingGlyphActive)
        return;

    // Rotate the glyph
    if (HealingGlyphMesh)
    {
        FRotator NewRotation = HealingGlyphMesh->GetComponentRotation();
        NewRotation.Yaw += GlyphRotationSpeed * DeltaTime;
        HealingGlyphMesh->SetWorldRotation(NewRotation);
    }

    // Handle the intensity of both the glyph and light
    HealingGlyphTimer += DeltaTime;

    if (!bIsHealingGlyphFading && HealingGlyphTimer >= HealingGlyphDuration)
    {
        bIsHealingGlyphFading = true;
        HealingGlyphTimer = 0.0f;
    }

    if (bIsHealingGlyphFading)
    {
        // Calculate fade factor (1.0 to 0.0)
        float FadeFactor = 1.0f - FMath::Clamp(HealingGlyphTimer / HealingGlyphFadeOutDuration, 0.0f, 1.0f);
        
        // Apply to both the mesh and light with the same fade factor
        if (HealingGlyphMesh)
        {
            // For the mesh, we'll adjust opacity if it has a material that supports it
            UMaterialInstanceDynamic* DynamicMaterial = nullptr;
            if (!DynamicMaterial)
            {
                DynamicMaterial = UMaterialInstanceDynamic::Create(HealingGlyphMesh->GetMaterial(0), this);
                HealingGlyphMesh->SetMaterial(0, DynamicMaterial);
            }
            
            if (DynamicMaterial)
            {
                DynamicMaterial->SetScalarParameterValue(FName("Opacity"), FadeFactor);
            }
        }
        
        // Apply the same fade factor to the light intensity
        if (HealingGlyphLight)
        {
            float CurrentIntensity = HealingGlyphInitialIntensity * FadeFactor;
            HealingGlyphLight->SetIntensity(CurrentIntensity);
            
            // Make sure the light stays visible as long as the glyph is visible
            HealingGlyphLight->SetVisibility(FadeFactor > 0.01f);
        }
        
        // Deactivate everything when fully faded
        if (HealingGlyphTimer >= HealingGlyphFadeOutDuration)
        {
            DeactivateHealingGlyph();
        }
    }
}

void AMyProjectTest2Character::QuickAttack()
{
    // Prevent aiming if falling
    if (bIsFalling)
    {
        return;
    }
    if (Crossbow_arrows <= 0)
    {
        return;
    }
    // Don't allow quick attack if already in one
    if (bInQuickAttack || bInQuickAttackFall)
    {
        return;
    }

    // Only allow quick attack when exiting a roll or jump
    if (!(RollFinished || isJumping == false))
    {
        return;
    }

	isCrouching = false;

    bInQuickAttack = true;

    // Store original movement capability to restore later
    EMovementMode OriginalMovementMode = GetCharacterMovement()->MovementMode;
    float OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

    // Lock movement during quick attack
    GetCharacterMovement()->DisableMovement();

    // Apply the same camera logic as when aiming the bow
    FVector OriginalCameraPosition = FollowCamera->GetRelativeLocation();
    FRotator OriginalCameraRotation = FollowCamera->GetRelativeRotation();

    // Temporarily position camera like when aiming
    FollowCamera->SetRelativeLocation(FVector(50.f, 70.f, 0.f));

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
    {
        // Reset camera position and quick attack state before returning
        FollowCamera->SetRelativeLocation(OriginalCameraPosition);
        FollowCamera->SetRelativeRotation(OriginalCameraRotation);
        bInQuickAttack = false;
        GetCharacterMovement()->SetMovementMode(OriginalMovementMode);
        GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
        return;
    }

	// Turn the player to face the camera direction
	FRotator CameraRotation = FollowCamera->GetComponentRotation();
	FRotator NewRotation = FRotator(0.0f, CameraRotation.Yaw, 0.0f); // Only take the Yaw from camera
	SetActorRotation(NewRotation);

	int32 ViewportSizeX, ViewportSizeY;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

	// Apply the correct crosshair offset
	FVector2D ScreenCenter = FVector2D((ViewportSizeX * 0.5f), (ViewportSizeY * 0.5f) - 118.f);
    float SearchAreaWidth = 800.5f; // This defines how large the search area around the center is
    float SearchAreaHeight = 800.5f;

    // Calculate the search area bounds (relative to screen center)
    FVector2D SearchAreaMin = ScreenCenter - FVector2D(SearchAreaWidth, SearchAreaHeight);
    FVector2D SearchAreaMax = ScreenCenter + FVector2D(SearchAreaWidth, SearchAreaHeight);

    AAI_Character* NearestEnemy = nullptr;
    float NearestDistanceSquared = FLT_MAX;  // Start with the largest possible distance

    // Iterate over all enemies to find the one closest to the center of the screen within the defined margin
    for (TActorIterator<AAI_Character> It(GetWorld()); It; ++It)
    {
        AAI_Character* Enemy = *It;

        if (Enemy && !Enemy->bIsDead)
        {
            // Project the enemy's world location to screen space
            FVector2D EnemyScreenPos;
            FVector EnemyWorldPos = Enemy->GetActorLocation();
            if (UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), EnemyWorldPos, EnemyScreenPos))
            {
                // Check if the enemy is within the search area
                if (EnemyScreenPos.X >= SearchAreaMin.X && EnemyScreenPos.X <= SearchAreaMax.X &&
                    EnemyScreenPos.Y >= SearchAreaMin.Y && EnemyScreenPos.Y <= SearchAreaMax.Y)
                {
                    // Calculate the squared distance to the center of the screen
                    float DistanceSquared = FVector2D::DistSquared(EnemyScreenPos, ScreenCenter);

                    // Check if this enemy is closer to the center than the previous closest one
                    if (DistanceSquared < NearestDistanceSquared)
                    {
                        NearestEnemy = Enemy;
                        NearestDistanceSquared = DistanceSquared;
                    }
                }
            }
        }
    }

    if (NearestEnemy)
    {
        // Calculate direction to the nearest enemy
        FVector DirectionToEnemy = NearestEnemy->GetActorLocation() - GetActorLocation();
        DirectionToEnemy.Normalize();

        // Apply aim assist by adding a random "error" to the direction
        float AimErrorRange = 0.1f; // Controls the amount of error in the aim (adjust as needed)
        FVector Error = FVector(
            FMath::RandRange(-AimErrorRange, AimErrorRange),
            FMath::RandRange(-AimErrorRange, AimErrorRange),
            FMath::RandRange(-AimErrorRange, AimErrorRange)
        );

        DirectionToEnemy += Error; // Apply error to the direction
        DirectionToEnemy.Normalize(); // Ensure the direction is still normalized

        // Calculate the rotation to face the enemy (with error applied)
        FRotator TargetRotation = DirectionToEnemy.Rotation();
        SetActorRotation(TargetRotation);


    	// Convert rotations to quaternions
    	FQuat NewRotationQuat = FQuat(NewRotation);
    	FQuat TargetRotationQuat = FQuat(TargetRotation);

    	// Average the two quaternions (spherical linear interpolation)
    	FQuat AveragedRotationQuat = FQuat::Slerp(NewRotationQuat, TargetRotationQuat, 0.5f); // 0.5f is the interpolation factor, adjust as needed

    	// Convert the averaged quaternion back to a FRotator
    	FRotator AveragedRotation = AveragedRotationQuat.Rotator();

    	// Apply the averaged rotation
    	//SetActorRotation(AveragedRotation);

    	// Average the camera's rotation and the calculated target rotation
    	FQuat CameraQuat = FQuat(CameraRotation);
    	FQuat TargetQuat = FQuat(TargetRotation);
    	FQuat AveragedQuat = FQuat::Slerp(CameraQuat, TargetQuat, 0.5f); // Blend them with a factor (0.5f is halfway)

    	FRotator FinalRotation = TargetRotation;

    	// Set the character's rotation to face the enemy direction smoothly
    	SetActorRotation(FinalRotation);

    	// Now reposition the camera based on the updated character rotation
    	// Example: Keep the camera slightly behind the character
    	FVector CameraOffset = FVector(50.f, 70.f, 50.f); // You can adjust this offset as needed
    	FollowCamera->SetRelativeLocation(CameraOffset);
    	FollowCamera->SetRelativeRotation(FinalRotation);  // Match the camera's rotation to the character's rotation


    	// Clamp the camera rotation
    	float MinPitch = -89.0f; // Minimum angle (looking down)
    	float MaxPitch = 89.0f; // Maximum angle (looking up)
    	float ClampedPitch = FMath::Clamp(FinalRotation.Pitch, MinPitch, MaxPitch);
    	ClampedPitch += 90;
    	ClampedPitch /= 180;

    	AimBlend = FMath::Clamp(ClampedPitch, 0.05f, 0.95f) * 100.0f;
    }

    // Delay the actual shooting by 0.4 seconds
    GetWorldTimerManager().SetTimer(
        QuickAttackShootTimerHandle,
        [this, OriginalMovementMode, OriginalMaxWalkSpeed]()
        {
            if (!ProjectileClass || !bCanShootArrow)
            {
                // Reset the quick attack state if we can't shoot
                bInQuickAttack = false;
                GetCharacterMovement()->SetMovementMode(OriginalMovementMode);
                GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
                FollowCamera->SetRelativeLocation(DefaultCameraPosition);
                FollowCamera->SetRelativeRotation(DefaultCameraRotation);
                return;
            }

            // Set cooldown to prevent rapid firing
            bCanShootArrow = false;
            ArrowCooldownTimer = 0.0f;

            // Get the socket transform from the left hand (where the bow is held)
        	FTransform SocketTransform = GetMesh()->GetSocketTransform("CrossbowAimSocket", RTS_World);

// Get the player controller
APlayerController* PC = Cast<APlayerController>(GetController());
if (!PC) return;

// Get the viewport size
int32 ViewportSizeX, ViewportSizeY;
PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

// Calculate the center of the screen in screen space
FVector2D ScreenCenter((ViewportSizeX * 0.5f), (ViewportSizeY * 0.5f) - 118.f);

// Convert screen position to world position and direction
FVector WorldLocation, WorldDirection;
PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, WorldLocation, WorldDirection);

// Perform a line trace to find what's under the crosshair
FHitResult HitResult;
FVector TraceStart = WorldLocation;
FVector TraceEnd = WorldLocation + (WorldDirection * 10000.0f); // Trace far enough

FCollisionQueryParams QueryParams;
QueryParams.AddIgnoredActor(this); // Ignore self

bool bHit = GetWorld()->LineTraceSingleByChannel(
	HitResult,
	TraceStart,
	TraceEnd,
	ECC_Visibility,
	QueryParams
);

// Calculate target point - use hit location if we hit something, otherwise use a point far in the distance
FVector TargetPoint = bHit ? HitResult.Location : (WorldLocation + (WorldDirection * 5000.0f));

// Calculate direction from socket to target point
FVector ShootDirection = (TargetPoint - SocketTransform.GetLocation()).GetSafeNormal();

            // Spawn the projectile at the socket location
            AProjectile_Arrow_Base* Projectile = GetWorld()->SpawnActor<AProjectile_Arrow_Base>(
                ProjectileClass,
                SocketTransform.GetLocation(),
                ShootDirection.Rotation() // Set initial rotation to match shooting direction
            );

            if (Projectile && !bIsInDamageState)
            {
            	CurrentProjectile = Projectile;
            	
            	Projectile->Scale(0.3);
            	UGameplayStatics::PlaySoundAtLocation(
					this,           // World context object
					BowReleaseSound,// Sound to play
					GetActorLocation(), // Location to play sound
					1.0f,           // Volume multiplier
					1.0f,           // Pitch multiplier
					0.0f,           // Start time
					nullptr,        // Attenuation settings
					nullptr         // Concurrency settings
				);
            	Projectile->SetDamage(50.f);
                if (Crossbow_arrows <= 0)
                {
                    GetWorldTimerManager().SetTimer(
                        QuickAttackTimerHandle,
                        [this, OriginalMovementMode, OriginalMaxWalkSpeed]()
                        {
                            // Re-enable movement explicitly
                            GetCharacterMovement()->SetMovementMode(OriginalMovementMode);
                            GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;

                            // Reset camera to original position
                            FollowCamera->SetRelativeLocation(DefaultCameraPosition);
                            FollowCamera->SetRelativeRotation(DefaultCameraRotation);

                            bInQuickAttack = false;
                            bInQuickAttackFall = false;
                        },
                        0.7f, // Adjust this time as needed (0.5 seconds)
                        false // Don't loop
                    );

                    // Set a timer to enable quick attack fall state after a small delay
                    GetWorldTimerManager().SetTimer(
                        QuickAttackFallTimerHandle,
                        [this]()
                        {
                            bInQuickAttackFall = true;
                        },
                        0.6f, // Small delay before enabling fall state (adjust as needed)
                        false // Don't loop
                    );
                    return; // Return if no arrows left
                }
                Crossbow_arrows--;
            	LastCrossbowTime = 0.f;
                // Set the velocity for the arrow
                float ArrowSpeed = 2500.f;
                FVector LaunchVelocity = ShootDirection * ArrowSpeed;

                // Apply velocity to the projectile
                Projectile->SetVelocity(LaunchVelocity);

                // Set this character as the instigator (for damage attribution)
                Projectile->SetInstigator(this);

                // If the projectile has a mesh with physics enabled, apply impulse
                if (Projectile->GetArrowMesh() && Projectile->GetArrowMesh()->IsSimulatingPhysics())
                {
                    Projectile->GetArrowMesh()->AddImpulse(LaunchVelocity, NAME_None, true);
                }

                // Apply a subtle backwards force to the character
                FVector BackwardsForce = -GetActorForwardVector() * 300.0f; // Adjust force magnitude as needed
                LaunchCharacter(BackwardsForce, false, false);
            }
        },
        0.4f, // Delay the shot by 0.4 seconds
        false // Don't loop
    );

    // Set a timer to turn bInQuickAttack back to false after a short delay and restore movement
    GetWorldTimerManager().SetTimer(
        QuickAttackTimerHandle,
        [this, OriginalMovementMode, OriginalMaxWalkSpeed]()
        {
            // Re-enable movement explicitly
            GetCharacterMovement()->SetMovementMode(OriginalMovementMode);
            GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;

            // Reset camera to original position
            FollowCamera->SetRelativeLocation(DefaultCameraPosition);
            FollowCamera->SetRelativeRotation(DefaultCameraRotation);

            bInQuickAttack = false;
            bInQuickAttackFall = false;
        },
        0.7f, // Adjust this time as needed (0.7 seconds)
        false // Don't loop
    );

    // Set a timer to enable quick attack fall state after a small delay
    GetWorldTimerManager().SetTimer(
        QuickAttackFallTimerHandle,
        [this]()
        {
            bInQuickAttackFall = true;
        },
        0.6f, // Small delay before enabling fall state (adjust as needed)
        false // Don't loop
    );
}

void AMyProjectTest2Character::EnterDamageState(float StunDuration)
{

	if (bInQuickAttack)
	{
		// Cancel the quick attack if it's still in progress
        bInQuickAttack = false;
        GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
        GetCharacterMovement()->MaxWalkSpeed = 600.0f;
        FollowCamera->SetRelativeLocation(DefaultCameraPosition);
        FollowCamera->SetRelativeRotation(DefaultCameraRotation);

		QuickAttackCurrentTime = QuickAttackTotalDuration;

		// Destroy the projectile if it exists
		if (CurrentProjectile)
		{
			if (!CurrentProjectile->hasCollided)
			{
				if (CurrentProjectile->Destroy())
				{
					Crossbow_arrows++;
				}
			}
			
			CurrentProjectile = nullptr;
		}
	}
	if (FMath::IsNearlyEqual(Health, 0.f, 0.5f))
	{
		HandleDeath();
		return;
	}
	// Don't re-enter damage state if already in it
	if (bIsInDamageState)
	{
		return;
	}
	bIsInDamageState = true;

	// Store current states to restore later
	bool WasAttacking = IsAttacking;
	bool WasRolling = IsRolling;
	bool WasAiming = bIsAiming;

	// Cancel any current actions
	if (IsAttacking)
	{
		StopAttack();
	}

	if (IsRolling)
	{
		IsRolling = false;
		StopRolling();
	}

	if (bIsAiming)
	{
		bIsAiming = false;
		StopAiming();
	}

	// Temporarily disable movement and actions
	GetCharacterMovement()->DisableMovement();

	// Set timer to exit damage state
	GetWorldTimerManager().SetTimer(
		DamageStateTimerHandle,
		[this]()
		{
			ExitDamageState();
		},
		StunDuration,
		false
	);
}

// Add a method to exit damage state
void AMyProjectTest2Character::ExitDamageState()
{
	bIsInDamageState = false;
	if (GotKicked)
	{
		GotUp = false;
		GotKicked = false;
		GetWorldTimerManager().SetTimer(
		   GetUpTimerHandle,
		   [this]()
		   {
			   // Enable movement after getting up
		   		GotUp = true;
		   		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		   },
		   0.05f, // Adjust this time as needed (2 seconds)
		   false // Don't loop
	   );
		
	}
	// Re-enable movement
	if (!GotKicked)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

float AMyProjectTest2Character::GetQuickAttackProgress() const
{
	if (!bInQuickAttack)
		return 0.0f;
    
	return FMath::Clamp(QuickAttackCurrentTime / QuickAttackTotalDuration, 0.0f, 1.0f);
}

void AMyProjectTest2Character::PauseStaminaRegeneration()
{
	bIsStaminaRegenPaused = true;
	StaminaRegenPauseTimer = 0.0f;
	StaminaRegenRate = BaseStaminaRegenRate; // Reset regen rate when paused
}
// Add this new method to implement the restart functionality
void AMyProjectTest2Character::RestartGame()
{
	// Get the current level name
	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this);
    
	// Restart the current level
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
    
	// Alternative: Load a specific level by name
	// UGameplayStatics::OpenLevel(this, FName("YourLevelName"));
}

void AMyProjectTest2Character::PauseGame(const FInputActionValue& Value)
{
	bIsGamePaused = !bIsGamePaused;
	TogglePauseState();
}

void AMyProjectTest2Character::TogglePauseState()
{
	// Get the player controller
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
		return;

	// Toggle the pause state
	//bIsGamePaused = !bIsGamePaused;

	// Set the game pause state
	if (bIsGamePaused)
	{
		// Pause the game
		UGameplayStatics::SetGamePaused(GetWorld(), true);
        
		// Show the pause menu (you would implement this in Blueprint)
		// This will broadcast an event that your UI can listen to
		//OnGamePaused.Broadcast();
        
		// Optionally show mouse cursor for menu navigation
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeUIOnly());
	}
	else
	{
		// Unpause the game
		UGameplayStatics::SetGamePaused(GetWorld(), false);
        
		// Hide the pause menu (you would implement this in Blueprint)
		// This will broadcast an event that your UI can listen to
		//OnGameUnpaused.Broadcast();
        
		// Hide mouse cursor and return to game input
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

float AMyProjectTest2Character::KickDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (isJumping || IsRolling) return -1.0f;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = 0.f;
	
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	GotKicked = true;

	if (!bIsDead)
	{
		UGameplayStatics::PlaySoundAtLocation(
				this,           // World context object
				TakeDamageSound,// Sound to play
				GetActorLocation(), // Location to play sound
				1.0f,           // Volume multiplier
				1.0f,           // Pitch multiplier
				0.0f,           // Start time
				nullptr,        // Attenuation settings
				nullptr         // Concurrency settings
			);
	}

	UGameplayStatics::PlaySoundAtLocation(
					this,           // World context object
					FallSound,// Sound to play
					GetActorLocation(), // Location to play sound
					1.0f,           // Volume multiplier
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
    
	if (DamageAmount > 0.0f)
	{
		// Update the actual health value
		Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
        
		// Set the target for lerping
		TargetHealth = Health;
		bIsHealthLerping = true;

		//if (Health > 0.0f)
		{
			// Calculate stun duration based on damage amount
			// More damage = longer stun, but capped at a maximum
			float StunDuration = FMath::Clamp(ActualDamage / 20.0f, 0.2f, 1.0f);
			if (!bIsInDamageState)
			{
				EnterDamageState(1.26f); // Stun duration is 2 seconds
			}
		}
        
        
		// Handle death if needed
		if (Health <= 0.0f)
		{
			//HandleDeath();
		}
	}
	
    
	return ActualDamage;
}


void AMyProjectTest2Character::ReachOfJudgement(float Distance, float DeltaTime)
{
	// If the player is outside the safe range, start tracking time in the zone
	if (Distance >= 1000.f)
	{
		bIsInReachOfJudgement = true;
		TimeInJudgementZone += DeltaTime; // Increase time spent in the zone

		// Apply increasing damage over time
		float BaseDamage = .2f;
		float ScaledDamage = BaseDamage * (1.0f + (TimeInJudgementZone * 0.1f)); // Damage scales over time
		float ActualDamage = ScaledDamage * DeltaTime;
		
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
		TargetHealth = Health;
		bIsHealthLerping = true;

		// Stamina drains faster over time
		Stamina -= (.2f + (TimeInJudgementZone * 0.05f)) * DeltaTime; 
		PauseStaminaRegeneration();
	}

	if (Distance >= 1200.f)
	{
		bIsInReachOfJudgement = true;
		TimeInJudgementZone += DeltaTime;

		float BaseDamage = 1.f;
		float ScaledDamage = BaseDamage * (1.0f + (TimeInJudgementZone * 0.15f)); // Damage scales faster
		float ActualDamage = ScaledDamage * DeltaTime;
		
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
		TargetHealth = Health;
		bIsHealthLerping = true;

		// Stamina drains even faster
		Stamina -= (1.f + (TimeInJudgementZone * 0.1f)) * DeltaTime;
		PauseStaminaRegeneration();
	}

	if (Distance >= 1600.f)
	{
		bIsInReachOfJudgement = true;
		TimeInJudgementZone += DeltaTime;

		float BaseDamage = 10.f;
		float ScaledDamage = BaseDamage * (1.0f + (TimeInJudgementZone * 0.2f)); // Most punishing
		float ActualDamage = ScaledDamage * DeltaTime;
		
		Health = FMath::Clamp(Health - ActualDamage, 0.0f, MaxHealth);
		TargetHealth = Health;
		bIsHealthLerping = true;

		// Instantly drain stamina
		Stamina = 0.0f;
		PauseStaminaRegeneration();
	}

	// If the player moves back inside the safe range, reset the timer
	if (Distance < 1200.f)
	{
		bIsInReachOfJudgement = false;
		TimeInJudgementZone = 0.0f; // Reset time counter
	}
}