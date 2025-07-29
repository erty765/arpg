import os
import re
import sys

# ─── 미리 TNAEdBridgeRegistry.h를 찾을 경로들 ───
search_paths = []   # __main__ 안에서 채워줄 예정

# === 헬퍼 함수들 ===========================================================================================

# C++ 소스 코드 문자열에서 주석을 제거합니다.
# - /* ... */ 형태의 블록 주석과 // 형태의 한 줄 주석을 모두 제거합니다.
# - 정규식을 활용하여 추출된 인터페이스 본문 및 Registry 본문을 정리하는 데 사용됩니다.
def remove_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"//.*", "", text)
    return text


# 원본 헤더에서 인터페이스 선언부 이전에 등장하는 forward declaration(struct/class X;)들을 추출
def extract_forward_decls(content: str, interface_name: str) -> list:
    lines = content.splitlines()
    forward_decls = []
    interface_pattern = re.compile(rf'class\s+(?:\w+\s+)?{interface_name}\s*[^{{]*{{')
    forward_pattern = re.compile(r'^(?:struct|class)\s+\w+\s*;\s*$')

    for line in lines:
        if interface_pattern.search(line):
            break

        stripped = line.strip()

        if forward_pattern.match(stripped):
            forward_decls.append(stripped)

    return forward_decls


# 인터페이스 클래스 본문에서 public/protected/private 접근 지정자별로 함수 시그니처를 파싱합니다.
# - 함수 정의는 여러 줄에 걸쳐 있을 수 있으므로, 중괄호 및 세미콜론 유무를 기반으로 완성 시그니처를 추적합니다.
# - 함수 정의(바디 포함)는 무시하고 시그니처만 추출되며, 래퍼 생성에 사용됩니다.
# - template 함수나 static 함수도 시그니처가 명확한 경우 포함됩니다.
def extract_interface_body(content: str, interface_name: str) -> str:
    pattern = rf"class\s+(?:(?:/\*.*?\*/\s*|\w+\s+))*{interface_name}\s*[^{{]*{{"
    match = re.search(pattern, content, flags=re.DOTALL)
    if not match:
        return None

    start = match.end()
    brace_count = 1
    end = start

    while brace_count > 0 and end < len(content):
        if content[end] == '{':
            brace_count += 1
        elif content[end] == '}':
            brace_count -= 1
        end += 1

    return content[start:end - 1]


# 주어진 C++ 코드에서 TNAEdBridgeRegistry<T> 템플릿 클래스의 본문을 추출합니다.
# - 중괄호 깊이를 추적하여 '{' ~ '}' 사이의 전체 클래스 정의를 문자열로 반환합니다.
# - 이후 Registry 메서드 파싱 및 래퍼 생성에 활용됩니다.
def extract_template_class_body(content: str, template_name: str) -> str:
    # template<…> class TNAEdBridgeRegistry { … }
    pattern = rf"template\s*<[^>]+>\s*class\s+{template_name}\s*[^{{]*{{"
    match = re.search(pattern, content)
    if not match:
        return None
    start = match.end()
    brace_count = 1
    end = start
    while brace_count > 0 and end < len(content):
        if content[end] == '{':
            brace_count += 1
        elif content[end] == '}':
            brace_count -= 1
        end += 1

    return content[start:end - 1]


def make_multi_template(decl: str):
    # decl 예: "G##NamespaceName##Registry_##InterfaceName##_Inst"
    parts = re.split(r'##(.+?)##', decl)

    # parts == ["G", "NamespaceName", "Registry_", "InterfaceName", "_Inst"]
    token_names = [part for idx, part in enumerate(parts) if idx % 2 == 1]
    template = ''.join(
        part if idx % 2 == 0 else "{}"
        for idx, part in enumerate(parts)
    )

    # template == "G{}Registry_{}_Inst"
    return template, token_names


