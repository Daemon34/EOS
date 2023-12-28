// Fill out your copyright notice in the Description page of Project Settings.


#include "EOS_PlayerController.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemTypes.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

// Default class constructor
AEOS_PlayerController::AEOS_PlayerController()
{
}

// On BeginPlay call our login function. This is only on the GameClient, not on the DedicatedServer.
void AEOS_PlayerController::BeginPlay()
{
	Super::BeginPlay(); // Call parent class BeginPlay
	Login(); // Login
}

/*
This function will access the EOS OSS via the OSS identity interface to log first into Epic Account Services, and then into Epic Game Services.
It will bind a delegate to handle the callback event once login call succeeeds or fails.
All functions that access the OSS will have this structure: 
	1 - Get OSS interface
	2 - Bind delegate for callback
	3 - Call OSS interface function (which will call the correspongin EOS OSS function)
*/
void AEOS_PlayerController::Login()
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface(); // Generic OSS interface that will access the EOS Online Subsystem.

	// If you're logged in, don't try to login again.
	// This can happen if your player travels to a dedicated server or different maps as BeginPlay() will be called each time.
	FUniqueNetIdPtr NetId = Identity->GetUniquePlayerId(0);
	if (NetId != nullptr && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn) {
		return;
	}

	// This binds a delegate so we can run our function when the callback completes. 0 represents the player number.
	LoginDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateUObject(this, &AEOS_PlayerController::HandleLoginCompleted));

	// Grab command line parameters. If empty call hardcoded login function
	FString AuthType, AuthLogin, AuthPassword;
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), AuthType);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), AuthLogin);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), AuthPassword);

	if (!AuthType.IsEmpty() && !AuthLogin.IsEmpty() && !AuthPassword.IsEmpty()) {
		// Automatically log a player in using the parameters passed via CLI.
		UE_LOG(LogTemp, Log, TEXT("Auto-logging into EOS..."));

		if (!Identity->AutoLogin(0)) {
			UE_LOG(LogTemp, Warning, TEXT("Failed to auto-login !"));
			Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginDelegateHandle);
			LoginDelegateHandle.Reset();
		}
	}
	else {
		// Fallback if the CLI parameters are empty
		// The account type here could be any of this : https://dev.epicgames.com/docs/epic-account-services/auth/auth-interface#preferred-login-types-for-epic-account
		FOnlineAccountCredentials Credentials("AccountPortal", "", "");
		UE_LOG(LogTemp, Log, TEXT("Login into EOS..."));

		if (!Identity->Login(0, Credentials)) {
			UE_LOG(LogTemp, Warning, TEXT("Failed to login !"));
			Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginDelegateHandle);
			LoginDelegateHandle.Reset();
		}
	}
}

/*
This function handles the callback from logging in. You should not proceed with any EOS features until this function is called.
This function will remove the delegate that was bound in the Login() function.
*/
void AEOS_PlayerController::HandleLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
	if (bWasSuccessful) {
		UE_LOG(LogTemp, Log, TEXT("Login callback completed !"));
		FindSessions();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("EOS Login Failed !"));
	}

	Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);
	LoginDelegateHandle.Reset();
}

void AEOS_PlayerController::FindSessions(FName SearchKey, FString SearchValue)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
	TSharedRef<FOnlineSessionSearch> Search = MakeShared<FOnlineSessionSearch>();

	// Remove the default settings that FOnlineSessionSearch set
	Search->QuerySettings.SearchParams.Empty();

	// Search using key/value attributes
	Search->QuerySettings.Set(SearchKey, SearchValue, EOnlineComparisonOp::Equals);
	FindSessionsDelegateHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &AEOS_PlayerController::HandleFindSessionsCompleted, Search));

	UE_LOG(LogTemp, Log, TEXT("Finding sessions..."));

	if (!Session->FindSessions(0, Search)) {
		UE_LOG(LogTemp, Warning, TEXT("Failed to find session !"));
	}
}

void AEOS_PlayerController::HandleFindSessionsCompleted(bool bWasSuccesful, TSharedRef<FOnlineSessionSearch> Search) {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	if (bWasSuccesful) {
		UE_LOG(LogTemp, Log, TEXT("Found sessions !"));

		for (FOnlineSessionSearchResult SessionInSearchResult : Search->SearchResults) {
			// Check if Session is valid => Currently a bug in EOS make IsValid() to always return false on DS so we skip this step

			// Ensure the connection string is resolvable and store the info in ConnectInfo and in SessionToJoin
			if (Session->GetResolvedConnectString(SessionInSearchResult, NAME_GamePort, ConnectString)) {
				SessionToJoin = &SessionInSearchResult;
			}

			// Currently, just take the first session. TODO : Find the BEST session
			break;
		}
		JoinSession();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Failed to find session ! (From Callback)"));
	}

	Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
	FindSessionsDelegateHandle.Reset();
}

void AEOS_PlayerController::JoinSession() {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	JoinSessionDelegateHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &AEOS_PlayerController::HandleJoinSessionCompleted));

	UE_LOG(LogTemp, Log, TEXT("Joining Session..."));
	if (!Session->JoinSession(0, "SessionName", *SessionToJoin)) {
		UE_LOG(LogTemp, Warning, TEXT("Join Session Failed !"));
	}
}

void AEOS_PlayerController::HandleJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result) {
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();

	if (Result == EOnJoinSessionCompleteResult::Success) {
		UE_LOG(LogTemp, Log, TEXT("Joined Session !"));
		if (GEngine) {
			ConnectString = "127.0.0.1:7777"; // Override Connect String to localhost for development purpose
			FURL DedicatedServerURL(nullptr, *ConnectString, TRAVEL_Absolute);
			FString DedicatedServerJoinError;
			EBrowseReturnVal::Type DedicatedServerJoinStatus = GEngine->Browse(GEngine->GetWorldContextFromWorldChecked(GetWorld()), DedicatedServerURL, DedicatedServerJoinError);
			if (DedicatedServerJoinStatus == EBrowseReturnVal::Failure) {
				UE_LOG(LogTemp, Error, TEXT("Failed to browse for dedicated server. Error is: %s"), *DedicatedServerJoinError);
			}

			// No check of NetworkError or TravelError events
		}
	}
	Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
	JoinSessionDelegateHandle.Reset();
}
