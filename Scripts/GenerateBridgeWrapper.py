import os
import re
import sys

def remove_comments(text: str) -> str:
    """Strip C++ style // and /* */ comments from text."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"//.*", "", text)
    return text

def extract_interface_body(content, interface_name):
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

def extract_sections_by_access(body: str):
    sections = {'public': [], 'protected': [], 'private': []}
    current_section = 'private'

    lines = body.splitlines()
    buffer = []

    for line in lines:
        stripped = line.strip()
        if re.match(r'^(public|protected|private)\s*:\s*$', stripped):
            if buffer:
                sections[current_section].extend(buffer)
                buffer.clear()
            current_section = stripped.replace(':', '')
        else:
            buffer.append(stripped)

    if buffer:
        sections[current_section].extend(buffer)
    return sections

def generate_function_wrapper(line, interface_name):
    stripped = line.strip()
    if not stripped or interface_name in stripped or stripped.startswith("~") or "operator" in stripped:
        return None

    cleaned = re.sub(r"\b(?:virtual|static|inline|constexpr|override|final)\b", "", stripped)
    cleaned = cleaned.strip()

    method_pattern = re.compile(r'^(?P<ret>[^\(]+?)\s+(?P<name>\w+)\s*\((?P<args>[^\)]*)\)\s*(?:const)?\s*(?:=\s*0)?\s*;')
    match = method_pattern.match(cleaned)
    if not match:
        return None

    return_type, name, args = match.group('ret').strip(), match.group('name'), match.group('args')

    arg_names = []
    args_parts = []

    if args.strip():
        split_args = [a.strip() for a in args.split(",")]
        for arg in split_args:
            parts = arg.split()
            if len(parts) >= 2:
                arg_names.append(parts[-1].replace("*", "").replace("&", "").strip())
                args_parts.append(arg)

    arg_list = ", ".join(args_parts)
    call_args = ", ".join(arg_names)

    return f'static {return_type} {name}({arg_list})' \
           f' {{ return BridgeRegistry::Get()->{name}({call_args}); }}'

def generate_wrapper_struct(namespace, interface_name, body, api_macro=''):
    sections = extract_sections_by_access(body)
    lines = []

    lines.append("#pragma once\n")
    lines.append(f"class {interface_name};\n")
    if api_macro:
        lines.append(f"struct {api_macro} {namespace} final {{")
    else:
        lines.append(f"struct {namespace} final {{")
    lines.append(f"\tusing BridgeRegistry = TNAEdBridgeRegistry<{interface_name}>;\n")

    for line in body.splitlines():
        stripped = line.strip()
        if stripped.startswith("friend") and namespace not in stripped:
            lines.append(f"\t{stripped}")

    for access in ['public', 'protected', 'private']:
        if not sections[access]:
            continue
        lines.append(f"\n{access}:")
        for line in sections[access]:
            if line.strip().startswith("friend"):
                continue
            wrapper = generate_function_wrapper(line, interface_name)
            if wrapper:
                lines.append(f"\t{wrapper}")

    lines.append("};")
    return "\n".join(lines)

def insert_wrapper_include_after_macro(header_path: str, interface_name: str, macro_name: str):
    include_line = f'#include "BridgeWrapper/{interface_name}.wrapper.h"\n'

    with open(header_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    if any(f'BridgeWrapper/{interface_name}.wrapper.h' in line for line in lines):
        # print(f"[BridgeWrapper] 이미 include 되어 있음: {interface_name}.wrapper.h")
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
        # print(f"[BridgeWrapper] friend struct 이미 존재: {namespace}")
        return

    class_decl_idx = -1
    brace_open_idx = -1

    # 정규표현식으로 클래스 선언 검색 (API 매크로 등 허용)
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

def main():
    if len(sys.argv) != 4:
        print("Usage: python GenerateBridgeWrapper.py <InterfaceName> <NamespaceName> <HeaderFilePath>")
        return

    interface_name, namespace, header_path = sys.argv[1:4]

    header_dir = os.path.dirname(os.path.abspath(header_path))
    output_dir = os.path.join(header_dir, "BridgeWrapper")
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

        wrapper_code = generate_wrapper_struct(namespace, interface_name, interface_body, api_macro)

        output_path = os.path.join(output_dir, f"{interface_name}.wrapper.h")
        with open(output_path, "w", encoding="utf-8") as out_file:
            out_file.write(wrapper_code)

        print(f"[BridgeWrapper] {interface_name}.wrapper.h 생성")
    except Exception as e:
        print(f"[BridgeWrapper] 오류 발생: {str(e)}")
        return

    insert_wrapper_include_after_macro(header_path, interface_name, "DECLARE_NA_EDITOR_BRIDGE_WRAPPER")
    insert_friend_struct_if_missing(header_path, namespace, interface_name)

if __name__ == "__main__":
    main()
