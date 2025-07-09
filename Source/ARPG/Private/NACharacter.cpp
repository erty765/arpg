// Copyright Epic Games, Inc. All Rights Reserved.

#include "NACharacter.h"
#include "AbilitySystemComponent.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "NAGameStateBase.h"
#include "NAPlayerState.h"
#include "ARPG/ARPG.h"
#include "Combat/ActorComponent/NAMontageCombatComponent.h"
#include "../Public/Combat/PhysicsHandleComponent/NAKineticComponent.h"
#include "HP/ActorComponent/NAVitalCheckComponent.h"
#include "HP/GameplayAbility/NAGA_Revive.h"
#include "HP/WidgetComponent/NAReviveWidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Ability/GameplayAbility/NAGA_Suplex.h"
#include "Interaction/NAInteractionComponent.h"
#include "Inventory/Component/NAInventoryComponent.h"
#include "Item/PickableItem//NAWeapon.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Weapon/PickableItemActor/NAWeaponAmmoBox.h"

DEFINE_LOG_CATEGORY( LogTemplateCharacter );

//////////////////////////////////////////////////////////////////////////
// AARPGCharacter

// 공격시 반복되는 함수 패턴
bool TryAttack( const AActor* Hand )
{
	if ( Hand )
	{
		if ( UNAMontageCombatComponent* CombatComponent = Hand->GetComponentByClass<UNAMontageCombatComponent>() )
		{
			if ( CombatComponent->IsAbleToAttack() )
			{
				CombatComponent->StartAttack();
				return true;
			}
		}
	}

	return false;
}

bool TryStopAttack( const AActor* Hand )
{
	if ( Hand )
	{
		if ( UNAMontageCombatComponent* CombatComponent = Hand->GetComponentByClass<UNAMontageCombatComponent>() )
		{
			if ( CombatComponent->IsAttacking() )
			{
				CombatComponent->StopAttack();
				return true;
			}
		}
	}

	return false;
}

ANACharacter::ANACharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetCollisionObjectType( ECC_Pawn);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate
	
	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 200.0f; // The camera follows at this distance behind the character
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 80.f);
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f));
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	DefaultCombatComponent = CreateDefaultSubobject<UNAMontageCombatComponent>( TEXT( "DefaultCombatComponent" ) );

	InteractionComponent = CreateDefaultSubobject<UNAInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetIsReplicated( true );
	InteractionComponent->SetNetAddressable();

	InventoryAngleBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("InventoryAngleBoom"));
	InventoryAngleBoom->SetupAttachment(RootComponent);
	InventoryAngleBoom->SetRelativeLocation(FVector(0.f, 73.f, 27.f));
	InventoryAngleBoom->SetRelativeRotation(FRotator(-8.f, 13.f, 0.f));
	InventoryAngleBoom->TargetArmLength = 176.f;
	InventoryAngleBoom-> bUsePawnControlRotation = false;
	InventoryAngleBoom-> bInheritPitch = false;
	InventoryAngleBoom-> bInheritYaw = true;
	InventoryAngleBoom-> bInheritRoll = false;
	InventoryAngleBoom->bDoCollisionTest = false;
	
	InventoryWidgetBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("InventorySpringArm"));
	InventoryWidgetBoom->SetupAttachment(RootComponent);
	InventoryWidgetBoom->SetRelativeLocation(FVector(0.f, 85.f, -10.f));
	InventoryWidgetBoom->SetRelativeRotation(FRotator::ZeroRotator);
	InventoryWidgetBoom->TargetArmLength = 160.f;
	InventoryWidgetBoom-> bUsePawnControlRotation = false;
	InventoryWidgetBoom-> bInheritPitch = false;
	InventoryWidgetBoom-> bInheritYaw = true;
	InventoryWidgetBoom-> bInheritRoll = false;
	InventoryWidgetBoom->bDoCollisionTest = false;
	InventoryWidgetBoom->bEnableCameraRotationLag = true;
	InventoryWidgetBoom->CameraRotationLagSpeed = 28.f;
	
	InventoryComponent = CreateDefaultSubobject<UNAInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetupAttachment(InventoryWidgetBoom, USpringArmComponent::SocketName);
	InventoryComponent->SetRelativeLocation(FVector(0.f, -28.f, 31.f));
	InventoryComponent->SetRelativeRotation(FRotator(9.f, 0.f, 0.f));
	InventoryComponent->SetRelativeScale3D(FVector(0.42f));

	LeftHandChildActor = CreateDefaultSubobject<UChildActorComponent>(TEXT("LeftHandChildActor"));
	RightHandChildActor = CreateDefaultSubobject<UChildActorComponent>(TEXT("RightHandChildActor"));

	VitalCheckComponent = CreateDefaultSubobject<UNAVitalCheckComponent>(TEXT("VitalCheckComponent"));
	ReviveWidget = CreateDefaultSubobject<UNAReviveWidgetComponent>( TEXT("ReviveWidgetComponent") );
	ReviveWidget->SetGenerateOverlapEvents( false );
	ReviveWidget->SetSimulatePhysics( false );
	ReviveWidget->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	KineticComponent = CreateDefaultSubobject<UNAKineticComponent>( TEXT("KineticComponent") );

	ApplyAttachments();
	
	bReplicates = true;
	ACharacter::SetReplicateMovement( true );

	AbilitySystemComponent->SetNetAddressable();
	DefaultCombatComponent->SetNetAddressable();
	LeftHandChildActor->SetNetAddressable();
	RightHandChildActor->SetNetAddressable();
	KineticComponent->SetNetAddressable();
	
	KineticComponent->SetIsReplicated( true );
	LeftHandChildActor->SetIsReplicated( true );
	RightHandChildActor->SetIsReplicated( true );
}

