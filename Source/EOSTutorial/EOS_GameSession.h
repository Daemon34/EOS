// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "EOS_GameSession.generated.h"

/**
 * 
 */
UCLASS()
class EOSTUTORIAL_API AEOS_GameSession : public AGameSession
{
	GENERATED_BODY()
	
private:
	virtual void BeginPlay();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
	virtual bool ProcessAutoLogin();
	virtual void NotifyLogout(const APlayerController* ExitingPlayer);
	void RegisterPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite);
	void CreateSession(FName KeyName = "KeyName", FString KeyValue = "KeyValue");
	void StartSession();
	void UnregisterPlayer(const APlayerController* ExitingPlayer);
	void EndSession();
	void DestroySession();

	void HandleCreateSessionCompleted(FName EOSSessionName, bool bWasSuccessful);
	void HandleRegisterPlayerCompleted(FName EOSSessionName, const TArray<FUniqueNetIdRef>& PlayerIds, bool bWasSuccesful);
	void HandleStartSessionCompleted(FName EOSSessionName, bool bWasSuccessful);
	void HandleUnregisterPlayerCompleted(FName EOSSessionName, const TArray<FUniqueNetIdRef>& PlayerIds, bool bWasSuccessful);
	void HandleEndSessionCompleted(FName SessionName, bool bWasSuccessful);
	void HandleDestroySessionCompleted(FName EOSSessionName, bool bWasSuccessful);

	FDelegateHandle CreateSessionDelegateHandle; // Delegate to bind callback event for create session
	FDelegateHandle RegisterPlayerDelegateHandle;
	FDelegateHandle StartSessionDelegateHandle;
	FDelegateHandle UnregisterPlayerDelegateHandle;
	FDelegateHandle EndSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;

	bool bSessionExists = false; // Track if the server already create a session or not

	const int MaxNumberOfPlayersInSession = 2; // Maximum Number of players in the session
	int NumberOfPlayersInSession = 0; // Tracking of number of player in Session

	FName SessionName = "SessionName";
};
