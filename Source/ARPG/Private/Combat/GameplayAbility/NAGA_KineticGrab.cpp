﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/GameplayAbility/NAGA_KineticGrab.h"

#include "AbilitySystemComponent.h"
#include "Combat/AbilityTask/NAAT_ConsumeKineticGrabAP.h"
#include "Combat/AbilityTask/NAAT_MoveActorTo.h"
#include "Combat/AttributeSet/NAKineticAttributeSet.h"
#include "ARPG/Public/Combat/PhysicsHandleComponent/NAKineticComponent.h"

FVector UNAGA_KineticGrab::EvaluateActorPosition(
	const AActor* OriginActor,
	const UPrimitiveComponent* TargetBoundComponent,
	const FVector& OriginForwardVector,
	float Distance )
{
	const FVector ShouldDistanced = GetMinimumDistance( OriginActor, TargetBoundComponent, OriginForwardVector ) + ( OriginForwardVector * Distance );
	const FVector OriginPosition = OriginActor->GetActorLocation();
	const FVector TargetPosition = OriginPosition + ShouldDistanced;

	return TargetPosition;
}

FVector UNAGA_KineticGrab::EvaluateActorPosition( const AActor* OriginActor, const FVector& OriginForwardVector, const float MinimumDistance )
{
	const FVector ShouldDistanced = OriginForwardVector * MinimumDistance;
	const FVector OriginPosition = OriginActor->GetActorLocation();
	const FVector TargetPosition = OriginPosition + ShouldDistanced;

	return TargetPosition;
}

FVector UNAGA_KineticGrab::GetMinimumDistance( const AActor* OriginActor,
                                               const UPrimitiveComponent* TargetBoundComponent, const FVector& ForwardVector )
{
	const FVector TargetDimension = TargetBoundComponent->Bounds.BoxExtent;
	FVector OriginOrigin, OriginExtents;
	OriginActor->GetActorBounds( true, OriginOrigin, OriginExtents );
	const FVector OriginDimension = OriginExtents;

	const FVector OriginDimensionToForward = OriginDimension * ForwardVector;
	const FVector TargetDimensionToForward = TargetDimension * ForwardVector;
	return OriginDimensionToForward + TargetDimensionToForward;
}

UNAGA_KineticGrab::UNAGA_KineticGrab(): PreviousResponse(), APConsumeTask( nullptr ), MoveActorToTask( nullptr )
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

bool UNAGA_KineticGrab::CommitAbility( const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                       FGameplayTagContainer* OptionalRelevantTags )
{
	bool bResult = Super::CommitAbility( Handle, ActorInfo, ActivationInfo, OptionalRelevantTags );

	if ( bResult )
	{
		const UNAKineticAttributeSet* AttributeSet = Cast<UNAKineticAttributeSet>( ActorInfo->AbilitySystemComponent->GetAttributeSet( UNAKineticAttributeSet::StaticClass() ) );
		bResult &= AttributeSet != nullptr;
		
		if ( AttributeSet )
		{
			bResult &= AttributeSet->GetAP() > 0.f;
		}
	}

	return bResult;
}

void UNAGA_KineticGrab::EndAbility( const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled )
{
	Super::EndAbility( Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled );

	if ( HasAuthority( &ActivationInfo ) )
	{
		UNAKineticComponent* Component = ActorInfo->AvatarActor->GetComponentByClass<UNAKineticComponent>();
		check( Component );

		if ( Component && Component->bIsGrab )
		{
			UPrimitiveComponent* Grabbed = Component->GetGrabbedComponent();
			Component->ReleaseComponent();
			Grabbed->WakeAllRigidBodies();
			
			// 왼클릭이 공격과 겹치기 떄문에 잡기 상태를 한틱 늦게 업데이트
			GetWorld()->GetTimerManager().SetTimerForNextTick( [ Component ]()
			{
				Component->bIsGrab = false;
			} );
		}

		if ( APConsumeTask )
		{
			APConsumeTask->OnAPDepleted.RemoveAll( this );
			APConsumeTask->EndTask();
			APConsumeTask = nullptr;
		}

		if ( MoveActorToTask )
		{
			MoveActorToTask->EndTask();
			MoveActorToTask = nullptr;
		}
	}
}

void UNAGA_KineticGrab::CancelAbility( const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                       bool bReplicateCancelAbility )
{
	Super::CancelAbility( Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility );
}

