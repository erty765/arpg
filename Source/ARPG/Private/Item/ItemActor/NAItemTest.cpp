// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemActor/NAItemTest.h"
#include "Components/ArrowComponent.h"


// Sets default values
ANAItemTest::ANAItemTest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TestStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TestStaticMesh"));
	if (ItemMesh)
	{
		TestStaticMeshComponent->SetupAttachment(ItemMesh);
	}
}

void ANAItemTest::PostInitProperties()
{
	Super::PostInitProperties();
}

void ANAItemTest::PostLoad()
{
	Super::PostLoad();
}

void ANAItemTest::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();
}

void ANAItemTest::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
}

void ANAItemTest::PostActorCreated()
{
	Super::PostActorCreated();
}

void ANAItemTest::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

// Called when the game starts or when spawned
void ANAItemTest::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANAItemTest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

