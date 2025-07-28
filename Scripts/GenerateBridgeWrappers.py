import os
import re
import sys

# === 헬퍼 함수들 ===

def remove_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"//.*", "", text)
    return text

def extract_forward_decls(content: str, interface_name: str) -> list:
    """
    원본 헤더에서 인터페이스 선언부 이전에 등장하는 forward declaration(struct/class X;)들을 추출합니다.
    """
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

def extract_interface_body(content: str, interface_name: str) -> str:
    pattern = rf"class\s+(?:\w+\s+)?{interface_name}\s*[^{{]*{{"
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

def generate_function_wrapper(signature: str, interface_name: str):
    stripped = signature.strip()
    if not stripped or interface_name in stripped or stripped.startswith("~") or "operator" in stripped:
        return None
     # 끝의 세미콜론 제거
    if stripped.endswith(';'):
        stripped = stripped[:-1].strip()
    # 매크로 제거
    stripped = re.sub(r'__declspec\([^)]*\)', '', stripped)
    stripped = re.sub(r'__attribute__\s*\(\([^)]*\)\)', '', stripped)
    # ← 여기에 virtual, override, final, friend, inline, constexpr, static 전부 제거
    stripped = re.sub(
        r'\b(?:virtual|override|final|friend|inline|constexpr|static)\b',
        '',
        stripped
    ).strip()
    # ──────────────────────────────────────────────────────────────────────────
    # 2) template<…> + optional requires절을 수동 추출
    prefix = ""
    if stripped.startswith("template"):
        # 2-1) 중첩 <> 카운팅으로 template 파라미터 뽑기
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
        # 2-2) 이어서 requires절이 있으면 'requires …' 전체를 뽑기
        if stripped.startswith("requires"):
            # (1) requires 블록 추출: return-type lookahead에서
            #     - 공백 제외
            #     - optional typename 지원
            req_match = re.match(
                r'^(requires\s+.+?)(?='
                  r'\s*(?:const\s+)?(?:typename\s+)?'           # const/typename 허용
                  r'[\w:\<\>\,\*\&]+'                            # 공백 NO
                  r'\s+[A-Za-z_]\w*'                             # 함수명
                  r'\s*\()', 
                stripped
            )
            if req_match:
                prefix += req_match.group(1).strip() + " "
                stripped = stripped[req_match.end():].lstrip()
    # ──────────────────────────────────────────────────────────────────────────
    template_param_text = ""
    template_params = []
    if prefix.strip().startswith("template"):
        match = re.search(r'template\s*<([^>]+)>', prefix)
        if match:
            template_param_text = match.group(1)
            # 쉼표 단위로 분리 + 기본값 제거 + typename/class 키워드 제거
            raw_params = [p.strip() for p in template_param_text.split(',')]
            for p in raw_params:
                base = p.split('=')[0].strip()  # 기본값 제거
                # typename/class 제거 후 파라미터 이름만
                param_name = re.sub(r'^(typename|class)\s+', '', base)
                template_params.append(param_name)
    # virtual, inline, constexpr, override, final, friend 키워드 제거
    cleaned = re.sub(r"\b(?:virtual|inline|constexpr|override|final|friend)\b", "", stripped)
    # 남은 static도 다 지워주고 (우리 wrapper에만 한 번 붙일 거야)
    cleaned = re.sub(r"\bstatic\b", "", cleaned).strip()
    # 리턴 타입 그룹을 non‑lazy → greedy 로 바꿔야 올바른 타입을 통째로 잡아냄
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
        f" {{ return BridgeRegistry::Get()->{name}{template_call_suffix}({call_args}); }}"
    )

def generate_wrapper_struct(namespace: str, interface_name: str, body: str, api_macro: str = '', forward_decls: list = None) -> str:
    sections = parse_methods_by_access(body)
    lines = []
    lines.append("#pragma once\n")
    # 인터페이스 forward declaration
    lines.append(f"class {interface_name};\n")

    # @TODO: BridgeRegistry 래퍼 구조체 파싱하는 함수 추가하기

    # 원본 헤더에서 추출한 forward declarations 추가
    if forward_decls:
        for decl in forward_decls:
            if decl and decl != f"class {interface_name};":
                lines.append(f"{decl}\n")
    # 래퍼 구조체 선언
    if api_macro:
        lines.append(f"struct {api_macro} {namespace} final {{")
    else:
        lines.append(f"struct {namespace} final {{")
    lines.append(f"\tusing BridgeRegistry = TNAEdBridgeRegistry<{interface_name}>;\n")
    # friend declarations
    for line in body.splitlines():
        stripped = line.strip()
        if stripped.startswith("friend") and namespace not in stripped:
            lines.append(f"\t{stripped}")
    # 메서드 래핑
    for access in ['public', 'protected', 'private']:
        if not sections[access]:
            continue
        lines.append(f"\n{access}:")
        for sig in sections[access]:
            if sig.strip().startswith("friend"):
                continue
            wrapper = generate_function_wrapper(sig, interface_name)
            if wrapper:
                lines.append(f"\t{wrapper}")
    lines.append("};")
    return "\n".join(lines)

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

def main_internal(interface_name, namespace, header_path, output_root):
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
        wrapper_code = generate_wrapper_struct(namespace, interface_name, interface_body, api_macro, forward_decls)
        output_path = os.path.join(output_dir, f"{interface_name}.wrapper.h")
        with open(output_path, "w", encoding="utf-8") as out_file:
            out_file.write(wrapper_code)
        print(f"[BridgeWrapper] {interface_name}.wrapper.h 생성 → {output_path}")
        insert_wrapper_include_after_macro(header_path, interface_name, "DECLARE_NA_EDITOR_BRIDGE_WRAPPER")
        insert_friend_struct_if_missing(header_path, namespace, interface_name)
    except Exception as e:
        print(f"[BridgeWrapper][Erro] 파싱 실패패: {e}")
        return

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("[BridgeWrapper][Error] module_root 경로를 인자로 전달해야 함.")
        sys.exit(1)

    # module_root, project_root, module_name, output_root 계산
    module_root = os.path.abspath(sys.argv[1])
    # 엔진 경로(예: .../Engine/)가 포함되어 있으면 에러
    if "Engine" in module_root.split(os.sep):
        print(f"[BridgeWrapper][Error] 잘못된 module_root 경로: {module_root}")
        sys.exit(1)
    if not os.path.isdir(module_root):
        print(f"[BridgeWrapper][Error] '{module_root}'가 디렉토리가 아님.")
        sys.exit(1)

    project_root  = os.path.abspath(os.path.join(module_root, os.pardir, os.pardir))
    module_name   = os.path.basename(module_root)
    output_root   = os.path.join(project_root, "Intermediate", "BridgeWrappers", module_name)
    os.makedirs(output_root, exist_ok=True)

    macro_def = re.compile(r'#\s*define\s+DECLARE_NA_EDITOR_BRIDGE_WRAPPER_EXTERN')
    macro_pat = re.compile(r'DECLARE_NA_EDITOR_BRIDGE_WRAPPER\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)')
    target_exts = [".h"]
    tasks = set()

    print(f"[BridgeWrapper] 모듈 스캔 시작: {module_root}")
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

    unique = sorted(tasks, key=lambda x: (x[0], x[2]))  # 인터페이스명, 헤더 경로 기준 정렬
    for iface, ns, hdr in unique:
        print(f"[BridgeWrapper] 파싱 대상: {iface} in {hdr}")
        main_internal(iface, ns, hdr, output_root)

    print(f"[BridgeWrapper] 완료: {len(unique)}개 래핑 파일 생성")