void ANACharacter::ApplyAttachments() const
{
	TFunction<bool(USkeletalMeshComponent*, USceneComponent*, const FName&)> Attacher;
	if ( FUObjectThreadContext::Get().IsInConstructor )
	{
		Attacher = [](USkeletalMeshComponent* Parent, USceneComponent* Component, const FName& SocketName)
		{
			Component->SetupAttachment( Parent, SocketName );
			return true;
		};
	}
	else
	{
		Attacher = [](USkeletalMeshComponent* Parent, USceneComponent* Component, const FName& SocketName)
		{
			return Component->AttachToComponent( Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName );
		};
	}
	
	if ( GetMesh()->GetSkeletalMeshAsset() )
	{
		Attacher(GetMesh(), LeftHandChildActor, LeftHandSocketName);
		Attacher(GetMesh(), RightHandChildActor, RightHandSocketName);
		Attacher(GetMesh(), ReviveWidget, TEXT("ReviveWidgetSocket"));
	}
}

void ANACharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	if ( HasAuthority() )
	{
		LeftHandChildActor->OnChildActorCreated().AddUObject( this, &ANACharacter::InitializeChildActor );
		RightHandChildActor->OnChildActorCreated().AddUObject( this, &ANACharacter::InitializeChildActor );
	}

	if ( GetController() == GetWorld()->GetFirstPlayerController() )
	{
		// 클라이언트의 BeginPlay에 맞춰 직접 RPC로 요청
    	// 서버에서 직접 수행할 경우 클라이언트에서의 순서:
    	// - Character -> BeginPlay -> GiveAbility -> PlayerState -> AbilityComponent 초기화
    	// 초기화가 되지 않은 시점에서의 GiveAbility의 Replication을 받은 Client은 제대로된 값을 받지 못함
		Server_RequestReviveAbility();
		Server_RequestSuplexAbility();
		Server_RequestKineticGrabAbility();
		
		// 총알을 소모했을때, 인벤토리에 있는 총알의 갯수도 동기화, 인벤토리 상태는 클라이언트에서 업데이트
		AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject( this, &ANACharacter::SyncAmmoConsumptionWithInventory );
		VitalCheckComponent->OnCharacterStateChanged.AddUObject( this, &ANACharacter::HideInventoryIfNotAlive );
	}

	InteractionComponent->SetActive( true );
}

void ANACharacter::PostNetInit()
{
	Super::PostNetInit();
}

void ANACharacter::PreReplication( IRepChangedPropertyTracker& ChangedPropertyTracker )
{
	Super::PreReplication( ChangedPropertyTracker );
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST( ANACharacter, ReplicatedControlRotation, PredicateControlRotationReplication() );
}

bool ANACharacter::PredicateControlRotationReplication() const
{
	bool bShouldReplicate = true;
	const ANAPlayerState* ThisPlayerState = GetPlayerState<ANAPlayerState>();
	bShouldReplicate &= ThisPlayerState != nullptr;
	if ( ThisPlayerState )
	{
		bShouldReplicate &= ThisPlayerState->IsAlive();
	}

	const ANAGameStateBase* CastedGameState = GetWorld()->GetGameState<ANAGameStateBase>();
	bShouldReplicate &= CastedGameState != nullptr;
	
	if ( CastedGameState )
	{
		bShouldReplicate &= CastedGameState->HasAnyoneDead();
	}

	return bShouldReplicate;
}

void ANACharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if ( HasAuthority() )
	{
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->InitAbilityActorInfo(GetPlayerState(), this);
		}

		SetOwner(NewController);
	}

	if (InventoryComponent && NewController->IsLocalPlayerController())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(NewController))
		{
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				InventoryComponent->SetOwnerPlayer(LocalPlayer);
			}
		}
	}
}

