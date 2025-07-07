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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere)
	class UArrowComponent* TestArrowComponent;
};
