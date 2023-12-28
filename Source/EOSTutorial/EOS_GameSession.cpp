// Fill out your copyright notice in the Description page of Project Settings.


#include "EOS_GameSession.h"
#include "EOS_PlayerController.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineStatsInterface.h"
#include "GameFramework/PlayerState.h"

void AEOS_GameSession::BeginPlay() {
	Super::BeginPlay();
	// Only create a session if running as a dedicated server and session doesn't exist
	if (IsRunningDedicatedServer() && !bSessionExists) {
		CreateSession("KeyName", "KeyValue");
	}
}

bool AEOS_GameSession::ProcessAutoLogin() {
	// Overide base function as players need to login before joining the session and we don't want to call AutoLogin on server.
	return true;
}

// Override base function to register player in EOS Session - Called Automatically when player join the server
void AEOS_GameSession::RegisterPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite) {
	Super::RegisterPlayer(NewPlayer, UniqueId, bWasFromInvite);

	// Only run on Dedicated Server
	if (IsRunningDedicatedServer()) {
		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
		IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

		RegisterPlayerDelegateHandle = Session->AddOnRegisterPlayersCompleteDelegate_Handle(FOnRegisterPlayersCompleteDelegate::CreateUObject(this, &AEOS_GameSession::HandleRegisterPlayerCompleted));
	
		if (!Session->RegisterPlayer(SessionName, *UniqueId, false)) {
			UE_LOG(LogTemp, Warning, TEXT("Failed to register player !"));
			Session->ClearOnRegisterPlayersCompleteDelegate_Handle(RegisterPlayerDelegateHandle);
			RegisterPlayerDelegateHandle.Reset();
		}
	}
}

void AEOS_GameSession::HandleRegisterPlayerCompleted(FName EOSSessionName, const TArray<FUniqueNetIdRef>& PlayerIds, bool bWasSuccessful ) {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	if (bWasSuccessful) {
		UE_LOG(LogTemp, Log, TEXT("Player Registered in EOS Session !"));
		NumberOfPlayersInSession++;
		if (NumberOfPlayersInSession == MaxNumberOfPlayersInSession) {
			StartSession(); // Start the session when we reached the maximum number of players in the session
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Failed to register player ! (From Callback)"));
	}

	// Clear and reset delegate
	Session->ClearOnRegisterPlayersCompleteDelegate_Handle(RegisterPlayerDelegateHandle);
	RegisterPlayerDelegateHandle.Reset();
}

void AEOS_GameSession::StartSession() {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	StartSessionDelegateHandle = Session->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &AEOS_GameSession::HandleStartSessionCompleted));

	if (!Session->StartSession(SessionName)) {
		UE_LOG(LogTemp, Warning, TEXT("Failed to start session !"));
		Session->ClearOnStartSessionCompleteDelegate_Handle(StartSessionDelegateHandle);
		StartSessionDelegateHandle.Reset();
	}
}

void AEOS_GameSession::HandleStartSessionCompleted(FName EOSSessionName, bool bWasSuccessful) {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	if (bWasSuccessful) {
		UE_LOG(LogTemp, Log, TEXT("Session started !"));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Failed to start session ! (From callback)"));
	}

	Session->ClearOnStartSessionCompleteDelegate_Handle(StartSessionDelegateHandle);
	StartSessionDelegateHandle.Reset();
}

void AEOS_GameSession::UnregisterPlayer(const APlayerController* ExitingPlayer) {
	Super::UnregisterPlayer(ExitingPlayer);

	if (IsRunningDedicatedServer()) {
		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
		IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
		IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();

		// This is null if player left because crashes or network failure
		if (ExitingPlayer->PlayerState) {
			UnregisterPlayerDelegateHandle = Session->AddOnUnregisterPlayersCompleteDelegate_Handle(FOnUnregisterPlayersCompleteDelegate::CreateUObject(this, &AEOS_GameSession::HandleUnregisterPlayerCompleted));

			if (!Session->UnregisterPlayer(SessionName, *ExitingPlayer->PlayerState->UniqueId)) {
				UE_LOG(LogTemp, Warning, TEXT("Failed to Unregister Player !"));
				Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(UnregisterPlayerDelegateHandle);
				UnregisterPlayerDelegateHandle.Reset();
			}
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("Failed to Unregister Player ! Player probably disconnected ungracefully."));
			Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(UnregisterPlayerDelegateHandle);
			UnregisterPlayerDelegateHandle.Reset();
		}
	}
}

void AEOS_GameSession::HandleUnregisterPlayerCompleted(FName EOSSessionName, const TArray<FUniqueNetIdRef>& PlayerIds, bool bWasSuccessful) {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	if (bWasSuccessful) {
		UE_LOG(LogTemp, Log, TEXT("Player unregistered in EOS Session !"));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Failed to unregister player ! (From callback)"));
	}

	Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(UnregisterPlayerDelegateHandle);
	UnregisterPlayerDelegateHandle.Reset();
}

