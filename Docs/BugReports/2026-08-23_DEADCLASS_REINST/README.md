# 버그 리포트: 맵 로드 시 DEADCLASS / REINST_ 클래스 크래시와 아이템 컴포넌트 계층 평탄화

- 작성일: 2026-08-23
- 브랜치: `Revise_ItemSubsys`
- 엔진: UE 5.4.4 (로컬 소스 빌드, `C:\UE_5.4`, Development Editor)
- 관련 커밋: `d8be22a`, `8625174`, `ddb2006`, `dcd1283`, `9454325` (본문 §7 참고)
- 관련 소스
  - `Source/ARPG/Private/Item/ItemActor/NAItemActor.cpp`
  - `Source/ARPG/Private/Item/EngineSubsystem/NAItemEngineSubsystem.cpp`
  - `Source/ARPGEditor/Private/NAEditor_Item/ItemActorEditor/NAEdItemActorEditorUtils.cpp`
  - `Source/ARPGEditor/Private/NAEditor_Item/EditorSubsystem/NAEdItemEditorSubsystem.cpp`

이 문서는 버그 리포트이면서 동시에 엔진 내부 동작(블루프린트 컴파일·리인스턴싱, 컴포넌트 어태치, PIE 복제, 델타 직렬화)을 나중에 다시 공부할 때 쓸 학습 자료를 겸한다. 로그 발췌와 캡처는 `assets/`에 있다.

---

## 1. 요약

한 줄 요약: **레벨 액터 인스턴스가 `OnConstruction` 안에서 자기 블루프린트를 동기 컴파일했고, 그 컴파일이 같은 콜스택 위에서 리인스턴싱을 일으켜 `this`의 클래스가 `REINST_*`(죽은 클래스)로 바뀌었다.** 그 여파로 컴포넌트 등록이 거부되고(`DEADCLASS`), 메타데이터 조회가 실패하고(`check(MetaData)`), PIE에서 `bAllRegistered` assert로 크래시했다.

조사 과정에서 서로 얽힌 버그 세 개가 드러났다.

| # | 증상 | 근본 원인 | 수정 커밋 |
|---|---|---|---|
| A | 맵 로드 시 `DEADCLASS` 경고, `check(MetaData)` assert, BP 링크 클릭 시 `Type mismatch ... REINST_` ensure, PIE 진입 시 `bAllRegistered` assert | 인스턴스 `OnConstruction` → `ReconstructItemSubobjectsFromMetaData()` 내부의 동기 `CompileBlueprint` 호출 | `d8be22a`, `8625174` |
| B | 메타데이터(MeshType) 변경 후 컴파일하면 `AmmoIndicator`/`MuzzleFlash`가 `ItemMesh` 대신 `StubRoot`(PIE에서는 `ItemCollision`) 아래로 평탄화 | 교체된 구 `ItemMesh`를 `DestroyComponent`할 때 엔진이 그 자식들을 구 컴포넌트의 *부모*로 재어태치 | `ddb2006` |
| C | 위 B로 깨진 상태가 레벨에 저장된 뒤에는 BP 재컴파일로도 복구되지 않고 PIE/재로드마다 재현 | 인스턴스별 `AttachParent` 오버라이드가 직렬화됨 → 리인스턴싱이 그 값을 그대로 복사 | `dcd1283` |

"예전에는 문제 없었다"에 대한 답: 마지막 패키징 성공 커밋(`43cbda0`, 2025-07-07)에는 A의 원인 블록이 없었다. 블록은 2025-07-17(`7e0f6e1`)에 추가됐고 2025-07-28(`dca0f96`)에 현재 형태가 됐으며, 그 뒤로 이 레벨을 열어 검증한 적이 없었다. 엔진 교체(런처 → 소스 빌드)와는 무관하다.

---

## 2. 환경과 타임라인

