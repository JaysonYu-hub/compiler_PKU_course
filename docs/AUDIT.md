# 代码库审计报告

## 范围

本次审计覆盖驱动、Flex/Bison 前端、AST/符号系统、IR、活跃性分析、线性扫描、目标配置、
调用约定、栈布局、汇编生成和测试脚本。重点采用静态检查、历史差异、单文件复现和
`h_functional` 全量 RV64 回归。

## 已修复问题

### 1. IR RTTI 分类错误

`IRNameInstruction::classof()` 曾把 `IRCall` 视作其子类，但 C++ 继承关系并非如此，导致
`isa` 成功后发生非法 `static_cast`。现已移除该错误分类。

### 2. CFG 分析存在双重事实来源

活跃性分析曾读取 `next_1/next_2`，而真实控制流由 `jump/br` 决定。现改为通过
`GetSuccessors()` 从终结指令推导。

### 3. 超多参数覆盖栈帧

调用参数暂存区原来固定为 128 字节。40/64 参数调用会覆盖局部值或保存区。现按函数内
最大调用参数数量乘目标寄存器宽度动态分配。

### 4. 混合参数序言可能提前覆盖入参寄存器

参数保存和寄存器初始化原本交错执行。现分成保存寄存器参数、复制栈参数、初始化分配
寄存器三个阶段。

### 5. 大栈帧 GEP 基址被 scratch 覆盖

加载大偏移 index 到 `t6` 时 emitter 会使用 `t5` 作为地址 scratch。原 lowering 已先把
base 放在 `t5`，因此 base 被覆盖。现先加载 index，再加载 base。

### 6. 目标配置分散

优化等级已移入 `TargetConfig`，栈对齐统一读取 `RiscvTarget`。

## 高优先级后续项

1. **IR verifier**：校验终结指令、名称唯一性、操作数类型、块参数/边实参数量与类型。
2. **结构化类型系统**：当前 pointer/array pointee 信息不完整，mem2reg 和 alias 分析会受限。
3. **CFG 字段去重**：迁移完调用方后删除 `next_1/next_2`，只保留终结指令边。
4. **Pass API**：让 pass 接受 `IRFunction&`，返回 changed，并在 pass manager 中统一运行 verifier。
5. **调用约定 ABI 化**：当前调用者与被调用者内部一致，但浮点寄存器耗尽后的分配规则应与目标 ABI 明确对齐。
6. **错误处理**：大量 `assert(false)` 会在 Release 中失效，应逐步替换为诊断对象和非零退出码。

## 中优先级后续项

- `Context` 职责过多，拆出 IRBuilder。
- `liveness.cpp` 两次手写指令 uses switch，建议统一使用 `IRInstruction::GetUses()`。
- 活跃性容器以 `shared_ptr` 为 key 会延长 IR 生命周期，可换成稳定 raw pointer/ID。
- 线性活跃区间只能表达单区间，复杂 CFG 会较保守；可扩展为 multi-range。
- `CMakeLists.txt` 同时 glob `src/*.cpp` 又手工 append `codegen.cpp`，应去掉重复追加。
- 对生成文件与源文件的职责写入构建检查，避免手改生成代码。

## 低优先级与风格项

- 统一中文源码编码；当前部分历史注释存在乱码。
- 将公开字段逐步改为访问器，降低 pass 对容器布局的耦合。
- 使用 `const` 引用和 `string_view` 减少 Dump/查询路径复制。
- 给 `mem2reg`、CFG verifier、calling convention 增加独立单元测试。

## 验证结果

- 修改翻译单元通过 C++17 `-fsyntax-only`。
- `39_fp_params.sy` 单文件通过。
- `debug/2026/h_functional` RV64：40/40 通过。
