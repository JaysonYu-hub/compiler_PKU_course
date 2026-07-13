with open('my_autotest.py', 'r', encoding='utf-8') as f:
    for i, line in enumerate(f, 1):
        if 'sysy' in line or 'lib' in line or 'clang' in line or 'gcc' in line or '-L' in line:
            print(f"Line {i}: {line.strip()}")
