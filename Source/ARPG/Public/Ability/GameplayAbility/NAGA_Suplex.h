// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NAGA_Suplex.generated.h"

class USpringArmComponent;
class UCameraComponent;
/**
 * 
 */
UCLASS()
class ARPG_API UNAGA_Suplex : public UGameplayAbility
{
	GENERATED_BODY()
	
	UNAGA_Suplex();

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);

	UFUNCTION()
	void OnMontageFinished();

	virtual void SetSuplexMontage(UAnimMontage* InAnimMontage) { SuplexingMontage = InAnimMontage; }
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* SuplexingMontage;

	UPROPERTY()
	UCameraComponent* ActionCamera;

	UPROPERTY()
	ACameraActor* ActionCam;

	UPROPERTY()
	USpringArmComponent* ActionBoom;
};
