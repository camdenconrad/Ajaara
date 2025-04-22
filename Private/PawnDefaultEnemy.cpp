// Fill out your copyright notice in the Description page of Project Settings.


#include "PawnDefaultEnemy.h"

// Sets default values
APawnDefaultEnemy::APawnDefaultEnemy()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void APawnDefaultEnemy::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APawnDefaultEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APawnDefaultEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