def load_macro_template(registry_header_path):
    global inst_template
    global inst_token_names
    text = open(registry_header_path, encoding='utf-8').read()

    # 백슬래시로 이어진 라인들을 하나로 합치기
    text = re.sub(r'\\\s*\n', ' ', text)

    # 1) 매크로 바디 전체 캡처 (DOTALL + 다음 define 전까지)
    m = re.search(
        r'#define\s+DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN\(\s*NamespaceName\s*,\s*InterfaceName\s*\)\s+'
        r'(?P<body>.+?)(?=(\n#define|\Z))',
        text,
        flags=re.DOTALL
    )
    if not m:
        raise RuntimeError("매크로 정의를 못 찾음.")
    macro_body = m.group('body').strip()

    # WRAPPER_BODY_START/END 사이만 분리
    block_m = re.search(
        r'/\*\s*WRAPPER_BODY_START\s*\*/\s*(?P<body>.+?)\s*/\*\s*WRAPPER_BODY_END\s*\*/',
        macro_body,
        flags=re.DOTALL
    )
    body_block = block_m.group('body').strip() if block_m else macro_body

    # body_block: "extern TNAEdBridgeRegistry<InterfaceName> G##NamespaceName##Registry_Inst;"
    decl = body_block.rstrip(';').strip()

    # '##' 이 1회 이상 반복되는 구간(즉 G##…##…##… 같은) 모두 잡아내도록 패턴을 확장
    m = re.search(r'\b(?:\w+##)+\w+\b', decl)
    if not m:
        raise RuntimeError("인스턴스 패턴 못 찾음.")
    token = m.group(0)  # "G##NamespaceName##Registry_Inst"

    # 2) 플레이스홀더 템플릿 생성
    inst_template, inst_token_names = make_multi_template(token)
    # 전역 변수로 두거나, 함수 리턴값으로 넘겨두면 됨
    return inst_template, inst_token_names

# Bridge Wrapper 생성 시 사용되는 Registry 인스턴스 호출 템플릿 문자열입니다.
# - 예: 'TNAEdBridgeRegistry<InterfaceName>::Get()' 또는 'G{NamespaceName}Registry_Inst'
# - generate_registry_wrapper() 내부에서 format(*inst_token_names) 방식으로 치환되어 사용됩니다.
inst_template = None

# inst_template의 format 인자로 전달될 토큰 이름 목록입니다.
# - 예: ["NamespaceName", "InterfaceName"]
# - DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN(...) 매크로 인자의 순서와 일치해야 합니다.
inst_token_names = None


# TNAEdBridgeRegistry<T>의 각 메서드 시그니처를 기반으로 static 래퍼 함수를 생성합니다.
# - 전달받은 시그니처 내 'BridgeType'을 실제 인터페이스 이름으로 치환한 뒤, static wrapper 형태로 재구성합니다.
# - Wrapper 함수는 'static ReturnType Name(Args) { return Inst.Name(args); }' 형식으로 출력됩니다.
# - inst_call 인자는 레지스트리 인스턴스를 호출하는 표현식 문자열입니다.
def generate_registry_function_wrapper(signature: str, inst_call: str, interface_name: str) -> str:
    sig = signature.strip().rstrip(';')

    # BridgeType → 실제 인터페이스 이름으로 치환
    sig = sig.replace('BridgeType', interface_name)

    # 리턴 타입 / 이름 / 인자 파싱
    m = re.match(r'^(?P<ret>[\w:\<\>\,\s\*\&]+)\s+(?P<name>\w+)\s*\((?P<args>.*)\)$', sig)
    if not m:
        return None
    ret, name, args = m.group('ret').strip(), m.group('name'), m.group('args').strip()
    arg_list = args

    # 인자명만 뽑아서 호출 인자 구성
    call_args = ''
    if arg_list:
        names = []
        for part in re.split(r'\s*,\s*', arg_list):
            nm = part.split()[-1]
            names.append(nm)
        call_args = ', '.join(names)

    # static 래퍼 함수 문자열 반환
    return f"static {ret} {name}({arg_list}) {{ return {inst_call}.{name}({call_args}); }}"


