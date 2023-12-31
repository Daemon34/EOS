// Fill out your copyright notice in the Description page of Project Settings.


#include "EOS_GameInstance.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"

void UEOS_GameInstance::LoginWithEOS(FString ID, FString Token, FString LoginType)
{
	IOnlineSubsystem* SubsystemRef = Online::GetSubsystem(this->GetWorld());
	if (SubsystemRef) {
		IOnlineIdentityPtr IdentityPointerRef = SubsystemRef->GetIdentityInterface();
		if (IdentityPointerRef) {
			FOnlineAccountCredentials AccountDetails = { LoginType, ID, Token };
			IdentityPointerRef->OnLoginCompleteDelegates->AddUObject(this, &UEOS_GameInstance::LoginWithEOS_Return);
			IdentityPointerRef->Login(0, AccountDetails);
		}
	}
}

FString UEOS_GameInstance::GetPlayerUsername()
{
	IOnlineSubsystem* SubsystemRef = Online::GetSubsystem(this->GetWorld());
	if (SubsystemRef) {
		IOnlineIdentityPtr IdentityPointerRef = SubsystemRef->GetIdentityInterface();
		if (IdentityPointerRef) {
			if (IdentityPointerRef->GetLoginStatus(0) == ELoginStatus::LoggedIn) {
				return IdentityPointerRef->GetPlayerNickname(0);
			}
		}
	}
	return FString();
}

bool UEOS_GameInstance::IsPlayerLoggedIn()
{
	IOnlineSubsystem* SubsystemRef = Online::GetSubsystem(this->GetWorld());
	if (SubsystemRef) {
		IOnlineIdentityPtr IdentityPointerRef = SubsystemRef->GetIdentityInterface();
		if (IdentityPointerRef) {
			if (IdentityPointerRef->GetLoginStatus(0) == ELoginStatus::LoggedIn) {
				return true;
			}
		}
	}
	return false;
}

void UEOS_GameInstance::CreateEOSSession(bool bIsDedicatedServer, bool bIsLanServer, int32 NumberOfPublicConnections)
{
	IOnlineSubsystem* SubsystemRef = Online::GetSubsystem(this->GetWorld());
	if (SubsystemRef) {
		IOnlineSessionPtr SessionPtrRef = SubsystemRef->GetSessionInterface();
		if (SessionPtrRef) {
			FOnlineSessionSettings SessionCreationInfo;
			SessionCreationInfo.bIsDedicated = bIsDedicatedServer;
			SessionCreationInfo.bAllowInvites = false;
			SessionCreationInfo.bIsLANMatch = bIsLanServer;
			SessionCreationInfo.NumPublicConnections = NumberOfPublicConnections;
			SessionCreationInfo.bUseLobbiesIfAvailable = false;
			SessionCreationInfo.bUsesPresence = false;
			SessionCreationInfo.bAllowJoinViaPresence = false;
			SessionCreationInfo.bAllowJoinViaPresenceFriendsOnly = false;
			SessionCreationInfo.bShouldAdvertise = true;
			SessionCreationInfo.Set(SEARCH_KEYWORDS, FString("RandomHi"), EOnlineDataAdvertisementType::ViaOnlineService);
			SessionPtrRef->OnCreateSessionCompleteDelegates.AddUObject(this, &UEOS_GameInstance::OnCreateSessionCompleted);
			SessionPtrRef->CreateSession(0, FName("MainSession"), SessionCreationInfo);
		}
	}
}

void UEOS_GameInstance::FindSessionAndJoin()
{
	IOnlineSubsystem* SubsystemRef = Online::GetSubsystem(this->GetWorld());
	if (SubsystemRef) {
		IOnlineSessionPtr SessionPtrRef = SubsystemRef->GetSessionInterface();
		if (SessionPtrRef) {
			SessionSearch = MakeShareable(new FOnlineSessionSearch());
			SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
			SessionSearch->bIsLanQuery = false;
			SessionSearch->MaxSearchResults = 20;
			SessionSearch->QuerySettings.SearchParams.Empty();
			SessionPtrRef->OnFindSessionsCompleteDelegates.AddUObject(this, &UEOS_GameInstance::OnFindSessionCompleted);
			SessionPtrRef->FindSessions(0, SessionSearch.ToSharedRef());
		}
	}
}

void UEOS_GameInstance::JoinSession()
{
}

void UEOS_GameInstance::DestroySession()
{
	IOnlineSubsystem* SubsystemRef = Online::GetSubsystem(this->GetWorld());
	if (SubsystemRef) {
		IOnlineSessionPtr SessionPtrRef = SubsystemRef->GetSessionInterface();
		if (SessionPtrRef) {
			SessionPtrRef->OnDestroySessionCompleteDelegates.AddUObject(this, &UEOS_GameInstance::OnDestroySessionCompleted);
			SessionPtrRef->DestroySession(FName("MainSession"));
		}
	}
}

void UEOS_GameInstance::LoginWithEOS_Return(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserID, const FString& Error)
{
	if (bWasSuccessful) {
		UE_LOG(LogTemp, Warning, TEXT("Login Successful"));
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Login Fail Reason - %s"), *Error);
	}
}

void UEOS_GameInstance::OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful) {
		GetWorld()->ServerTravel(OpenLevelText);
	}
}

void UEOS_GameInstance::OnDestroySessionCompleted(FName SessionName, bool bWasSuccessful)
{
}

void UEOS_GameInstance::OnFindSessionCompleted(bool bWasSuccessful)
{
	if (bWasSuccessful) {
		IOnlineSubsystem* SubsystemRef = Online::GetSubsystem(this->GetWorld());
		if (SubsystemRef) {
			IOnlineSessionPtr SessionPtrRef = SubsystemRef->GetSessionInterface();
			if (SessionPtrRef) {
				if (SessionSearch->SearchResults.Num() > 0) {
					SessionPtrRef->OnJoinSessionCompleteDelegates.AddUObject(this, &UEOS_GameInstance::OnJoinSessionCompleted);
					SessionPtrRef->JoinSession(0, FName("MainSession"), SessionSearch->SearchResults[0]);
				}
				else {
					UE_LOG(LogTemp, Error, TEXT("Server Not Found"));
					//CreateEOSSession(false, false, 10);
				}
			}
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Server Not Found"));
		//CreateEOSSession(false, false, 10);
	}
}

void UEOS_GameInstance::OnJoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success) {
		if (APlayerController* PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0)) {
			FString JoinAddress;
			IOnlineSubsystem* SubsystemRef = Online::GetSubsystem(this->GetWorld());
			if (SubsystemRef) {
				IOnlineSessionPtr SessionPtrRef = SubsystemRef->GetSessionInterface();
				if (SessionPtrRef) {
					SessionPtrRef->GetResolvedConnectString(FName("MainSession"), JoinAddress);
					UE_LOG(LogTemp, Warning, TEXT("Joining Address : %s"), *JoinAddress);
					if (!JoinAddress.IsEmpty()) {
						PlayerControllerRef->ClientTravel(JoinAddress, ETravelType::TRAVEL_Absolute);
					}
				}
			}
		}
	}
}
