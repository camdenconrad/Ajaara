// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "NiagaraSystem.h"
#include "Projectile_Arrow_Base.h"
#include "MyProjectTest2Character.generated.h"

class AAI_Elite;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);


UCLASS(config=Game)
class AMyProjectTest2Character : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* IA_Forward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* IA_Backward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* IA_Right;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* IA_Left;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RollAction;
	FTimerHandle JumpTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* HealthAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* QuickAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RestartAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PauseAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BowMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SwordRightMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SwordLeftMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* CrossbowMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BowArrowMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* HealingVialMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UPointLightComponent* HealingAmbientLight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* HealingGlyphMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USpotLightComponent* HealingGlyphLight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SwordRightMeshInner;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SwordLeftMeshInner;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BowOnBackRef;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ArrowInQuiver0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ArrowInQuiver1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ArrowInQuiver2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ArrowInQuiver3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* CrossBowOnBeltRef;
	
	UPROPERTY(EditDefaultsOnly, Category = "Healing")
	float GlyphRotationSpeed = 90.0f; // Degrees per second
    
	UPROPERTY()
	FTimerHandle HealingGlyphTimerHandle;
    
	UPROPERTY()
	bool bIsHealingGlyphActive = false;

	UPROPERTY()
	UStaticMeshComponent* TempFallingVial;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UClass* ProjectileClass;
	FVector DefaultCameraPosition;
	FRotator DefaultCameraRotation;
	bool bCanAim = true;
	FTimerHandle RagdollTimerHandle;
	FTimerHandle QuickAttackTimerHandle;
	FTimerHandle QuickAttackFallTimerHandle;
	FTimerHandle ShootAnimationTimerHandle;
	FTimerHandle VialSmashAnimationTimerHandle;
	TEnumAsByte<enum EMovementMode> StoredMovementMode;
	float StoredMaxWalkSpeed;
	FTimerHandle VialThrowTimerHandle;
	
	UPROPERTY()
	float HealingGlyphTimer = 0.0f;

	UPROPERTY()
	bool bIsHealingGlyphFading = false;

	UPROPERTY()
	float HealingGlyphInitialIntensity = 5000.0f;
	float HealingGlyphDuration = 0.5f;
	float HealingGlyphFadeOutDuration = 1.0f;
	float TargetHealth = 100.0f;
	bool bIsHealthLerping = false;
	float HealthLerpSpeed = 3.0f;
	int bIsGamePaused;
	FTimerHandle RestartGameTimerHandle;
	bool bIsStaminaRegenPaused = false;
	float StaminaRegenPauseTimer = 0.f;
	float StaminaRegenCooldown = 1.5;
	UAudioComponent* WalkingSoundComponent;
	float FootstepTimer;
	bool bHasPlayedBowAimSound;
	class UAudioComponent* BowPullbackAudio;
	AProjectile_Arrow_Base* CurrentProjectile = nullptr;
	float FrameStartTime;
	double FrameEndTime;
	double FrameDuration;
	int MaxArrows = 3;
	int MaxCrossbowArrows = 2;
	int MaxHealVials = 3;
	FTimerHandle GetUpTimerHandle;
	float TimeInJudgementZone = 0.0f;
	float HeartTimer = 0.0f;
	FTimerHandle JumpCooldownTimerHandle;
	bool bCanJump = true;
	float LastRollTime;
	bool bCanThrowVial = true;
	FTimerHandle GetUpStartTimerHandle;

public:
	AMyProjectTest2Character();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool bIsStrafing = false;
	FVector MoveDirection;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool bIsInReachOfJudgement;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool GotKicked;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool GotUp = true;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	AAI_Elite* EliteBoss;

	// Example variable to control speed or any other relevant value

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool IsRolling;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	int FrameCounter = 0; // To track the number of frames since rolling started

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool isJumping;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	int JumpFrameCounter = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool isCrouching = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool IsAttacking = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	int AttackFrameCounter = 104; // To track the number of frames since rolling started
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool bDidShoot = false;

	UPROPERTY(BlueprintReadWrite, Category = "Health")
	float Health = 100.f;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int Arrows = 2;
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int Crossbow_arrows = 2;
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int Health_vials = 2;

	UPROPERTY(BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadWrite, Category = "`")
	float Stamina = 400.f;

	UPROPERTY(BlueprintReadWrite, Category = "Stamina")
	float MaxStamina = 400.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FVector CachedVelocity;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float CachedSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsFalling;

	// New booleans for animation finished states
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool AttackFinished = true;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool RollFinished = true;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsAiming;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float AimBlend;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float AimTime;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool IsMovingForwad;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool IsMovingBackward;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool IsMovingRight;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool IsMovingLeft;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bInQuickAttack;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bInQuickAttackFall;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bIsInDamageState = false;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bIsSmashingVial = false;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float CurrentDisplayHealth = 100.f;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float DeathTime = 0.f;

	bool bIsBowEquipped;
	
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	bool bIsDead;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	bool WasJumping = false; // Tracks the previous jumping state

	void AddHealth(float in);

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float QuickAttackTotalDuration = 0.7f;  // Total duration of quick attack (matches your timer)
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float QuickAttackCurrentTime = 0.0f;
	UPROPERTY(BlueprintReadWrite, Category = "Animation")// Current time in the quick attack sequence
	bool bIsQuickAttackInProgress = false;  // Flag to track if quick attack is currently happening

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stamina")
	float BaseStaminaRegenRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stamina")
	float MaxStaminaRegenRate = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stamina")
	float StaminaRegenAcceleration = .8f;

	// Current stamina regeneration rate
	float StaminaRegenRate = BaseStaminaRegenRate;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Stamina")
	bool StaminaDepletion = false;

	UPROPERTY(EditAnywhere, Category="Effects")
	UNiagaraSystem* HitEffect;

	UPROPERTY(EditAnywhere, Category="Effects")
	UNiagaraSystem* GlassShatter;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 CurrentAttackNumber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* MeleeSwingSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* MeleeHitSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* TakeDamageSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* DeathSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* WalkingSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* HeartSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* BowpullbackSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* BowReleaseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* VialThrowSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* FallSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* BowRestockSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* CrossbowRestockSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* VialRestockSound;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float LastArrowTime = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float LastCrossbowTime = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float LastVialTime = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float LastArrowTimeGoal = 15.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float LastCrossbowTimeGoal = 5.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float LastVialTimeGoal = 25.f;


protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	void BeginPlay();
	void UpdateQuiverArrowsVisibility();

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Roll();
	void StopAttack();
	void ShootBow();
	void ResetShootAnimation();
	void AimBow();
	void RegenerateInventory(float DeltaTime);
	bool FindNearestEliteBoss();
	virtual void Tick(float DeltaTime) override;
	void StopRolling();
	void OnSpaceBarPressed();
	virtual void Jump() override;
	void StartSprinting();
	void StopSprinting();

	void StartCrouching();
	void StopCrouching();

	void StartAiming();
	void StopAiming();

	virtual void NotifyControllerChanged() override;

	void StartAttack();
	void SetMovingForward();
	void StopMovingForward();
	void SetMovingBackward();
	void StopMovingBackward();
	void SetMovingRight();
	void StopMovingRight();
	void SetMovingLeft();
	void StopMovingLeft();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void EnableRagdoll();
	void SetHealth(float NewHealth);
	
	void AddHealthInput(const FInputActionValue& Value);
	void StartVialSmashAnimation();
	void ThrowVial();
	void OnVialOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void EndVialSmashAnimation();
	UFUNCTION() 
	void OnVialHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	               FVector NormalImpulse, const FHitResult& Hit);
	void StartHealingGlyphFadeOut();
	void ActivateHealingGlyph();
	void DeactivateHealingGlyph();
	void UpdateHealingGlyph(float DeltaTime);
	void QuickAttack();
	UFUNCTION()
	void OnCapsuleOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	void ToggleWeapon();
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void UpdateWeaponVisibility();
	virtual float TakeDamage(float DamageAmount, const struct FDamageEvent& DamageEvent,
	                         class AController* EventInstigator, AActor* DamageCauser) override;
	float GetDisplayHealth() const;
	void HandleDeath();
	void CheckForMeleeHits();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	void EnterDamageState(float StunDuration);
	void ExitDamageState();
	float GetQuickAttackProgress() const;
	void PauseStaminaRegeneration();
	void RestartGame();
	void PauseGame(const FInputActionValue& Value);
	void TogglePauseState();
	float KickDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	                 AActor* DamageCauser);
	void ReachOfJudgement(float Distance, float DeltaTime);

private:
	float RollDuration = 0.0f;
	float MaxRollDuration = 0.8f; // 0.5 seconds for roll duration

	float AttackTimeCounter = 0.0f;
	float AttackDuration = 0.7f; // 0.5 seconds for attack duration

	float AttackCooldownTimer = 0.0f;
	float AttackCooldownDuration = 0.3f; // 0.3 seconds cooldown between attacks

	float JumpCooldownTimer = 0.0f;
	float JumpCooldownDuration = 0.1f; // 0.5 seconds cooldown between jumps

	float StaminaZeroTimer = 0.0f;
	float StaminaRecoveryDelay = 2.0f;

	// Arrow shooting cooldown
	float ArrowCooldownTimer = 0.0f;
	float ArrowCooldownDuration = 0.5f; // Half second cooldown between shots
	bool bCanShootArrow = true;


	bool bIsWaitingForDoubleTap = false; // Flag to wait for the second tap
	float LastSpaceBarPressTime = 0.f; // Time of the last space `bar press
	float DoubleTapThreshold = 0.3f; // Time threshold to detect double tap (in seconds)

	int MaxFramesForRolling = 86; // Maximum number of frames before stopping the roll

	bool WasCrouching = false;
	bool WasSprinting = false;

	int JumpCooldown = 58;
	int FramesSinceLastJump = 0;

	float JumpCost = 20.f;
	float SprintCost = .5f;
	float RollCost = 15.f;

	int StaminaZeroFrame = 0;
	int StaminaFrameTime = 60 * 4;

	float LastHealthAddTime = 0.0f;
	float HealthAddCooldown = 0.3f;
	FTimerHandle DamageStateTimerHandle;
	FTimerHandle QuickAttackShootTimerHandle;

};
