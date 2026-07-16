#include "AutoMixin/AutoMixinBPToolBar.h"

#include "AutoMixinUtils.h"
#include "BlueprintEditorModule.h"
#include "Editor.h"
#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "PuerTSToolStyle.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FPuerTSToolEditorModule"

namespace
{
	// 当前蓝图对应 TS 的绑定状态，用于一级菜单按钮文案。
	enum class EAutoMixinBindStatus
	{
		NoBlueprint,
		NoTsFile,
		Bound,
		PreMixinError
	};

	// 一级菜单展示需要的状态和路径信息。
	struct FAutoMixinStatusInfo
	{
		EAutoMixinBindStatus Status = EAutoMixinBindStatus::NoBlueprint;
		FString TsFilePath;
		FString ImportPath;
	};

	// 计算当前蓝图的一级菜单状态：无 TS、已绑定、或 PreMixin 缺失/注释异常。
	FAutoMixinStatusInfo GetAutoMixinStatus(const UBlueprint* Blueprint)
	{
		FAutoMixinStatusInfo StatusInfo;
		AutoMixinUtils::FTsPathInfo PathInfo;
		if (!AutoMixinUtils::TryBuildTsPathInfo(Blueprint, PathInfo))
		{
			StatusInfo.Status = EAutoMixinBindStatus::NoBlueprint;
			return StatusInfo;
		}

		StatusInfo.TsFilePath = PathInfo.TsFilePath;
		StatusInfo.ImportPath = PathInfo.ImportPath;
		if (!FPaths::FileExists(PathInfo.TsFilePath))
		{
			StatusInfo.Status = EAutoMixinBindStatus::NoTsFile;
			return StatusInfo;
		}

		AutoMixinUtils::FPreMixinImportInfo PreMixinInfo;
		if (!AutoMixinUtils::LoadPreMixinImports(PreMixinInfo))
		{
			StatusInfo.Status = EAutoMixinBindStatus::PreMixinError;
			return StatusInfo;
		}

		const FString ImportKey = AutoMixinUtils::MakeImportKey(PathInfo.ImportPath);
		StatusInfo.Status = PreMixinInfo.ActiveImports.Contains(ImportKey)
			? EAutoMixinBindStatus::Bound
			: EAutoMixinBindStatus::PreMixinError;
		return StatusInfo;
	}

	// 一级菜单按钮文字，随当前激活蓝图实时刷新。
	FText GetAutoMixinStatusText(const UBlueprint* Blueprint)
	{
		switch (GetAutoMixinStatus(Blueprint).Status)
		{
		case EAutoMixinBindStatus::NoTsFile:
			return LOCTEXT("AutoMixinStatusNoTs", "未绑定TS");
		case EAutoMixinBindStatus::Bound:
			return LOCTEXT("AutoMixinStatusBound", "已绑定TS");
		case EAutoMixinBindStatus::PreMixinError:
			return LOCTEXT("AutoMixinStatusPreMixinError", "PreMixin异常");
		default:
			return LOCTEXT("AutoMixinStatusNoBlueprint", "AutoMixin");
		}
	}

	// 一级菜单 tooltip 显示具体路径，方便直接定位是 TS 文件缺失还是 PreMixin 异常。
	FText GetAutoMixinStatusTooltip(const UBlueprint* Blueprint)
	{
		const FAutoMixinStatusInfo StatusInfo = GetAutoMixinStatus(Blueprint);
		switch (StatusInfo.Status)
		{
		case EAutoMixinBindStatus::NoTsFile:
			return FText::Format(LOCTEXT("AutoMixinNoTsTooltip", "未在TypeScript目录找到对应TS文件:\n{0}"), FText::FromString(StatusInfo.TsFilePath));
		case EAutoMixinBindStatus::Bound:
			return FText::Format(LOCTEXT("AutoMixinBoundTooltip", "TS文件已存在，且PreMixin.ts已启用import:\n{0}"), FText::FromString(StatusInfo.ImportPath));
		case EAutoMixinBindStatus::PreMixinError:
			return FText::Format(LOCTEXT("AutoMixinPreMixinErrorTooltip", "TS文件已存在，但PreMixin.ts中未添加或已注释:\n{0}"), FText::FromString(StatusInfo.ImportPath));
		default:
			return LOCTEXT("AutoMixinNoBlueprintTooltip", "打开蓝图编辑器后使用AutoMixin工具");
		}
	}

