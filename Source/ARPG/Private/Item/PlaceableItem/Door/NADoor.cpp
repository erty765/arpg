// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PlaceableItem/Door/NADoor.h"
#include "Algo/Find.h"
#include "Components/SphereComponent.h"
#include "Item/ItemWidget/NAItemWidgetComponent.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
ANADoor::ANADoor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bNeedItemCollision = false;
	
	TriggerSphere->SetSphereRadius(280);
	TriggerSphere->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
}

void ANADoor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
}

void ANADoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ANADoor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

bool ANADoor::BeginInteract_Implementation(AActor* Interactor)
{
	if (Super::BeginInteract_Implementation(Interactor))
	{
		CollapseItemWidgetComponent();
		// InitInteraction(Interactor);
		// TickInteraction.BindUObject(this, &ThisClass::ExecuteInteract_Implementation);
		return true;
	}
	return false;
}

bool ANADoor::ExecuteInteract_Implementation(AActor* Interactor)
{
	Super::ExecuteInteract_Implementation(Interactor);

	Server_ToggleDoor();
	return true;
}

bool ANADoor::EndInteract_Implementation(AActor* Interactor)
{
	if (Super::EndInteract_Implementation(Interactor))
	{
		// InitInteraction(nullptr);
		// TickInteraction.Unbind();
		return true;
	}
	return false;
}

// Called when the game starts or when spawned
void ANADoor::BeginPlay()
{
	Super::BeginPlay();

	TArray<UStaticMeshComponent*> TargetComponents; 
	GetComponents(TargetComponents,true);

	for (UStaticMeshComponent* Comp : TargetComponents)
	{
		if (Comp->GetName() == "Door01")
			Door1 = Comp;
		if (Comp->GetName() == "Door02")
			Door2 = Comp;
	}
	
	OriginTF1_Local = Door1->GetRelativeTransform();
	OriginTF2_Local = Door2->GetRelativeTransform();
	
	PivotTF = RootComponent->GetRelativeTransform();

	DestTF1_Local = OriginTF1_Local;
	DestTF2_Local = OriginTF2_Local;
	
	FVector DestLoc_Door1 = Door1->GetComponentLocation();
	FVector DestLoc_Door2 = Door2->GetComponentLocation();
		
	FVector UpVec = GetActorUpVector();
	FVector RightVec = GetActorRightVector();
		
	switch (DoorType)
	{
	case EDoorType::UpDown:
		DestLoc_Door1 += UpVec * MoveDist;
		DestLoc_Door2 += -UpVec * MoveDist;
		break;
			
	case EDoorType::LeftRight:
		DestLoc_Door1 += RightVec * MoveDist;
		DestLoc_Door2 += -RightVec * MoveDist;
		break;
			
	default:
		ensureAlways(false);
		return;
	}

	DestLoc_Door1 = GetActorTransform().InverseTransformPosition(DestLoc_Door1);
	DestLoc_Door2 = GetActorTransform().InverseTransformPosition(DestLoc_Door2);

	DestTF1_Local.SetLocation(DestLoc_Door1);
	DestTF2_Local.SetLocation(DestLoc_Door2);
}

// Called every frame
void ANADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if (bIsOnInteract)
	// {
	// 	CurrentTime += DeltaTime;
	// 	TickInteraction.Execute(CurrentInteractingActor);
	// }
}

void ANADoor::InitInteraction(AActor* Interactor = nullptr)
{
	CurrentInteractingActor = Interactor;
}

void ANADoor::ActiveDoor()
{
	//if (CurrentTime > Duration) return;
	
	FVector OriginPos1 = OriginTF1_Local.GetLocation();
	FVector OriginPos2 = OriginTF2_Local.GetLocation();

	FVector Direction = DoorType == EDoorType::UpDown ? PivotTF.GetRotation().GetUpVector() : PivotTF.GetRotation().GetRightVector();
						
	FVector TargetPos1 = OriginTF1_Local.GetLocation() + Direction * MoveDist;
	FVector TargetPos2 = OriginTF2_Local.GetLocation() - Direction * MoveDist;

	float Alpha = CurrentTime / Duration;
	
	FVector MovingPos1 = FMath::InterpEaseIn(OriginPos1, TargetPos1, Alpha, 8.f);
	FVector MovingPos2 = FMath::InterpEaseIn(OriginPos2, TargetPos2, Alpha, 8.f);

	Door1->SetWorldLocation(MovingPos1);
	Door2->SetWorldLocation(MovingPos2);
}

