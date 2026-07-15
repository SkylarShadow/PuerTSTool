#include "AutoMixin/AutoMixinEditorTool.h"

#include "AutoMixinUtils.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "PuerTSToolLogChannels.h"
#include "PuerTSToolSettings.h"
#include "Widgets/Notifications/SNotificationList.h"


#define LOCTEXT_NAMESPACE "FPuerTSToolEditorModule"

FAutoMixinEditorTool::FAutoMixinEditorTool()
{
}

void FAutoMixinEditorTool::Initialize()
{
}

void FAutoMixinEditorTool::Uninitialize()
{
}

void FAutoMixinEditorTool::BindCommands()
{
}

void FAutoMixinEditorTool::GenerateTS(const UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return;
	}

	const UPuerTSToolSettings* Settings = GetDefault<UPuerTSToolSettings>();
	const FString BlueprintPath = Blueprint->GetPathName();

	AutoMixinUtils::FTsPathInfo PathInfo;
	if (!AutoMixinUtils::TryBuildTsPathInfo(Blueprint, PathInfo))
	{
		UE_LOG(LogPuerTSToolEditor, Warning, TEXT("Blueprint path parse failed: %s"), *BlueprintPath);
		return;
	}

	const FString PreMixinFilePath = AutoMixinUtils::GetPreMixinFilePath();

	// 如果 TS 文件已经存在，只需要修复/确认 PreMixin import，然后打开文件。
	if (FPaths::FileExists(PathInfo.TsFilePath))
	{
		AutoMixinUtils::EnsurePreMixinImport(PreMixinFilePath, PathInfo.ImportPath);
		OpenCodeEditorForBpTS(PathInfo.TsFilePath);
		return;
	}

	// 解析蓝图对象名作为模板里的 TS_NAME。
	FString PackagePath;
	FString ObjectName;
	BlueprintPath.Split(TEXT("."), &PackagePath, &ObjectName);
	if (ObjectName.IsEmpty())
	{
		ObjectName = Blueprint->GetName();
	}

	TArray<FString> NameParts;
	ObjectName.ParseIntoArray(NameParts, TEXT("/"), false);
	const FString FileName = NameParts.IsEmpty() ? ObjectName : NameParts.Last();

	const FString TemplateFileName = FindTemplateForBlueprint(Blueprint);
	const FString TemplatePath = FPaths::Combine(
		IPluginManager::Get().FindPlugin("PuerTSTool")->GetBaseDir(),
		Settings->TemplateDir,
		TemplateFileName
	);

	FString TemplateContent;
	if (!FFileHelper::LoadFileToString(TemplateContent, *TemplatePath))
	{
		UE_LOG(LogPuerTSToolEditor, Warning, TEXT("MixinTemplate.ts不存在"));
		return;
	}

	const FString TypeScriptRootPath = AutoMixinUtils::GetTypeScriptRootPath();
	const FString TsFileDir = FPaths::GetPath(PathInfo.TsFilePath);

	// 计算从当前 TS 文件目录回到 TypeScript 根目录的相对路径，用于模板里的 ROOT_PATH。
	// MakePathRelativeTo 要求目录路径带尾部斜杠，否则同名目录场景可能算错。
	FString RootRelativePath = TypeScriptRootPath / TEXT("");
	const FString TsFileDirWithSlash = TsFileDir / TEXT("");
	if (FPaths::MakePathRelativeTo(RootRelativePath, *TsFileDirWithSlash))
	{
		RootRelativePath.RemoveFromEnd(TEXT("/"));
		if (RootRelativePath.IsEmpty())
		{
			RootRelativePath = TEXT(".");
		}
	}
	else
	{
		RootRelativePath = TEXT(".");
	}

	UE_LOG(LogPuerTSToolEditor, Log, TEXT("计算出的相对路径: %s"), *RootRelativePath);

	const FString TsContent = ProcessTemplate(TemplateContent, BlueprintPath, FileName, RootRelativePath);

	// 目标目录可能不存在，先递归创建，避免 SaveStringToFile 因目录缺失失败。
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(PathInfo.TsFilePath), true);
	if (!FFileHelper::SaveStringToFile(TsContent, *PathInfo.TsFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		return;
	}

	FNotificationInfo Info(FText::Format(LOCTEXT("TsFileGenerated", "TS文件生成成功->路径{0}"), FText::FromString(PathInfo.TsFilePath)));
	Info.ExpireDuration = 12.f;
	Info.Hyperlink = FSimpleDelegate::CreateLambda([TsFilePath = PathInfo.TsFilePath]()
	{
		OpenCodeEditorForBpTS(TsFilePath);
	});
	Info.HyperlinkText = LOCTEXT("Open TS File In Code Editor", "在代码编辑器中打开TypeScript文件");
	FSlateNotificationManager::Get().AddNotification(Info);

	OpenCodeEditorForBpTS(PathInfo.TsFilePath);

	// 生成 PuerTS 类型定义。
	if (GEditor)
	{
		GEditor->Exec(nullptr, TEXT("Puerts.Gen"), *GLog);
	}

	// 更新 PreMixin，确保新生成的 mixin 会被加载。
	AutoMixinUtils::EnsurePreMixinImport(PreMixinFilePath, PathInfo.ImportPath);
}

