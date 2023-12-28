// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EOS_PlayerController.generated.h"

/**
 * Child class of APlayerController to hols EOS code
 */
class FOnlineSessionSearch;
class FOnlineSessionSearchResult;

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

	void FindSessions(FName SearchKey = "KeyName", FString SearchValue = "KeyValue");

	void HandleFindSessionsCompleted(bool bWasSuccessful, TSharedRef<FOnlineSessionSearch> Search);

	FDelegateHandle FindSessionsDelegateHandle;

	FString ConnectString;

	FOnlineSessionSearchResult* SessionToJoin;

	void JoinSession();

	void HandleJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	FDelegateHandle JoinSessionDelegateHandle;
};
