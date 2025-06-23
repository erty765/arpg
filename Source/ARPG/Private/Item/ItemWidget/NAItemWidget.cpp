﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemWidget/NAItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Item/ItemWidget/NAItemWidgetComponent.h"
#include "Item/ItemData/NAItemData.h"
#include "Misc/StringUtils.h"

void UNAItemWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UNAItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false);
	ForceLayoutPrepass();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UNAItemWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UNAItemWidget::ReleaseItemWidget()
{
	if (!OwningItemWidgetComponent.IsValid()) return;
	
	bReleaseItemWidget = true;
	
	if (Widget_Appear)
	{
		PlayAnimationForward(Widget_Appear, 1.5f);
	}
}

void UNAItemWidget::OnItemWidgetReleased()
{
	if (bReleaseItemWidget)
	{
		if (Widget_VisibleLoop)
		{
			PlayAnimation(Widget_VisibleLoop, 0.f, 0);
		}
	}
	else
	{
		if (Widget_VisibleLoop)
		{
			StopAnimation(Widget_VisibleLoop);
		}
	}
}

void UNAItemWidget::CollapseItemWidget()
{
	if (!OwningItemWidgetComponent.IsValid()) return;
	
	bReleaseItemWidget = false;
	if (Widget_Appear)
	{
		PlayAnimationReverse(Widget_Appear, 1.8f);
	}
}

void UNAItemWidget::OnItemWidgetCollapsed()
{
	if (bReleaseItemWidget)
	{
		OwningItemWidgetComponent->Activate();
		OwningItemWidgetComponent->SetVisibility(true);
		OwningItemWidgetComponent->SetWindowVisibility(EWindowVisibility::Visible);
		OwningItemWidgetComponent->SetEnableUpdateTransform(true);
		
		SetIsEnabled(true);
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Hidden);
		SetIsEnabled(false);

		OwningItemWidgetComponent->SetEnableUpdateTransform(false);
		OwningItemWidgetComponent->SetWindowVisibility(EWindowVisibility::SelfHitTestInvisible);
		OwningItemWidgetComponent->SetVisibility(false);
		OwningItemWidgetComponent->Deactivate();
	}
}

void UNAItemWidget::SetInteractionNameText(const FString& NewString) const
{
	if (NewString.IsEmpty()) return;

	Interaction_Name->SetText(FText::FromString(NewString));
}

void UNAItemWidget::InitItemWidget(UNAItemWidgetComponent* OwningComp, UNAItemData* ItemData)
{
	check(OwningComp != nullptr);
	check(ItemData != nullptr);

	OwningItemWidgetComponent = OwningComp;
	
	if (Item_Name)
	{
		Item_Name->SetText(FText::FromString(ItemData->GetItemName()));
	}
	if (Item_Icon)
	{
		Item_Icon->SetBrushResourceObject(ItemData->GetItemIcon());
	}
	if (Item_Type)
	{
		FString ItemTypeStr = FStringUtils::EnumToDisplayString(ItemData->GetItemType());
		ItemTypeStr.RemoveFromStart("IT_");
		ItemTypeStr = FStringUtils::InsertSpacesBeforeUppercaseSmart(ItemTypeStr);
		Item_Type->SetText(FText::FromString(ItemTypeStr));
	}
	if (Interaction_Name)
	{
		FNAInteractableData InteractableData;
		if (ItemData->GetInteractableData(InteractableData))
		{
			Interaction_Name->SetText(InteractableData.InteractionName);
		}
	}
}
