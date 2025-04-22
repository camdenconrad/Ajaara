// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectTest2GameMode.h"
#include "MyProjectTest2Character.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"

AMyProjectTest2GameMode::AMyProjectTest2GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	//static ConstructorHelpers::FClassFinder<UUserWidget> HUDWidgetClass(TEXT("/Game/ThirdPerson/Blueprints/HUD.HUD"));
 // Change path accordingly
	// if (HUDWidgetClass.Succeeded())
	// {
	// 	HUDWidget = HUDWidgetClass.Class;
	// }
}

void AMyProjectTest2GameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Get the real-time seconds from the game world
	float FrameEndTime = GetWorld()->GetRealTimeSeconds();
	float FrameDuration = FrameEndTime - FrameStartTime;


	FrameRate = 1.0f / FrameDuration;
	// Update the start time for the next frame
	FrameStartTime = FrameEndTime;
}

void AMyProjectTest2GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HUDWidget != nullptr)
	{
		UUserWidget* WidgetInstance = CreateWidget<UUserWidget>(GetWorld(), HUDWidget);
		if (WidgetInstance != nullptr)
		{
			WidgetInstance->AddToViewport();
		}
	}
	// Enable ticking
	PrimaryActorTick.bCanEverTick = true;
}