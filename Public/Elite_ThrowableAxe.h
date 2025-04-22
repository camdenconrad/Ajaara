// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "GameFramework/Actor.h"
#include "Elite_ThrowableAxe.generated.h"

UCLASS()
class MYPROJECTTEST2_API AElite_ThrowableAxe : public AActor
{
	GENERATED_BODY()
	
public:
	AElite_ThrowableAxe();
	void OnAxeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                  int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	void ThrowAxe(FVector TargetLocation, AActor* Boss, AActor* TargetActor);
	void HitPlayer();
	auto OnAxeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	              FVector NormalImpulse, const FHitResult& Hit) -> void;
	void HandleAxeCollision(AActor* OtherActor);

	UPROPERTY(EditAnywhere, Category="Effects")
	UNiagaraSystem* Effect;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UStaticMeshComponent* AxeMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SpinSpeed = 3 * 720.f;

	FVector StartLocation;
	FVector EndLocation;
	AActor* BossReference;
	bool bReturning = false;
	float ThrowStartTime = 0.0f;
	float ReturnDelay = 1.3f;
	bool bHasAppliedDamageInCurrentAttack = false;
	FRotator SpinRotation;
	AActor* Player;
	float ThrowDuration;
	float ReturnDuration;
	FVector Center;
	float Radius;
};
