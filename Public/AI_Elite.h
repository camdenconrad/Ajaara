// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI_Character.h"
#include "GameFramework/Character.h"

#include "Components/CapsuleComponent.h"
#include "MyProjectTest2/CPP_Repo/MyProjectTest2Character.h"
#include "AI_Elite.generated.h"

UCLASS()
class MYPROJECTTEST2_API AAI_Elite : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAI_Elite();

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float CachedSpeed = 0.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCameraComponent* CameraRef;
	float AttackRange = 250.f;
	bool bCanAttack = true;
	bool bIsExecutingAttack = false;

	UPROPERTY(EditDefaultsOnly, Category = "Axe Throw")
	TSubclassOf<class AElite_ThrowableAxe> EliteThrowableAxeClass;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDead = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool IsAttacking = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsInDamageState = false;
	APawn* CurrentTarget = nullptr;
	bool bHasAppliedDamageInCurrentAttack;
	
	FTimerHandle AttackExecutionTimerHandle;
	float AttackWindupTime = 1.f;
	FTimerHandle AttackTimerHandle;
	float AttackCooldown = 2.0f;
	FTimerHandle AttackFinishTimerHandle;
	float AttackAnimationTime = 1.f;

	FTimerHandle KickExecutionTimerHandle;
	float KickWindupTime = 0.1f;
	FTimerHandle KickTimerHandle;
	float KickCooldown = 6.0f;
	FTimerHandle KickFinishTimerHandle;
	float KickAnimationTime = 0.4f;
	
	FTimerHandle SummonExecutionTimerHandle;
	float SummonWindupTime = 3.0f;
	FTimerHandle SummonTimerHandle;
	float SummonCooldown = 8.0f;
	FTimerHandle SummonFinishTimerHandle;
	float SummonAnimationTime = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Health = 4000.f;
	FTimerHandle RagdollTimerHandle;
	FTimerHandle DamageStateTimerHandle;
	float DamageStateStunDuration = 0.7;

	UPROPERTY(EditAnywhere, Category="Effects")
	UNiagaraSystem* HitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* TakeDamageSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* DeathSound;
		
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsKicking = false;
	
	bool bIsExecutingKick = false;
	bool bCanKick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Name")
	FString Name = "Amritaghātī";

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float MaxHealth = 4000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	int32 CurrentAttackNumber;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsExecutingSummon = false;
	bool bCanSummonGrunts = true;
	UPROPERTY(EditDefaultsOnly, Category = "Summoning")
	UClass* GruntClass;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool isThrowingAxe = false;
	
	bool bCanThrowAxe = true;
	FTimerHandle AxeThrowCooldownTimerHandle;
	float AxeThrowCooldown = 6.f;
	FTimerHandle AxeThrowTimerHandle;
	FTimerHandle AxeThrowCooldownExecutionHandle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* AxeMesh;
	float RunningSpeed = 300.f;
	float WalkingSpeed = 250.f;
	int SummonedGrunts;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class AElite_ChainProjectile> ChainProjectileClass;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bUsingChain;
	FTimerHandle ChainTimerHandle;
	FTimerHandle ChainCooldownTimerHandle;
	bool bCanUseChain = true;
	FTimerHandle ChainExecuteTimerHandle;

	FTimerHandle BlockTimerHandle;
	bool bCanBlock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	bool bIsBlocking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float BlockDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float BlockCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float BlockDetectionRange;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float StompRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float StompForce = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float StompDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	USoundBase* StompSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	UNiagaraSystem* StompEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stomp")
	bool bIsStomping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stomp")
	bool bCanStomp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stomp")
	float StompDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stomp")
	float StompCooldown;

	FTimerHandle StompEndTimerHandle;
	FTimerHandle StompCooldownTimerHandle;
	bool bHasDoneStompDamage = false;
	FTimerHandle StompTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SummonSound;
	FTimerHandle BlockDropTimerHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* ShieldHitSound;
	UStaticMeshComponent* RingMesh;
	UStaticMeshComponent* DomeMesh;
	UStaticMeshComponent* ShieldMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* AxeSwingSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* AxeThrowSound;
	bool bShieldDetached = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	APlayerController* PlayerController;
	bool bHasFoundPlayer = false;

public:
	void AttackPlayer(APawn* Pawn, AController* AIController);
	void ExecuteKickDamage();
	void ClearKickTimers();
	void ResetKickCooldown();
	void KickPlayer(APawn* Pawn, AController* AIController);
	void ExecuteAttackDamage();
	void ResetAttackCooldown();
	void ClearAttackTimers();
	void ClearAllAttackTimers();
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	                 AActor* DamageCauser);
	void DetachShield();
	void DetachAxe();
	void ExitDamageState();
	void EnableRagdoll();
	float GetHealth();
	float GetMaxHealth();
	void ResetSummonCooldown();
	bool AreAllGruntsDead();
	void ThrowChain();
	void SummonGrunts();
	void ExecuteSummon();
	void SpawnGrunt(FVector Location, FRotator Rotation);
	void PerformAxeThrow();
	void ExecuteKickamage();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void MoveToRandomLocation();
	void ThrowAxe();
	bool AreAllGruntsDead() const;
	void OnGruntDeath(AAI_Character* DeadGrunt);

	bool IsPlayerAttacking();
	void TriggerShieldBlock();
	void EndShieldBlock();

	void PerformStomp();
	
	UPROPERTY()
	APawn* PlayerPawn;

	// Called to bind functionality to input
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// private:
	// 	constexpr float DetectionRadius = 600.0f; // AI notices player within this range
	// 	constexpr float StopRadius = 100.0f;

};