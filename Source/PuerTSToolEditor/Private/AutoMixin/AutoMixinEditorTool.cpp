#include "AutoMixin/AutoMixinEditorTool.h"
#include "AutoMixin/AutoMixinCommands.h"
#include "AutoMixin/AutoMixinStyle.h"
#include "Engine/Blueprint.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/MessageDialog.h"
#include "UObject/Package.h"
#include "Editor.h"
#include "Interfaces/IPluginManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Notifications/SNotificationList.h"


#define LOCTEXT_NAMESPACE "FPuerTSToolEditorModule"

// 常量定义
static const FString TEMPLATE_NAME = TEXT("MixinTemplate.ts"); // 模板文件名
static const FString TYPE_SCRIPT_DIR = TEXT("TypeScript"); // TypeScript文件夹
static const FString PUERTS_FRAMEWORK_PATH = TEXT("PuerTSTool/Resources"); // Puerts资源路径

static const FString AUTO_IMPORT_NAME = TEXT("PreMixin.ts"); // 自动导入文件名

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

			// 读取模板文件
			const FString TemplatePath = FPaths::Combine(FPaths::ProjectPluginsDir(), PUERTS_FRAMEWORK_PATH, TEMPLATE_NAME);
			FString TemplateContent;
			if (FFileHelper::LoadFileToString(TemplateContent, *TemplatePath))
			{


				const FString TypeScriptRootPath = FPaths::Combine(FPaths::ProjectDir(), TYPE_SCRIPT_DIR);
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
					Info.ExpireDuration = 5.f;
					FSlateNotificationManager::Get().AddNotification(Info);

					// 更新AutoImport.ts文件
					const FString AutoImportTsPath = FPaths::Combine(FPaths::ProjectDir(), TYPE_SCRIPT_DIR, AUTO_IMPORT_NAME);
					FString AutoImportTsContent;

					// 读取现有内容
					if (FFileHelper::LoadFileToString(AutoImportTsContent, *AutoImportTsPath))
					{
						// 确保没有重复的导入语句
						const FString ImportStatement = TEXT("import \"./") + ActualPath.Mid(1) + "\";";
						if (!AutoImportTsContent.Contains(ImportStatement))
						{
							AutoImportTsContent += ImportStatement + TEXT("\n");
							FFileHelper::SaveStringToFile(AutoImportTsContent, *AutoImportTsPath, FFileHelper::EEncodingOptions::ForceUTF8);
							UE_LOG(LogTemp, Log, TEXT("AutoImport.ts更新成功"));
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
	}
}


//TODO: 改造这个
// 处理模板
FString FAutoMixinEditorTool::ProcessTemplate(const FString& TemplateContent, FString BlueprintPath, const FString& FileName, const FString& RootRelativePath)
{
	FString Result = TemplateContent;

	// 获取蓝图完整类名（包括_C后缀）
	BlueprintPath += TEXT("_C");
	const FString BlueprintClass = TEXT("UE") + BlueprintPath.Replace(TEXT("/"), TEXT("."));

	const FString ROOT_PATH = TEXT("ROOT_PATH"); // 脚本根目录路径
	const FString BLUEPRINT_PATH = TEXT("BLUEPRINT_PATH"); // 蓝图路径
	const FString MIXIN_BLUEPRINT_TYPE = TEXT("MIXIN_BLUEPRINT_TYPE"); // 混入蓝图类型
	const FString TS_NAME = TEXT("TS_NAME"); // TS文件名

	Result = Result.Replace(*ROOT_PATH, *RootRelativePath); // 替换 脚本根目录路径
	Result = Result.Replace(*BLUEPRINT_PATH, *BlueprintPath); // 替换 蓝图路径
	Result = Result.Replace(*MIXIN_BLUEPRINT_TYPE, *BlueprintClass); // 替换 混入蓝图类型
	Result = Result.Replace(*TS_NAME, *FileName); // 替换 TS文件名

	return Result;
}

#undef LOCTEXT_NAMESPACE