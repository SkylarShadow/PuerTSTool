#include "AutoMixin/AutoMixinEditorTool.h"
#include "Engine/Blueprint.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/MessageDialog.h"
#include "UObject/Package.h"
#include "Editor.h"
#include "PuerTSToolSettings.h"
#include "Interfaces/IPluginManager.h"
#include "Framework/Notifications/NotificationManager.h"
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



void FAutoMixinEditorTool::GenerateTS(const UBlueprint * Blueprint)
{
	const UPuerTSToolSettings* Settings = GetDefault<UPuerTSToolSettings>();
	
	if (Blueprint)
	{
		// 获取蓝图的路径名称
		const FString BlueprintPath = Blueprint->GetPathName();
		FString Lefts, Rights;
		BlueprintPath.Split(".", &Lefts, &Rights);

		TArray<FString> OutStrings;
		Lefts.ParseIntoArray(OutStrings,TEXT("/"), true);

		// 实际路径
		FString ActualPath;

		if (OutStrings[0] == "Game")ActualPath = Lefts.Mid(5);
		else ActualPath = Lefts;

		// ts文件路径
		FString TsFilePath = FString(TEXT("TypeScript")) + ActualPath;
		TsFilePath = FPaths::Combine(FPaths::ProjectDir(), *(TsFilePath + ".ts"));

		// 如果ts文件不存在，则创建它
		if (!FPaths::FileExists(TsFilePath))
		{
			// 解析蓝图路径以获取文件名
			TArray<FString> StringArray;
			Rights.ParseIntoArray(StringArray,TEXT("/"), false);
			const FString FileName = StringArray[StringArray.Num() - 1];
			
			const FString TemplateFileName = FindTemplateForBlueprint(Blueprint);
			// 读取模板文件
			const FString TemplatePath = FPaths::Combine(
				IPluginManager::Get().FindPlugin("PuerTSTool")->GetBaseDir(), 
				Settings->TemplateDir, TemplateFileName);
			FString TemplateContent;
			if (FFileHelper::LoadFileToString(TemplateContent, *TemplatePath))
			{


				const FString TypeScriptRootPath = FPaths::Combine(FPaths::ProjectDir(), Settings->TypeScriptDir);
				const FString TsFileDir = FPaths::GetPath(TsFilePath);
				
				// 计算从TS文件目录到TypeScript根目录的相对路径
				// 需要在目录路径后添加 "/" 以确保 MakePathRelativeTo 正确工作
				FString RootRelativePath = TypeScriptRootPath / TEXT("");
				const FString TsFileDirWithSlash = TsFileDir / TEXT("");
				
				if (FPaths::MakePathRelativeTo(RootRelativePath, *TsFileDirWithSlash))
				{
					// 移除末尾的斜杠（如果有）
					RootRelativePath.RemoveFromEnd(TEXT("/"));
					// 如果路径为空，说明在同一目录
					if (RootRelativePath.IsEmpty())
					{
						RootRelativePath = TEXT(".");
					}
				}
				else
				{
					// 如果计算失败，使用默认值
					RootRelativePath = TEXT(".");
				}

				UE_LOG(LogTemp, Log, TEXT("计算出的相对路径: %s"), *RootRelativePath);

				// 处理模板并生成ts文件内容
				const FString TsContent = ProcessTemplate(TemplateContent, BlueprintPath, FileName, RootRelativePath);
				// 保存生成的内容到文件
				if (FFileHelper::SaveStringToFile(TsContent, *TsFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
				{
					// 显示通知
					FNotificationInfo Info(FText::Format(LOCTEXT("TsFileGenerated", "TS文件生成成功->路径{0}"), FText::FromString(TsFilePath)));
					Info.ExpireDuration = 12.f;
					
					Info.Hyperlink = FSimpleDelegate::CreateLambda([TsFilePath]()
					{
						OpenCodeEditorForBpTS(TsFilePath);
					});

					// 超链接显示文字
					Info.HyperlinkText = LOCTEXT("Open TS File In Code Editor", "在代码编辑器中打开TypeScript文件");

					
					FSlateNotificationManager::Get().AddNotification(Info);
					
					OpenCodeEditorForBpTS(TsFilePath);
					
					// 生成Puerts类型定义
					if (GEditor)
					{
						GEditor->Exec(nullptr, TEXT("Puerts.Gen"), *GLog);
					}
					
					// 更新premixin文件
					const FString ImportMixinTs = FPaths::Combine(FPaths::ProjectDir(), Settings->TypeScriptDir, Settings->ImportMixinFileName);
					FString ImportMixinTsContent;

					// 读取现有内容
					if (FFileHelper::LoadFileToString(ImportMixinTsContent, *ImportMixinTs))
					{
						// 确保没有重复的导入语句
						const FString ImportStatement = TEXT("import \"./") + ActualPath.Mid(1) + "\";";
						if (!ImportMixinTsContent.Contains(ImportStatement))
						{
							ImportMixinTsContent += ImportStatement + TEXT("\n");
							FFileHelper::SaveStringToFile(ImportMixinTsContent, *ImportMixinTs, FFileHelper::EEncodingOptions::ForceUTF8);
							UE_LOG(LogTemp, Log, TEXT("Premixin.ts更新成功"));

						}
					}
				}
			}
			else
			{
				// 如果模板文件不存在，记录警告
				UE_LOG(LogTemp, Warning, TEXT("MixinTemplate.ts不存在"));
			}
		}
		else
		{
			OpenCodeEditorForBpTS(TsFilePath);
		}
	}
}


// 处理模板
FString FAutoMixinEditorTool::ProcessTemplate(const FString& TemplateContent, FString BlueprintPath, const FString& FileName, const FString& RootRelativePath)
{
	FString Result = TemplateContent;

	// 获取蓝图完整类名（包括_C后缀）
	BlueprintPath += TEXT("_C");
	const FString BlueprintClass = TEXT("UE") + BlueprintPath.Replace(TEXT("/"), TEXT("."));

	const FString ROOT_PATH = TEXT("%ROOT_PATH%"); // 脚本根目录路径
	const FString BLUEPRINT_PATH = TEXT("%BLUEPRINT_PATH%"); // 蓝图路径
	const FString MIXIN_BLUEPRINT_TYPE = TEXT("%MIXIN_BLUEPRINT_TYPE%"); // 混入蓝图类型
	const FString TS_NAME = TEXT("%TS_NAME%"); // TS文件名

	Result = Result.Replace(*ROOT_PATH, *RootRelativePath); // 替换 脚本根目录路径
	Result = Result.Replace(*BLUEPRINT_PATH, *BlueprintPath); // 替换 蓝图路径
	Result = Result.Replace(*MIXIN_BLUEPRINT_TYPE, *BlueprintClass); // 替换 混入蓝图类型
	Result = Result.Replace(*TS_NAME, *FileName); // 替换 TS文件名

	return Result;
}

FString FAutoMixinEditorTool::FindTemplateForBlueprint(const UBlueprint* Blueprint)
{	
	const UPuerTSToolSettings* Settings = GetDefault<UPuerTSToolSettings>();
	if (!Blueprint) return Settings->DefaultTemplateName; // fallback

	UClass* BPClass = Blueprint->GeneratedClass;
	if (!BPClass) return Settings->DefaultTemplateName;

	for (const FPuerTSTemplateMapping& Mapping : Settings->TemplateMappings)
	{
		UClass* BaseClass = Mapping.BaseClass.LoadSynchronous();
		if (!BaseClass) continue;

		if (BPClass->IsChildOf(BaseClass))
		{
			return Mapping.TemplateName;
		}
	}

	// fallback 默认模板
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
		// 自定义编辑器（只传路径）
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