	// 一级菜单图标跟随状态变化
	FName GetAutoMixinStatusIconName(const UBlueprint* Blueprint)
	{
		switch (GetAutoMixinStatus(Blueprint).Status)
		{
		case EAutoMixinBindStatus::NoTsFile:
			return TEXT("PuerTSAutoMixinTool.NoTs");
		case EAutoMixinBindStatus::Bound:
			return TEXT("PuerTSAutoMixinTool.Bound");
		case EAutoMixinBindStatus::PreMixinError:
			return TEXT("PuerTSAutoMixinTool.PreMixinError");
		default:
			return TEXT("PuerTSAutoMixinTool.NoBlueprint");
		}
	}

	const FSlateBrush* GetAutoMixinStatusIconBrush(const UBlueprint* Blueprint)
	{
		const ISlateStyle& ToolStyle = FPuerTSToolStyle::Get();
		const FSlateBrush* FallbackBrush = ToolStyle.GetBrush(TEXT("PuerTSAutoMixinTool.PluginAction"));
		return ToolStyle.GetOptionalBrush(GetAutoMixinStatusIconName(Blueprint), nullptr, FallbackBrush);
	}
}

// 存储最后一个标签页
static TWeakPtr<SDockTab> LastForegroundTab = nullptr;

// 标签切换事件句柄
static FDelegateHandle TabForegroundedHandle;

// 获取AssetEditorSubsystem
static UAssetEditorSubsystem* AssetEditorSubsystem = nullptr;

// 编辑器窗口
static IAssetEditorInstance* AssetEditorInstance = nullptr;

// 最后激活的蓝图指针
static UBlueprint* LastBlueprint = nullptr;


void FAutoMixinBPToolBar::Initialize()
{
	FAutoMixinEditorTool::Initialize();
	AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	RegisterButton();
	
	TabForegroundedHandle = FGlobalTabmanager::Get()->OnTabForegrounded_Subscribe(
		FOnActiveTabChanged::FDelegate::CreateLambda([](const TSharedPtr<SDockTab>& NewlyActiveTab, const TSharedPtr<SDockTab>& PreviouslyActiveTab)
		{
			if (!NewlyActiveTab.IsValid() || NewlyActiveTab == LastForegroundTab.Pin())
			{
				return;
			}
			// 不是主要的Tab（比如EventGraph，等图标）也返回
			if (NewlyActiveTab.Get()->GetTabRole() != MajorTab) return;
			LastForegroundTab = NewlyActiveTab;
			if (LastForegroundTab.IsValid())
			{
				// UE_LOG(LogTemp, Log, TEXT("标签页已切换: %s"), *LastForegroundTab.Pin().Get()->GetTabLabel().ToString());
			}
		})
	);
}

void FAutoMixinBPToolBar::Uninitialize()
{
	FAutoMixinEditorTool::Uninitialize();
	// 取消订阅标签切换事件
	FGlobalTabmanager::Get()->OnTabForegrounded_Unsubscribe(TabForegroundedHandle);
}

