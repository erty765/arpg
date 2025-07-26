import os
import re
import subprocess

# === 사용자 설정 ===

script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, ".."))  # 프로젝트 루트로 이동
module_root = os.path.join(project_root, "Source", "ARPGEditor")  # 절대 경로: ProjectRoot/Source/ARPGEditor
generate_script_path = os.path.join(script_dir, "GenerateBridgeWrapper.py")  # 절대 경로: Script/GenerateBridgeWrapper.py

macro_definition_pattern = re.compile(r'#\s*define\s+DECLARE_NA_EDITOR_BRIDGE_WRAPPER')
macro_pattern = re.compile(r'DECLARE_NA_EDITOR_BRIDGE_WRAPPER\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)')
target_extensions = [".h"]

tasks = set()

# === 헤더 파일 스캔 ===
# print(f"[BridgeWrapper] ARPGEditor 모듈 스캔 시작")

for dirpath, dirnames, filenames in os.walk(module_root):
    for filename in filenames:
        if not any(filename.endswith(ext) for ext in target_extensions):
            continue

        file_path = os.path.join(dirpath, filename)

        try:
            with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                file_contents = f.read()
                
                # 매크로 정의가 포함된 파일은 제외
                if macro_definition_pattern.search(file_contents):
                    continue

                # 파일 내용 줄 단위 재처리
                for line in file_contents.splitlines():
                    match = macro_pattern.search(line)
                    if match:
                        namespace, interface = match.groups()
                        tasks.add((interface, namespace, file_path))
                        
        except Exception as e:
                print(f"[BridgeWrapper] 파일 열기 오류: {file_path}, 에러: {e}")
            
# 중복 제거를 위해 set 사용 후 list로
unique_tasks = list(tasks)

# === 파싱기 실행 ===
for interface_name, namespace_name, header_path in unique_tasks:
    print(f"[BridgeWrapper] 파싱 대상 발견: {interface_name} in {header_path}")
    subprocess.run([
        "python",
        generate_script_path,
        interface_name,
        namespace_name,
        header_path
    ])

print(f"[BridgeWrapper] 총 {len(unique_tasks)}개의 인터페이스가 자동 래핑 처리됨")
