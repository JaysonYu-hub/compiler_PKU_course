# 自定义 IR 规范

## 1. 设计目标

该 IR 是强类型、显式控制流、接近 SSA 的三地址 IR。文本格式参考 Koopa，但内存模型和
实现细节由本项目自行维护。当前 AST lowering 仍主要使用 `alloc/load/store` 表示局部变量；
未来 mem2reg 将把可提升标量改写为 SSA 值和基本块参数。

## 2. 类型

| `IRType` | 文本 | 含义 |
|---|---|---|
| `IR_INT32` | `i32` | 32 位整数/布尔值 |
| `IR_FLOAT` | `float`/`f32` | IEEE-754 单精度 |
| `IR_VOID` | `void` | 无返回值 |
| `IR_POINTER` | `*T` | 目标宽度指针 |
| `IR_ARRAY` | `[T, N]` | 聚合数组类型 |
| `IR_DEFAULT` | 无 | 未推导类型，不应进入后端 |

当前 `IRType` 只保存粗粒度类别，数组维度和部分 pointee 信息分散在 `IRAlloc`、
`IRGlobalAlloc` 与 AST 符号中。长期建议引入结构化 `IRType` 对象。

## 3. Value 分类

- 常量：`IRConstant`。
- 全局对象：`IRGlobalAlloc`。
- 函数参数：`IRFuncParam`。
- 基本块参数：`IRBlockParam`。
- 指令结果：`IRNameInstruction` 或有返回值的 `IRCall`。
- 基本块和函数也继承 `IRValue`，可作为跳转目标和调用目标。

`isa/cast/dyn_cast` 依赖每个类型的 `classof()`。`classof()` 必须与真实 C++ 继承关系一致；
不能仅因为某类指令“也有名字”就把它声明为 `IRNameInstruction`。

## 4. 指令

### 计算与转换

```text
%r = add %a, %b
%cmp = lt %a, %b
%f = sitofp %i
%i = fptosi %f
```

比较结果为 `i32`。浮点二元操作由 `IRBinary` 的 operand type 区分并输出浮点操作形式。

### 内存

```text
%x = alloc i32
store 1, %x
%v = load %x
%p = getelemptr @array, %index
%q = getptr %base, %index
zero_init %array, 40
```

- `getelemptr`：对数组对象/子数组寻址。
- `getptr`：对已是指针的值寻址，典型用途是数组形参。
- `elem_size` 由 IR 生成阶段计算，后端将 index 乘以它。
- 大栈帧下加载 index 可能触发大偏移展开；当前后端约定先加载 index，再加载 base。

### 调用与返回

```text
%r = call @foo(%a, %b)
call @putint(%r)
ret %r
ret
```

`IRCall` 有自己的结果名，但不继承 `IRNameInstruction`。无返回值调用不能作为操作数。

### 终结指令

```text
jump %next
br %cond, %then, %else
ret %value
```

终结指令必须是块内最后一条指令。CFG 应从终结指令查询，避免单独维护的边信息失真。

## 5. 基本块参数

本项目不使用 Phi 指令，而使用 block arguments：

```text
br %cond, %then, %else

%then:
  jump %end(2)

%else:
  jump %end(3)

%end(%a_3: i32):
  ret %a_3
```

内存 API：

```cpp
auto a3 = end->AddParam("%a_3", IRType::IR_INT32);
jump->SetArgs({value});
branch->SetThenArgs({...});
branch->SetElseArgs({...});
```

必须保持以下不变量：

1. 每条入边实参数量等于目标块参数数量。
2. 对应参数类型一致。
3. 块参数只在所属块及其支配区域使用。
4. 删除 load 后使用 `IRFunction::ReplaceAllUses` 替换其用户。

## 6. mem2reg 计划

建议采用 Cytron 风格算法，但将 Phi 放置替换为块参数放置：

1. 只选择入口块中的标量 `alloc`，排除数组、地址逃逸和非直接 load/store 使用。
2. 构建 predecessors、dominators、immediate dominator 和 dominance frontier。
3. 对每个变量收集 store 定义块。
4. 在迭代支配边界中的块添加参数。
5. 沿支配树 DFS，用每个变量的值栈完成重命名。
6. load 替换为栈顶 SSA 值；store 更新栈顶。
7. 在前驱的 jump/branch 对应边追加实参。
8. 删除被提升变量的 alloc/load/store。
9. 校验边参数数量和类型。

循环头块参数自然表达 loop-carried value，无需特殊 Phi 类。

## 7. IR 修改建议

- 新指令必须补齐 enum、类、`classof`、DumpText、uses、类型和后端 dispatch。
- 操作数应放入 `IRInstruction::operands`，这样 `ReplaceUse` 才能生效。
- CFG 目标不计作普通数据 use；边实参必须计作 use。
- Pass 删除指令前应先完成 use replacement。
- 增加 verifier 后，应在 IR 生成后和每个变换 pass 后运行。
