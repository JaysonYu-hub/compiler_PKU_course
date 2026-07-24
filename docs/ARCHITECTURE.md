# 编译器总体架构

## 1. 驱动层

`src/main.cpp` 负责解析命令行、启动 Bison parser、调用 `GenerateIR`，并根据输出模式选择
IR 文本或 RISC-V 后端。目标相关选项集中在 `Back::TargetConfig`：

- `is_64bit`：选择 RV32/RV64。
- `word_size`：SysY 标量字长，目前为 4。
- `pointer_size`：目标指针宽度。
- `stack_align`：ABI 栈对齐要求。
- `optimization_level`：`O0` 或 `O1`。

新增目标选项时应优先放入 `TargetConfig`，避免驱动层和后端各保存一份状态。

## 2. 前端

`sysy.l` 负责词法规则，`sysy.y` 负责语法和 AST 构造。`sysy.lex.cpp`、
`sysy.tab.cpp/.hpp` 是生成文件，不应直接手工修改。

AST 位于 `src/ast`：

- `ast.hpp` 定义节点、符号、作用域、类型和值。
- `ast_visitor.hpp` 定义 visitor 接口。
- `ast.cpp` 实现常量求值等语义行为。
- `codegen.cpp` 将 AST 降低为内存 IR。

`Context` 同时管理当前函数、当前基本块、作用域、名称生成器和字符串到 IR Value 的映射。
这一设计简单，但职责较多；后续可拆成 `IRBuilder`、`SymbolTable` 和 `NameAllocator`。

## 3. IR 层

IR 使用 `shared_ptr` 管理 Value 生命周期。函数拥有基本块，基本块拥有指令，指令拥有操作数。
控制流目标也由终结指令持有。详细格式见 [IR.md](IR.md)。

Pass 位于 `src/IR/passes`：

- `liveness`：块级数据流、指令级活跃集合和线性活跃区间。
- `mem2reg`：预留入口，计划使用基本块参数而不是 Phi 指令。

## 4. 后端

后端按以下顺序工作：

1. 对整个程序运行活跃变量分析。
2. 为每个函数构造线性活跃区间。
3. 线性扫描分配物理寄存器，剩余值 spill。
4. 统计局部对象、spill、保存寄存器、出栈参数和调用参数暂存区。
5. 生成函数序言、指令和函数尾声。

主要组件：

- `riscv_target.hpp`：目标和优化配置。
- `calling_convention.hpp`：整数、浮点与栈参数位置。
- `stack_layout.hpp`：栈帧分区及对齐。
- `linear_scan_allocator.*`：线性扫描寄存器分配。
- `asm_emitter.hpp`：指令输出和大偏移访存展开。
- `riscv_generator.*`：IR 到汇编的 lowering。

### 栈帧概念布局

```text
低地址 / 当前 sp
+--------------------------+
| outgoing stack arguments |
+--------------------------+
| register argument buffer |
+--------------------------+
| spills / saved s* / local|
+--------------------------+
| saved ra + alignment     |
+--------------------------+
高地址 / caller sp
```

参数暂存区必须按函数内最大调用参数数动态分配。固定大小会在超多参数调用中覆盖局部变量或返回地址。

## 5. 所有权与不变量

- 每个基本块最多一个终结指令，并且终结指令位于最后。
- `IRBranch`/`IRJump` 的目标决定真实 CFG；`next_1/next_2` 仅为兼容字段。
- 有结果的指令名称在函数内唯一。
- 块参数由目标块定义，由每条入边提供同数量、同类型实参。
- 数组元素为 4 字节；指针宽度由 target 决定。
- 后端大偏移 helper 可能使用 `t5/t6` 作为 scratch，调用者必须遵守其 clobber 约定。
