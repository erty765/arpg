// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActor/NAPlaceableItemActor.h"
#include "NADoor.generated.h"


UENUM(Blueprintable)
enum class EDoorType : uint8
{
	UpDown,
	LeftRight,
	Max
};

DECLARE_DELEGATE_RetVal_OneParam(bool, FOnTickInteraction, AActor*);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FOnGimicClear, AActor*, InActor);

UCLASS(Abstract)
class ARPG_API ANADoor : public ANAPlaceableItemActor
{
	friend class ANAPlayerController;
	
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ANADoor(const FObjectInitializer& ObjectInitializer);

	virtual void PostRegisterAllComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

//======================================================================================================================
// Interactable Interface Implements
//======================================================================================================================
protected:
	virtual bool BeginInteract_Implementation(AActor* Interactor) override;
	virtual	bool ExecuteInteract_Implementation(AActor* Interactor) override;
	virtual bool EndInteract_Implementation(AActor* Interactor) override;
	
private:
	void InitInteraction(AActor* Interactor);

	void ActiveDoor();

	void ToggleDoor();

	UFUNCTION( Server, Reliable )
	void Server_ToggleDoor();

	UFUNCTION( NetMulticast, Reliable )
	void Multi_ToggleDoor();
	
	UFUNCTION()
	void OnDoorOpeningFinished();
	UFUNCTION()
	void OnDoorClosingFinished();

public:
	virtual void ReleaseItemWidgetComponent() override;
	virtual void CollapseItemWidgetComponent() override;

protected:
	UPROPERTY()
	AActor* CurrentInteractingActor = nullptr;
	
	// 상하, 좌우 순으로 등록할 것
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door")
	UStaticMeshComponent* Door1 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door")
	UStaticMeshComponent* Door2 = nullptr;

	UPROPERTY(EditAnywhere)
	EDoorType DoorType;

	UPROPERTY(EditAnywhere)
	double MoveDist = 260.0;
	
	UPROPERTY()
	float Duration = 1.5f;

	UPROPERTY()
	float CurrentTime = 0.f;

	UPROPERTY()
	FTransform PivotTF;
	
	UPROPERTY()
	FTransform OriginTF1_Local;

	UPROPERTY()
	FTransform OriginTF2_Local;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	bool bIsLocked = false;
	
	// Delegate
	FOnTickInteraction TickInteraction;

	uint8 bIsDoorMoving = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bIsOpened : 1 = false;

	UPROPERTY()
	FTransform DestTF1_Local = FTransform::Identity;

	UPROPERTY()
	FTransform DestTF2_Local = FTransform::Identity;
};
