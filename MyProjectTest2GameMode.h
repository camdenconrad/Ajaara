// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyProjectTest2GameMode.generated.h"

UCLASS(minimalapi)
class AMyProjectTest2GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyProjectTest2GameMode();
	void Tick(float DeltaTime);
	void BeginPlay();

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	float FrameRate = 1.0f;
	
	float FrameStartTime;
	TSubclassOf<UUserWidget> HUDWidget;
};
