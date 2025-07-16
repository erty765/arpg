// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PickableItem//NAWeapon.h"

#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Combat/ActorComponent/NAMontageCombatComponent.h"
#include "Item/ItemDataStructs/NAWeaponDataStructs.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/WidgetComponent/NAAmmoIndicatorComponent.h"
#include "Components/ShapeComponent.h"

// Sets default values
ANAWeapon::ANAWeapon() : ANAPickableItemActor(FObjectInitializer::Get())
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CombatComponent = CreateDefaultSubobject<UNAMontageCombatComponent>( TEXT("CombatComponent") );
	// 무기가 캐릭터의 Child Actor로 부착될 것이기에 CombatComponent의 설정이 Parent Actor로 가도록
	CombatComponent->SetConsiderChildActor( true );

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>( TEXT("AbilitySystemComponent") );
	MuzzleFlashComponent = CreateDefaultSubobject<UNiagaraComponent>( TEXT( "MuzzleFlashComponent") );
	MuzzleFlashComponent->SetAutoActivate( false );

	AmmoIndicatorComponent = CreateDefaultSubobject<UNAAmmoIndicatorComponent>( TEXT("AmmoIndicatorComponent") );
	// 플레이어를 바라보도록
	AmmoIndicatorComponent->SetRelativeRotation( {0.f, -90.f, 0.f} );
	AmmoIndicatorComponent->SetRelativeLocation( {0.f, 0.f, 10.f} );
	
	PickupMode = EPickupMode::PM_Inventory;

	if ( ItemMesh )
	{
		AmmoIndicatorComponent->SetupAttachment( ItemMesh,TEXT("Indicator") );
		MuzzleFlashComponent->SetupAttachment( ItemMesh,TEXT( "Muzzle" ) );
	}
}

EFireArmType ANAWeapon::GetFireArmType() const
{
	return FireArmType;
}

// Called when the game starts or when spawned
void ANAWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 몽타주랑 공격이 설정되어 있는지 확인
	check( CombatComponent->GetMontage() && CombatComponent->GetAttackAbility() );
	CombatComponent->SetActive( true );

	MuzzleFlashComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	MuzzleFlashComponent->SetGenerateOverlapEvents( false );

	AmmoIndicatorComponent->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	AmmoIndicatorComponent->SetGenerateOverlapEvents( false );
	
	MuzzleFlashComponent->SetActive( false );

	// 서버에서 막 ChildActorComponent에 의해 스폰된 경우
	if ( HasAuthority() )
	{
		if ( USceneComponent* ParentActorComponent = GetParentComponent() )
		{
			if ( AActor* OwningActor = ParentActorComponent->GetOwner() )
			{
				SetOwner( OwningActor );

				if ( const FNAWeaponTableRow* WeaponTable = static_cast<const FNAWeaponTableRow*>( UNAItemEngineSubsystem::Get()->GetItemMetaDataByClass( GetClass() ) ) )
				{
					// 부착된 상태에서 오프셋 조정
					// 액터는 초기에 생성된 시점에서 ChildActorComponent에 부착된 상태가 아니라서
					// 설정한 상대 위치가 소실되므로 대신에 ChildActorComponent의 상대 위치를 조정.
					PreviousParentComponentTransform = ParentActorComponent->GetRelativeTransform();
					ParentActorComponent->SetRelativeTransform( WeaponTable->AttachmentTransform );
				}
			}
		}
	}
	else
	{
		if ( AActor* Actor = GetAttachParentActor() )
		{
			AbilitySystemComponent->InitAbilityActorInfo( Actor, this );
		}
	}
}

void ANAWeapon::EndPlay( const EEndPlayReason::Type EndPlayReason )
{
	Super::EndPlay( EndPlayReason );

	if ( USceneComponent* ParentActorComponent = GetParentComponent() )
	{
		ParentActorComponent->SetRelativeTransform( PreviousParentComponentTransform );
	}
}

void ANAWeapon::GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	DOREPLIFETIME( ANAWeapon, CombatComponent );
	DOREPLIFETIME( ANAWeapon, AbilitySystemComponent );
}

void ANAWeapon::OnConstruction( const FTransform& Transform )
{
	Super::OnConstruction( Transform );
	
	if ( const FNAWeaponTableRow* WeaponTable = static_cast<const FNAWeaponTableRow*>( UNAItemEngineSubsystem::Get()->GetItemMetaDataByClass( GetClass() ) ) )
	{
		FireArmType = WeaponTable->FirearmStatistics.FireArmType;
	}
}

// Called every frame
void ANAWeapon::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
}