void ANACharacter::Server_RequestSuplexAbility_Implementation()
{
	const FGameplayAbilitySpec SpecHandle(UNAGA_Suplex::StaticClass(), 1.f, static_cast<int32>(EAbilityInputID::Grab));
	AbilitySystemComponent->GiveAbility(SpecHandle);
}

void ANACharacter::Server_RequestKineticGrabAbility_Implementation()
{
	if ( KineticComponent )
	{
		KineticComponent->ToggleGrabAbility( true );
	}
}

void ANACharacter::Server_RequestReviveAbility_Implementation()
{
	// 부활 기능 추가
	const FGameplayAbilitySpec SpecHandle( UNAGA_Revive::StaticClass(), 1.f, static_cast<int32>( EAbilityInputID::Revive ) );
	AbilitySystemComponent->GiveAbility( SpecHandle );
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANACharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANACharacter::Look);

		EnhancedInputComponent->BindAction(LeftMouseAttackAction, ETriggerEvent::Started, this, &ANACharacter::StartLeftMouseAttack);
		EnhancedInputComponent->BindAction(LeftMouseAttackAction, ETriggerEvent::Completed, this, &ANACharacter::StopLeftMouseAttack);
		
		EnhancedInputComponent->BindAction( ReviveAction, ETriggerEvent::Started, this, &ANACharacter::TryRevive );
		EnhancedInputComponent->BindAction( ReviveAction, ETriggerEvent::Completed, this, &ANACharacter::StopRevive );
		// Interact
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ANACharacter::TryInteraction);
		// Toggle Inventory
		EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ANACharacter::ToggleInventoryWidget);

		EnhancedInputComponent->BindAction(SelectInventoryButtonAction, ETriggerEvent::Started, this, &ANACharacter::SelectInventorySlot);
		EnhancedInputComponent->BindAction(RemoveItemFromInventoryAction, ETriggerEvent::Started, this, &ANACharacter::RemoveItemFromInventory);
		
		EnhancedInputComponent->BindAction(MedPackShortcutAction, ETriggerEvent::Started, this, &ANACharacter::Server_UseMedPackByShortcut);
		EnhancedInputComponent->BindAction(StasisPackShortcutAction, ETriggerEvent::Started, this, &ANACharacter::Server_UseStasisPackByShortcut);

		EnhancedInputComponent->BindAction(RightMouseAttackAction, ETriggerEvent::Started, this, &ANACharacter::Zoom);
		
		EnhancedInputComponent->BindAction(ToggleWeaponEquippedAction, ETriggerEvent::Started, this, &ANACharacter::Server_ToggleWeaponEquipped);
		
		EnhancedInputComponent->BindAction(SelectWeaponAction, ETriggerEvent::Started, this, &ANACharacter::SelectWeaponByMouseWheel);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ANACharacter::RetrieveAsset(const AActor* InCDO)
{
	if (const ANACharacter* DefaultAsset = Cast<ANACharacter>(InCDO))
	{
		FObjectPropertyUtility::CopyClassPropertyIfTypeEquals<ANACharacter, UInputMappingContext, UInputAction>( this, DefaultAsset );
		
		struct LazyUpdatePair
		{
			USceneComponent* OldParent;
			USceneComponent* OldChild;
			USceneComponent* NewParent;
			USceneComponent* NewChild;
		};

		const TArray WhiteList
		{
			UInputComponent::StaticClass(), // 만들어질때 초기화됨
			UAbilitySystemComponent::StaticClass() // Replicate된 Instance가 파괴됨
		};
		
		TSet<USceneComponent*> Initialized;
		TArray<LazyUpdatePair> LazyUpdates;
		
		// 블루프린트의 컴포넌트 속성 복사
		for ( TFieldIterator<FObjectProperty> It(GetClass()); It; ++It  )
		{
			if ( It->PropertyClass->IsChildOf( UActorComponent::StaticClass() ) )
			{
				bool bSkip = WhiteList.ContainsByPredicate( [ &It ]( const UClass* Class )
				{
					return It->PropertyClass->IsChildOf( Class );
				} );

				if ( bSkip )
				{
					continue;
				}
					
				UActorComponent* ThisComponent = Cast<UActorComponent>( It->GetObjectPropertyValue_InContainer( this ) );
				UActorComponent* OriginComponent = Cast<UActorComponent>( It->GetObjectPropertyValue_InContainer( DefaultAsset ) );

				if ( !ThisComponent || !OriginComponent )
				{
					continue;
				}
				
				// 프로퍼티 복사 후 컴포넌트 재등록하면서 갱신
				ThisComponent->UnregisterComponent();
				
				if ( IsValid( GEngine ) )
				{
					UEngine::FCopyPropertiesForUnrelatedObjectsParams Params{};
					Params.bDoDelta = true;
					Params.bClearReferences = false;
					Params.bSkipCompilerGeneratedDefaults = true;
					Params.bReplaceInternalReferenceUponRead = true;
					std::remove_pointer_t<decltype(Params.OptionalReplacementMappings)> TempReplacementMappings;
					Params.OptionalReplacementMappings = &TempReplacementMappings;

					// Scene Component이면...
					if ( USceneComponent* OriginSceneComponent = Cast<USceneComponent>( OriginComponent ) )
					{
						// 부모로 부착된 컴포넌트가 있는지 확인하고...
						if ( USceneComponent* OriginParentComponent = OriginSceneComponent->GetAttachParent() )
						{
							// 똑같은 컴포넌트를 프로퍼티로 찾아서...
							for ( TFieldIterator<FObjectProperty> ParentIt( GetClass() ); ParentIt; ++ParentIt )
							{
								if ( ParentIt->PropertyClass->IsChildOf( USceneComponent::StaticClass() ) )
								{
									USceneComponent* ComponentPtrFromOrigin = Cast<USceneComponent>( ParentIt->GetObjectPropertyValue_InContainer( DefaultAsset ) );

									// 이 객체의 프로퍼티에 있는 컴포넌트에 똑같이 적용
									if ( ComponentPtrFromOrigin == OriginParentComponent )
									{
										USceneComponent* ThisParentComponent = Cast<USceneComponent>( ParentIt->GetObjectPropertyValue_InContainer( this ) );
										USceneComponent* ThisSceneComponent = Cast<USceneComponent>( ThisComponent );
										
										TempReplacementMappings.Emplace( OriginParentComponent, ThisParentComponent );
									
										if ( ThisParentComponent && Initialized.Contains( ThisParentComponent ) )
										{
											ThisSceneComponent->AttachToComponent( ThisParentComponent, FAttachmentTransformRules::KeepRelativeTransform, OriginSceneComponent->GetAttachSocketName() );
											Initialized.Emplace( Cast<USceneComponent>( ThisComponent ) );
										}
										else
										{
											// 일시적으로 부착을 풀고
											ThisSceneComponent->DetachFromComponent( FDetachmentTransformRules::KeepRelativeTransform );
											LazyUpdates.Emplace( OriginParentComponent, OriginSceneComponent, ThisParentComponent, ThisSceneComponent );
										}
										break;
									}
								}
							}
						}
						else
						{
							// 부모가 없다면 그대로 초기화 판정
							Initialized.Emplace( Cast<USceneComponent>( ThisComponent ) );
						}
					}
					
					GEngine->CopyPropertiesForUnrelatedObjects( OriginComponent, ThisComponent, Params );
				}

				ThisComponent->RegisterComponent();
			}
		}

		// 부모가 초기화가 안된 상태에서 자식을 붙이려 한 경우에 대해 게으른 초기화
		for ( auto SetIt = LazyUpdates.CreateIterator(); SetIt; ++SetIt )
		{
			const auto& [Old, OldChild, This, ThisChild] = *SetIt;
			if ( Initialized.Contains( This ) )
			{
				ThisChild->AttachToComponent( This, FAttachmentTransformRules::KeepRelativeTransform, OldChild->GetAttachSocketName() );
				Initialized.Emplace( ThisChild );
				SetIt.RemoveCurrentSwap();
			}
		}

		// 블루프린트 기존에 있던 ASC 초기화 값 복사
		for ( const FAttributeDefaults& Attribute : DefaultAsset->GetAbilitySystemComponent()->DefaultStartingData )
		{
			GetAbilitySystemComponent()->InitStats( Attribute.Attributes, Attribute.DefaultStartingTable );
		}
		
		check( LazyUpdates.IsEmpty() );
		
		ApplyAttachments();
		const FTransform Transform = DefaultAsset->GetMesh()->GetRelativeTransform();
		
		GetMesh()->SetRelativeTransform(Transform);
		// 리플리케이션을 위한 Mesh Offset
		CacheInitialMeshOffset( Transform.GetLocation(), Transform.Rotator() );
	}
}

void ANACharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	GetAbilitySystemComponent()->InitAbilityActorInfo
	(
		GetPlayerState<APlayerState>(),
		this
	);
}

void ANACharacter::FaceRotation( FRotator NewControlRotation, float DeltaTime )
{
	if ( HasAuthority() )
	{
		if ( !bStopOverrideControlRotation )
		{
			Super::FaceRotation( NewControlRotation, DeltaTime );	
		}
	}
}

void ANACharacter::OnConstruction( const FTransform& Transform )
{
	Super::OnConstruction( Transform );

#if WITH_EDITOR
	if ( GetWorld()->IsEditorWorld() )
	{
		ApplyAttachments();	
	}
#endif
}

FRotator ANACharacter::GetReplicatedControlRotation() const
{
	return ReplicatedControlRotation.Rotation();
}

void ANACharacter::SetStopOverrideControlRotation( bool bFlag, const FRotator& Rotator )
{
	bStopOverrideControlRotation = bFlag;

	if ( bStopOverrideControlRotation )
	{
		CustomControlRotation = Rotator;
		SetActorRotation( CustomControlRotation );
	}
}

void ANACharacter::InitializeChildActor( AActor* Actor )
{
	Actor->SetOwner( this );

	if ( UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>( Actor->GetRootComponent() ) )
	{
		PrimitiveComponent->SetSimulatePhysics( false );
	}

	if ( const TScriptInterface<IAbilitySystemInterface>& Interface = Actor )
	{
		Interface->GetAbilitySystemComponent()->InitAbilityActorInfo( this, Actor );
	}
}

void ANACharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ANACharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ANACharacter::StartLeftMouseAttack()
{
	if ( VitalCheckComponent->GetCharacterStatus() != ECharacterStatus::Alive )
	{
		return;
	}
	
	if ( KineticComponent->HasGrabbed() )
	{
		return;
	}

	if ( RightHandChildActor->GetChildActor() )
	{
		TryAttack( RightHandChildActor->GetChildActor() );
		return;
	}
	
	if ( DefaultCombatComponent->IsAbleToAttack() && !DefaultCombatComponent->IsAttacking() )
	{
		DefaultCombatComponent->StartAttack();	
	}
}

void ANACharacter::StopLeftMouseAttack()
{
	if ( RightHandChildActor->GetChildActor() )
	{
		TryStopAttack( RightHandChildActor->GetChildActor() );
		return;
	}

	if ( DefaultCombatComponent->IsAttacking() )
	{
		DefaultCombatComponent->StopAttack();	
	}
}

void ANACharacter::OnRep_Zoom()
{
	// 서버가 바뀌면 다른 클라한테 전달
	if (!IsLocallyControlled())
	{
		ZoomImpl(bIsZoom);
	}
}

void ANACharacter::Jump()
{
	if ( VitalCheckComponent->GetCharacterStatus() != ECharacterStatus::Alive )
	{
		return;	
	}
	
	Super::Jump();
}

void ANACharacter::Zoom()
{
	if ( VitalCheckComponent->GetCharacterStatus() != ECharacterStatus::Alive )
	{
		return;
	}
	
	if (InventoryComponent->IsVisible()) return;
	
	SetZoom();
}

void ANACharacter::ZoomImpl(bool bZoom)
{
	// 카메라 처리 과정 입니다
	// zoom 상태
	if (bZoom)
	{
		CameraBoom->TargetArmLength = 0;
		bUseControllerRotationYaw = true; // 서버클라 동기화 필요
	}
	//Zoom 상태 해제
	else if (!bZoom)
	{		
		CameraBoom->TargetArmLength = 200;
		bUseControllerRotationYaw = false;// 서버클라 동기화 필요
	}
}

void ANACharacter::SetZoom()
{
	bIsZoom = !bIsZoom;
	ZoomImpl(bIsZoom);
	GetMesh()->SetOwnerNoSee(bIsZoom);
	
	if (!HasAuthority())
	{
		ServerSetZoom(bIsZoom);
	}
}

void ANACharacter::ServerSetZoom_Implementation(bool bZoom)
{
	bIsZoom = bZoom;
	ZoomImpl(bZoom);
}

bool ANACharacter::ServerSetZoom_Validate(bool bZoom)
{
	return true;
}

void ANACharacter::TryInteraction()
{
	if ( VitalCheckComponent->GetCharacterStatus() != ECharacterStatus::Alive )
	{
		return;
	}
	
	if (ensure(InteractionComponent != nullptr))
	{
		// 인벤토리 정보는 클라이언트에서 개별적으로 관리 
		InteractionComponent->ToggleInteraction();
	}
}

bool ANACharacter::CanToggleInventoryWidget() const
{
	return InventoryComponent &&
		   !bIsExpandingInventoryWidget &&
		   VitalCheckComponent->GetCharacterStatus() == ECharacterStatus::Alive;
}