void UNAGA_KineticGrab::OnAPDepleted()
{
	EndAbility( GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false );
}

void UNAGA_KineticGrab::ActivateAbility( const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                         const FGameplayEventData* TriggerEventData )
{
	Super::ActivateAbility( Handle, ActorInfo, ActivationInfo, TriggerEventData );

	if ( HasAuthority( &ActivationInfo ) )
	{
		if ( !CommitAbility( Handle, ActorInfo, ActivationInfo ) )
		{
			EndAbility( Handle, ActorInfo, ActivationInfo, true, true );
			return;
		}

		bool bSuccess = false;

		FHitResult Hit;
		UNAKineticComponent* Component = ActorInfo->AvatarActor->GetComponentByClass<UNAKineticComponent>();
		
		if ( Component )
		{
			if ( const APawn* Pawn = Cast<APawn>( ActorInfo->AvatarActor ) )
			{
				const FVector StartLocation = ActorInfo->AvatarActor->GetActorLocation();
				const FVector ForwardVector = Pawn->GetControlRotation().Vector();
				const FVector EndLocation = StartLocation + ForwardVector * Component->GetRange();

				TArray<AActor*> ChildActors;
				ActorInfo->AvatarActor->GetAllChildActors( ChildActors );
		
				FCollisionQueryParams Params;
				Params.AddIgnoredActor( ActorInfo->AvatarActor.Get() );
				Params.AddIgnoredActors( ChildActors );

				const bool bHit = GetWorld()->LineTraceSingleByChannel
				(
					Hit,
					StartLocation,
					EndLocation,
					ECC_WorldDynamic,
					Params
				);

#if WITH_EDITOR || UE_BUILD_DEBUG
				DrawDebugLine
				(
					GetWorld(),
					StartLocation,
					EndLocation,
					bHit ? FColor::Green : FColor::Red,
					false,
					2.f
				);
#endif	
			}
		}

		if ( Hit.IsValidBlockingHit() )
		{
			if ( Hit.GetComponent()->IsSimulatingPhysics() )
			{
				Component->ForceUpdateControlForward();
				Component->GrabComponentAtLocationWithRotation
				(
					Hit.GetComponent(),
					NAME_None,
					EvaluateActorPosition
					(
						ActorInfo->AvatarActor.Get(),
						Hit.GetComponent(),
						Component->GetControlForward(),
						Component->GetGrabDistance()
					),
					Component->GetControlForward().Rotation()
				);

				UE_LOG(LogTemp, Log, TEXT("%hs: Grabbing %s"), __FUNCTION__, *GetNameSafe( Hit.GetActor() ) )

				Component->bIsGrab = true;
				bSuccess = true;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Object %s physics is not simulated, will not grab."), *GetNameSafe( Hit.GetActor() ) )
			}
		}

		if ( !bSuccess )
		{
			EndAbility( Handle, ActorInfo, ActivationInfo, true, true );
		}
		else
		{
			MoveActorToTask = UNAAT_MoveActorTo::MoveActorTo( this, TEXT( "MoveActorTo" ), ActorInfo->AvatarActor.Get(), Hit.GetActor(), Hit.GetComponent() );
			MoveActorToTask->ReadyForActivation();
			
			APConsumeTask = UNAAT_ConsumeKineticGrabAP::WaitAPDepleted( this, TEXT( "CheckDepleted" ) );
			APConsumeTask->OnAPDepleted.AddUObject( this, &UNAGA_KineticGrab::OnAPDepleted );
			APConsumeTask->ReadyForActivation();
		}
	}
}

void UNAGA_KineticGrab::InputReleased( const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo )
{
	EndAbility( Handle, ActorInfo, ActivationInfo, true, false );
}

void UNAGA_KineticGrab::Throw()
{
	if ( const FGameplayAbilityActivationInfo& Info = GetCurrentActivationInfoRef();
		 HasAuthority( &Info ) )
	{
		if ( const UNAKineticComponent* Component = GetCurrentActorInfo()->AvatarActor->GetComponentByClass<UNAKineticComponent>() )
		{
			if ( Component->bIsGrab )
			{
				const float ImpulseForce = Component->GetForce();
				const FVector ForwardVector = Component->GetControlForward();
				UPrimitiveComponent* OtherComponent = Component->GetGrabbedComponent();
				OtherComponent->AddImpulse( ForwardVector * ImpulseForce );
			}
		}

		EndAbility( GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false );
	}
}