### 2.1 환경
- Windows 11, UE 5.4.4 소스 빌드(`Compiled: Aug 23 2026 00:20:08`), 프로젝트 `ARPG`
- 프로젝트 경로는 `C:\Users\BHJ\Repos\My_Portfolio\arpg-ReviseItem_Subsys` 심볼릭 링크를 통해 열림 (실제 경로 `01. UE_5.4\arpg-ReviseItem_Subsys`). 로그의 경로가 다르게 보이는 이유.
- 레벨 `Level_NAMainGame`에 아이템 액터 5개 배치: `BP_PlasmaCutter_C_1`, `BP_PlasmaRifle_C_1`, `BP_Small_MedPack_C_1`, `BP_Test2_C_2`, `BP_Test_C_1`

### 2.2 코드 이력 (git)
| 날짜 | 커밋 | 내용 |
|---|---|---|
| 2025-07-07 | `43cbda0` | "패키징 테스트 완료" — 인스턴스 컴파일 블록 없음 |
| 2025-07-17 | `7e0f6e1` | `ReconstructItemSubobjectsFromMetaData()`에 `MarkBlueprintAsStructurallyModified` + `CompileBlueprint` 블록 추가 (CDO 가드 없음) |
| 2025-07-28 | `dca0f96` | `!HasAnyFlags(RF_ClassDefaultObject)` 가드 추가, `CompileBlueprintWithOptionalStructuralMark` 유틸로 분리. 이 파일의 마지막 수정 |
| 2025-07-29 | DevLog 마지막 기록 | |
| 2026-08-23 | 본 리포트 | 약 1년 만에 프로젝트를 열자 즉시 재현 |

### 2.3 DevLog(origin/DevLog)에서 확인한 당시 설계 의도
- 05/29: "블루프린트 클래스가 Mark Dirty 상태가 되면 클래스 자체가 바뀜. `GetClass()` 하면 `REINST_BP_어쩌구_C_숫자`가 나옴" — 이미 REINST 클래스를 인지하고 있었음.
- 07/03: "AddInstanceComponent 생성자에서 제거 — 런타임에서 Hierarchy는 유지되지만 에디터상으로만 깨진 채로 나오는 거라 에디터 로드 시점에서 에셋 전체 리컴파일을 돌림"
- 07/16: "동적으로 구성되는 서브오브젝트(ItemCollision || ItemMesh)에 코드로 어태치된 서브오브젝트가 있는 경우, 에디터 런타임 중 해당 객체의 어태치 부모를 변경하면 코드로 어태치된 서브오브젝트의 어태치먼트가 삭제됨 → 수동으로 어태치 자식을 이관하지 않고, 리플렉션 uproperty에 최신 객체의 주소를 다시 할당한 후 블루프린트 컴파일(델타 직렬화)을 한번 더 하는 방식으로 해결"

즉 07/17 블록은 "에디터 런타임 중 메타데이터 편집" 케이스용이었는데, 조건이 `!BP->IsPossiblyDirty()`(= `Status`가 `BS_Dirty`/`BS_Unknown`이 아님)라서 **로드 직후의 깨끗한 BP에도 참**이었고, 결과적으로 맵 로드 중 모든 `OnConstruction`에서 컴파일이 발화했다.

---

## 3. 증상 (로그 기준)

### 3.1 첫 번째 실행 — `check(MetaData)` assert (`ARPG-backup-2026.08.22-16.24.24.log`, 로그 로테이션으로 삭제됨, 발췌는 당시 기록)
```
[16:23:41:458][0] LogActorComponent: RegisterComponentWithWorld: Owner belongs to a DEADCLASS   (x4)
[16:23:41:466][0] LogWindows: Error: appError called: Assertion failed: MetaData [File:...\NAItemActor.cpp] [Line: 591]
```
콜스택(ARPG/Engine 프레임만):
```
ANAItemActor::ReconstructItemSubobjectsFromMetaData_Impl()
ANAItemActor::ReconstructItemSubobjectsFromMetaData()
ANAItemActor::OnConstruction()                               NAItemActor.cpp:872
AActor::ExecuteConstruction()                                ActorConstruction.cpp:974
AActor::RerunConstructionScripts()                           ActorConstruction.cpp:588
ULevel::IncrementalRunConstructionScripts()                  Level.cpp:1823
ULevel::IncrementalUpdateComponents()                        Level.cpp:1687
UWorld::UpdateWorldComponents()                              World.cpp:2636
UEditorEngine::Map_Load()                                    EditorServer.cpp:2676
```

