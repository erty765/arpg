// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Assets/Interface/NAManagedAsset.h"
#include "Combat/Interface/NAHandActor.h"
#include "GameFramework/Character.h"
#include "HP/ActorComponent/NAVitalCheckComponent.h"
#include "Logging/LogMacros.h"
#include "NACharacter.generated.h"

class UNAKineticComponent;
class UNAReviveWidgetComponent;
class UWidgetComponent;
class UNAVitalCheckComponent;
class UNAMontageCombatComponent;
class UNAAttributeSet;
class UGameplayEffect;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAbilitySystemComponent;
class UMaterialInstanceConstant;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ANACharacter : public ACharacter, public IAbilitySystemInterface, public INAManagedAsset, public INAHandActor
{
	GENERATED_BODY()
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Gameplay, meta=(AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta=(AllowPrivateAccess="true"))
	UChildActorComponent* LeftHandChildActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, meta=(AllowPrivateAccess="true"))
	UChildActorComponent* RightHandChildActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	UNAVitalCheckComponent* VitalCheckComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	UNAReviveWidgetComponent* ReviveWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asset", meta=(AllowPrivateAccess="true"))
	FName AssetName;


	// 상호작용 컴포넌트, RPC 사용을 위해 Replication
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Interaction", meta=(AllowPrivateAccess="true"))
	TObjectPtr<class UNAInteractionComponent> InteractionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<class UNAInventoryComponent> InventoryComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> InventoryWidgetBoom;

	// 인벤토리 활성 시, FollowCamera를 부착할 때 사용하는 스프링 암
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> InventoryAngleBoom;

	// 양손에 무기가 없을때 사용되는 전투 컴포넌트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Combat", meta=(AllowPrivateAccess="true"))
	UNAMontageCombatComponent* DefaultCombatComponent;
	
	UPROPERTY(VisibleAnywhere, BLueprintReadOnly, Replicated, Category = "Kinesis", meta=(AllowPrivateAccess="true"))
	UNAKineticComponent* KineticComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Zoom, Category = "AnimInstance", meta = (AllowPrivateAccess = "true"))
	bool bIsZoom;

	bool bStopOverrideControlRotation;

	FRotator CustomControlRotation;

// Default MappingContext & Input Actions //////////////////////////////////////////////////////////////////////////////
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftMouseAttackAction;

	/* Zoom Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightMouseAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ReviveAction;
	
	/* Grab */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* GrabAction;

	/* Equip/UnequipWeapon*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleWeaponEquippedAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* SelectWeaponAction;
	
	/* Interaction Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	/* Inventory Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleInventoryAction;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MedPackShortcutAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* StasisPackShortcutAction;
	
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
// Inventory MappingContext & Input Actions ////////////////////////////////////////////////////////////////////////////
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* InventoryMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* SelectInventoryButtonAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RemoveItemFromInventoryAction;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Input", meta = (AllowPrivateAccess = "true"))
	FVector_NetQuantizeNormal ReplicatedControlRotation;
	
public:
	ANACharacter();
	
	virtual void OnConstruction(const FTransform& Transform) override;

	bool IsZoom() const { return bIsZoom; }

	FRotator GetReplicatedControlRotation() const;

	void SetStopOverrideControlRotation( bool bFlag, const FRotator& Rotator );

protected:
	void InitializeChildActor( AActor* Actor );

	virtual void SetAssetNameDerivedImplementation(const FName& InAssetName) override { AssetName = InAssetName; }

	virtual FName GetAssetName() const override { return AssetName; }

	virtual void RetrieveAsset(const AActor* InCDO) override;

	virtual void OnRep_PlayerState() override;

	virtual	void FaceRotation(FRotator NewControlRotation, float DeltaTime = 0) override;

	// 블루프린트 타입 CDO로부터 컴포넌트 부착 정보를 알아올 수 없음
	// 모든 네이티브 및 블루프린트 정보가 적용된 후 호출하여 구조를 유지
	void ApplyAttachments() const;
	
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// 왼쪽 클릭으로 공격을 시작할 경우
	UFUNCTION()
	void StartLeftMouseAttack();

	// 오른 클릭으로 공격을 시작할 경우
	UFUNCTION()
	void StopLeftMouseAttack();

	UFUNCTION()
	void OnRep_Zoom();

	virtual void Jump() override;

	// 무기 Zoom 상태 
	UFUNCTION()
	void Zoom();

	void ZoomImpl(bool bZoom);

	void SetZoom();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetZoom(bool bZoom);

	// Interaction Input
	UFUNCTION()
	void TryInteraction();

	// Inventory Widget Release/Collapse
	uint8 bIsExpandingInventoryWidget : 1 = false;
	bool CanToggleInventoryWidget() const;
	UFUNCTION()
	void ToggleInventoryWidget();

	UFUNCTION(Server, Reliable)
	void Server_ToggleWeaponEquipped();

	class ANAItemActor* EquipWeapon(class UNAItemData* WeaponToEquip);
	bool UnequipWeapon();

	// Rotate Inventory Widget
	UFUNCTION()
	void RotateSpringArmForInventory(bool bExpand, float Overtime);

	// Toggle the transition between camera view and inventory view 
	UFUNCTION()
	void ToggleInventoryCameraView( bool bEnable, USpringArmComponent* InNewBoom, float Overtime, const FRotator& Rotation );

	UFUNCTION()
	void OnInventoryCameraEnterFinished();
	UFUNCTION()
	void OnInventoryCameraExitFinished();

	void SelectInventorySlot(const FInputActionValue& Value);
	void RemoveItemFromInventory(const FInputActionValue& Value);
	
	// 힐팩 단축키로 자동 사용: 높은 등급부터
	UFUNCTION( Server, Reliable )
	void Server_UseMedPackByShortcut();

	UFUNCTION( Server, Reliable )
	void Server_UseStasisPackByShortcut();

	void SelectWeaponByMouseWheel(const FInputActionValue& Value);
	float LastWheelInputTime = 0.f;
	const float InputDebounceDelay = 0.18f;

	UFUNCTION( Server, Unreliable )
	void Server_SwapWeapon( const int32 Direction );
	
	void HideInventoryIfNotAlive( ECharacterStatus Old, ECharacterStatus New );
	
protected:
	void TryRevive();

	void StopRevive();
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SyncAmmoConsumptionWithInventory( const FActiveGameplayEffect& ActiveGameplayEffect );

	virtual void Tick(float DeltaSeconds) override;
	
	// To add mapping context
	virtual void BeginPlay() override;

	virtual void PostNetInit() override;

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	bool PredicateControlRotationReplication() const;
	
	virtual void PossessedBy(AController* NewController) override;

	UFUNCTION( Server, Reliable )
	void Server_RequestReviveAbility();

	UFUNCTION( Server, Reliable )
	void Server_RequestKineticGrabAbility();

	UFUNCTION( Server, Unreliable )
	void Server_BeginInteraction();

	UFUNCTION( Server, Reliable)
	void Server_RequestSuplexAbility();

protected:
	virtual UChildActorComponent* GetLeftHandChildActorComponent() const override { return LeftHandChildActor; }
	virtual UChildActorComponent* GetRightHandChildActorComponent() const override { return RightHandChildActor; }
	
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }
	FORCEINLINE UNAReviveWidgetComponent* GetReviveWidget() const { return ReviveWidget; }
};