// 处理模板内容，替换占位符。
FString FAutoMixinEditorTool::ProcessTemplate(const FString& TemplateContent, FString BlueprintPath, const FString& FileName, const FString& RootRelativePath)
{
	FString Result = TemplateContent;

	// 获取蓝图完整类名（包括 _C 后缀）。
	BlueprintPath += TEXT("_C");
	const FString BlueprintClass = TEXT("UE") + BlueprintPath.Replace(TEXT("/"), TEXT("."));

	const FString ROOT_PATH = TEXT("%ROOT_PATH%"); // 脚本根目录路径。
	const FString BLUEPRINT_PATH = TEXT("%BLUEPRINT_PATH%"); // 蓝图路径。
	const FString MIXIN_BLUEPRINT_TYPE = TEXT("%MIXIN_BLUEPRINT_TYPE%"); // 混入蓝图类型。
	const FString TS_NAME = TEXT("%TS_NAME%"); // TS 文件名。

	Result = Result.Replace(*ROOT_PATH, *RootRelativePath); // 替换脚本根目录路径。
	Result = Result.Replace(*BLUEPRINT_PATH, *BlueprintPath); // 替换蓝图路径。
	Result = Result.Replace(*MIXIN_BLUEPRINT_TYPE, *BlueprintClass); // 替换混入蓝图类型。
	Result = Result.Replace(*TS_NAME, *FileName); // 替换 TS 文件名。

	return Result;
}

FString FAutoMixinEditorTool::FindTemplateForBlueprint(const UBlueprint* Blueprint)
{
	const UPuerTSToolSettings* Settings = GetDefault<UPuerTSToolSettings>();
	if (!Blueprint)
	{
		return Settings->DefaultTemplateName;
	}

	UClass* BPClass = Blueprint->GeneratedClass;
	if (!BPClass)
	{
		return Settings->DefaultTemplateName;
	}

	for (const FPuerTSTemplateMapping& Mapping : Settings->TemplateMappings)
	{
		UClass* BaseClass = Mapping.BaseClass.LoadSynchronous();
		if (!BaseClass)
		{
			continue;
		}

		if (BPClass->IsChildOf(BaseClass))
		{
			return Mapping.TemplateName;
		}
	}

	// fallback 默认模板。
	return Settings->DefaultTemplateName;
}

void FAutoMixinEditorTool::OpenCodeEditorForBpTS(const FString& TsFilePath)
{
	const UPuerTSToolSettings* Settings = GetDefault<UPuerTSToolSettings>();
	if (!Settings->bOpenCodeEditor)
	{
		return;
	}

	if (TsFilePath.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("TsFilePath is empty")));
		return;
	}

	if (!FPaths::FileExists(TsFilePath))
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(
				FText::FromString(TEXT("TS file not found:\n{0}")),
				FText::FromString(TsFilePath)
			)
		);
		return;
	}

	const FString EditorCommand = Settings->GetCodeEditorCommand();
	const EPuertsCodeEditorType EditorType = Settings->PuertsCodeEditorType;

	FString ProcExecutable;
	FString ProcArgs;

	// -----------------------------
	// VSCode / 类 VSCode 编辑器
	// -----------------------------
	if (EditorType == EPuertsCodeEditorType::VSCode)
	{
		const FString CmdArgs = FString::Printf(TEXT("-r -g \"%s:%d\""), *TsFilePath, 1);

		if (EditorCommand.EndsWith(TEXT(".cmd")) || EditorCommand.EndsWith(TEXT(".bat")))
		{
			ProcExecutable = TEXT("cmd.exe");
			ProcArgs = FString::Printf(TEXT("/c %s %s"), *EditorCommand, *CmdArgs);
		}
		else
		{
			ProcExecutable = EditorCommand;
			ProcArgs = CmdArgs;
		}
	}
	else
	{
		// 自定义编辑器（只传路径）。
		const FString CmdArgs = FString::Printf(TEXT("\"%s\""), *TsFilePath);

		if (EditorCommand.EndsWith(TEXT(".cmd")) || EditorCommand.EndsWith(TEXT(".bat")))
		{
			ProcExecutable = TEXT("cmd.exe");
			ProcArgs = FString::Printf(TEXT("/c %s %s"), *EditorCommand, *CmdArgs);
		}
		else
		{
			ProcExecutable = EditorCommand;
			ProcArgs = CmdArgs;
		}
	}

	// -----------------------------
	// 启动进程
	// -----------------------------
	FProcHandle Handle = FPlatformProcess::CreateProc(
		*ProcExecutable,
		*ProcArgs,
		true,
		false,
		false,
		nullptr,
		0,
		nullptr,
		nullptr
	);

	if (!Handle.IsValid())
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::Format(
				FText::FromString(TEXT("Failed to launch editor:\n{0}")),
				FText::FromString(EditorCommand)
			)
		);
	}
}

#undef LOCTEXT_NAMESPACE