### 3.2 두 번째 실행 — `check`를 경고로 바꾼 뒤 (`ARPG-backup-2026.08.22-16.33.42.log`, 삭제됨)
맵 로드:
```
[16:32:07:058] NAItem: [CreateItemDataByActor] 아이템 데이터 생성 완료. ID: PlasmaCutter_1, 관련 액터: BP_PlasmaCutter_C_1   (5건, 클래스 조회 성공)
[16:32:07:239] LogActorComponent: RegisterComponentWithWorld: Owner belongs to a DEADCLASS   (x4, 같은 ms)
[16:32:07:239] LogTemp: Warning: [ReconstructItemSubobjectsFromMetaData_Impl] 아이템 메타데이터 없음.   (5건, 클래스 조회 실패)
```
같은 액터가 150ms 사이에 "클래스 조회 성공 → 실패"로 바뀌었다. 클래스 포인터가 바뀐 것.

디테일 패널에서 BP 링크 클릭:
```
Ensure condition failed: NewObject->IsA(this->GeneratedClass)  [Blueprint.cpp] [Line: 890]
Type mismatch: Expected BP_PlasmaCutter_C, Found REINST_BP_PlasmaCutter_C_139
  UBlueprint::SetObjectBeingDebugged()
  FEditorClassUtils::GetSourceLink'::Local::OnEditBlueprintClicked()
```
PIE 진입:
```
LogActor: Error: AActor::IncrementalRegisterComponents parent component
  '/Engine/Transient.World_2:PersistentLevel.BP_PlasmaCutter_C_0.StubRootComponent' cannot be registered in actor '...BP_PlasmaCutter_C_0'
LogWindows: Error: appError called: Assertion failed: bAllRegistered [File:...\Actor.cpp] [Line: 5371]
```
671MB짜리 백업 로그에는 PIE 중 `DEADCLASS`가 2,987,616줄 찍혀 있었다 (PIE 월드 액터들도 `OnConstruction`마다 재컴파일을 반복).

### 3.3 계층 평탄화 (`ARPG-backup-2026.08.22-17.35.53.log`, `assets/log_03_flatten_ensure_callstack.txt`)
```
Ensure condition failed: GIsTransacting  [SceneComponent.cpp] [Line: 1349]
Component '...BP_PlasmaCutter_C_0.ItemMesh(Static)' has '...AmmoIndicatorComponent' in its AttachChildren array,
however, 'AmmoIndicatorComponent' believes it is attached to '...StubRootComponent'
  USceneComponent::OnComponentDestroyed()
  UActorComponent::DestroyComponent()
  ANAItemActor::ReconstructItemSubobjectsFromMetaData()
  ANAItemActor::OnConstruction()
  AActor::ExecuteConstruction()
  FActorReplacementHelper::Finalize()                         KismetReinstanceUtilities.cpp
  FBlueprintCompileReinstancer::ReplaceInstancesOfClass_Inner()
  FBlueprintCompileReinstancer::BatchReplaceInstancesOfClass()
  FBlueprintCompilationManagerImpl::FlushReinstancingQueueImpl()
  FBlueprintCompilationManagerImpl::CompileSynchronouslyImpl()
  FKismetEditorUtilities::CompileBlueprint()
  FBlueprintEditor::Compile()      ← 블루프린트 에디터 "컴파일" 버튼
```

캡처:
- `assets/02_bp_editor_after_stage1.png` — A 수정 후 BP 에디터. `ItemMesh(Static)` 아래 `AmmoIndicator`/`MuzzleFlash`가 정상.
- `assets/03_pie_flat_hierarchy_bug.png` — B/C 증상. PIE에서 `ItemCollision(Box)` 아래에 전부 평탄하게 붙어 있음.

---

## 4. 원인 분석

### 4.1 버그 A — 인스턴스가 자기 BP를 동기 컴파일

