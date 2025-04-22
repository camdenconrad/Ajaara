#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "MyProjectTest2/CPP_Repo/MyProjectTest2Character.h"
#include "AI_Character.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeathSignature, AAI_Character*, DeadCharacter);
UCLASS()

class MYPROJECTTEST2_API AAI_Character : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAI_Character();

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float CachedSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AttackRange = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool IsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float TargetHealth  = 100.f;
	UPROPERTY(BlueprintReadWrite, Category = "Health")
	float Health = 100.f;  

	UPROPERTY(BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackCooldown = 0.5f;
	FTimerHandle RagdollTimerHandle;
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsInDamageState = false;
	FTimerHandle DamageStateTimerHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DamageStateStunDuration = 1.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* HealthBarWidget;
	FTimerHandle HealthBarTimerHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float CurrentHealth = 100.f;
	bool bIsHealthLerping = false;
	double HealthLerpStartTime;
	float HealthLerpDuration;
	FTimerHandle HealthLerpTimerHandle;
	bool bHasAppliedDamageInCurrentAttack = false;

	UPROPERTY(EditAnywhere, Category="Effects")
	UNiagaraSystem* HitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* TakeDamageSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* DeathSound;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsJumping = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping")
	float JumpHeight = 800.f;

	UPROPERTY(BlueprintReadOnly, Category = "AI Status", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;
	float FootstepTimer = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* WalkingSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCameraComponent* CameraRef;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool KickStun = false;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//float GetHealthPercent() const;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void StartJump();
	void StopJump();
	bool IsJumpInputPressed() const;
	void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity);
	void ClearAttackTimers();
	void SpawnHealingEffect(AMyProjectTest2Character* PlayerCharacter);
	float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator,
	                 AActor* DamageCauser);
	void UpdateHealthLerp();
	void ExitDamageState();
	bool IsPendingKill();

	// Damage to apply when colliding with the pla`yer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Damage = 45.f;
	bool bHasFoundPlayer;

	// Attack wind-up time (seconds before damage is applied)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackWindupTime = 0.1f;
    
	// Total attack animation time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackAnimationTime = 1.0f;
    
	// Current target for attack
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	APawn* CurrentTarget;

private:
	// Attack cooldown timerqhandle
	FTimerHandle AttackTimerHandle;
    
	// Whether the AI can currently attack
	bool bCanAttack;
	float ErrorTolerance = 5.f;

	// Function to reset attack cooldown
	void ResetAttackCooldown();

    
	// Timer handle for the attack execution
	FTimerHandle AttackExecutionTimerHandle;

	// Whether the AI is currently in the process of attacking
	bool bIsExecutingAttack;

	// Function to handle attacking the player
	void AttackPlayer(APawn* PlayerPawn, AAIController* AIController);
    
	// Function to actually apply damage after attack animation has progressed
	void ExecuteAttackDamage();
    
	// Flag to track if the AI is dead
	
    
	// Function to enable ragdoll physics on death
	void EnableRagdoll();
	float GetKicked(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	                AActor* DamageCauser);
	

	FTimerHandle AttackFinishTimerHandle;
	float AttackFinishDelay = 0.5f; 
	
};
