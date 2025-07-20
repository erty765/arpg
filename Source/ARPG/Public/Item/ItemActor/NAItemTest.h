// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NAPickableItemActor.h"
#include "NAItemTest.generated.h"

UCLASS()
class ARPG_API ANAItemTest : public ANAPickableItemActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANAItemTest(const FObjectInitializer& ObjectInitializer);
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PreRegisterAllComponents() override;
	virtual void PostRegisterAllComponents() override;
	virtual void PostActorCreated() override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(Transient, VisibleAnywhere)
	UStaticMeshComponent* TestStaticMeshComponent;
};