문제 코드(삭제 전, `NAItemActor.cpp:546-556`):
```cpp
if (!HasAnyFlags(RF_ClassDefaultObject))
{
    if (UBlueprint* BP = Cast<UBlueprint>(UBlueprint::GetBlueprintFromClass(GetClass())))
    {
        if (!BP->IsPossiblyDirty())   // 로드 직후 BP는 항상 "깨끗" → 항상 참
        {
            FNAEdItemActorEditorUtils::CompileBlueprintWithOptionalStructuralMark(BP, true);
        }
    }
}
```
`CompileBlueprintWithOptionalStructuralMark`는 `FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified` + `FKismetEditorUtilities::CompileBlueprint(SkipSave | SkipGarbageCollection | UseDeltaSerializationDuringReinstancing)`. `SkipReinstancing`이 없으므로 **즉시 리인스턴싱**까지 돈다.

무슨 일이 일어나는가 (엔진 흐름은 §5.1):
1. `Map_Load` → `ULevel::IncrementalRunConstructionScripts` → 액터 A의 `OnConstruction`.
2. 그 안에서 `CompileBlueprint` → `FlushReinstancingQueueImpl` → `BatchReplaceInstancesOfClass`: 새 클래스 `BP_X_C`를 만들고 구 클래스를 `REINST_BP_X_C_N`으로 개명하며 `CLASS_NewerVersionExists` 플래그를 세움. 구 인스턴스 A는 새 액터 A'로 대체된다.
3. 하지만 A는 아직 `OnConstruction` 스택 위에서 살아 있다. `A->GetClass()`는 이제 `REINST_*`.
4. A가 이어서 컴포넌트를 등록하면 `UActorComponent::RegisterComponentWithWorld`가 오너 클래스의 `CLASS_NewerVersionExists`를 보고 거부한다 → `Owner belongs to a DEADCLASS`. 액터당 씬 컴포넌트 4개 → 4줄.
5. A가 `FindItemMetaData(GetClass())`를 하면 맵 키는 새 클래스로 교체돼 있으므로 `REINST_*`로는 못 찾는다 → `check(MetaData)`.
6. A의 복제본이 PIE로 복사되면 PIE 액터도 죽은 클래스 → `IncrementalRegisterComponents`에서 루트부터 등록 실패 → `check(bAllRegistered)`.

왜 `RerunConstructionScripts`의 재진입 가드가 못 막았나: `AActor::RerunConstructionScripts`는 `bActorIsBeingConstructed`와 `CLASS_NewerVersionExists`를 검사하지만(ActorConstruction.cpp:240~300), 이는 *다시 컨스트럭션을 시작할 때*의 가드다. 컨스트럭션 도중에 클래스가 바뀌는 상황은 엔진이 상정하지 않는다 — 컴파일은 UI/에디터 이벤트에서 오는 것이지 액터 코드에서 오지 않기 때문이다.

### 4.2 버그 B — 교체된 컴포넌트 파괴 시 자식이 평탄화

메타데이터 `MeshType`이 바뀌면 `ReconstructItemSubobjectsFromMetaData_Impl`이 구 `ItemMesh(Static)`을 `DestroyComponent`하고 CDO에서 온 새 `ItemMesh(Skeletal)`을 프로퍼티에 재할당한다. 그런데 구 컴포넌트에는 `ANAWeapon` 생성자가 `SetupAttachment(ItemMesh, "Indicator"/"Muzzle")`로 붙여둔 `AmmoIndicatorComponent`/`MuzzleFlashComponent`가 달려 있다.

`USceneComponent::OnComponentDestroyed`(SceneComponent.cpp:1300~1370)의 동작:
- 자식의 `AttachParent == this`이면 → `this->GetAttachParent()`(= `StubRootComponent`)로 `KeepWorldTransform` 재어태치.
- 자식의 `AttachParent != this`(배열 불일치)이면 → `ensureAlwaysMsgf(GIsTransacting, ...)` 후 `AttachChildren.Pop()`.

그래서 자식이 `StubRoot`로 내려간다. 이후 PIE에서 `ReplaceRootWithItemCollisionIfNeeded`가 `StubRoot`의 직계 자식을 전부 `ItemCollision` 아래로 옮기므로 화면에는 "ItemCollision 밑에 평탄하게"로 보인다.

### 4.3 버그 C — 인스턴스 오버라이드가 직렬화되어 복구 불가

