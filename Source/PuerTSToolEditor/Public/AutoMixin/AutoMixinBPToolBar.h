#pragma once

#include "AutoMixinEditorTool.h"

class FAutoMixinBPToolBar : FAutoMixinEditorTool
{
public:
	virtual void Initialize() override;
	virtual void Uninitialize() override;

protected:
	void RegisterButton();
	void GenTsButtonPressed();

	/** 执行完整 Puerts 类型生成命令，对应菜单中的 Puerts.Gen Full。 */
	void ExecutePuertsGenFull() const;

	/** 检查所有 @mixin TS 文件和 PreMixin.ts import 是否互相匹配。 */
	void CheckAllTsMixin() const;

	static UBlueprint* GetActiveBlueprint();
};
