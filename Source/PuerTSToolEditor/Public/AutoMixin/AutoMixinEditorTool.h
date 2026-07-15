#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

class UBlueprint;

class FAutoMixinEditorTool
{
public:
	FAutoMixinEditorTool();

	virtual ~FAutoMixinEditorTool() = default;

	/** 初始化：绑定命令 */
	virtual void Initialize();

	virtual void Uninitialize();

	/** Mixin 按钮执行回调：生成/打开 TS 混入文件并更新 PreMixin.ts */
	static void GenerateTS(const UBlueprint* Blueprint);

	/** 处理模板内容，替换占位符 */
	static FString ProcessTemplate(const FString& TemplateContent, FString BlueprintPath, const FString& FileName, const FString& RootRelativePath);

	static FString FindTemplateForBlueprint(const UBlueprint* Blueprint);

	static void OpenCodeEditorForBpTS(const FString& TsFilePath);

protected:
	/** 绑定命令（命令到执行代理） */
	virtual void BindCommands();
};
