// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemWidget/NAItemWidgetComponent.h"

#include "Item/ItemActor/NAPickableItemActor.h"
#include "Item/ItemActor/NAPlaceableItemActor.h"
#include "Item/ItemWidget/NAItemWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UNAItemWidgetComponent::UNAItemWidgetComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetEnableGravity(false);
	CanCharacterStepUpOn = ECB_No;
	SetSimulatePhysics(false);
	bApplyImpulseOnDamage = false;
	bReplicatePhysicsToAutonomousProxy = false;
	BodyInstance.SetInertiaConditioningEnabled(false);
	BodyInstance.bGenerateWakeEvents = false;
	BodyInstance.bUpdateMassWhenScaleChanges = false;

	SetWidgetSpace(EWidgetSpace::World);
	SetGeometryMode(EWidgetGeometryMode::Plane);
	OpacityFromTexture = 1.f;
	SetBlendMode(EWidgetBlendMode::Masked);
	SetWindowFocusable(false);

	static const FSoftClassPath PickableItemWidgetClassPath(TEXT("/Game/00_ProjectNA/01_Blueprint/01_Widget/Item/BP_NAPickableItemDefaultWidget.BP_NAPickableItemDefaultWidget_C"));
	PickableItemWidgetClassRef = PickableItemWidgetClassPath;
	
	static const FSoftClassPath PlaceableItemWidgetClassPath(TEXT("/Game/00_ProjectNA/01_Blueprint/01_Widget/Item/BP_NAPlaceableItemDefaultWidget.BP_NAPlaceableItemDefaultWidget_C"));
	PlaceableItemWidgetClassRef = PlaceableItemWidgetClassPath;

	static const FSoftObjectPath ItemWidgetMaterialPath(TEXT("/Game/00_ProjectNA/01_Blueprint/01_Widget/Item/Materials/M_ItemWidgetDefault.M_ItemWidgetDefault"));
	ItemWidgetMaterialRef = ItemWidgetMaterialPath;
}

void UNAItemWidgetComponent::PostInitProperties()
{
	Super::PostInitProperties();

	InitItemWidgetClass();
	InitItemWidgetTransform();
}

void UNAItemWidgetComponent::InitItemWidgetClass()
{
	if (GetOwner() && GetWidgetClass() == nullptr)
	{
		if (GetOwner()->GetClass()->IsChildOf<ANAPickableItemActor>())
		{
			if (PickableItemWidgetClassRef.IsNull()) return;
			
			SetWidgetClass(PickableItemWidgetClassRef.LoadSynchronous());
		}
		else if (GetOwner()->GetClass()->IsChildOf<ANAPlaceableItemActor>())
		{
			if (PlaceableItemWidgetClassRef.IsNull()) return;
			SetWidgetClass(PlaceableItemWidgetClassRef.LoadSynchronous());
		}
	}
}

void UNAItemWidgetComponent::InitItemWidgetTransform()
{
	if (!GetWidgetClass()) return;
	
	if (GetOwner()->GetClass()->IsChildOf<ANAPickableItemActor>())
	{
		PickableWidgetRelativeOffset = FVector(0.f, 80.f, 170.f);
		SetRelativeLocation(PickableWidgetRelativeOffset);

		PickableWidgetRelativeRotation = FRotator(10.f, 180.0f, 0.0f);
		SetRelativeRotation(PickableWidgetRelativeRotation);

		SetRelativeScale3D(FVector(0.4f));
		SetDrawSize(FVector2D(340, 410));
	}
	else if (GetOwner()->GetClass()->IsChildOf<ANAPlaceableItemActor>())
	{
		FVector ForwardLoc = FVector(80.f, 0.f, 0.f);
		FRotator ForwardRot = FRotator(0.f, 0.f, 0.f);

		FVector BackwardLoc = FVector(-80.f, 0.f, 0.f);
		FRotator BackwardRot = FRotator(0.f, 180.f, 0.f);

		PlaceableWidgetRelativeForwardTransform.SetLocation(ForwardLoc);
		PlaceableWidgetRelativeForwardTransform.SetRotation(ForwardRot.Quaternion());

		PlaceableWidgetRelativeBackwardTransform.SetLocation(BackwardLoc);
		PlaceableWidgetRelativeBackwardTransform.SetRotation(BackwardRot.Quaternion());

		SetRelativeTransform(PlaceableWidgetRelativeBackwardTransform);

		SetDrawSize(FVector2D(260, 50));
	}
}

// Called when the game starts
void UNAItemWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	// SetMaterial은 어느 시점에서 불러도 렌더러를 요구
	SetMaterial(0, ItemWidgetMaterialRef.LoadSynchronous());
	SetVisibility(false);
	Deactivate();
}


// Called every frame
void UNAItemWidgetComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUpdateTransformFacingCamera)
	{
		UpdateTransformFacingCamera();
	}
	if (bUpdateTransformFacingCharacter)
	{
		UpdateTransformFacingCharacter();
	}
}

void UNAItemWidgetComponent::InitWidget()
{
	Super::InitWidget();
	
	if (GetWorld()->IsPreviewWorld()) return;
	if (!GetItemDataFromOwner()) return;
	
	if (GetItemWidget())
	{
		GetItemWidget()->InitItemWidget(this, GetItemDataFromOwner());
	}
}

