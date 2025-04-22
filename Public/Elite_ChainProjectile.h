#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Elite_ChainProjectile.generated.h"

class AAI_Elite;
class UStaticMeshComponent;
class UCapsuleComponent;
class ACharacter;

UCLASS()
class MYPROJECTTEST2_API AElite_ChainProjectile : public AActor
{
	GENERATED_BODY()
    
public:    
	AElite_ChainProjectile();

	virtual void Tick(float DeltaTime) override;

	void LaunchChain(APawn* TargetLocation, AAI_Elite* Boss);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ChainMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CollisionCapsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	float ChainSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	float MaxChainLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	float PullForce;
	FVector LeftHandLocation;
	FTimerHandle ChainTimerHandle;

private:

	void PullPlayer(ACharacter* PlayerCharacter);

	AAI_Elite* BossReference;
	APawn* TargetRef;
	float CurrentChainLength;
	bool bIsRetracting;
};