void ANACharacter::ToggleInventoryWidget()
{
	if (bIsZoom) return;
	
	if (ensure(InventoryComponent != nullptr))
	{
		if (CanToggleInventoryWidget())
		{
			if (!InventoryComponent->IsInventoryWidgetVisible())
			{
				RotateSpringArmForInventory(true, 0.6f);
				ToggleInventoryCameraView(true, InventoryAngleBoom, 0.6f, {} );
				InventoryComponent->ReleaseInventory();
				if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
				{
					if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
					{
						Subsystem->AddMappingContext(InventoryMappingContext, 0);
					}
				}
			}
			else
			{
				RotateSpringArmForInventory(false, 0.4f);
				ToggleInventoryCameraView(false, CameraBoom, 0.4f, { -20.f, 0.f, 0.f } );
				InventoryComponent->CollapseInventory();
				if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
				{
					if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
					{
						Subsystem->RemoveMappingContext(InventoryMappingContext);
					}
				}
			}
		}
	}
}

void ANACharacter::Server_ToggleWeaponEquipped_Implementation()
{
	if (!RightHandChildActor || !LeftHandChildActor || !InventoryComponent)
		return;

	ANAWeapon* WeaponToUnequip_Right = Cast<ANAWeapon>(RightHandChildActor->GetChildActor());
	ANAWeapon* WeaponToUnequip_Left = Cast<ANAWeapon>(LeftHandChildActor->GetChildActor());
	const bool bShouldEquipWeapon = !WeaponToUnequip_Right && !WeaponToUnequip_Left;
	
	if (bShouldEquipWeapon)
	{
		// 무기 어태치, 인벤토리에 소지된 무기가 있다면, 가장 작은 넘버의 슬롯에 적재된 무기로 장착 시도
		UNAItemData* WeaponToEquip = InventoryComponent->FindSameClassItem(ANAWeapon::StaticClass());
		if (WeaponToEquip)
		{
			if (EquipWeapon(WeaponToEquip))
			{
				InventoryComponent->SetEquippedWeaponIndex(WeaponToEquip);
			}
		}
	}
	else
	{
		// 무기 장착만 해제, 인벤토리에서 무기를 드랍하지 않음
		if (UnequipWeapon())
		{
			InventoryComponent->SetEquippedWeaponIndex(nullptr);
		}
	}
}

ANAItemActor* ANACharacter::EquipWeapon(UNAItemData* WeaponToEquip)
{
	if (!RightHandChildActor || !LeftHandChildActor || !InventoryComponent)
	return nullptr;
	
	ANAItemActor* NewlyEquippedWeapon = nullptr;
	if (WeaponToEquip)
	{
		AActor* NewlyAttachedWeapon = nullptr;
		UClass* WeaponClass = WeaponToEquip->GetItemActorClass();
		
		if (!RightHandChildActor->GetChildActor())
		{
			RightHandChildActor->SetChildActorClass(WeaponClass);
			NewlyAttachedWeapon = RightHandChildActor->GetChildActor();
		}
		else if (!LeftHandChildActor->GetChildActor())
		{
			LeftHandChildActor->SetChildActorClass(WeaponClass);
			NewlyAttachedWeapon = LeftHandChildActor->GetChildActor();
		}

		if (NewlyAttachedWeapon)
		{
			NewlyEquippedWeapon = CastChecked<ANAWeapon>(NewlyAttachedWeapon);
			ANAItemActor::AssignItemDataToChildActor(WeaponToEquip, NewlyEquippedWeapon);
		}
	}
	
	return NewlyEquippedWeapon;
}

bool ANACharacter::UnequipWeapon()
{
	if (!RightHandChildActor || !LeftHandChildActor || !InventoryComponent)
		return false;

	ANAWeapon* WeaponToUnequip_Right = Cast<ANAWeapon>(RightHandChildActor->GetChildActor());
	ANAWeapon* WeaponToUnequip_Left = Cast<ANAWeapon>(LeftHandChildActor->GetChildActor());
	
	if (WeaponToUnequip_Right)
	{
		RightHandChildActor->DestroyChildActor();
		RightHandChildActor->SetChildActorClass(nullptr);
	}
	else if (WeaponToUnequip_Left)
	{
		LeftHandChildActor->DestroyChildActor();
		LeftHandChildActor->SetChildActorClass(nullptr);
	}

	return !RightHandChildActor->GetChildActor() && !LeftHandChildActor->GetChildActor();
}

void ANACharacter::Server_UseStasisPackByShortcut_Implementation()
{
	if ( VitalCheckComponent->GetCharacterStatus() != ECharacterStatus::Alive )
	{
		return;
	}
	
	if (GEngine) {
		FString Log = TEXT("UseStasisPackByShortcut");
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, *Log);
	}

	if (InventoryComponent)
	{
		InventoryComponent->UseStasisPackAutomatically(this);
	}
}

