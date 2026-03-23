// Copyright Epic Games, Inc. All Rights Reserved.

#include "PuerTSToolEditor.h"

#include "AutoMixin/AutoMixinBPToolBar.h"
#include "AutoMixin/AutoMixinStyle.h"
#include "AutoMixin/AutoMixinCommands.h"
#include "AutoMixin/AutoMixinBPToolBar.h"
#include "AutoMixin/AutoMixinCMToolBar.h"

#define LOCTEXT_NAMESPACE "FPuerTSToolEditorModule"

static const FString TYPE_SCRIPT_DIR = TEXT("TypeScript"); // TypeScript文件夹
static const FString PUERTS_FRAMEWORK_PATH = TEXT("PuerTSTool/Typescript"); // Puerts资源路径



TSharedPtr<FSlateStyleSet> FPuerTSToolEditorModule::StyleSet = nullptr;


void FPuerTSToolEditorModule::StartupModule()
{
	DeployPuerTSFramework();
	FAutoMixinStyle::Initialize();
	FAutoMixinStyle::ReloadTextures();
	FAutoMixinCommands::Register();
	
	AutoMixinBPToolBar = MakeShared<FAutoMixinBPToolBar>();
	AutoMixinBPToolBar->Initialize();
	
	AutoMixinCMToolBar = MakeShared<FAutoMixinCMToolBar>();
	AutoMixinCMToolBar->Initialize();
	
}




void FPuerTSToolEditorModule::DeployPuerTSFramework() const
{
	// 插件内 TypeScript 框架目录
	const FString SourceDir = FPaths::Combine(
		FPaths::ProjectPluginsDir(),
		PUERTS_FRAMEWORK_PATH
	);

	// 项目目录下 TypeScript
	const FString TargetDir = FPaths::Combine(
		FPaths::ProjectDir(),
		TYPE_SCRIPT_DIR
	);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 1️ 确保目标目录存在
	if (!PlatformFile.DirectoryExists(*TargetDir))
	{
		PlatformFile.CreateDirectoryTree(*TargetDir);
	}

	// 2️ 递归复制目录
	const bool bOverwrite = false; // 是否覆盖已有文件 //TODO:暴露给设置

	bool bSuccess = PlatformFile.CopyDirectoryTree(
		*TargetDir,
		*SourceDir,
		bOverwrite
	);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("Deploy PuerTS Framework Success:\n%s -> %s"), *SourceDir, *TargetDir);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Deploy PuerTS Framework Failed:\n%s -> %s"), *SourceDir, *TargetDir);
	}
}



void FPuerTSToolEditorModule::ShutdownModule()
{
	FAutoMixinStyle::Shutdown();
	
	FAutoMixinCommands::Unregister();
	
	AutoMixinBPToolBar->Uninitialize();
	AutoMixinCMToolBar->Uninitialize();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPuerTSToolEditorModule, PuerTSToolEditor)