B가 한 번 일어난 상태로 레벨을 저장하면 `AmmoIndicatorComponent.AttachParent = StubRootComponent`, `AttachSocketName = None`이 **인스턴스 값으로 직렬화**된다(아키타입 값과 다르므로 델타 직렬화에 포함됨). 이후:
- BP 재컴파일 → 리인스턴싱은 구 인스턴스의 값을 새 인스턴스로 **복사**하므로 그대로 `StubRoot`.
- 에디터 디테일 패널에는 컴파일 직후 잠시 정상처럼 보였지만(UI), 실제 데이터는 평탄. PIE 복제본과 재로드가 진실을 보여줌.
- 새로 배치한 `BP_PlasmaCutter_C_2`는 정상 — 저장된 오버라이드가 없으니까.

Python 커맨드렛으로 저장된 레벨을 덤프한 결과(수정 전):
```
=== ACTOR BP_PlasmaCutter_C_1
   MuzzleFlashComponent     parent=StubRootComponent   socket=None
   AmmoIndicatorComponent   parent=StubRootComponent   socket=None
=== ACTOR BP_PlasmaCutter_C_2
   MuzzleFlashComponent     parent=ItemMesh(Static)    socket=Muzzle
   AmmoIndicatorComponent   parent=ItemMesh(Static)    socket=Indicator
```
두 액터 모두 `CreationMethod=Native`, 아키타입은 같은 `Default__BP_PlasmaCutter_C`의 템플릿. 차이는 오직 직렬화된 인스턴스 값뿐.

---

## 5. 엔진 데이터 흐름 (학습용)

### 5.1 맵 로드 → 컨스트럭션 스크립트
```
UEditorEngine::Map_Load                      EditorServer.cpp
└ UWorld::UpdateWorldComponents               World.cpp
  └ ULevel::IncrementalUpdateComponents       Level.cpp
    ├ (각 액터) AActor::IncrementalRegisterComponents
    │   └ UActorComponent::RegisterComponentWithWorld
    │        └ if (Owner->GetClass()->HasAnyClassFlags(CLASS_NewerVersionExists)) → "DEADCLASS" 로그 후 return
    └ ULevel::IncrementalRunConstructionScripts
      └ AActor::RerunConstructionScripts       ActorConstruction.cpp
        └ AActor::ExecuteConstruction
          └ (UCS 실행) → AActor::OnConstruction   ← 프로젝트 코드 진입점
```
`IncrementalRegisterComponents`는 "모든 컴포넌트가 등록됐는지"를 `bAllRegistered`로 체크한다(Actor.cpp:5371). 루트가 DEADCLASS로 거부되면 여기서 assert.

### 5.2 블루프린트 컴파일과 리인스턴싱
```
FKismetEditorUtilities::CompileBlueprint                      Kismet2.cpp:777
└ FBlueprintCompilationManager::CompileSynchronously         BlueprintCompilationManager.cpp
  └ FBlueprintCompilationManagerImpl::CompileSynchronouslyImpl
    ├ ensure(!bIsRegeneratingOnLoad), ensure(!bSkipReinstancing)   ← 로드 중 컴파일은 다른 경로여야 한다는 엔진의 전제
    ├ FlushCompilationQueueImpl   (새 UBlueprintGeneratedClass 생성, 구 클래스 → REINST_*, CLASS_NewerVersionExists)
    └ FlushReinstancingQueueImpl
      └ FBlueprintCompileReinstancer::BatchReplaceInstancesOfClass
        └ ReplaceInstancesOfClass_Inner
          ├ 구 인스턴스마다 새 클래스로 액터 스폰
          ├ 프로퍼티 복사 (UseDeltaSerializationDuringReinstancing: 구 CDO와 다른 값만)
          ├ 참조 교체 (FArchiveReplaceObjectRef: UPROPERTY TMap 키까지 포함)
          └ FActorReplacementHelper::Finalize → 새 액터 ExecuteConstruction → OnConstruction
```
로드 중 "compile on load"는 별도 경로다: `FBlueprintCompilationManager::NotifyBlueprintLoaded` → `QueueForCompilation(IsRegeneratingOnLoad)` → `EndLoad` 시점의 `FlushCompilationQueue`. 우리는 이 경로가 아니라 `CompileSynchronously`를 로드 스택 위에서 불렀다.