void ANADoor::ToggleDoor()
{
	if (DoorType == EDoorType::Max)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ToggleDoor]  DoorType was Max."));
		return;
	}
	if (bIsDoorMoving) return;

	bIsDoorMoving = true;
	if (!bIsOpened) // 문 열기
	{
		FLatentActionInfo LatentInfo;
		LatentInfo.CallbackTarget = this;
		LatentInfo.Linkage = 1;
		LatentInfo.ExecutionFunction = TEXT("OnDoorOpeningFinished");
		
		LatentInfo.UUID = 0001;
		UKismetSystemLibrary::MoveComponentTo(
				Door1,
				DestTF1_Local.GetLocation(),
				DestTF1_Local.GetRotation().Rotator(),
				true, true,
				Duration,
				true,
				EMoveComponentAction::Move,
				LatentInfo
			);
		
		LatentInfo.UUID = 0002;
		UKismetSystemLibrary::MoveComponentTo(
				Door2,
				DestTF2_Local.GetLocation(),
				DestTF2_Local.GetRotation().Rotator(),
				true, true,
				Duration,
				true,
				EMoveComponentAction::Move,
				LatentInfo
			);
	}
	else // 문 닫기
	{
		FLatentActionInfo LatentInfo;
		LatentInfo.CallbackTarget = this;
		LatentInfo.Linkage = 1;
		LatentInfo.ExecutionFunction = TEXT("OnDoorClosingFinished");
		
		LatentInfo.UUID = 0003;
		UKismetSystemLibrary::MoveComponentTo(
				Door1,
				OriginTF1_Local.GetLocation(),
				OriginTF1_Local.GetRotation().Rotator(),
				true, true,
				Duration,
				true,
				EMoveComponentAction::Move,
				LatentInfo
			);

		LatentInfo.UUID = 0004;
		UKismetSystemLibrary::MoveComponentTo(
				Door2,
				OriginTF2_Local.GetLocation(),
				OriginTF2_Local.GetRotation().Rotator(),
				true, true,
				Duration,
				true,
				EMoveComponentAction::Move,
				LatentInfo
			);
	}
}

void ANADoor::Server_ToggleDoor_Implementation()
{
	ToggleDoor();
	Multi_ToggleDoor();
}

void ANADoor::Multi_ToggleDoor_Implementation()
{
	ToggleDoor();
}

void ANADoor::OnDoorOpeningFinished()
{
	if (Door1->GetRelativeLocation().Equals(DestTF1_Local.GetLocation(), 0.1f) 
		&& Door2->GetRelativeLocation().Equals(DestTF2_Local.GetLocation(), 0.1f))
	{
		bIsOpened = true;
		bIsDoorMoving = false;
		
		if (bIsFocused)
		{
			ReleaseItemWidgetComponent();
		}
	}
}

void ANADoor::OnDoorClosingFinished()
{
	if (Door1->GetRelativeLocation().Equals(OriginTF1_Local.GetLocation(), 0.1f) 
		&& Door2->GetRelativeLocation().Equals(OriginTF2_Local.GetLocation(), 0.1f))
	{
		bIsOpened = false;
		bIsDoorMoving = false;
		
		if (bIsFocused)
		{
			ReleaseItemWidgetComponent();
		}
	}
}

void ANADoor::ReleaseItemWidgetComponent()
{
	if (!ItemWidgetComponent) return;
	if (!bIsOpened)
	{
		ItemWidgetComponent->SetItemInteractionName(TEXT("Open"));
	}
	else
	{
		ItemWidgetComponent->SetItemInteractionName(TEXT("Close"));
	}
	Super::ReleaseItemWidgetComponent();
}

void ANADoor::CollapseItemWidgetComponent()
{
	// if (!ItemWidgetComponent) return;
	// if (!bIsOpened)
	// {
	// 	ItemWidgetComponent->SetItemInteractionName(TEXT("Open"));
	// }
	// else
	// {
	// 	ItemWidgetComponent->SetItemInteractionName(TEXT("Close"));
	// }
	Super::CollapseItemWidgetComponent();
}

