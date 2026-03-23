#include "AutoMixin/AutoMixinBPToolBar.h"
#include "BlueprintEditorModule.h"
#include "AutoMixin/AutoMixinStyle.h"

#define LOCTEXT_NAMESPACE "FPuerTSToolEditorModule"

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
			// 添加工具栏按钮
			ToolbarBuilder.AddToolBarButton(
				FUIAction( // 按钮动作
					FExecuteAction::CreateLambda([this]()
					{
						ButtonPressed(); // 调用按钮点击处理函数
					})
				),
				NAME_None, // 没有命令名
				LOCTEXT("GenerateTemplate", "创建TS文件"), // 按钮显示文本
				LOCTEXT("GenerateTemplateTooltip", "从模板生成TypeScript文件"), // 按钮提示文本
				FSlateIcon( FAutoMixinStyle::GetStyleSetName(), "AutoMixinIcon") // 按钮图标（这里使用空图标）
			);
		})
	);

	// 将扩展器添加到蓝图编辑器的菜单扩展管理器
	BlueprintEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
}

void FAutoMixinBPToolBar::ButtonPressed()
{
	if (UBlueprint* Blueprint = GetActiveBlueprint())
	{
		GenerateTS(Blueprint);
	}
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