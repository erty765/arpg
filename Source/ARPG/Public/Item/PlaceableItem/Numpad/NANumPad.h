// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActor/NAPlaceableItemActor.h"
#include "NANumPad.generated.h"


DECLARE_DYNAMIC_DELEGATE(FOnCompleteGimic);
class UNANumpadWidget;

UCLASS(Abstract)
class ARPG_API ANANumPad : public ANAPlaceableItemActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANANumPad(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitProperties() override;
	virtual void PostRegisterAllComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

	//======================================================================================================================
	// Interactable Interface Implements
	//======================================================================================================================
public:
	virtual bool BeginInteract_Implementation(AActor* Interactor) override;
	virtual bool EndInteract_Implementation(AActor* Interactor) override;
	virtual bool ExecuteInteract_Implementation(AActor* Interactor) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	void ExecuteInteract_Wrapped();
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, AllowAnyActor))
	TSubclassOf<UUserWidget> NumpadWidget;
	
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, AllowAnyActor))
	ANAPlaceableItemActor* TargetActor;

	UPROPERTY()
	UNANumpadWidget* CachedWidget;

public:
	
	UPROPERTY()
	FOnCompleteGimic OnCompleteGimic;
};