void FAutoMixinBPToolBar::RegisterButton()
{
	// 获取蓝图编辑器模块
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");

	// 创建一个菜单扩展器
	const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

	// 向工具栏添加扩展，在"Debugging" 工具按钮前面添加
	MenuExtender->AddToolBarExtension(
		"Debugging", // 扩展点名称（在调试工具栏区域）
		EExtensionHook::First, // 插入位置（最前面）
		nullptr, // 不使用命令列表
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
		{
			// 使用自定义 SComboButton，为了让图标根据当前蓝图状态动态刷新。
			ToolbarBuilder.AddWidget(
				SNew(SComboButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.ContentPadding(FMargin(4.f, 2.f))
				.ToolTipText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([this]()
				{
					return GetAutoMixinStatusTooltip(GetActiveBlueprint());
				})))
				.OnGetMenuContent(FOnGetContent::CreateLambda([this]()
				{
					FMenuBuilder MenuBuilder(true, nullptr);

					// 没有 TS 时创建，有 TS 时直接打开。
					MenuBuilder.AddMenuEntry(
						LOCTEXT("CreateOrOpenTSFile", "创建/打开TS"),
						LOCTEXT("CreateOrOpenTSFileTooltip", "创建或打开当前蓝图对应的TypeScript文件"),
						FSlateIcon(FPuerTSToolStyle::GetStyleSetName(), "PuerTSAutoMixinTool.PluginAction"),
						FUIAction(FExecuteAction::CreateLambda([this]()
						{
							GenTsButtonPressed();
						}))
					);

					// 手动执行完整类型生成，避免只创建 TS 时才触发生成。
					MenuBuilder.AddMenuEntry(
						LOCTEXT("PuertsGenFull", "dts全量生成"),
						LOCTEXT("PuertsGenFullTooltip", "执行Puerts.Gen Full生成完整类型声明"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([this]()
						{
							ExecutePuertsGenFull();
						}))
					);

					// 全量检查 PreMixin 和所有 @mixin TS 文件，发现问题后统一弹窗。
					MenuBuilder.AddMenuEntry(
						LOCTEXT("CheckAllTsMixin", "检查 Mixin"),
						LOCTEXT("CheckAllTsMixinTooltip", "检查PreMixin.ts和所有@mixin TS文件是否互相匹配"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([this]()
						{
							CheckAllTsMixin();
						}))
					);

					return MenuBuilder.MakeWidget();
				}))
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SImage)
						.Image(TAttribute<const FSlateBrush*>::Create(TAttribute<const FSlateBrush*>::FGetter::CreateLambda([this]()
						{
							return GetAutoMixinStatusIconBrush(GetActiveBlueprint());
						})))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([this]()
						{
							return GetAutoMixinStatusText(GetActiveBlueprint());
						})))
					]
				],
				NAME_None,
				false
			);
		})
	);

	// 将扩展器添加到蓝图编辑器的菜单扩展管理器
	BlueprintEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
}

void FAutoMixinBPToolBar::GenTsButtonPressed()
{
	if (UBlueprint* Blueprint = GetActiveBlueprint())
	{
		GenerateTS(Blueprint);
	}
}

// 菜单动作：执行 Puerts 的完整声明生成命令。
void FAutoMixinBPToolBar::ExecutePuertsGenFull() const
{
	if (GEditor)
	{
		GEditor->Exec(nullptr, TEXT("Puerts.Gen Full"), *GLog);
	}
}

