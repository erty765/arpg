// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Item/ItemActor/NAPickableItemActor.h"
#include "GameFramework/Actor.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "NAWeapon.generated.h"

enum class EFireArmType : uint8;
class UNAAmmoIndicatorComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UNAMontageCombatComponent;

UCLASS(Abstract)
class ARPG_API ANAWeapon : public ANAPickableItemActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

	friend class UNAItemEngineSubsystem;
	friend struct FCombatUpdatePredication;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Replicated, Category="Combat", meta=(AllowPrivateAccess="true"))
	UNAMontageCombatComponent* CombatComponent;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Replicated, Category="Combat", meta=(AllowPrivateAccess="true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="FX", meta=(AllowPrivateAccess="true"))
	UNiagaraComponent* MuzzleFlashComponent;

	UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category="Widget", meta=(AllowPrivateAccess="true"))
	UNAAmmoIndicatorComponent* AmmoIndicatorComponent;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category="Combat", meta=(AllowPrivateAccess="true") )
	EFireArmType FireArmType;

	FTransform PreviousParentComponentTransform;

public:
	// Sets default values for this actor's properties
	ANAWeapon();

	UFUNCTION(BlueprintCallable)
	EFireArmType GetFireArmType() const;
	
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// Called every frame
	virtual void Tick( float DeltaTime ) override;
};