`UBlueprint::IsPossiblyDirty()`는 `Status == BS_Dirty || Status == BS_Unknown`(Blueprint.h:797). 컴파일 직후·로드 직후는 `BS_UpToDate`라 `false`.

`REINST_` 클래스 이름의 숫자(`_139`)는 `MakeUniqueObjectName`의 클래스별 카운터다. 세션 중 생성된 UClass 개수를 반영할 뿐 리인스턴싱 횟수가 아니다.

`UClass::GetAuthoritativeClass()`(Class.h:2986)는 `UBlueprintGeneratedClass`에서 `ClassGeneratedBy->GeneratedClass`를 돌려준다. `REINST_`/`SKEL_` 클래스에서 "진짜" 클래스로 가는 공식 경로.

### 5.3 컴포넌트 파괴와 자식 재어태치
```
UActorComponent::DestroyComponent             ActorComponent.cpp:1556
└ USceneComponent::OnComponentDestroyed       SceneComponent.cpp:1300~1370
  └ for each Child in AttachChildren (역순 Pop):
      if Child->AttachParent == this:
          NewParent = this->GetAttachParent() (파괴 예정이면 위로 거슬러 올라감)
          Child->AttachToComponent(NewParent, KeepWorldTransform)   ← 소켓 정보 유실
      else:
          ensureAlwaysMsgf(GIsTransacting, "has X in AttachChildren, however X believes it is attached to Y")
          AttachChildren.Pop()
```
즉 컴포넌트를 파괴하면 자식은 "조부모"로 올라간다. 자식을 특정 새 부모로 보내고 싶으면 파괴 **전에** 직접 `AttachToComponent`해야 한다.

`USceneComponent::AttachToComponent`는 `Parent == GetAttachParent() && SocketName 동일 && Parent->AttachChildren.Contains(this)`일 때만 early-return. 리인스턴싱 직후처럼 `AttachParent`는 새 부모인데 구 부모의 `AttachChildren`에 남아 있는 "반쯤 끊긴" 상태에서는 재어태치가 실제로 수행되고, 구 부모 배열은 그대로 남아 파괴 시 위 ensure가 뜬다(무해).

### 5.4 PIE 월드 복제와 델타 직렬화
- PIE는 에디터 월드를 `StaticDuplicateObject`로 복제한다(로그: `PIE: Created PIE world by copying editor world`).
- 복제·저장 모두 `SerializeTaggedProperties`가 **아키타입(컴포넌트의 경우 CDO 템플릿)과 같은 값은 생략**한다. 오브젝트 참조 프로퍼티는 "같은 이름의 기본 서브오브젝트"를 동일로 취급하므로, `AttachParent = 인스턴스의 ItemMesh(Static)`은 CDO 템플릿의 `ItemMesh(Static)`과 동일로 판정되어 생략되고, 복제본/로드본은 자기 인스턴스의 `ItemMesh(Static)`을 받는다.
- 반대로 `AttachParent = StubRootComponent`(아키타입과 다름)는 **직렬화된다**. 이것이 버그 C가 "저장 후 고정"되는 메커니즘이다.
- 인스턴스 컴포넌트의 `CreationMethod`(Native/SimpleConstructionScript/UserConstructionScript/Instance)와 `GetArchetype()`는 이런 디버깅에서 가장 먼저 찍어볼 값이다.

