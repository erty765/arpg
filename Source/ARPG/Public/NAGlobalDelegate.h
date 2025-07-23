#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

DECLARE_MULTICAST_DELEGATE_OneParam( FOnNewPlayerStateChanged, APlayerState* );
extern FOnNewPlayerStateChanged GOnNewPlayerStateChanged;