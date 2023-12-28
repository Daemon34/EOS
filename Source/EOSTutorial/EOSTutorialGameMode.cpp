// Copyright Epic Games, Inc. All Rights Reserved.

#include "EOSTutorialGameMode.h"
#include "EOSTutorialCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "EOS_PlayerController.h"
#include "EOS_GameSession.h"

AEOSTutorialGameMode::AEOSTutorialGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	PlayerControllerClass = AEOS_PlayerController::StaticClass(); // Set the PlayerController to our custome one.
	GameSessionClass = AEOS_GameSession::StaticClass(); // Set the GameDession to our custom one.

	// Network Travel : https://docs.unrealengine.com/4.27/en-US/InteractiveExperiences/Networking/Travelling/
}