# TNAEdBridgeRegistry<T>를 기반으로 하는 정적 Wrapper 구조체를 생성합니다.
# - Interface의 public 메서드를 static 형태로 재정의한 래퍼 구조체를 문자열로 반환합니다.
# - 반환된 구조체는 `F{Interface}Registry`의 형태이며, Register / Get / Unregister 등의 메서드를 래핑합니다.
# - 인스턴스 접근 방식은 inst_template / inst_token_names를 활용해 동적으로 결정됩니다.
# - 생성된 struct 이름은 struct_name 리스트에 저장됩니다 (외부 참조용).
def generate_registry_wrapper(namespace_name: str,
                              interface_name: str,
                              sections: dict,
                              struct_name: list,
                              api_macro: str = '') -> str:
    # namespace_name: 매크로의 첫 번째 인자 (예: FNAEdItemBridge)
    # interface_name: 매크로의 두 번째 인자 (예: INAEdItemBridge)
    # api_macro: DLL export 매크로
    global inst_template
    global inst_token_names

    lines = []

    # struct 이름 결정 및 외부 참조용 리스트에 저장
    struct_name.clear()
    struct_name.append(f"{namespace_name}Registry")
    if api_macro:
        lines.append(f"struct {api_macro} {struct_name[0]} final {{")
    else:
        lines.append(f"struct {struct_name[0]} final {{")

    # public 메서드만 래핑
    public_methods = sections.get('public', [])

    # 매크로 파라미터 '순서대로' 실제 값 준비
    # inst_token_names 예: ["NamespaceName", "InterfaceName"]
    mapping = {
        "NamespaceName": namespace_name,
        "InterfaceName": interface_name,
    }
    ph_args = [mapping[token] for token in inst_token_names]
    
    # 전역 레지스트리 인스턴스 이름 미리 추출
    inst_call = inst_template.format(*ph_args)

    if public_methods:
        lines.append("public:")
        for sig in public_methods:
            wrapper = generate_registry_function_wrapper(sig, inst_call, interface_name)
            if wrapper:
                lines.append(f"    {wrapper}")

    lines.append("};\n")

    return "\n".join(lines)


# 인터페이스 클래스 본문에서 public/protected/private 접근 지정자별로 함수 시그니처를 파싱합니다.
# - 함수 정의는 여러 줄에 걸쳐 있을 수 있으므로, 중괄호 및 세미콜론 유무를 기반으로 완성 시그니처를 추적합니다.
# - 함수 정의(바디 포함)는 무시하고 시그니처만 추출되며, 래퍼 생성에 사용됩니다.
# - template 함수나 static 함수도 시그니처가 명확한 경우 포함됩니다.
def parse_methods_by_access(body: str) -> dict:
    sections = {'public': [], 'protected': [], 'private': []}
    current = 'private'
    lines = body.splitlines()
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i].strip()

        # 접근 지정자
        access_match = re.match(r'^(public|protected|private)\s*:\s*$', line)
        if access_match:
            current = access_match.group(1)
            i += 1
            continue

        # 빈 줄, 전처리기 무시
        if not line or line.startswith('#'):
            i += 1
            continue

        sig_lines = [line]
        brace_depth = line.count('{') - line.count('}')
        semicolon_found = ';' in line
        body_detected = '{' in line

        # 여러 줄 함수 누적
        while i + 1 < n and (not semicolon_found and brace_depth <= 0):
            i += 1
            next_line = lines[i].strip()
            sig_lines.append(next_line)
            brace_depth += next_line.count('{') - next_line.count('}')
            semicolon_found = ';' in next_line
            if '{' in next_line:
                body_detected = True

        signature = ' '.join(sig_lines).strip()

        # 함수 바디 제거
        if '{' in signature:
            signature = signature.split('{')[0].strip()
        if not signature.endswith(';'):
            signature += ';'

        # template + static 정의 함수도 시그니처로 강제 등록
        if len(signature) > 10:
            sections[current].append(signature)

        # 함수 정의 바디 스킵
        if body_detected and brace_depth > 0:
            while i < n and brace_depth > 0:
                i += 1
                brace_depth += lines[i].count('{') - lines[i].count('}')

        i += 1

    return sections