// 菜单动作：扫描 PreMixin.ts 和 TypeScript 目录下所有 @mixin 文件，列出缺失、注释或误删问题。
void FAutoMixinBPToolBar::CheckAllTsMixin() const
{
	const FString TypeScriptRootPath = AutoMixinUtils::GetTypeScriptRootPath();
	const FString PreMixinFilePath = AutoMixinUtils::GetPreMixinFilePath();

	// 检查
	// 1. 从 PreMixin 出发，确认每个启用 import 都能找到对应 TS 文件。
	//    这类问题通常来自 TS 文件被删除、移动、改名，但 PreMixin 还保留旧引用。
	// 2. 从 TypeScript 目录出发，找所有包含 @mixin 的 TS 文件，确认它们都在 PreMixin 中启用。
	//    这类问题通常来自手动新增 TS、误删 PreMixin 行，或者把 import 注释掉。
	// 3. 对每个 @mixin TS 解析 AssetPath，确认它指向的蓝图包仍然存在。
	//    这类问题通常来自蓝图被删除、移动、改名，但 TS 文件还留着旧路径。
	//
	// PreMixin 中的路径统一转成不带 ./ 和 .ts 的 key；TS 文件路径也转成同样格式。
	// 这样可以避免 ./A/B、A/B.ts、反斜杠、大小写差异导致误判。
	AutoMixinUtils::FPreMixinImportInfo PreMixinInfo;
	TArray<FString> Issues;
	if (!AutoMixinUtils::LoadPreMixinImports(PreMixinInfo))
	{
		AutoMixinUtils::AddIssue(Issues, FString::Printf(TEXT("找不到或无法读取PreMixin文件: %s"), *PreMixinFilePath));
	}

	// PreMixin -> TS：
	// 对每条启用 import 拼出 TypeScript 根目录下的 .ts 文件路径。
	// 如果文件不存在，运行时 import 会失败，或者 mixin 根本无法加载。
	for (const FString& ImportKey : PreMixinInfo.ActiveImports)
	{
		const FString DisplayPath = PreMixinInfo.DisplayPaths.Contains(ImportKey) ? PreMixinInfo.DisplayPaths[ImportKey] : ImportKey;
		const FString TsFilePath = FPaths::Combine(TypeScriptRootPath, DisplayPath + TEXT(".ts"));
		if (!FPaths::FileExists(TsFilePath))
		{
			AutoMixinUtils::AddIssue(Issues, FString::Printf(TEXT("PreMixin引用的TS不存在: %s"), *DisplayPath));
		}
	}

	// 注释 import：
	// 如果某个路径只有注释 import，没有启用 import，说明这个 mixin 当前不会被 PreMixin 加载。
	// 如果同一路径同时存在启用 import 和历史注释行，则以启用 import 为准，不再提示异常。
	for (const FString& ImportKey : PreMixinInfo.CommentedImports)
	{
		if (PreMixinInfo.ActiveImports.Contains(ImportKey))
		{
			continue;
		}

		const FString DisplayPath = PreMixinInfo.DisplayPaths.Contains(ImportKey) ? PreMixinInfo.DisplayPaths[ImportKey] : ImportKey;
		AutoMixinUtils::AddIssue(Issues, FString::Printf(TEXT("PreMixin中的Mixin被注释: %s"), *DisplayPath));
	}

	// TS -> PreMixin：
	// 扫描所有 .ts，但只有包含 @mixin 装饰器的文件才算自动 mixin 文件。
	// 这里先把每个 TS 文件读入并剥离注释，再同时用于 @mixin 判断和 AssetPath 解析；
	// 避免先 IsMixinTsFile 读一次、再 TryExtractMixinAssetPath 读第二次。
	TArray<FString> TsFiles;
	IFileManager::Get().FindFilesRecursive(TsFiles, *TypeScriptRootPath, TEXT("*.ts"), true, false);
	for (const FString& TsFile : TsFiles)
	{
		FString NormalizedTsFile = TsFile;
		FPaths::NormalizeFilename(NormalizedTsFile);
		if (NormalizedTsFile.Equals(PreMixinFilePath, ESearchCase::IgnoreCase))
		{
			continue;
		}

		FString CodeWithoutComments;
		if (!AutoMixinUtils::LoadTsCodeWithoutComments(NormalizedTsFile, CodeWithoutComments) || !AutoMixinUtils::IsMixinTsCode(CodeWithoutComments))
		{
			continue;
		}

		const FString ImportPath = AutoMixinUtils::MakeRelativeImportPathForTsFile(NormalizedTsFile, TypeScriptRootPath);
		const FString ImportKey = AutoMixinUtils::MakeImportKey(ImportPath);

		// 先检查 PreMixin 绑定状态：已启用则通过；未启用则继续区分“被注释”和“未添加”。
		const bool bIsActiveInPreMixin = PreMixinInfo.ActiveImports.Contains(ImportKey);
		if (!bIsActiveInPreMixin)
		{
			// 未启用但存在注释 import，提示“被注释”；完全找不到 import，提示“未添加”。
			if (PreMixinInfo.CommentedImports.Contains(ImportKey))
			{
				AutoMixinUtils::AddIssue(Issues, FString::Printf(TEXT("Mixin TS在PreMixin中被注释: %s"), *ImportPath));
			}
			else
			{
				AutoMixinUtils::AddIssue(Issues, FString::Printf(TEXT("Mixin TS未添加到PreMixin: %s"), *ImportPath));
			}
		}

		// 再检查 TS -> 蓝图：AssetPath 是运行时 mixin 绑定的真实目标。
		// 即使 PreMixin 已启用，如果蓝图被移动或删除，运行时也无法把 TS mixin 挂到对应类上。
		FString MixinAssetPath;
		if (!AutoMixinUtils::TryExtractMixinAssetPathFromCode(CodeWithoutComments, MixinAssetPath))
		{
			AutoMixinUtils::AddIssue(Issues, FString::Printf(TEXT("Mixin TS缺少AssetPath，无法检查对应蓝图: %s"), *ImportPath));
		}
		else
		{
			FString BlueprintPackageName;
			if (!AutoMixinUtils::DoesBlueprintPackageExistForMixinAssetPath(MixinAssetPath, BlueprintPackageName))
			{
				AutoMixinUtils::AddIssue(Issues, FString::Printf(TEXT("Mixin TS对应蓝图不存在: %s -> %s"), *ImportPath, *MixinAssetPath));
			}
		}
	}

	// 没有问题也弹窗反馈，避免用户不知道检查是否执行完成。
	if (Issues.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("AutoMixinCheckPassed", "所有TS Mixin检查通过"));
		return;
	}

	const int32 MaxDisplayCount = 80;
	TArray<FString> DisplayIssues;
	for (int32 Index = 0; Index < Issues.Num() && Index < MaxDisplayCount; ++Index)
	{
		DisplayIssues.Add(FString::Printf(TEXT("%d. %s"), Index + 1, *Issues[Index]));
	}

	// 弹窗最多展示前 80 条，避免异常过多时对话框过长。
	FString Message = FString::Printf(TEXT("发现%d个TS Mixin问题:\n\n%s"), Issues.Num(), *FString::Join(DisplayIssues, TEXT("\n")));
	if (Issues.Num() > MaxDisplayCount)
	{
		Message += FString::Printf(TEXT("\n\n还有%d个问题未显示，请查看PreMixin.ts和TypeScript目录。"), Issues.Num() - MaxDisplayCount);
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
}

UBlueprint* FAutoMixinBPToolBar::GetActiveBlueprint()
{
	// 遍历所有被编辑的资产 并找到活动蓝图
	if (!AssetEditorSubsystem) return nullptr;

	for (const TArray<UObject*> EditedAssets = AssetEditorSubsystem->GetAllEditedAssets(); UObject* EditedAsset : EditedAssets)
	{
		AssetEditorInstance = AssetEditorSubsystem->FindEditorForAsset(EditedAsset, false);

		if (!AssetEditorInstance || !IsValid(EditedAsset) || !EditedAsset->IsA<UBlueprint>()) continue;

		TSharedPtr<SDockTab> Tab = LastForegroundTab.Pin();
		if (!Tab.IsValid()) return nullptr;

		if (
			Tab->GetTabLabel().ToString() ==
			AssetEditorInstance->GetAssociatedTabManager().Get()->GetOwnerTab().Get()->GetTabLabel().ToString()
		)
		{
			LastBlueprint = CastChecked<UBlueprint>(EditedAsset);
			break;
		}
	}

	// 返回最后激活的蓝图
	return LastBlueprint;
}


#undef LOCTEXT_NAMESPACE
