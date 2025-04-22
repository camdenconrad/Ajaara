#include "Elite_ChainProjectile.h"
#include "AI_Elite.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Elite_ChainProjectile.h"
#include "AI_Elite.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AElite_ChainProjectile::AElite_ChainProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    // Initialize variables
    ChainSpeed = 30.f;
    MaxChainLength = 5.f;
}

void AElite_ChainProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Find the existing Chain mesh component
    ChainMesh = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("Chain")));
    if (!ChainMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("ChainMesh component not found!"));
    }

    // Find the existing collision capsule
    CollisionCapsule = Cast<UCapsuleComponent>(GetDefaultSubobjectByName(TEXT("CollisionCapsule")));
    if (CollisionCapsule)
    {
        CollisionCapsule->SetCollisionProfileName(TEXT("Trigger"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("CollisionCapsule component not found!"));
    }

    CurrentChainLength = 0.0f;
    this->SetActorScale3D(FVector(.2, 0.2f, .2f));
}

void AElite_ChainProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Calculate the distance to target
    float DistanceToTarget = FVector::Dist(LeftHandLocation, TargetRef->GetActorLocation());
    
    LeftHandLocation = BossReference->GetMesh()->GetSocketLocation(FName("LeftHandSocket"));
    this->SetActorLocation(LeftHandLocation);

    FVector Direction = (TargetRef->GetActorLocation() - LeftHandLocation).GetSafeNormal();
    FRotator NewRotation = Direction.Rotation() - FRotator(90.f, 0.f, 90.f);
    this->SetActorRotation(NewRotation);

    // If the chain hasn't reached the target, extend it
    if (CurrentChainLength < DistanceToTarget && CurrentChainLength < MaxChainLength)
    {
        CurrentChainLength += ChainSpeed * DeltaTime;
    }

    // Clamp the chain length to make sure it doesn't exceed the target distance or max length
    CurrentChainLength = FMath::Clamp(CurrentChainLength, 0.f, FMath::Min(DistanceToTarget, MaxChainLength));

    // Set the chain's mesh scale based on the current length
    ChainMesh->SetWorldScale3D(FVector(.4f, CurrentChainLength, .4f));  // You can adjust the scaling factors as needed
}

void AElite_ChainProjectile::LaunchChain(APawn* Target, AAI_Elite* Boss)
{
    BossReference = Boss;
    TargetRef = Target;
    CurrentChainLength = 0.f;
    bIsRetracting = false;

    // Set initial position to the boss's left hand socket
    if (BossReference)
    {
        SetActorLocation(BossReference->GetMesh()->GetSocketLocation(FName("LeftHandSocket")));
    }

    GetWorldTimerManager().SetTimer(
        ChainTimerHandle,
        [this]()
        {
            Destroy();
        },
        2.f,
        false
    );
}

void AElite_ChainProjectile::PullPlayer(ACharacter* PlayerCharacter)
{
    if (PlayerCharacter && BossReference)
    {
        FVector PullDirection = (BossReference->GetActorLocation() - PlayerCharacter->GetActorLocation()).GetSafeNormal();
        PlayerCharacter->GetCharacterMovement()->AddImpulse(PullDirection * PullForce, true);
    }
}