# 단일 인터페이스 함수 시그니처를 기반으로 정적 래퍼 함수를 생성합니다.
# - template 및 requires 구문이 포함된 복잡한 함수도 파싱하여 static 함수 형태로 재작성합니다.
# - virtual, static, override 등 함수 지정자는 제거한 후 클린한 래핑 코드를 생성합니다.
# - BridgeRegistry::Get()->Method(...) 형태로 위임 호출하는 static 함수 문자열을 반환합니다.
def generate_function_wrapper(signature: str, interface_name: str, bridgeRegistry_name: str):
    stripped = signature.strip()
    if not stripped or interface_name in stripped or stripped.startswith("~") or "operator" in stripped:
        return None
    if not bridgeRegistry_name:
        print(f"[BridgeWrapper] [Error] bridge registry 이름이 유효하지 않음")
        return None

     # 끝의 세미콜론 제거
    if stripped.endswith(';'):
        stripped = stripped[:-1].strip()

    # 매크로 제거
    stripped = re.sub(r'__declspec\([^)]*\)', '', stripped)
    stripped = re.sub(r'__attribute__\s*\(\([^)]*\)\)', '', stripped)

    # virtual, override, final, friend, inline, constexpr, static 전부 제거
    stripped = re.sub(
        r'\b(?:virtual|override|final|friend|inline|constexpr|static)\b',
        '',
        stripped
    ).strip()

    # template<…> + optional requires절을 수동 추출
    prefix = ""
    if stripped.startswith("template"):
        # 중첩 <> 카운팅으로 template 파라미터 뽑기
        depth = 0
        for idx, ch in enumerate(stripped):
            if ch == '<': depth += 1
            elif ch == '>':
                depth -= 1
                if depth == 0:
                    template_end = idx + 1                   
                    break
        prefix = stripped[:template_end].strip() + " "
        stripped = stripped[template_end:].lstrip()

        # 이어서 requires절이 있으면 'requires …' 전체를 뽑기
        if stripped.startswith("requires"):
            # requires 블록 추출: return-type lookahead에서
            #     - 공백 제외
            #     - optional typename 지원
            req_match = re.match(
                r'^(requires\s+.+?)(?='
                  r'\s*(?:const\s+)?(?:typename\s+)?'            # const/typename 허용
                  r'[\w:\<\>\,\*\&]+'                            # 공백 NO
                  r'\s+[A-Za-z_]\w*'                             # 함수명
                  r'\s*\()', 
                stripped
            )
            if req_match:
                prefix += req_match.group(1).strip() + " "
                stripped = stripped[req_match.end():].lstrip()
    
    template_param_text = ""
    template_params = []
    if prefix.strip().startswith("template"):
        match = re.search(r'template\s*<([^>]+)>', prefix)
        if match:
            template_param_text = match.group(1)
            # 쉼표 단위로 분리 + 기본값 제거 + typename/class 키워드 제거
            raw_params = [p.strip() for p in template_param_text.split(',')]
            for p in raw_params:
                # 기본값 제거
                base = p.split('=')[0].strip()
                # typename/class 제거 후 파라미터 이름만
                param_name = re.sub(r'^(typename|class)\s+', '', base)
                template_params.append(param_name)

    # virtual, inline, constexpr, override, final, friend 키워드 제거
    cleaned = re.sub(r"\b(?:virtual|inline|constexpr|override|final|friend)\b", "", stripped)

    # 남은 static도 다 지움
    cleaned = re.sub(r"\bstatic\b", "", cleaned).strip()

    # 리턴 타입 그룹을 non‑lazy → greedy 로 바꿔서 올바른 리턴 타입을 통째로 잡음
    method_pattern = re.compile(
        r'^(?P<ret>[\w:\<\>\,\s\*\&]+)\s+'
        r'(?P<name>[A-Za-z_]\w*)\s*'
        r'\((?P<args>[^\)]*)\)'
    )
    match = method_pattern.match(cleaned)
    if not match:
        return None

    return_type, name, args = match.group('ret').strip(), match.group('name'), match.group('args')
    arg_names = []
    args_parts = []

    if args.strip():
        for arg in [a.strip() for a in args.split(",")]:
            parts = arg.split()
            if len(parts) >= 2:
                # 인자 이름만 뽑아서 호출 인자로 사용
                arg_names.append(parts[-1].replace("*", "").replace("&", "").strip())
                args_parts.append(arg)

    arg_list = ", ".join(args_parts)
    call_args = ", ".join(arg_names)

    # wrapper 함수는 항상 static. stripped 에 static 이 이미 있었다 해도
    # 위에서 다 지웠으니 중복 없이 한 번만 붙음.
    static_prefix = 'static '

    # 템플릿 함수면, 호출 시에도 <…> 붙여야 함
    is_template_function = prefix.strip().startswith("template")
    template_call_suffix = f"<{', '.join(template_params)}>" if template_params else ""

   # 최종 래퍼 함수 시그니처 + 바디 생성
    return (
        f"{prefix}{static_prefix}{return_type} {name}({arg_list})"
        f" {{ return {bridgeRegistry_name}::Get()->{name}{template_call_suffix}({call_args}); }}")