### 5.5 프로젝트 측 흐름 (수정 후)
```
에디터 시작
└ UNAItemEngineSubsystem::Initialize           DT 로드, SoftItemMetaData(TSoftClassPtr → RowHandle) 구성
└ UNAEdItemEditorSubsystem::Initialize         BP 클래스 LoadSynchronous → ItemMetaData(UClass* → RowHandle)
    └ BroadcastItemClassRegisteredToMetaData → CDO->HandleItemClassRegisteredToMetaData(DuringInstancing)
        └ CompileBlueprintWithOptionalStructuralMark(BP)      ← CDO 경로의 동기 컴파일 (로드 스택 아님, 유지)
└ OnPostEngineInit → SaveDirtyPackages

맵 로드 / 리인스턴싱 / 배치
└ ANAItemActor::OnConstruction
  └ ReconstructItemSubobjectsFromMetaData
    ├ _Impl: 메타데이터와 현재 컴포넌트 비교 → 교체 필요 시
    │    구 컴포넌트를 StaleItemMesh/StaleItemCollision에 보관, 새 컴포넌트 프로퍼티 재할당,
    │    DestroyStaleItemSubobject(구, 새): 자식을 새 부모로 소켓 유지 이관 후 파괴          (B 수정)
    ├ 프로퍼티에 없는 잔여 씬 컴포넌트 정리 (같은 이관 경로)
    ├ RestoreNativeAttachmentsFromArchetype: 네이티브 컴포넌트의 부모/소켓이 아키타입과 다르면 복구   (C 수정)
    └ 에디터 월드 인스턴스에서 실제 교체가 있었을 때만 RequestDeferredCompile(BP)             (A 대체)
         └ FTSTicker 다음 틱: 로드/비동기로드/리인스턴싱/저장/PIE 중이면 재연기, 아니면 1회 컴파일
PIE
└ PostRegisterAllComponents → ReplaceRootWithItemCollisionIfNeeded (StubRoot → ItemCollision 루트 교체)
```

---

## 6. 재현 절차 (수정 전 코드 기준)

1. `Revise_ItemSubsys`에서 `d8be22a` 이전 커밋(`0a459c2`)을 체크아웃하고 빌드.
2. 에디터 실행 → `Level_NAMainGame` 자동 로드 → 출력 로그에 `Owner belongs to a DEADCLASS` 4줄, `Assertion failed: MetaData` 크래시.
3. (`check`를 경고로 바꾸면) 아웃라이너에서 `BP_PlasmaCutter` 선택 → 디테일의 "BP_PlasmaCutter 편집" 링크 클릭 → `Type mismatch ... REINST_` ensure.
4. PIE → `Assertion failed: bAllRegistered`.

버그 B/C 재현(A 수정 후):
1. `DT_Weapon`의 `PlasmaCutter` 행 `MeshType`을 Static↔Skeletal로 변경 후 저장.
2. `BP_PlasmaCutter` 에디터에서 컴파일 → 출력 로그에 `Ensure condition failed: GIsTransacting ... believes it is attached to StubRootComponent`.
3. PIE → `BP_PlasmaCutter_C_1`의 `AmmoIndicator`/`MuzzleFlash`가 `ItemCollision` 직계 자식으로 보임. 레벨 저장 후 재시작하면 에디터에서도 평탄.

---

## 7. 수정 내역

| 커밋 | 파일 | 내용 |
|---|---|---|
| `d8be22a` | `NAItemActor.cpp`, `NAItemEngineSubsystem.cpp` | 인스턴스 `OnConstruction` 내 동기 컴파일 블록 삭제. `check(MetaData)` → `ensureMsgf` + early return. `FindItemMetaDataImpl` 키를 `GetAuthoritativeClass()`로 정규화 |
| `8625174` | `NAEdItemActorEditorUtils.h/.cpp`, `NAItemActor.h/.cpp`, `NAEdItemEditorSubsystem.cpp` | `RequestDeferredCompile`/`ShutdownDeferredCompiles`(FTSTicker 1회성, 안전 조건 미충족 시 재연기). `_Impl`이 `bool` 반환. 에디터 월드의 비CDO 인스턴스에서 실제 교체가 있었을 때만 요청 |
| `ddb2006` | `NAItemActor.h/.cpp` | `DestroyStaleItemSubobject`: 구 컴포넌트의 `AttachChildren`을 소켓 유지한 채 새 컴포넌트(없으면 구 컴포넌트의 부모)로 이관 후 파괴. `_Impl`은 재할당 후 이관·파괴 |
| `dcd1283` | `NAItemActor.h/.cpp` | `RestoreNativeAttachmentsFromArchetype`: 네이티브 씬 컴포넌트의 부모/소켓이 아키타입과 다르면 같은 이름의 인스턴스 컴포넌트로 재어태치 (저장된 오버라이드 치유) |
| `9454325` | `NAItemActor.h/.cpp` | 임시 `DumpItemSubobjectHierarchy` 로그 제거 |

