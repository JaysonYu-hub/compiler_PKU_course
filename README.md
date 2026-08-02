# SysY Compiler

一个以 C++17 实现的 SysY 编译器。前端使用 Flex/Bison，内部采用自定义、接近 Koopa
文本语法的 IR，后端直接从内存 IR 生成 RV32/RV64 汇编。

## 编译流水线

```text
SysY source
  -> lexer / parser
  -> AST + symbol scopes
  -> custom IR + CFG
  -> liveness + linear-scan allocation
  -> stack layout + calling convention
  -> RISC-V assembly
```

主要目录：

- `src/ast`：AST、作用域、常量求值和 IR 生成。
- `src/IR`：IR 数据模型、文本输出和分析/变换 pass。
- `src/backend`：目标配置、调用约定、寄存器分配、栈布局和汇编生成。
- `debug/2026`：功能与高阶功能测试。

## 构建与使用

```bash
cmake -B build -S .
cmake --build build --parallel
```

```bash
./build/compiler -koopa input.sy -o output.koopa
./build/compiler -S input.sy -o output.s
./build/compiler -S -O1 input.sy -o output.s
```

Windows 环境下可使用 VM 测试脚本：

```bat
sync_and_test.bat debug/2026/functional/02_var_defn3.sy 64
sync_and_test.bat debug/2026/h_functional 64
```

测试入口既接受目录，也接受单个 `.sy` 文件。

## 文档

- [总体架构](docs/ARCHITECTURE.md)
- [IR 规范与基本块参数](docs/IR.md)
- [开发、调试与测试](docs/DEVELOPMENT.md)
- [代码库审计报告](docs/AUDIT.md)

## 当前状态

- `h_functional` RV64：40/40 通过。
- IR 已支持基本块参数和控制流边实参，可用于实现 block-argument 形式的 mem2reg。
- `mem2reg.hpp` 目前只是入口骨架，尚未实现支配树、支配边界和变量重命名。
