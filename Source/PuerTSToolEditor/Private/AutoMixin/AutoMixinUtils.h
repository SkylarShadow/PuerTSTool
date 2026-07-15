#pragma once

#include "CoreMinimal.h"

class UBlueprint;

namespace AutoMixinUtils
{
	// 蓝图资源路径换算到 TypeScript 文件和 PreMixin import 所需的信息。
	struct FTsPathInfo
	{
		FString ActualPath;
		FString TsFilePath;
		FString ImportPath;
	};

	// PreMixin.ts 中的 import 分为启用和注释两类，检查和修复时需要分别处理。
	struct FPreMixinImportInfo
	{
		TSet<FString> ActiveImports;
		TSet<FString> CommentedImports;
		TMap<FString, FString> DisplayPaths;
	};

	// 统一 import 路径格式，方便比较 ./A/B、A/B.ts、反斜杠等不同写法。
	FString NormalizeImportPath(FString ImportPath);

	// Windows 下路径大小写不敏感，比较 key 统一转成小写。
	FString MakeImportKey(const FString& ImportPath);

	// 从 import "./xxx"; 或被注释掉的 //import "./xxx"; 中提取 xxx。
	bool TryExtractImportPath(const FString& Line, FString& OutImportPath);

	// PreMixin 中注释掉的 import 不会执行，需要视为未绑定。
	bool IsCommentedImportLine(const FString& TrimmedLine);

	// 复用 GenerateTS 的路径规则：蓝图 /Game/A/B.B 映射到 TypeScript/A/B.ts。
	bool TryBuildTsPathInfo(const UBlueprint* Blueprint, FTsPathInfo& OutInfo);

	// TypeScript 根目录和 PreMixin 文件路径都来自插件设置，避免硬编码 TypeScript。
	FString GetTypeScriptRootPath();
	FString GetPreMixinFilePath();

	// 读取 PreMixin.ts，收集启用 import 和被注释 import，用于状态显示和全量检查。
	bool LoadPreMixinImports(FPreMixinImportInfo& OutInfo);

	// 确保 PreMixin.ts 中有启用状态的 import；缺失时追加，被注释时恢复。
	void EnsurePreMixinImport(const FString& PreMixinFilePath, const FString& ImportPath);

	// 去掉 TS 注释，避免注释里的 @mixin 或 AssetPath 影响检查结果。
	FString StripTsComments(const FString& Content);

	// 读取 TS 文件并返回剥离注释后的代码；全量检查中每个 TS 文件只需要调用一次。
	bool LoadTsCodeWithoutComments(const FString& TsFilePath, FString& OutCode);

	// 只把非注释代码里包含 @mixin 装饰器的 TS 当作自动 mixin 文件。
	bool IsMixinTsCode(const FString& CodeWithoutComments);

	// 从已剥离注释的 TS 内容中解析 const AssetPath = "...";。
	bool TryExtractMixinAssetPathFromCode(const FString& CodeWithoutComments, FString& OutAssetPath);

	// TS 里保存的是蓝图生成类路径，例如 /Game/A/B.B_C；这里反查蓝图资源包是否还存在。
	bool DoesBlueprintPackageExistForMixinAssetPath(const FString& AssetPath, FString& OutBlueprintPackageName);

	// 将 TypeScript 根目录下的绝对 TS 路径转回 PreMixin import 使用的相对路径。
	FString MakeRelativeImportPathForTsFile(const FString& TsFilePath, const FString& TypeScriptRootPath);

	// 同一个问题可能同时从 PreMixin 和 TS 扫描路径发现，这里去重后再弹窗。
	void AddIssue(TArray<FString>& Issues, const FString& Issue);
}