설계상 원칙으로 남긴 것:
- **액터 인스턴스는 컴파일을 요청만 한다.** 컴파일은 반드시 로드·컨스트럭션·리인스턴싱 스택 밖에서 실행한다.
- **컴포넌트를 파괴하기 전에 자식을 먼저 보낸다.** 엔진은 자식을 조부모로 올릴 뿐이다.
- 네이티브 컴포넌트의 어태치 관계는 C++ 생성자가 진실이며, 인스턴스 오버라이드는 복구 대상이다.

---

## 8. 검증

- 빌드: `Build.bat ARPGEditor Win64 Development` 성공.
- 무인 실행(`-unattended -ExecCmds=QUIT_EDITOR`) 3회: `DEADCLASS` 0, ensure/assert 0, 맵 로드·아이템 데이터 5건·정상 종료.
- Orca(computer-use)로 에디터 직접 조작: 로드 시 `RestoreNativeAttachmentsFromArchetype` 복구 로그 2건(`C_1`), PIE 진입·종료, 창 닫기까지 `DEADCLASS`/ensure 0 (`assets/log_04_verification_after_fix.txt`).
- 캡처
  - `assets/04_editor_C1_after_fix.png` — 에디터 월드 `BP_PlasmaCutter_C_1`: `StubRoot > ItemMesh(Static) > MuzzleFlash, AmmoIndicator`.
  - `assets/05_pie_C1_after_fix.png` — PIE(F8 eject): `ItemCollision(Box) > ItemMesh(Static) > MuzzleFlash, AmmoIndicator`.
- 저장된 레벨 덤프(`assets/dump_hier.py`) — 수정 후 로드 시 `C_1`, `C_2` 모두 `parent=ItemMesh(Static)`, 소켓 `Muzzle`/`Indicator`.

---

## 9. 미해결 / 후속

1. **DT 편집만으로는 CDO 경로 재컴파일이 돌지 않는다.** 18:10 세션 로그에서 `DT_Weapon` 저장 후 `NAEdItem`/컴파일 로그가 전혀 없고, 수동 BP 컴파일이 있어야 메타데이터 변경이 반영됐다. `HandleItemClassRegisteredToMetaData(DuringEditorRuntime)`가 DT 편집 이벤트에 연결돼 있는지 확인 필요.
2. 레벨은 메모리상 복구되지만 dirty 표시가 없어 다음 레벨 저장 전까지 매 로드마다 복구 경고 2줄이 찍힌다. 의도된 동작(로드 중 패키지를 dirty로 만들지 않음).
3. `UseDeltaSerializationDuringReinstancing`의 정확한 비교 기준(구 CDO vs 구 인스턴스)은 엔진 소스(`FBlueprintCompileReinstancer::ReplaceInstancesOfClass_Inner`, `UEngine::CopyPropertiesForUnrelatedObjects`)를 따로 읽어볼 것.

---

## 10. 디버깅에 쓴 도구

- 로그 검색: `grep -n "DEADCLASS\|Ensure condition\|Assertion\|REINST" Saved/Logs/*.log`. 백업 로그는 로테이션으로 사라지므로 결정적 발췌는 바로 복사해 둘 것(이번에 16:24/16:33 로그를 잃었다).
- 저장된 레벨의 컴포넌트 계층 덤프(에디터 UI 없이): `assets/dump_hier.py`
  ```
  UnrealEditor-Cmd.exe ARPG.uproject -run=pythonscript -script=dump_hier.py -unattended -nosplash -stdout -FullStdOutLogOutput
  ```
- 임시 계층 덤프 함수(`DumpItemSubobjectHierarchy`, 커밋 `dcd1283`에 있음, `9454325`에서 제거): 컴포넌트별 `AttachParent`, `AttachSocketName`, `CreationMethod`, `GetArchetype()`, `AttachChildren.Num()`을 찍는다. `Reconstruct` 끝과 `ReplaceRootWithItemCollisionIfNeeded` 전후에 호출했다.
- 에디터 조작 자동화: Orca computer-use. Slate는 접근성 트리가 얕아 좌표 클릭으로 조작했고, 툴팁 창(제목 없음, 78×36)이 메인 창을 가리면 `ShowWindow(hwnd, SW_HIDE)`로 숨긴 뒤 호출해야 했다.
