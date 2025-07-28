// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/NASessionListWidget.h"

#include "NAPlayerController.h"
#include "ARPG/ARPG.h"
#include "NAGlobalDelegate.h"
#include "Components/ListView.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/NAGameInstance.h"

void UNASessionListWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UNASessionListWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CachedGameInstance = Cast<UNAGameInstance>( GetGameInstance() );
	Button_CreateSession->OnClicked.AddDynamic(this,&ThisClass::OnClick_CreateSession);
	Button_StartGame->OnClicked.AddDynamic(this, &ThisClass::OnClick_StartGame);
	Button_Return->OnClicked.AddDynamic(this, &ThisClass::OnClick_Return);
	Button_Refresh->OnClicked.AddDynamic(this, &ThisClass::OnClick_Refresh);
	CachedGameInstance->OnSessionFound.AddUObject( this, &UNASessionListWidget::RefreshSessionList );

	// 세션에 참가하거나 새로 만들 경우 레벨이 새로 로드되면서 위젯이 다시 생성됨
	// 이전에 열려있던 위젯과 다르다는 점을 참고
	Button_StartGame->SetIsEnabled( CachedGameInstance->IsHosting() && !CachedGameInstance->HasJoined() );
	Button_Refresh->SetIsEnabled( !CachedGameInstance->IsHosting() && !CachedGameInstance->HasJoined() );
	
	if ( CachedGameInstance->IsHosting() || CachedGameInstance->HasJoined() )
	{
		UpdatePlayerStates( nullptr );
		GOnNewPlayerStateChanged.AddUObject( this, &ThisClass::UpdatePlayerStates );
	}
}

void UNASessionListWidget::NativeDestruct()
{
	Super::NativeDestruct();
	
	Button_CreateSession->OnClicked.RemoveAll( this );
	Button_StartGame->OnClicked.RemoveAll(this );
	Button_Return->OnClicked.RemoveAll( this );
	Button_Refresh->OnClicked.RemoveAll( this );
	CachedGameInstance->OnSessionFound.RemoveAll( this );

	GOnNewPlayerStateChanged.RemoveAll( this );
}

void UNASessionListWidget::OnClick_CreateSession()
{
	//UGameplayStatics::OpenLevel(GetWorld(),"/Game/00_ProjectNA/02_Level/Level_NALobby?listen");
	//SetVisibility(ESlateVisibility::Visible);
	auto SessionName = FString::FromInt(GetOwningPlayerState()->GetPlayerId());
	CachedGameInstance->CreateSession(FName(SessionName), true);

	Button_StartGame->SetIsEnabled( true );
	Button_Refresh->SetIsEnabled( false );
}

void UNASessionListWidget::OnClick_StartGame()
{
	CachedGameInstance->StartSession_Wrapped();
}

void UNASessionListWidget::OnClick_Return()
{
	CachedGameInstance->DestroySession();
	Button_Refresh->SetIsEnabled( true );
	Button_StartGame->SetIsEnabled( false );
	SetVisibility( ESlateVisibility::Hidden );
}

void UNASessionListWidget::OnClick_Refresh()
{
	CachedGameInstance->FindSessions();
}

UListView* UNASessionListWidget::GetSessionList() const
{
	return SessionListView;
}

void UNASessionListWidget::RefreshSessionList()
{
	while ( !ListMutex.TryLock() ) {}
	SessionSearch = CachedGameInstance->GetSessionSearch();

	auto Results = SessionSearch.Pin()->SearchResults;

	if (!Results.IsEmpty())
	{
		for (auto& It :  Results)
		{
			UNASessionListEntryData* Data = NewObject<UNASessionListEntryData>();
			Data->SearchResult = MakeShared<FOnlineSessionSearchResult>(It);

			SessionListView->AddItem( Data );
		}
	}
	
	if ( !CachedGameInstance->IsHosting() )
	{
		Button_StartGame->SetVisibility( ESlateVisibility::Collapsed );
	}

	ListMutex.Unlock();
}

void UNASessionListWidget::UpdatePlayerStates( APlayerState* /*PlayerState*/ )
{
	while ( !ListMutex.TryLock() ) {}

	SessionListView->ClearListItems();

	if ( AGameStateBase* GameState = GetPlayerContext().GetGameState() )
	{
		for ( const APlayerState* PlayerState : GameState->PlayerArray )
		{
			UNASessionListEntryData* Data = NewObject<UNASessionListEntryData>();
			Data->SessionName = PlayerState->GetPlayerName();
			Data->bPlayerValue = true;
			SessionListView->AddItem( Data );
		}
	}

	ListMutex.Unlock();
}