void AEOS_GameSession::NotifyLogout(const APlayerController* ExitingPlayer) {
	Super::NotifyLogout(ExitingPlayer); // This also call UnregisterPlayer function

	// Dedicated Server Only - No matter if it fails to unregister player, end the session when all players left
	if (IsRunningDedicatedServer()) {
		NumberOfPlayersInSession--;
		if (NumberOfPlayersInSession == 0) {
			EndSession();
		}
	}
}

void AEOS_GameSession::EndSession() {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	EndSessionDelegateHandle = Session->AddOnEndSessionCompleteDelegate_Handle(FOnEndSessionCompleteDelegate::CreateUObject(this, &AEOS_GameSession::HandleEndSessionCompleted));

	if (!Session->EndSession(SessionName)) {
		UE_LOG(LogTemp, Warning, TEXT("Failed to end session !"));
		Session->ClearOnEndSessionCompleteDelegate_Handle(EndSessionDelegateHandle);
		EndSessionDelegateHandle.Reset();
	}
}

void AEOS_GameSession::HandleEndSessionCompleted(FName SessionName, bool bWasSuccessful) {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	if (bWasSuccessful) {
		UE_LOG(LogTemp, Log, TEXT("Session ended !"));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Failed to end session !"));
	}

	Session->ClearOnEndSessionCompleteDelegate_Handle(EndSessionDelegateHandle);
	EndSessionDelegateHandle.Reset();
}

void AEOS_GameSession::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
	DestroySession();
}

void AEOS_GameSession::DestroySession() {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	DestroySessionDelegateHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &AEOS_GameSession::HandleDestroySessionCompleted));

	if (!Session->DestroySession(SessionName)) {
		UE_LOG(LogTemp, Warning, TEXT("Failed to destroy session"));
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		DestroySessionDelegateHandle.Reset();
	}
}

void AEOS_GameSession::HandleDestroySessionCompleted(FName EOSSessionName, bool bWasSuccessful) {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	if (bWasSuccessful) {
		bSessionExists = false;
		UE_LOG(LogTemp, Log, TEXT("Destroyed session successfully !"));
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Failed to destroy session !"));
	}

	Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
	DestroySessionDelegateHandle.Reset();
}

// Create an EOS Session - Dedicated Server Only
void AEOS_GameSession::CreateSession(FName KeyName, FString KeyValue)
{
	// Retrieve the session interface
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	// Bind delegate to callback function
	CreateSessionDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &AEOS_GameSession::HandleCreateSessionCompleted));

	// Set session settings
	TSharedRef<FOnlineSessionSettings> SessionSettings = MakeShared<FOnlineSessionSettings>();
	SessionSettings->NumPublicConnections = MaxNumberOfPlayersInSession;
	SessionSettings->bShouldAdvertise = true; // Creates a public match and will be searchable. Also set the session as joinable via presence.
	SessionSettings->bUsesPresence = false; // No presence on dedicated server. This requires a local user.
	SessionSettings->bAllowJoinViaPresence = false; // Superset by bShouldAdvertise and will be true on the backend.
	SessionSettings->bAllowJoinViaPresenceFriendsOnly = false; // Superset by bShouldAdvertise and will be true on the backend.
	SessionSettings->bAllowInvites = false; // Allow inviting players into session. This requires presence and a local user.
	SessionSettings->bAllowJoinInProgress = false; // Once the session is started, no one can join.
	SessionSettings->bIsDedicated = true; // Session created on dedicated server.
	SessionSettings->bUseLobbiesIfAvailable = false; // This is an EOS Session not an EOS Lobby as they aren't supported on Dedicated Servers.
	SessionSettings->bUseLobbiesVoiceChatIfAvailable = false; // We are not using lobbies
	SessionSettings->bUsesStats = true; // Required if we need to track player stats

	// Add custom attribute used when searching on GameClients
	SessionSettings->Settings.Add(KeyName, FOnlineSessionSetting((KeyValue), EOnlineDataAdvertisementType::ViaOnlineService));

	// Create the Session
	UE_LOG(LogTemp, Log, TEXT("Creating EOS Session..."));

	if (!Session->CreateSession(0, SessionName, *SessionSettings)) {
		UE_LOG(LogTemp, Warning, TEXT("Failed to create session !"));
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		CreateSessionDelegateHandle.Reset();
	}
}

// Dedicated Server Only
void AEOS_GameSession::HandleCreateSessionCompleted(FName EOSSessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	if (bWasSuccessful) {
		bSessionExists = true;
		UE_LOG(LogTemp, Log, TEXT("Session %s created !"), *EOSSessionName.ToString());
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Failed to create Session !"));
	}

	Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
	CreateSessionDelegateHandle.Reset();
}