# 주어진 인터페이스 본문과 메타 정보를 기반으로 최종 래퍼 구조체(.wrapper.h)를 생성합니다.
# - Registry 구조체(FXXXRegistry)와 인터페이스 래퍼 구조체(FXXX)를 모두 포함합니다.
# - 클래스 forward 선언, friend 선언, 접근자별 메서드 래핑을 포함한 전체 구조체 정의 문자열을 반환합니다.
# - 최종적으로 .wrapper.h 파일의 전체 내용을 구성합니다.
def generate_wrapper_struct(namespace_name: str, interface_name: str, body: str, api_macro: str = '', forward_decls: list = None) -> str:
    sections = parse_methods_by_access(body)
    lines = []
    lines.append("#pragma once\n")

    # 인터페이스 forward declaration
    lines.append(f"class {interface_name};\n")

    # Registry 래퍼 먼저 생성
    # 미리 파싱된 registry_sections 재활용, namespace_name, interface_name 전달
    bridgeRegistry_name = [""]
    lines.append(generate_registry_wrapper(
        namespace_name,
        interface_name,
        registry_sections,
        bridgeRegistry_name,
        api_macro
    ))

    # 원본 헤더에서 추출한 forward declarations 추가
    if forward_decls:
        for decl in forward_decls:
            if decl and decl != f"class {interface_name};":
                lines.append(f"{decl}\n")

    # 래퍼 구조체 선언
    if api_macro:
        lines.append(f"struct {api_macro} {namespace_name} final {{")
    else:
        lines.append(f"struct {namespace_name} final {{")

    # friend declarations
    for line in body.splitlines():
        stripped = line.strip()
        if stripped.startswith("friend") and namespace_name not in stripped:
            lines.append(f"\t{stripped}")

    # 메서드 래핑
    for access in ['public', 'protected', 'private']:
        if not sections[access]:
            continue
        lines.append(f"\n{access}:")
        for sig in sections[access]:
            if sig.strip().startswith("friend"):
                continue
            wrapper = generate_function_wrapper(sig, interface_name, bridgeRegistry_name[0])
            if wrapper:
                lines.append(f"\t{wrapper}")
    lines.append("};")

    return "\n".join(lines)


