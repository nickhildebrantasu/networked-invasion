// Copyright Epic Games, Inc. All Rights Reserved.

#include "CapstoneGameMode.h"
#include "CapstoneCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACapstoneGameMode::ACapstoneGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> NetworkPlayerBPClass( TEXT( "/Game/MultiplayerChat/BP_NetworkPlayerController" ) );
	if ( NetworkPlayerBPClass.Class != NULL ) PlayerControllerClass = NetworkPlayerBPClass.Class;
}
