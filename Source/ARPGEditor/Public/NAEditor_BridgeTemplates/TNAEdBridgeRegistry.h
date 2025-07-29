// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

template<typename BridgeType>
class TNAEdBridgeRegistry final
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
 * DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN
 * 
 * 에디터 인터페이스용 자동 Bridge Wrapper 생성을 위한 선언 매크로.
 * 
 * [주요 기능]
 * 1. GenerateBridgeWrappers.py 스크립트의 파싱 대상을 지정
 * 2. 지정된 인터페이스에 대한 전역 레지스트리 인스턴스의 extern 선언 생성
 * 3. 자동화 스크립트가 이 매크로를 기반으로 Static Wrapper 구조체를 생성
 * 
 * [생성되는 구조체]
 * NamespaceNameRegistry: TNAEdBridgeRegistry<T>의 static wrapper
 *   - static void Register(InterfaceName* Service)
 *   - static InterfaceName* Get()
 *   - static void Unregister()
 * 
 * NamespaceName: 인터페이스 메서드들의 static wrapper
 *   - 모든 메서드를 static 함수로 변환
 *   - 내부적으로 NamespaceNameRegistry::Get()->Method(...) 형태로 delegate
 *   - 원본의 접근 지정자(public/protected/private) 보존
 * 
 * @param NamespaceName 생성될 래퍼 구조체의 이름 (관례: F + InterfaceName)
 *                      예) INAEdItemBridge → FNAEdItemBridge
 * @param InterfaceName 래핑 대상 인터페이스 클래스 이름
 *                      반드시 동일 헤더에 'class InterfaceName' 정의가 존재해야 함
 *                      
 * [주의사항]
 * 반드시 .h 파일에서만 사용
 * .cpp 파일에 이 매크로와 대응하는 DECLARE_NA_EDITOR_BRIDGE_WRAPPER 매크로 필요
 * Engine/ 폴더 내 모듈에서는 사용 불가
 */
#define DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN(NamespaceName, InterfaceName) \
	/* WRAPPER_BODY_START */ \
	extern TNAEdBridgeRegistry<InterfaceName> G##NamespaceName##Registry_Inst; \
	/* WRAPPER_BODY_END */ \

/**
 * DECLARE_NA_EDITOR_BRIDGE_WRAPPER
 * 
 * Bridge Wrapper를 위한 전역 레지스트리 인스턴스 정의 매크로.
 * 
 * [주요 기능]
 * TNAEdBridgeRegistry<InterfaceName> 타입의 전역 인스턴스 생성
 * 해당 인스턴스는 자동 생성된 wrapper 구조체들이 참조하는 실체
 * Runtime에 실제 인터페이스 구현체의 등록/해제/접근을 담당
 * 
 * [생성되는 전역 변수]
 * G{NamespaceName}Registry_Inst: TNAEdBridgeRegistry<InterfaceName> 인스턴스
 *   - 이 인스턴스를 통해 실제 서비스 등록/접근이 이루어짐
 *   - NamespaceNameRegistry::Register() 등의 static 함수들이 내부적으로 참조
 * 
 * [라이프사이클]
 * 1. 모듈 로딩 시: 전역 인스턴스 자동 생성
 * 2. 런타임: Register()를 통한 구현체 등록
 * 3. 사용 중: Get()을 통한 구현체 접근
 * 4. 모듈 언로딩 시: Unregister() 호출 권장
 * 
 * @param NamespaceName DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN과 동일한 이름
 * @param InterfaceName DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN과 동일한 인터페이스 이름
 *  
 * [주의사항]
 * 반드시 .cpp 파일에서만 사용
 * .h 파일의 대응하는 DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN 매크로와 쌍을 이룸
 * 모듈당 하나의 인터페이스에 대해서만 한 번 정의해야 함
 */	
#define DECLARE_NA_EDITOR_BRIDGE_WRAPPER(NamespaceName, InterfaceName) \
	TNAEdBridgeRegistry<InterfaceName> G##NamespaceName##Registry_Inst; \


