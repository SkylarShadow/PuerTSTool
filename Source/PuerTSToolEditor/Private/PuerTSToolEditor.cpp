// Copyright Epic Games, Inc. All Rights Reserved.

#include "PuerTSToolEditor.h"

#include "ISettingsModule.h"
#include "PuerTSToolSettings.h"
#include "AutoMixin/AutoMixinBPToolBar.h"
#include "AutoMixin/AutoMixinCMToolBar.h"
#include "PuerTSToolStyle.h"
#include "PuerTSToolCommands.h"
#include "Interfaces/IPluginManager.h"
#include "PuerTSToolLogChannels.h"


#define LOCTEXT_NAMESPACE "FPuerTSToolEditorModule"



TSharedPtr<FSlateStyleSet> FPuerTSToolEditorModule::StyleSet = nullptr;


void FPuerTSToolEditorModule::StartupModule()
{

	FPuerTSToolStyle::Initialize();
	FPuerTSToolStyle::ReloadTextures();
	FPuerTSToolCommands::Register();

	RegisterSettings();
	
	DeployPuerTSFramework();
	
	FCoreDelegates::OnPostEngineInit.AddRaw(this,&FPuerTSToolEditorModule::OnPostEngineInit);
	
	AutoMixinBPToolBar = MakeShared<FAutoMixinBPToolBar>();
	AutoMixinCMToolBar = MakeShared<FAutoMixinCMToolBar>();
}




void FPuerTSToolEditorModule::DeployPuerTSFramework() const
{
	const UPuerTSToolSettings* Settings = GetDefault<UPuerTSToolSettings>();
	
	// 插件内 TypeScript 框架目录
	const FString SourceDir = IPluginManager::Get()
	.FindPlugin("PuerTSTool")
	->GetBaseDir() / Settings->TypeScriptDir;

	// 项目目录下 TypeScript	部署位置
	//原本是 FString TargetDir = FPaths::Combine(FPaths::ProjectDir(),Settings->TypeScriptDir);
	//因为需要进行白名单过滤所以更改
	const FString TargetDir = FPaths::Combine(
		FPaths::ProjectDir()
	);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 1️ 确保目标目录存在
	if (!PlatformFile.DirectoryExists(*TargetDir))
	{
		PlatformFile.CreateDirectoryTree(*TargetDir);
	}

	TArray<FString> IgnoreList = Settings->DoNotOverwritePaths;

	// 遍历源目录
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *SourceDir, TEXT("*.*"), true, false);

	for (const FString& SrcFile : Files)
	{
		// 转换为相对路径
		FString RelativePath = SrcFile;
		FPaths::MakePathRelativeTo(RelativePath, *SourceDir);

		const FString DestFile = FPaths::Combine(TargetDir, RelativePath);
		
		FString NormalizedPath = RelativePath;
		FPaths::NormalizeFilename(NormalizedPath);
		
		// 判断是否在忽略列表
		bool bSkip = false;
		for (FString Ignore : IgnoreList)
		{
			FPaths::NormalizeFilename(Ignore);

			// 目录匹配（以 / 结尾 或 不含 .）
			bool bIsDir = Ignore.EndsWith("/") || !Ignore.Contains(".");

			if (bIsDir)
			{
				// 确保是完整目录匹配
				if (NormalizedPath.StartsWith(Ignore))
				{
					bSkip = true;
					break;
				}
			}
			else
			{
				// 文件名匹配
				FString FileName = FPaths::GetCleanFilename(NormalizedPath);

				if (FileName.Equals(Ignore))
				{
					bSkip = true;
					break;
				}
			}
		}

		if (bSkip)
		{
			continue;
		}

		// 创建目录
		FString DestDir = FPaths::GetPath(DestFile);
		if (!PlatformFile.DirectoryExists(*DestDir))
		{
			PlatformFile.CreateDirectoryTree(*DestDir);
		}

		// 覆盖复制
		PlatformFile.CopyFile(*DestFile, *SrcFile);
	}

	UE_LOG(LogPuerTSToolEditor, Log, TEXT("Deploy PuerTS Framework Done (with ignore rules)"));
}



void FPuerTSToolEditorModule::ShutdownModule()
{
	UnregisterSettings();
	
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	
	AutoMixinBPToolBar->Uninitialize();
	AutoMixinCMToolBar->Uninitialize();
	
	FPuerTSToolStyle::Shutdown();
	FPuerTSToolCommands::Unregister();
}

void FPuerTSToolEditorModule::RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Project", "Plugins", "PuerTSTool",
			LOCTEXT("PuerTSToolSettingsName", "PuerTSTool"),
			LOCTEXT("PuerTSToolSettingsDescription", "Configure PuerTSTool Plugin Settings"),
			GetMutableDefault<UPuerTSToolSettings>()
		);
	}
}

void FPuerTSToolEditorModule::UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "PuerTSTool");
	}
}

void FPuerTSToolEditorModule::OnPostEngineInit()
{
	if (!GEditor)
	{
		return;
	}

	// 引擎初始化完成后再初始化工具栏，此时可以安全加载 Kismet 模块
	if (AutoMixinBPToolBar.IsValid())
	{
		AutoMixinBPToolBar->Initialize();
	}
	
	if (AutoMixinCMToolBar.IsValid())
	{
		AutoMixinCMToolBar->Initialize();
	}

	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPuerTSToolEditorModule, PuerTSToolEditor)