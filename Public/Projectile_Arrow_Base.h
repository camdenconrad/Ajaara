// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h" 
#include "Projectile_Arrow_Base.generated.h"


UCLASS()
class MYPROJECTTEST2_API AProjectile_Arrow_Base : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile_Arrow_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UStaticMeshComponent* GetArrowMesh() const { return ArrowMesh; }
	void SetVelocity(const FVector& Vector);
	
	void SetDamage(float Damage);
	void Scale(double X);

	UPROPERTY(EditAnywhere, Category="Effects")
	UNiagaraSystem* HitEffect;

	UPROPERTY(EditAnywhere, Category="Effects")
	UNiagaraSystem* TrailEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* ArrowHitSound;

	bool hasCollided = false;  // Whether the arrow has collided with something

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UStaticMeshComponent* ArrowMesh;

	FVector Velocity;  // The velocity of the arrow
	float Speed;  // Arrow speed

	UPROPERTY(EditAnywhere, Category = "Damage")
	float DamageAmount = 70.0f;  // Amount of damage the arrow deals

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	void StopArrowMovement();
	void AttachArrowToTarget(AActor* HitActor, UPrimitiveComponent* HitComponent, const FHitResult& Hit);
};
