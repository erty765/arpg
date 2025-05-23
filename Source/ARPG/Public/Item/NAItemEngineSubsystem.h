// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "NAItemEngineSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ARPG_API UNAItemEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;
	
};
