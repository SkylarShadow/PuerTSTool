// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AutoMixin/AutoMixinBPToolBar.h"
#include "AutoMixin/AutoMixinCMToolBar.h"
#include "Modules/ModuleManager.h"

class FPuerTSToolEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:

	// 部署框架文件
	void DeployPuerTSFramework() const;
	
	TSharedPtr<FAutoMixinBPToolBar> AutoMixinBPToolBar;
	TSharedPtr<FAutoMixinCMToolBar> AutoMixinCMToolBar;
	// 样式
	static TSharedPtr<FSlateStyleSet> StyleSet;

	// mixin文件路径
	const FString MixinPath = FPaths::Combine(FPaths::ProjectDir(),TEXT("TypeScript"),TEXT("mixin.ts"));
};
