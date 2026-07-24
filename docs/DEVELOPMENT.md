# 开发与测试指南

## 构建

项目要求 C++17、CMake、Flex 和 Bison。`CMakeLists.txt` 会递归收集源码并生成 lexer/parser。
生成的 `sysy.lex.cpp`、`sysy.tab.cpp/.hpp` 应通过源 `.l/.y` 更新。

## 命令行

- `-koopa`：输出 IR 文本。
- `-S`：输出 RV64 汇编。
- `-riscv`：当前驱动中选择 RV32。
- `-O0/-O1`：写入 `TargetConfig::optimization_level`。

## 测试策略

修改应按由小到大的顺序验证：

1. `g++ -fsyntax-only` 检查直接修改的翻译单元。
2. `.bat` 跑最小单文件。
3. 跑受影响的测试目录。
4. 后端、ABI、栈布局改动至少回归 `h_functional`。

示例：

```bat
sync_and_test.bat debug/2026/h_functional/39_fp_params.sy 64
sync_and_test.bat debug/2026/h_functional 64
```

`vm_autotest_sy.py` 会生成汇编、链接 ELF、运行并将 stdout 与进程返回码一起和 `.out` 比较。
若 `.in` 被单独替换而 `.out` 没同步，测试会产生看似编译器错误的 WA；排查时应检查时间戳和输入本身。

## 后端调试

- 段错误：使用 `gdb -batch` 获取崩溃标签和调用栈。
- 无限递归：在函数入口断点，比较控制参数在首次与递归调用中的值。
- 大栈帧：检查所有访存偏移是否超过 12 位立即数，以及 helper 会 clobber 哪个 scratch。
- 多参数：同时检查 outgoing stack area、argument staging buffer 和 callee incoming offset。

## 提交前清单

- `git diff --check` 无空白错误。
- 没有直接修改生成 parser/lexer 文件。
- `classof()` 与实际继承关系一致。
- 新 Value 能正确报告 type 和 uses。
- CFG 变换同步更新终结指令边实参。
- 栈帧大小按 target alignment 对齐。
- RV32 代码不含硬编码 8 字节 slot。