#pragma region NA EDITOR BRIDGE WRAPPER FRAMEWORK
/**
 * ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════
 * 
 *                                      NA EDITOR BRIDGE WRAPPER FRAMEWORK
 *                                               설계 문서 및 사용 가이드
 * 
 * ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════
 * 
 * [Bridge Wrapping 시스템 개요]
 * 
 * 이 Bridge Wrapper 프레임워크는 언리얼 에디터 전용 인터페이스를 기반으로 자동화된 Static Wrapper를 
 * 생성하는 시스템입니다. 주요 목적은 다음과 같습니다:
 * 
 * 1. Virtual Interface → Static Function Library 자동 변환
 * 2. Runtime 서비스 등록/해제를 통한 의존성 주입 패턴 구현
 * 3. Blueprint Function Library와 동일한 방식의 정적 접근 제공
 * 4. 인터페이스 변경 시 무분기 자동 재생성을 통한 유지보수성 향상
 * 
 * ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 * [자동화 스크립트: GenerateBridgeWrappers.py]
 *
 * 이 스크립트는 다음 단계를 거쳐 래퍼 구조체를 생성합니다:
 *
 * 1. 모듈 스캔 단계
 *	   - 지정된 모듈 디렉토리의 모든 .h 파일에서 DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN(...) 매크로를 검색합니다.
 *     - 이 매크로 인자에서 래퍼 구조체 이름(NamespaceName)과 인터페이스 이름(InterfaceName)을 추출합니다.
 *
 *  2. 인터페이스 파싱 단계
 *	   - forward declaration 추출합니다. (wrapper에서 재사용)
 *	   - InterfaceName 클래스 정의 내부에서 public/protected/private 영역의 메서드를 파싱합니다.
 *
 *  3. 코드 생성 단계
 *	   - Registry Wrapper: TNAEdBridgeRegistry<T>의 public 메서드들을 래핑합니다.
 *     - Interface Wrapper: 인터페이스의 모든 메서드를 static 함수로 변환합니다.
 *     - 각 static 함수는 RegistryWrapper::Get()->Method(...) 방식의 delegate 호출 형태로 래핑됩니다.
 *     - template 함수의 경우 template 파라미터까지 자동 처리됩니다.
 *
 *  4. 파일 통합 단계
 *     - 생성된 .wrapper.h 파일은 Intermediate/BridgeWrappers/ModuleName/... 경로에 자동 저장됩니다.
 *     - 원본 인터페이스 헤더에 #include "InterfaceName.wrapper.h"를 자동 삽입합니다.
 *     - 인터페이스 클래스에 래퍼 구조체에 대한 friend struct 선언을 자동 삽입합니다.
 *     
 * ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 * [사용 매크로]
 *
 * DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN(NamespaceName, InterfaceName)
 *    - .h 파일 전용.
 *    - 파싱 스크립트가 이 매크로를 기반으로 래퍼 생성 여부를 판단합니다.
 *    - extern 키워드를 통해 전역 Bridge Registry 인스턴스가 외부에 존재함을 명시합니다.
 *
 * DECLARE_NA_EDITOR_BRIDGE_WRAPPER(NamespaceName, InterfaceName)
 *    - .cpp 파일 전용.
 *    - GNamespaceNameRegistry_Inst(Bridge Registry) 정적 인스턴스를 생성합니다.
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 * [제약사항 및 주의사항]
 * 
 * 1. 모듈 위치 제한
 *   - Engine/ 폴더 내부 모듈에서는 사용 불가합니다. (스크립트가 자동 거부)
 *   - 프로젝트 레벨 모듈에서만 사용 가능합니다.
 * 
 * 2. 인터페이스 설계 요구사항
 *   - 인터페이스는 순수 가상 클래스여야 합니다.
 *   - 단일 헤더 파일에 완전히 정의되어 있어야 합니다.
 *   - 생성자/소멸자/operator 함수를 제외한 모든 메서드가 래핑됩니다. (virtual/static 무관)
 *   - 멤버 함수의 const 한정자는 래핑 시 제거됩니다. (파라미터/리턴값의 const는 보존)
 * 
 * 3. 매크로 사용 규칙
 *   - DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN은 .h 파일에만
 *   - DECLARE_NA_EDITOR_BRIDGE_WRAPPER는 .cpp 파일에만
 *   - 정확히 1:1 대응으로 사용해야 합니다.
 * 
 * 4. 스크립트 실행 요구사항
 *   - 인터페이스 변경 후 해당 모듈을 재로드 해야 변경 사항이 파싱됩니다. (Build.cs에 자동화 연동됨)
 *   - GenerateBridgeWrappers.py는 모듈 컴파일 시점에 자동 실행됩니다.
 *   - 수동 실행이 필요한 경우: python GenerateBridgeWrappers.py "ModulePath"
 * 
 * ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 * [문제 해결 가이드]
 * 
 * Q: Wrapper 파일이 생성되지 않을 때
 * A: 1) DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN / DECLARE_NA_EDITOR_BRIDGE_WRAPPER 매크로가 올바른 위치에 있는지 확인
 *    2) 스크립트 실행 시 에러 메시지 확인
 * 
 * Q: 컴파일 오류가 발생할 때
 * A: 1) friend struct 선언이 인터페이스에 추가되었는지 확인
 *    2) #include "InterfaceName.wrapper.h"가 헤더에 삽입되었는지 확인
 *    3) forward declaration이 올바르게 처리되었는지 확인
 * 
 * ═════════════════════════════════════════════════════════════════════════════════════════════════════════════════
 */
#pragma endregion
