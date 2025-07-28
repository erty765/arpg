// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

template<typename BridgeType>
class TNAEdBridgeRegistry
{
public:
	void Register(BridgeType* Service)
	{
		check(Service);
		Bridge = Service;
	}
	
	void Unregister()
	{
		Bridge = nullptr;
	}

	BridgeType* Get()
	{
		checkf(Bridge != nullptr, TEXT("NAEditorBridgeService has not been registered."));
		return Bridge;
	}
	
private:
	BridgeType* Bridge = nullptr;
};

/**
 * DECLARE_NA_EDITOR_BRIDGE_WRAPPER
 *
 * 이 매크로는 특정 에디터 인터페이스를 기반으로 자동 생성되는 Bridge Wrapper 구조체의 기준 정보를 선언합니다.
 * 해당 매크로를 사용하면 파싱 스크립트가 이 선언을 감지하여 아래 작업을 자동 수행합니다:
 *
 * [자동 수행 내용]
 * 1. 인터페이스(InterfaceName)의 virtual 함수들을 static 함수로 래핑한 Bridge Wrapper 구조체(NamespaceName)를 생성합니다.
 *
 * 2. 생성된 구조체는 동일 디렉토리 내 'BridgeWrapper/InterfaceName.wrapper.h' 경로로 저장됩니다.
 *
 * 3. interface 헤더 내부 DECLARE_NA_EDITOR_BRIDGE_WRAPPER 매크로 아래에 #include "BridgeWrapper/InterfaceName.wrapper.h" 코드가 삽입됩니다.
 *
 * 4. interface 스코프 내에 해당 래퍼 구조체에 대한 friend struct 선언이 없을 경우 자동으로 삽입됩니다.
 *
 * @param NamespaceName : 자동 생성될 래퍼 구조체의 이름입니다. 통상적으로 'F' + InterfaceName 형식으로 작성합니다.
 *                        예) INAEdItemBridge → FNAEdItemBridge
 * @param InterfaceName : 파싱의 대상이 되는 순수 가상 인터페이스 클래스 이름입니다. 
 *                        반드시 'class InterfaceName' 선언부가 해당 헤더에 존재해야 합니다.
 */
#define DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN(NamespaceName, InterfaceName) \
	extern TNAEdBridgeRegistry<InterfaceName> G##NamespaceName##RegistryImpl_Inst; \
	//struct ARPGEDITOR_API NamespaceName##Registry { static InterfaceName* Get(); static void Register(InterfaceName*); static void Unregister(); }; 

#define DECLARE_NA_EDITOR_BRIDGE_WRAPPER(NamespaceName, InterfaceName) \
	TNAEdBridgeRegistry<InterfaceName> G##NamespaceName##RegistryImpl_Inst; \
	//InterfaceName* NamespaceName##Registry::Get() { return GBridgeRegistry_##NamespaceName.Get(); } \
	//void NamespaceName##Registry::Register(InterfaceName* Ptr) { GBridgeRegistry_##NamespaceName.Register(Ptr); } \
	//void NamespaceName##Registry::Unregister() { GBridgeRegistry_##NamespaceName.Unregister(); }
	