# 인터페이스 헤더에 자동 생성된 wrapper 헤더 include 구문을 삽입합니다.
# - DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN(...) 매크로 하단 또는 #pragma once 바로 아래에 삽입됩니다.
# - 이미 include 되어 있다면 중복 삽입을 방지합니다.
def insert_wrapper_include_after_macro(header_path: str, interface_name: str, macro_name: str):
    include_line = f'#include "{interface_name}.wrapper.h"\n'
    with open(header_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    if any(f'{interface_name}.wrapper.h' in line for line in lines):
        return

    inserted = False
    for idx, line in enumerate(lines):
        if macro_name in line and interface_name in line:
            lines.insert(idx + 1, include_line)
            inserted = True
            break

    if not inserted:
        for idx, line in enumerate(lines):
            if line.strip() == "#pragma once":
                lines.insert(idx + 1, include_line)
                inserted = True
                break

    if not inserted:
        lines.insert(0, include_line)
    with open(header_path, "w", encoding="utf-8") as f:
        f.writelines(lines)

    print(f"[BridgeWrapper] {interface_name}.wrapper.h include 삽입")


# 인터페이스 클래스 내에 friend struct {NamespaceName}; 선언이 없을 경우 자동으로 삽입합니다.
# - 래퍼 구조체가 private/protected 멤버에 접근 가능하도록 보장합니다.
# - 클래스 선언 후 '{' 위치를 기준으로 friend 선언을 삽입합니다.
def insert_friend_struct_if_missing(header_path: str, namespace: str, interface_name: str):
    friend_declaration = f"    friend struct {namespace};\n"
    with open(header_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    if any(f"friend struct {namespace}" in line for line in lines):
        return

    class_decl_idx = -1
    brace_open_idx = -1
    class_pattern = re.compile(rf'class\s+[\w\s]*\b{interface_name}\b')

    for i, line in enumerate(lines):
        if class_pattern.search(line):
            class_decl_idx = i
            break

    if class_decl_idx == -1:
        print(f"[BridgeWrapper] 클래스 {interface_name} 선언을 찾을 수 없음")
        return

    for j in range(class_decl_idx, len(lines)):
        if '{' in lines[j]:
            brace_open_idx = j
            break

    if brace_open_idx == -1:
        print(f"[BridgeWrapper] 클래스 {interface_name}의 '{{' 스코프를 찾을 수 없음")
        return

    insert_line = brace_open_idx + 1

    lines.insert(insert_line, friend_declaration)
    with open(header_path, "w", encoding="utf-8") as f:
        f.writelines(lines)

    print(f"[BridgeWrapper] friend struct 선언 추가")


# === 핵심 로직 ===

# TNAEdBridgeRegistry의 public 메서드들을 파싱한 결과를 저장하는 전역 변수입니다.
# - preload_registry() 함수에서 초기화되며, 이후 Registry 래핑에 재사용됩니다.
registry_sections = None

# preload_registry() 내부에서 load_macro_template()의 반환값을 저장하기 위한 임시 변수입니다.
# - inst_template / inst_token_names 쌍을 포함하는 튜플을 저장합니다.
# - 이후 registry_wrapper 생성 시 매크로 토큰을 기반으로 래퍼 이름 구성 시 사용됩니다.
macro_template = None

# TNAEdBridgeRegistry.h 파일을 찾아 내부 메서드들을 파싱하여 registry_sections에 캐시합니다.
# - 이 정보는 인터페이스 wrapper 내에서 Registry를 위임 래핑하는 데 사용됩니다.
# - 스크립트 전체에서 단 한 번만 호출되어야 하며, search_paths를 순회하여 파일을 탐색합니다.
def preload_registry(template_name: str, search_paths: list):
    global registry_sections
    global macro_template

    # search_paths 중에서 TNAEdBridgeRegistry.h 찾기
    for inc in search_paths:
        candidate = os.path.join(inc, f"{template_name}.h")
        if os.path.isfile(candidate):
            with open(candidate, 'r', encoding='utf-8') as f:
                raw = f.read()
            body = extract_template_class_body(raw, template_name)
            if not body:
                raise RuntimeError(f"{template_name} 본문을 파싱하지 못함.")
            clean = remove_comments(body)
            registry_sections = parse_methods_by_access(clean)
            macro_template = load_macro_template(candidate)
            return
    raise FileNotFoundError(f"{template_name}.h 를 search_paths에서 찾을 수 없음: {search_paths}")


# 인터페이스 헤더 파일을 파싱하여 래퍼 구조체를 생성하고 wrapper 헤더를 저장합니다.
# - 인터페이스 본문 추출 → 메서드 파싱 → 래퍼 코드 생성 → .wrapper.h 파일로 출력합니다.
# - insert_wrapper_include_after_macro(), insert_friend_struct_if_missing() 호출로 헤더 수정까지 자동 수행합니다.
def main_internal(interface_name, namespace, header_path, output_root):
    global registry_sections
    if registry_sections is None:
        # 매크로 처리 전에 TNAEdBridgeRegistry 한 번만 미리 로드
        preload_registry('TNAEdBridgeRegistry', search_paths)

    output_dir = output_root
    os.makedirs(output_dir, exist_ok=True)

    try:
        with open(header_path, "r", encoding="utf-8") as f:
            content = f.read()

        interface_body = extract_interface_body(content, interface_name)
        if not interface_body:
            print(f"[BridgeWrapper] {interface_name} 클래스 정의를 찾을 수 없음")
            return

        interface_body = remove_comments(interface_body)
        api_macro_match = re.search(rf'class\s+(\w+)\s+{interface_name}', content)
        api_macro = api_macro_match.group(1) if api_macro_match else ''

        # 원본 헤더에서 forward declarations 추출
        forward_decls = extract_forward_decls(content, interface_name)
        # 래퍼 함수 작성
        wrapper_code = generate_wrapper_struct(namespace, interface_name, interface_body, api_macro, forward_decls)
        output_path = os.path.join(output_dir, f"{interface_name}.wrapper.h")
        with open(output_path, "w", encoding="utf-8") as out_file:
            out_file.write(wrapper_code)
        print(f"[BridgeWrapper] {interface_name}.wrapper.h 생성 → {output_path}")
        
        # 소스 헤더에 #Include "interface_name.wrapper.h" 삽입
        insert_wrapper_include_after_macro(header_path, interface_name, "DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN")

        # 인터페이스 내부에 래퍼 구조체에 대한 friend struct 선언 삽입
        insert_friend_struct_if_missing(header_path, namespace, interface_name)

    except Exception as e:
        print(f"[BridgeWrapper][Error] 파싱 실패: {e}")
        return


# 스크립트 진입점. 모듈 디렉토리 경로를 인자로 받아 전체 헤더 파일을 순회하며 자동 래핑을 수행합니다.
# - DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN(...) 매크로를 포함한 헤더를 탐색하여 래핑 대상을 식별합니다.
# - preload_registry()를 통해 Registry 파싱을 선행하고, 각 대상 인터페이스마다 main_internal() 호출합니다.
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("[BridgeWrapper][Error] module_root 경로를 인자로 전달해야 함.")
        sys.exit(1)

    # module_root 계산
    module_root = os.path.abspath(sys.argv[1])

    # module_name, project_root, output_root 계산
    module_name   = os.path.basename(module_root)
    project_root  = os.path.abspath(os.path.join(module_root, os.pardir, os.pardir))
    output_root   = os.path.join(project_root, "Intermediate", "BridgeWrappers", module_name)
    os.makedirs(output_root, exist_ok=True)

    # search_paths 초기화
    # 1) module_root/Public/NAEditor_BridgeTemplates 에서 우선 찾고
    # 2) 만약 여기 없으면 Public/, Private/ 까지 순차 검색
    search_paths = [
        os.path.join(module_root, 'Public', 'NAEditor_BridgeTemplates'),
        os.path.join(module_root, 'Public'),
        os.path.join(module_root, 'Private'),
    ]

    # 엔진 경로(예: .../Engine/)가 포함되어 있으면 에러
    if "Engine" in module_root.split(os.sep):
        print(f"[BridgeWrapper][Error] 잘못된 module_root 경로: {module_root}")
        sys.exit(1)

    # module_root 디렉토리 유효성 체크
    if not os.path.isdir(module_root):
        print(f"[BridgeWrapper][Error] '{module_root}'가 디렉토리가 아님.")
        sys.exit(1)

    macro_def = re.compile(r'#\s*define\s+DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN')
    macro_pat = re.compile(r'DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)')
    target_exts = [".h"]
    tasks = set()

    # 대상 모듈 내의 헤더 파일을 순회하며 DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN(...) 매크로를 탐색합니다.
    # - 각 매크로 인자(NamespaceName, InterfaceName)를 추출하여 파싱 대상 목록(tasks)에 추가합니다.
    print(f"[BridgeWrapper] 모듈 스캔 시작: {module_root}")
    preload_registry('TNAEdBridgeRegistry', search_paths)
    for dirpath, _, filenames in os.walk(module_root):
        for fn in filenames:
            if not any(fn.endswith(ext) for ext in target_exts):
                continue
            path = os.path.join(dirpath, fn)

            try:
                with open(path, "r", encoding="utf-8", errors="ignore") as f:
                    txt = f.read()
                if macro_def.search(txt):
                    continue
                for line in txt.splitlines():
                    m = macro_pat.search(line)
                    if m:
                        ns, iface = m.groups()
                        tasks.add((iface, ns, path))

            except Exception as e:
                print(f"[BridgeWrapper] 파일 열기 오류: {path}, 에러: {e}")

    # 인터페이스명, 헤더 경로 기준 정렬
    unique = sorted(tasks, key=lambda x: (x[0], x[2]))
    for iface, ns, hdr in unique:
        print(f"[BridgeWrapper] 파싱 대상: {iface} in {hdr}")
        main_internal(iface, ns, hdr, output_root)

    print(f"[BridgeWrapper] 완료: {len(unique)}개 래핑 파일 생성")