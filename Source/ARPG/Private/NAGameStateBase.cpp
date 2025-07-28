// Fill out your copyright notice in the Description page of Project Settings.


#include "NAGameStateBase.h"

#include "NAPlayerController.h"
#include "NAPlayerState.h"
#include "Algo/AllOf.h"
#include "ARPG/ARPG.h"
#include "NAGlobalDelegate.h"
#include "Combat/UserWidget/NAMissionFailedWidget.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

void ANAGameStateBase::HandleRevive( APlayerState* PlayerState, ECharacterStatus CharacterStatus )
{
	++AlivePlayer;
	--KnockDownPlayer;
}

void ANAGameStateBase::HandleKnockDown( APlayerState* PlayerState, ECharacterStatus CharacterStatus )
{
	--AlivePlayer;
	++KnockDownPlayer;
	CheckAndHandleFailed();
}

void ANAGameStateBase::HandleDead( APlayerState* PlayerState, ECharacterStatus CharacterStatus )
{
	--KnockDownPlayer;
	CheckAndHandleFailed();
}

void ANAGameStateBase::UpdateMissionFailedWidget() const
{
	if ( UNAMissionFailedWidget* Widget = Cast<UNAMissionFailedWidget>( FailedWidgetComponent->GetWidget() ) )
	{
		Widget->UpdateVoteArray( RestartVoteArray );
	}
}

void ANAGameStateBase::OnRep_RestartVoteArray() const
{
	UpdateMissionFailedWidget();
}

void ANAGameStateBase::OnRep_Failed()
{
	CheckAndHandleFailed();
}

void ANAGameStateBase::ShowFailedWidget() const
{
	FailedWidgetComponent->SetVisibility( true );
	if ( UUserWidget* Widget = FailedWidgetComponent->GetWidget() )
	{
		Widget->AddToPlayerScreen();
		
		if ( APlayerController* PlayerController = GetWorld()->GetFirstPlayerController() )
		{
			FInputModeGameAndUI Mode{};
			Mode.SetWidgetToFocus( Widget->GetCachedWidget() );
			Mode.SetHideCursorDuringCapture( false );
			Mode.SetLockMouseToViewportBehavior( EMouseLockMode::DoNotLock );
			PlayerController->SetInputMode( Mode );
		}
	}
}

void ANAGameStateBase::RestartRound() const
{
	if ( HasAuthority() )
	{
		FString MapName = GetWorld()->GetOutermost()->GetName();
#if WITH_EDITOR
		MapName = GetWorld()->RemovePIEPrefix( MapName );
#endif
		GetWorld()->ServerTravel( MapName + "?listen", true );
	}
}

void ANAGameStateBase::CheckAndHandleFailed()
{
	if ( AlivePlayer != 0 )
	{
		return;
	}

	if ( HasAuthority() )
	{
		bFailed = true;
	}

	RestartVoteArray.SetNumZeroed( PlayerArray.Num(), true );
	ShowFailedWidget();
}

bool ANAGameStateBase::HasAnyoneDead() const
{
	return AlivePlayer != PlayerArray.Num();
}

bool ANAGameStateBase::IsFailed() const
{
	return bFailed;
}

void ANAGameStateBase::RemoveFailedWidget()
{
	FailedWidgetComponent->SetVisibility( false );
	if ( UUserWidget* UserWidget = FailedWidgetComponent->GetWidget() )
	{
		UserWidget->RemoveFromParent();
	}

	if ( APlayerController* PlayerController = GetWorld()->GetFirstPlayerController() )
	{
		FInputModeGameOnly Mode{};
		PlayerController->SetInputMode( Mode );
	}
	
	if ( HasAuthority() )
	{
		RestartVoteArray.SetNumZeroed( PlayerArray.Num(), true );
	}
}

ANAGameStateBase::ANAGameStateBase()
{
	FailedWidgetComponent = CreateDefaultSubobject<UWidgetComponent>( TEXT("FailedWidgetComponent") );
	FailedWidgetComponent->SetWidgetSpace( EWidgetSpace::Screen );

	FIND_CLASS( FailedWidgetClass , "/Script/UMGEditor.WidgetBlueprint'/Game/00_ProjectNA/01_Blueprint/01_Widget/InGame/BP_NAMissionFailedWidget.BP_NAMissionFailedWidget_C'" )
	FailedWidgetComponent->SetWidgetClass( FailedWidgetClass );
}

void ANAGameStateBase::VoteForRestart( APlayerState* PlayerState, bool bValue )
{
	if ( !bFailed )
	{
		return;
	}
	
	const int32 Index = PlayerArray.Find( PlayerState );
	RestartVoteArray[ Index ] = bValue;

	if ( FailedWidgetComponent->GetWidget() )
	{
		// 리슨 서버용 위젯 업데이트
		UpdateMissionFailedWidget();	
	}

	if ( Algo::AllOf( RestartVoteArray, []( const bool Element )
	{
		return Element;
	} ) )
	{
		if ( ANAPlayerController* PlayerController = Cast<ANAPlayerController>( GetWorld()->GetFirstPlayerController() ) )
		{
			PlayerController->Multi_RemoveFailedWidget();
		}
		RemoveFailedWidget();
		RestartRound();
	}
}

void ANAGameStateBase::BeginPlay()
{
	Super::BeginPlay();
	FailedWidgetComponent->SetVisibility( false );
}

void ANAGameStateBase::AddPlayerState(APlayerState* PlayerState)
{
	const int32 Index = PlayerArray.Num() % 2;
	Super::AddPlayerState(PlayerState);
	// 서버방향 PlayerState 추가
	GOnNewPlayerStateChanged.Broadcast( PlayerState );

	if (ANAPlayerState* CastedPlayerState = Cast<ANAPlayerState>(PlayerState))
	{
		if ( HasAuthority() )
		{
			// 임시 플레이어 에셋 지정.. 서버에서 지정하면 리플리케이션으로 클라이언트에서 업데이트
			const FName CharacterAssetNames[]  = { "Male", "Female" };
			CastedPlayerState->SetPossessAssetName(CharacterAssetNames[Index]);

			// 서버에서 플레이어 상태 추적
			CastedPlayerState->OnRevived.AddUObject( this, &ANAGameStateBase::HandleRevive );
			CastedPlayerState->OnKnockDown.AddUObject( this, &ANAGameStateBase::HandleKnockDown );
			CastedPlayerState->OnDead.AddUObject( this, &ANAGameStateBase::HandleDead );

			++AlivePlayer;
		}
	}
}

void ANAGameStateBase::RemovePlayerState( APlayerState* PlayerState )
{
	Super::RemovePlayerState( PlayerState );
	--AlivePlayer;
	GOnNewPlayerStateChanged.Broadcast( PlayerState );
}

void ANAGameStateBase::PreReplication( IRepChangedPropertyTracker& ChangedPropertyTracker )
{
	Super::PreReplication( ChangedPropertyTracker );
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST( ANAGameStateBase, RestartVoteArray, bFailed )
}

void ANAGameStateBase::GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	DOREPLIFETIME( ANAGameStateBase, bFailed )
	DOREPLIFETIME_CONDITION( ANAGameStateBase, RestartVoteArray, COND_Custom )
}
