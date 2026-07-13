# AST CodeGenerator Visitor 重构记录

## 目标

将 AST 到自定义 IR 的类型分发从 `dynamic_cast` 顺序链改为 Visitor 双分派，并保持已有 IR 生成逻辑、返回值约定和 AST 数据结构语义不变。

这项改动优化的是编译器自身的分发开销和可维护性，不会直接提高生成程序在 RISC-V 机器上的运行速度。比赛的主要性能收益仍应来自中端优化、指令选择、寄存器分配和访存优化。

## 为什么选择 Visitor

当前 AST 已经是 C++ 多态类层次：所有具体节点都继承 `BaseAST` 或 `ValueAST`。Visitor 可以直接利用已有虚函数机制：

- 不需要再维护一份枚举 tag，避免“类类型”和“tag 值”两套类型信息不一致。
- 节点通过 `Accept` 定位自己的静态类型，Visitor 再通过 `Visit` 进入对应实现。
- `ASTVisitor` 为每个具体节点声明纯虚函数；新增节点后，遗漏 Visitor 实现会在编译期暴露。
- 后续可以在同一 AST 上增加语义检查、AST 打印或统计 Visitor。

`tag + switch` 更适合节点数量固定、以 C 风格 tagged union 表示的数据，或自定义 IR 的指令 opcode 分发。对当前 C++ AST 再加 tag 会产生重复状态，因此本次没有采用。

## 重构前后

重构前，`CodeGenerator::Generate(const BaseAST&)` 包含 64 次顺序 `dynamic_cast`：

```cpp
if (const auto* p = dynamic_cast<const CompUnitAST*>(&node)) return Generate(*p);
if (const auto* p = dynamic_cast<const FuncDefAST*>(&node)) return Generate(*p);
// ...
```

最坏情况下，每访问一个节点都要依次尝试多种 RTTI 判断；未知节点还会静默返回空字符串。

重构后，入口固定为：

```cpp
std::string CodeGenerator::Generate(const BaseAST& node) {
  return node.Accept(*this);
}
```

具体节点通过 `VisitableAST<Derived, Base>` 自动实现 `Accept`：

```cpp
template <typename Derived, typename Base = BaseAST>
class VisitableAST : public Base {
public:
  std::string Accept(ASTVisitor& visitor) const final {
    return visitor.Visit(static_cast<const Derived&>(*this));
  }
};
```

调用路径变为：

```text
Generate(BaseAST)
  -> node.Accept(visitor)      // 第一次虚分派：确定节点类型
  -> visitor.Visit(Derived)   // 第二次虚分派：确定操作类型
  -> 原有 IR 生成函数体
```

## 文件改动

- `src/ast/ast_visitor.hpp`
  - 新增 Visitor 接口，覆盖 65 个具体 AST 节点。
- `src/ast/ast.hpp`
  - `BaseAST` 新增纯虚 `Accept`。
  - 新增 `VisitableAST` CRTP 模板，避免在 65 个节点里重复编写相同 `Accept`。
  - 所有具体节点改为继承 `VisitableAST`。
  - `ValueAST` 增加聚合初始化值的结构化访问接口。
- `src/ast/codegen.hpp`
  - `CodeGenerator` 实现 `ASTVisitor`。
  - 原具体节点 `Generate` 重载改为 `Visit` 重载。
  - 删除未定义的旧声明 `ArrayLValueAST` 和 `CompoundInitValueAST`。
- `src/ast/codegen.cpp`
  - 删除主 `dynamic_cast` 分发链。
  - 保留一个统一 `Generate(BaseAST)` 入口，现有递归调用无需改写。
  - 数组初始化展开改用 `ValueAST` 的聚合元素接口。
- `src/ast/ast.cpp`
  - 常量/变量数组初始化值的展开不再使用 RTTI 类型判断。

## 数组初始化值处理

原实现还有 6 处 `dynamic_cast`，用来区分标量初始化值和花括号聚合初始化值。本次为 `ValueAST` 增加：

```cpp
virtual bool isAggregateInitializer() const;
virtual const std::vector<std::unique_ptr<ValueAST>>& getInitializerElements() const;
```

标量节点沿用 `getValue`；`CompoundConstInitValueAST` 和 `InitValListAST` 返回子元素。数组展平和补零算法本身没有改变。

## 新增 AST 节点时的操作

假设新增 `ExampleAST`：

1. 在 `ast.hpp` 中继承 `VisitableAST<ExampleAST>`；若它是表达式，则继承 `VisitableAST<ExampleAST, ValueAST>`。
2. 在 `ast_visitor.hpp` 中增加前向声明和 `Visit(const ExampleAST&)`。
3. 在 `codegen.hpp` 中增加带 `override` 的 `Visit` 声明。
4. 在 `codegen.cpp` 中实现对应 `Visit`。
5. 编译并运行回归用例，比较重构前后的 Koopa/自定义 IR 与 RISC-V 输出。

## 验证记录

- 具体 AST 节点、Visitor 接口、CodeGenerator 声明、CodeGenerator 实现数量均为 65，集合完全一致。
- `src/ast` 中 `dynamic_cast` 数量为 0。
- `git diff --check` 通过。
- 使用 MSVC C++17、UTF-8 模式对 `ast.cpp` 和 `codegen.cpp` 完成语法检查，检查通过；仅有仓库原有的未使用参数、转义字符和整数转换警告。
- Windows 下对整个项目做语法检查时，Flex 生成的 `sysy.lex.cpp` 依赖 POSIX `unistd.h`，因此不能用 MSVC 完成全项目验证。这与 Visitor 改动无关；仍需在 Linux/比赛容器中执行正式 CMake 构建和端到端用例。

Linux 环境建议执行：

```bash
cmake -S . -B build
cmake --build build -j
./build/compiler -koopa -o output.koopa testcase.sy
./build/compiler -S -o output.s testcase.sy
```

## 后续性能验证

不要只统计一次编译时间。应选择包含大量表达式和数组初始化的 SysY 文件，分别在重构前后重复编译至少 30 次，记录中位数和 P95。还要确认输出 IR/汇编与运行结果一致。Visitor 主要消除 AST 主分发的线性 RTTI 链；若词法、语法或 I/O 占主导，总编译时间提升可能有限。