// @TODO: 마우스 휠로 무기 바꾸기 -> 특정 선행 키 입력 도중에만 활성하기 (e.g. ctrl 누르면서 휠 돌릴때만 작동)
// @TODO: 선행 키 입력 도중 무기 퀵슬롯 위젯 표시?
void ANACharacter::SelectWeaponByMouseWheel(const FInputActionValue& Value)
{
	if (!InventoryComponent) return;

	if ( KineticComponent->HasGrabbed() )
	{
		return;
	}
	
	// 디바운스 처리
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastWheelInputTime < InputDebounceDelay)
	{
		return;
	}
	LastWheelInputTime = CurrentTime;
	
	const float AxisValue = Value.Get<float>();
	if (FMath::IsNearlyZero(AxisValue)) return;

	const int32 Direction = AxisValue > 0.f ? 1 : -1;
	Server_SwapWeapon( Direction );
}

void ANACharacter::Server_SwapWeapon_Implementation( const int32 Direction )
{
	UNAItemData* SelectedWeapon = InventoryComponent->SelectNextWeapon(Direction);
	if (SelectedWeapon)
	{
		// 무기 교체
		if (EquipWeapon(SelectedWeapon))
		{
			InventoryComponent->SetEquippedWeaponIndex(SelectedWeapon);
		}
	}
	else
	{
		// 무기 장착 해제
		if (UnequipWeapon())
		{
			InventoryComponent->SetEquippedWeaponIndex(nullptr);
		}
	}
}

void ANACharacter::HideInventoryIfNotAlive( ECharacterStatus Old, ECharacterStatus New )
{
	if ( New != ECharacterStatus::Alive )
	{
		if ( InventoryComponent && InventoryComponent->IsInventoryWidgetVisible() )
		{
			ToggleInventoryWidget();
		}
	}
}

void ANACharacter::RotateSpringArmForInventory(bool bExpand, float Overtime)
{
	if (!InventoryComponent || !InventoryWidgetBoom) return;

	FVector TargetLocation = InventoryWidgetBoom->GetRelativeLocation();
	FRotator TargetRotation;
	if (bExpand)
	{
		TargetRotation = FRotator(0.0f, -163.f, 0.0f);
	}
	else
	{
		TargetRotation = FRotator::ZeroRotator;
	}

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = NAME_None;
	LatentInfo.Linkage = INDEX_NONE;
	LatentInfo.UUID = INDEX_NONE;

	// 4) World-space 이동 (MoveComponentTo: Attach Parent에 대한 Relative Loc & Rot을 활용하여 보간)
	UKismetSystemLibrary::MoveComponentTo(
		InventoryWidgetBoom,
		TargetLocation,
		TargetRotation,
		true, true,
		Overtime,
		true,
		EMoveComponentAction::Move,
		LatentInfo
	);
}

void ANACharacter::ToggleInventoryCameraView(const bool bEnable, USpringArmComponent* InNewBoom, float Overtime, const FRotator& Rotation)
{
	if (!InNewBoom|| !FollowCamera) return;
	
	bIsExpandingInventoryWidget = true;
	
	FollowCamera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	FName CallbackFunc;
	if (bEnable) // Release (Move To Inventory Camera View)
	{
		CallbackFunc = TEXT("OnInventoryCameraEnterFinished");
	}
	else // Collapse (Move From Inventory Camera View)
	{
		CallbackFunc = TEXT("OnInventoryCameraExitFinished");
	}
	// 인벤토리 연출 순서: 인벤 위젯 회전 -> 카메라 앵글 변경(한 프레임 뒤에서)
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda(
		[this, Rotation,InNewBoom, Overtime, CallbackFunc]()
		{
			// 1) 목표 위치/회전 계산 (NewBoom의 월드 위치/회전 등)
			FollowCamera->AttachToComponent(InNewBoom, FAttachmentTransformRules::KeepWorldTransform,
											USpringArmComponent::SocketName);
			FVector TargetLocation = FVector::ZeroVector;
			FRotator TargetRotation = Rotation;

			// 2) 월드 공간에서 MoveComponentTo로 보간
			FLatentActionInfo LatentInfo;
			LatentInfo.CallbackTarget = this;
			LatentInfo.ExecutionFunction = CallbackFunc;
			LatentInfo.Linkage = 1;
			LatentInfo.UUID = 1211; // 임의의 고유 값

			// 3) World-space 이동 (MoveComponentTo: Attach Parent에 대한 Relative Loc & Rot을 활용하여 보간)
			UKismetSystemLibrary::MoveComponentTo(
				FollowCamera,
				TargetLocation,
				TargetRotation,
				true, true,
				Overtime,
				true,
				EMoveComponentAction::Move,
				LatentInfo
			);
		}));
}

void ANACharacter::OnInventoryCameraEnterFinished()
{
	bIsExpandingInventoryWidget = false;
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		CameraBoom->bUsePawnControlRotation = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
	}
}
void ANACharacter::OnInventoryCameraExitFinished()
{
	// 회전 컨트롤 세팅 바뀔때 약간 끊겨서 다음 프레임 틱에서 세팅 변경
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]()
	{
		bIsExpandingInventoryWidget = false;
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			PC->SetMouseLocation(0,0);
			PC->SetControlRotation(CameraBoom->GetComponentRotation());
			CameraBoom->bUsePawnControlRotation = true;
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
		}
	}));
}

