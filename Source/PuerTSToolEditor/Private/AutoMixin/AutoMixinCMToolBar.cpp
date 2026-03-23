#include "AutoMixin/AutoMixinCMToolBar.h"

#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "AutoMixin/AutoMixinStyle.h"

#define LOCTEXT_NAMESPACE "FPuerTSToolEditorModule"

void FAutoMixinCMToolBar::Initialize()
{
	FAutoMixinEditorTool::Initialize();
	RegisterContextMenuButton();
}

void FAutoMixinCMToolBar::Uninitialize()
{
	FAutoMixinEditorTool::Uninitialize();
}

void FAutoMixinCMToolBar::RegisterContextMenuButton() const
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuAssetExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	// 创建菜单扩展委托
	FContentBrowserMenuExtender_SelectedAssets MenuExtenderDelegate;
	MenuExtenderDelegate.BindLambda([this](const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();

		Extender->AddMenuExtension(
			"GetAssetActions", // 扩展菜单的锚点位置
			EExtensionHook::After, // 在指定锚点之后插入
			nullptr, // 无命令列表
			FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
			{
				// 添加菜单项
				MenuBuilder.AddMenuEntry(
					LOCTEXT("GenerateTSFile", "创建TS文件"), // 菜单文本
					LOCTEXT("GenerateTSFileTooltip", "生成TypeScript文件"), // 提示信息
					FSlateIcon(FAutoMixinStyle::GetStyleSetName(), "AutoMixinIcon"), // 使用自定义图标
					FUIAction(
						FExecuteAction::CreateLambda([this, SelectedAssets]()
						{
							// 点击时处理选中的资源
							ContextMenuButtonPressed(SelectedAssets);
						}),
						FCanExecuteAction::CreateLambda([SelectedAssets]()
						{
							// 可选：验证是否允许执行（例如至少选中一个资产）
							return SelectedAssets.Num() > 0;
						})
					)
				);
			})
		);
		return Extender;
	});

	// 注册扩展委托
	CBMenuAssetExtenderDelegates.Add(MenuExtenderDelegate);
}

void FAutoMixinCMToolBar::ContextMenuButtonPressed(const TArray<FAssetData>& SelectedAssets)
{
	// 确保至少有一个资产被选中
	if (SelectedAssets.IsEmpty()) return;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		// 获取选中的蓝图
		if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			GenerateTS(Blueprint);
		}
	}
}
#undef LOCTEXT_NAMESPACE