UNAItemData* UNAItemWidgetComponent::GetItemDataFromOwner() const
{
	if (ANAItemActor* ItemActor = Cast<ANAItemActor>(GetOwner()))
	{
		return ItemActor->GetItemData();
	}
	return nullptr;
}

void UNAItemWidgetComponent::ReleaseItemWidgetPopup()
{
	if (GetWorld()->IsPreviewWorld()) return;
	if (!GetItemDataFromOwner()) return;
	if (!GetItemWidget()) return;


	GetItemWidget()->ReleaseItemWidget();
}

void UNAItemWidgetComponent::CollapseItemWidgetPopup()
{
	if (GetWorld()->IsPreviewWorld()) return;
	if (!GetItemDataFromOwner()
		&& !GetOwner()->GetClass()->IsChildOf<ANAItemWidgetPopupActor>()) return;
	if (!GetItemWidget()) return;
	
	GetItemWidget()->CollapseItemWidget();
}

void UNAItemWidgetComponent::SetItemInteractionName(const FString& NewString) const
{
	if (GetItemWidget())
	{
		GetItemWidget()->SetInteractionNameText(NewString);
	}
}

void UNAItemWidgetComponent::SetEnableUpdateTransform(const bool bEnable)
{
	if (GetOwner()->GetClass()->IsChildOf<ANAPickableItemActor>())
	{
		bUpdateTransformFacingCharacter = false;
		bUpdateTransformFacingCamera = bEnable;
	}
	else if (GetOwner()->GetClass()->IsChildOf<ANAPlaceableItemActor>())
	{
		bUpdateTransformFacingCamera = false;
		bUpdateTransformFacingCharacter = bEnable;
		if (bUpdateTransformFacingCharacter)
		{
			UpdateTransformFacingCharacter();
		}
	}
}

class UNAItemWidget* UNAItemWidgetComponent::GetItemWidget() const
{
	return Cast<UNAItemWidget>(GetWidget());
}

void UNAItemWidgetComponent::UpdateTransformFacingCamera()
{
	if (!bUpdateTransformFacingCamera) return;
	if (!GetAttachParent()) return;
	
	int32 PlayerIndex = GetOwnerPlayer()->GetLocalPlayerIndex();
	APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, PlayerIndex);
	if (!PCM) return;
	
	FVector ToCameraLoc = PCM->GetCameraLocation() - GetComponentLocation();
	FRotator LookAtRot = UKismetMathLibrary::MakeRotFromX(ToCameraLoc);
	LookAtRot.Pitch = PickableWidgetRelativeRotation.Pitch;
	LookAtRot.Roll = PickableWidgetRelativeRotation.Roll;
	
	SetWorldRotation(LookAtRot);

	FVector RootWorldLoc = GetAttachParent()->GetComponentLocation();
	const FRotator CameraRot = PCM->GetCameraRotation();
	
	const FVector CameraForward = CameraRot.Quaternion().GetForwardVector();
	const FVector CameraRight = CameraRot.Quaternion().GetRightVector();
	const FVector CameraUp = CameraRot.Quaternion().GetUpVector();

	FVector WidgetWorldLoc = RootWorldLoc
		+ (CameraForward * PickableWidgetRelativeOffset.X)
		+ (CameraRight * PickableWidgetRelativeOffset.Y)
		+ (CameraUp * PickableWidgetRelativeOffset.Z);

	SetWorldLocation(WidgetWorldLoc);
}

void UNAItemWidgetComponent::UpdateTransformFacingCharacter()
{
	if (!GetAttachParent()) return;

	// 1. 루트(부모) 위치, Forward 벡터
	const FVector RootLoc = GetAttachParent()->GetComponentLocation();
	const FVector RootForward = GetAttachParent()->GetForwardVector();

	// 2. 캐릭터(플레이어) 위치 구하기
	APlayerController* PC = GetOwnerPlayer()->GetPlayerController(GetWorld());
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn) return;
	const FVector CharacterLoc = PlayerPawn->GetActorLocation();

	// 3. 루트 기준 캐릭터가 앞에 있는지, 뒤에 있는지 판정
	const FVector ToCharacter = (CharacterLoc - RootLoc).GetSafeNormal();
	float Dot = FVector::DotProduct(RootForward, ToCharacter); // >0: 앞, <0: 뒤

	FVector WidgetOffset;
	FQuat WidgetQuat;

	if (Dot > 0.f)
	{
		// 캐릭터가 루트의 "앞"에 있음
		WidgetOffset = PlaceableWidgetRelativeForwardTransform.GetLocation();
		WidgetQuat = PlaceableWidgetRelativeForwardTransform.GetRotation();
	}
	else
	{
		// 캐릭터가 루트의 "뒤"에 있음
		WidgetOffset = PlaceableWidgetRelativeBackwardTransform.GetLocation();
		WidgetQuat = PlaceableWidgetRelativeBackwardTransform.GetRotation();
	}

	// 4. 루트 위치 + 오프셋(루트의 로컬 -> 월드 변환)
	FVector WidgetWorldLoc = RootLoc + GetAttachParent()->GetComponentTransform().TransformVectorNoScale(WidgetOffset);
	SetWorldLocation(WidgetWorldLoc);

	// 5. 루트의 월드 회전에 오프셋 회전 합성
	FQuat RootQuat = GetAttachParent()->GetComponentQuat();
	FQuat WorldQuat = WidgetQuat * RootQuat; // 오프셋 -> 루트 회전

	SetWorldRotation(WorldQuat);
}

