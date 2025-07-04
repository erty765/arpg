// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interaction/NAInteractableInterface.h"
#include "NAWeakInteractableHandle.generated.h"

// INAInteractableInterface 객체에 대한 약한 참조를 유지하는 래퍼 구조체. uproperty 대응


USTRUCT(BlueprintType)
struct FWeakInteractableHandle {
	GENERATED_BODY()

	FWeakInteractableHandle() = default;
	FWeakInteractableHandle(std::nullptr_t InPtr) noexcept
	: ObjectPtr(InPtr) {}
	// TScriptInterface<INAInteractableInterface> 생성자
	FWeakInteractableHandle(
		const TScriptInterface<INAInteractableInterface>& InInterface)
	{
		INAInteractableInterface* InRawInterface = InInterface.GetInterface();
		UObject* Obj = InRawInterface ? InRawInterface->_getUObject() : nullptr;
		ObjectPtr = Obj;
		// 런타임에 인터페이스 구현 여부를 검증
		if (ObjectPtr != nullptr)
		{
			check(ObjectPtr.IsValid() && ObjectPtr->Implements<UNAInteractableInterface>());
		}
	}
	// INAInteractableInterface* 생성자 
	FWeakInteractableHandle(INAInteractableInterface* InRawInterface)
	{
		UObject* Obj = InRawInterface ? InRawInterface->_getUObject() : nullptr;
		ObjectPtr = Obj;
		if (ObjectPtr != nullptr)
		{
			check(ObjectPtr.IsValid() && ObjectPtr->Implements<UNAInteractableInterface>());
		}
	}
	FWeakInteractableHandle(AActor* InActor)
	{
		InActor && InActor->Implements<UNAInteractableInterface>()
				? ObjectPtr		= InActor
				: ObjectPtr  = nullptr;
	}

	bool IsValid() const
	{
		return ObjectPtr.IsValid() && ObjectPtr->Implements<UNAInteractableInterface>();
	}
	const TWeakObjectPtr<UObject>& GetWeakObject() const
	{
		return ObjectPtr;
	}
	UObject* GetRawObject() const
	{
		return GetWeakObject().Get();
	}
	
	// 다시 스크립트 인터페이스로 변환
	TScriptInterface<INAInteractableInterface> ToScriptInterface() const
	{
		UObject* Obj = ObjectPtr.Get();
		if (Obj && Obj->Implements<UNAInteractableInterface>())
		{
			return TScriptInterface<INAInteractableInterface>(Obj);
		}
		return nullptr;
	}
	INAInteractableInterface* ToRawInterface() const
	{
		return ToScriptInterface().GetInterface();
	}
	TWeakInterfacePtr<INAInteractableInterface> ToWeakInterface() const
	{
		return TWeakInterfacePtr<INAInteractableInterface>(ToRawInterface());
	}
	
	bool operator==(const FWeakInteractableHandle& Other) const
	{
		return ObjectPtr == Other.ObjectPtr;
	}
	bool operator==(INAInteractableInterface* Other) const
	{
		return ToRawInterface() == Other;
	}
	bool operator!=(const FWeakInteractableHandle& Other) const noexcept
	{
		return !(*this == Other);
	}
	bool operator!=(INAInteractableInterface*& Other) const noexcept
	{
		return !(*this == Other);
	}
	
private:
	// UObject 기반으로만 약한 참조를 보관 (UHT 지원)
	UPROPERTY()
	TWeakObjectPtr<UObject> ObjectPtr;
};

// 해시 함수 오버로드
FORCEINLINE uint32 GetTypeHash(const FWeakInteractableHandle& Key)
{
	return GetTypeHash(Key.GetRawObject());
}