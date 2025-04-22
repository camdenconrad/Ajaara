#include "Elite_ThrowableAxe.h"

#include "NiagaraFunctionLibrary.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MyProjectTest2/CPP_Repo/MyProjectTest2Character.h"

class AMyProjectTest2Character;

AElite_ThrowableAxe::AElite_ThrowableAxe()
{
	PrimaryActorTick.bCanEverTick = true;

	AxeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AxeMesh"));
	RootComponent = AxeMesh;

	// Enable collision and ignore the boss
	//AxeMesh->SetCollisionProfileName(TEXT("BlockAll"));
//	AxeMesh->SetGenerateOverlapEvents(true);

	// Set up collision
	// AxeMesh->SetNotifyRigidBodyCollision(true);
	// AxeMesh->OnComponentHit.AddDynamic(this, &AElite_ThrowableAxe::OnAxeHit);
	//
	// AxeMesh->SetNotifyRigidBodyCollision(true);
	// AxeMesh->OnComponentHit.AddDynamic(this, &AElite_ThrowableAxe::OnAxeHit);
	// AxeMesh->OnComponentBeginOverlap.AddDynamic(this, &AElite_ThrowableAxe::OnAxeOverlap);
}

void AElite_ThrowableAxe::OnAxeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//HandleAxeCollision(OtherActor);
}

void AElite_ThrowableAxe::OnAxeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//HandleAxeCollision(OtherActor);
}

void AElite_ThrowableAxe::HandleAxeCollision(AActor* OtherActor)
{
	// if (!OtherActor) return;
	//
	// // Check if we hit the player
	// AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(OtherActor);
	// if (!PlayerCharacter)
	// {
	// 	// If the cast failed, try to get the player character from the hit actor's owner
	// 	AActor* OwnerActor = OtherActor->GetOwner();
	// 	if (OwnerActor)
	// 	{
	// 		PlayerCharacter = Cast<AMyProjectTest2Character>(OwnerActor);
	// 	}
	// }
	//
	// if (PlayerCharacter)
	// {
	// 	// Call the KickDamage function instead of applying damage directly
	// 	FDamageEvent DamageEvent;
	// 	PlayerCharacter->KickDamage(100.0f, DamageEvent, nullptr, this);
	// 	bHasAppliedDamageInCurrentAttack = true;
	//
	// 	// Start returning after hitting the player
	// 	bReturning = true;
	// 	ThrowStartTime = GetWorld()->GetTimeSeconds();
	//
	// 	// Debug Message
	// 	UE_LOG(LogTemp, Warning, TEXT("Axe hit the player and kicked them!"));
	// }
	// // Check if we hit the boss
	// else if (OtherActor == BossReference)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Axe hit the boss and is being destroyed!"));
	// 	Destroy();
	// }
}

void AElite_ThrowableAxe::BeginPlay()
{
	Super::BeginPlay();

	StartLocation = GetActorLocation();

	// Rotate the axe 90 degrees forward so it lays flat
	FRotator MidWay = FRotator(0.0f, 0.0f, -90.0f);
	AxeMesh->SetWorldRotation(MidWay);
}

void AElite_ThrowableAxe::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),    // Get world context
			Effect,     // Niagara system to spawn
			this->GetActorLocation(),   // Location of the hit
			this->GetActorRotation()    // Rotation to align with surface
		);

	// Calculate the rotation around the Center
	float ElapsedTime = GetWorld()->GetTimeSeconds() - ThrowStartTime;
	float RotationAngle = 300.0f * DeltaTime; // Use DeltaTime for consistent rotation speed

	// Calculate the new position
	FVector RotationAxis = FVector::UpVector; // Rotate around the Z-axis
	FVector CurrentPosition = GetActorLocation();
	FVector RelativePosition = CurrentPosition - Center;
	FVector NewRelativePosition = RelativePosition.RotateAngleAxis(RotationAngle, RotationAxis);
	FVector NewPosition = Center + NewRelativePosition.GetSafeNormal() * Radius;

	// Set the new position
	SetActorLocation(NewPosition);

	if (FVector::Dist(this->GetActorLocation(), BossReference->GetActorLocation()) <= 300.0f && ElapsedTime >= .5f)
	{
		Destroy();
	}
	HitPlayer();
    
	// Make the axe spin
	AxeMesh->AddLocalRotation(FRotator(SpinSpeed * DeltaTime, 0.0f, 0.0f));
}

void AElite_ThrowableAxe::ThrowAxe(FVector TargetLocation, AActor* Boss, AActor* TargetActor)
{
	EndLocation = TargetLocation;
	BossReference = Boss;
	bReturning = false;
	ThrowStartTime = GetWorld()->GetTimeSeconds();
	Player = Cast<AMyProjectTest2Character>(TargetActor);

	Center = (EndLocation + BossReference->GetActorLocation()) * 0.5f;

	// Calculate the radius of the circular path
	Radius = (EndLocation - BossReference->GetActorLocation()).Size() * 0.5f;

	// Set up collision
	// AxeMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// AxeMesh->SetCollisionObjectType(ECC_WorldDynamic);
	// AxeMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	// AxeMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	// AxeMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	// AxeMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	if (BossReference)
	{
		AxeMesh->IgnoreActorWhenMoving(BossReference, true);
	}
}

void AElite_ThrowableAxe::HitPlayer()
{
	if (FVector::Dist(this->GetActorLocation(), Player->GetActorLocation()) <= 150.0f)
	{
		AMyProjectTest2Character* PlayerCharacter = Cast<AMyProjectTest2Character>(Player);
		if (!PlayerCharacter)
		{
			// If the cast failed, try to get the player character from the hit actor's owner
			AActor* OwnerActor = Player->GetOwner();
			if (OwnerActor)
			{
				PlayerCharacter = Cast<AMyProjectTest2Character>(OwnerActor);
			}
		}
		if (PlayerCharacter)
		{
			if (!bHasAppliedDamageInCurrentAttack)
			{
				bHasAppliedDamageInCurrentAttack = true;
				// Call the KickDamage function instead of applying damage directly
				FDamageEvent DamageEvent;
				PlayerCharacter->KickDamage(50.0f, DamageEvent, nullptr, this);
	
				// Start returning after hitting the player
				bReturning = true;
			}
		}
	}
}