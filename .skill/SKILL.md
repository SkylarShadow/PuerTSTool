---
name: puerts-coding-helper
description: 协助用户在 Puerts + Unreal Engine 5 项目中定位 TypeScript 声明文件 (.d.ts) 并正确使用声明的 UE API 编写 TypeScript 代码。触发场景包括：(1) 用户询问某个 UE 类型/API 的 TS 声明文件在哪 (2) 用户不知道如何正确导入/使用 UE 容器类型 (TArray/TMap/TSet) (3) 用户需要编写 UE 装饰器 (UCLASS/UFUNCTION/UPROPERTY) (4) 用户遇到 TypeScript 编译错误提示找不到模块或类型 (5) 用户需要使用 Puerts 运行时 API ($ref/blueprint/toDelegate 等) 或 FFI 调用原生库。
---

# Puerts TypeScript 编码助手

## 核心原理

Puerts 通过反射系统自动生成 `.d.ts` 文件，路径和命名规则与 UE5 插件目录结构强关联。

### 文件路径规则

默认生成目录为项目下的 `Typing\`，结构如下：

```
Typing/
├── ffi/
│   └── index.d.ts            -- FFI（外部函数接口，直接调用原生 C DLL）
├── puerts/
│   └── index.d.ts            -- Puerts 核心运行时 API
└── ue/
    ├── index.d.ts            -- UE 类型定义入口（引用聚合）
    ├── puerts.d.ts           -- UE 核心容器/委托/工具类型
    ├── puerts_decorators.d.ts-- UE 装饰器系统
    └── ue_bp.d.ts            -- （生成）项目中自定义 Blueprint 类的 TS 声明
```

## 协作查询流程

### 当用户询问类型声明位置时

1. **确认插件目录**：直接指向 `Typing\` 目录
2. **根据类名推断文件**：
   - UE 内置类型（如 `TArray`、`TMap`）→ `Typing\ue\puerts.d.ts`
   - UE 装饰器（`uclass`、`ufunction`、`uproperty`）→ `Typing\ue\puerts_decorators.d.ts`
   - Puerts 运行时 API（`$ref`、`blueprint`、`toDelegate`）→ `Typing\puerts\index.d.ts`
   - FFI 调用（`binding`、`makeStruct`）→ `Typing\ffi\index.d.ts`
   - 自定义 Blueprint 类（如 `UMyCppClass`）→ `Typing\ue\ue_bp.d.ts`
3. **若文件缺失**：提示用户在 UE 编辑器中运行 Puerts 的 "Puerts.Gen" 命令
4. **搜索技巧**：在 `Typing\` 目录下用 Grep 工具(或者findstr（CMD环境）和 Select-String（PowerShell环境）)搜索 API 名称

### 当用户需要编写 TS 代码时

1. **确定使用场景**：是操作 UE 容器、编写装饰器、调用运行时 API，还是 FFI？
2. **查阅对应声明文件**获取正确类型签名
3. **指导正确的 import 路径**：所有 UE 类型通过 `ue` 模块导入
4. **提供代码示例**（见下方各模块说明）

## 各模块详细指南

详细的类型系统文档、接口说明和代码示例见 [references/declaration-reference.md](references/declaration-reference.md)。按需查阅以下章节：

- **FFI 外部函数接口**：`binding()`、`makeStruct()`、`closure` 等 API 用法
- **Puerts 运行时 API**：`$ref/$unref/$set`、`blueprint.mixin`、`toDelegate`、`on/off/emit` 等
- **UE 容器类型映射**：`TArray<T>`、`TSet<T>`、`TMap<K,V>`、`$Delegate<T>`、`$MulticastDelegate<T>` 的完整接口
- **UE 装饰器系统**：`@uclass`、`@ufunction`、`@uproperty` 装饰器的所有说明符和元数据键
- **类型入口聚合**：`ue/index.d.ts` 如何组织各模块

## 沟通模板

**用户提问**："我想用 TypeScript 调用 FMyStruct，但找不到声明文件。"

**回答结构**：
1. **路径指引**："请检查 `Typing\ue\ue_bp.d.ts` 是否存在"
2. **生成建议**："如果文件缺失，请运行 UE 编辑器中的 'Rebuild DTS' 命令重新生成"
3. **搜索技巧**："也可以在 VS Code 中用 Ctrl+P 搜索 `Typing\` 下的文件"
4. **使用指导**：查阅 `ue` 模块中对应的类型声明和使用方式


## 官方文档
[bug描述](./doc/bugs.md)
[demo](./doc/demo.md)
[引擎(或纯C++)调用脚本](doc/engine_call_script.md)
[faq](doc/faq.md)
[蓝图mixin功能原理、解析、使用方法](doc/mixin.md)
[脚本调用引擎API](doc/script_call_uclass.md)
[基于模板的静态绑定](doc/template_binding.md)

## 常见问题快速索引