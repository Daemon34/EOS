// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EOS_PlayerController.generated.h"

/**
 * Child class of APlayerController to hols EOS code
 */
UCLASS()
class EOSTUTORIAL_API AEOS_PlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	// Class constructor
	AEOS_PlayerController();

protected:
	// Function called when play begins
	virtual void BeginPlay();

	// Function to log in to EOS Game Services
	void Login();

	// Callback function runned when user have logged in to EOS
	void HandleLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	// Delegate to bind callback event for login
	FDelegateHandle LoginDelegateHandle;
};
