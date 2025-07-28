// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/NALobbyHUD.h"
#include "CineCameraActor.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"	
#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/NALobbyWidget.h"

ANALobbyHUD::ANALobbyHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> widget(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/00_ProjectNA/01_Blueprint/01_Widget/Lobby/BP_NALobby.BP_NALobby_C'"));
	if (widget.Succeeded())
		LobbyWidgetClass = widget.Class;
}

void ANALobbyHUD::BeginPlay()
{
	Super::BeginPlay();

	// 메인화면 카메라액터, 라이트컴포넌트 find
	ACineCameraActor* Cam = nullptr;
	UDirectionalLightComponent* LightComp = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetName().Contains(TEXT("CineCameraActor")))
		{
			Cam = Cast<ACineCameraActor>(*It);
		}

		if (It->GetName().Contains(TEXT("DirectionalLight")))
		{
			LightComp = It->GetComponentByClass<UDirectionalLightComponent>();
		}
	}

	// 타이틀화면 카메라 세팅
	APlayerController* Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Controller->bShowMouseCursor = true;
	Controller->SetViewTarget(Cam);

	// 로비 ui 호출
	UUserWidget* instance = CreateWidget<UUserWidget>(GetWorld(), LobbyWidgetClass);
	instance->AddToViewport();
	LobbyWidget = Cast<UNALobbyWidget>(instance);
	
	
}
