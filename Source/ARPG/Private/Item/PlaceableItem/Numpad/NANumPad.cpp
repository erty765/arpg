// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PlaceableItem/Numpad/NANumPad.h"

#include "Item/PlaceableItem/Numpad/NANumpadWidget.h"


// Sets default values
ANANumPad::ANANumPad(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ANANumPad::PostInitProperties()
{
	Super::PostInitProperties();
}

void ANANumPad::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
}

void ANANumPad::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ANANumPad::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

bool ANANumPad::BeginInteract_Implementation(AActor* Interactor)
{
	if (Super::BeginInteract_Implementation(Interactor))
	{
		// UNAWorldEventHandler::GetInstance()->RegisterEvent(TEXT("Test"), [this]()
		// {
		// 	ExecuteInteract_Wrapped();
		// });

		CachedWidget->SetVisibility(ESlateVisibility::Visible);
		return true;
	}

	return false;
}

bool ANANumPad::EndInteract_Implementation(AActor* Interactor)
{
	return Super::EndInteract_Implementation(Interactor);

	//UNAWorldEventHandler::GetInstance()->UnRegisterEvent(TEXT("Test"));
}

bool ANANumPad::ExecuteInteract_Implementation(AActor* Interactor)
{
	return true;
}

// Called when the game starts or when spawned
void ANANumPad::BeginPlay()
{
	Super::BeginPlay();
	
	UUserWidget* instance = CreateWidget<UUserWidget>(GetWorld(), NumpadWidget);
	if (!instance) return;

	instance->AddToViewport();
	instance->SetVisibility(ESlateVisibility::Hidden);

	CachedWidget = Cast<UNANumpadWidget>(instance);
}

void ANANumPad::ExecuteInteract_Wrapped()
{
	if (TargetActor == nullptr) return;
	
	TargetActor->Execute_BeginInteract(this, this);
}

// Called every frame
void ANANumPad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