void ANACharacter::SelectInventorySlot(const FInputActionValue& Value)
{
	// if (GEngine) {
	// 	FString Log = TEXT("SelectInventorySlot");
	// 	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, *Log);
	// }
	if (InventoryComponent)
	{
		// @TODO: 인벤토리에서 '선택' 액션 구현하기
		InventoryComponent->SelectInventorySlotButton();
	}
}

void ANACharacter::RemoveItemFromInventory(const FInputActionValue& Value)
{
	if (GEngine) {
		FString Log = TEXT("RemoveItemFromInventory");
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, *Log);
	}
	if (InventoryComponent)
	{
		InventoryComponent->RemoveItemAtInventorySlot();
	}
}

void ANACharacter::Server_UseMedPackByShortcut_Implementation()
{
	if ( VitalCheckComponent->GetCharacterStatus() != ECharacterStatus::Alive )
	{
		return;
	}
	
	if (GEngine) {
		FString Log = TEXT("UseMedPackByShortcut");
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, *Log);
	}

	if ( InventoryComponent )
	{
		InventoryComponent->UseMedPackAutomatically( this );
	}
}

void ANACharacter::TryRevive()
{
	// todo: GAS의 Input 감지를 이용할 수 있음
	if ( AbilitySystemComponent )
	{
		AbilitySystemComponent->AbilityLocalInputPressed( static_cast<int32>( EAbilityInputID::Revive ) );
	}
}

void ANACharacter::StopRevive()
{
	// todo: GAS의 Input 감지를 이용할 수 있음
	if ( AbilitySystemComponent )
	{
		AbilitySystemComponent->AbilityLocalInputReleased( static_cast<int32>( EAbilityInputID::Revive ) );
	}
}

void ANACharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME( ANACharacter, AbilitySystemComponent );
	DOREPLIFETIME( ANACharacter, DefaultCombatComponent );
	DOREPLIFETIME( ANACharacter, LeftHandChildActor );
	DOREPLIFETIME( ANACharacter, RightHandChildActor );
	DOREPLIFETIME( ANACharacter, bIsZoom);
	DOREPLIFETIME_CONDITION( ANACharacter, InteractionComponent, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( ANACharacter, ReplicatedControlRotation, COND_Custom );
	DOREPLIFETIME( ANACharacter, KineticComponent );
}

void ANACharacter::SyncAmmoConsumptionWithInventory( const FActiveGameplayEffect& ActiveGameplayEffect )
{
	FGameplayTagContainer Container;
	FGameplayTagContainer AmmoParentContainer;
	AmmoParentContainer.AddTag( FGameplayTag::RequestGameplayTag( "Weapon.Ammo" ) );
	ActiveGameplayEffect.Spec.GetAllAssetTags( Container );

	// 현재 활성화된 모든 게임태그를 가져온 다음 총알 관련 태그로만 분류
	if ( !Container.HasAny( AmmoParentContainer ) )
	{
		return;
	}

	// 인벤토리에서 총알 관련 아이템만을 가져옴
	TArray<UNAItemData*> OutItems;
	InventoryComponent->FindSameClassItems( ANAWeaponAmmoBox::StaticClass(), OutItems );

	const UNAItemData* DesignatedAmmo = nullptr;
	// 소모된 게임 이펙트에 해당하는 총알로 구분해 액터를 가져옴
	for ( const UNAItemData* Item : OutItems )
	{
		if ( ANAWeaponAmmoBox* AmmoBox = Cast<ANAWeaponAmmoBox>( Item->GetItemActorClass()->GetDefaultObject() );
			 ensureAlways( AmmoBox ) ) // AmmoBox에서 파생된 클래스가 아닌데 잡힘
		{
			if ( ActiveGameplayEffect.Spec.Def->IsA( AmmoBox->GetAmmoEffectType() ) )
			{
				DesignatedAmmo = Item;
				break;
			}
		}
	}

	// 인벤토리 수량 감소
	if ( ensureAlways( DesignatedAmmo ) ) // 총알이 변했는데 변한 총알을 찾을 수 없는 경우..
	{
		check( InventoryComponent->TryRemoveItem( InventoryComponent->FindSlotIDForItem( DesignatedAmmo ), 1 ) );
	}
}

void ANACharacter::Tick( float DeltaSeconds )
{
	Super::Tick( DeltaSeconds );

	if ( Controller && HasAuthority() )
	{
		ReplicatedControlRotation = Controller->GetControlRotation().Vector();
	}
}
