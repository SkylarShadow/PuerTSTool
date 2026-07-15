// Copyright Epic Games, Inc. All Rights Reserved.

#include "PuerTSToolStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FPuerTSToolStyle::StyleInstance = nullptr;

void FPuerTSToolStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FPuerTSToolStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FPuerTSToolStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("PuerTSToolStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
TSharedRef< FSlateStyleSet > FPuerTSToolStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("PuerTSToolStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("PuerTSTool")->GetBaseDir() / TEXT("Resources"));

	Style->Set("PuerTSAutoMixinTool.PluginAction", new IMAGE_BRUSH(TEXT("AutoMixinIcon40"), Icon20x20));
	// AutoMixin 一级菜单状态图标，资源文件后续补到 Resources 目录下即可。
	Style->Set("PuerTSAutoMixinTool.NoTs", new IMAGE_BRUSH(TEXT("AutoMixinNoTs"), Icon20x20));
	Style->Set("PuerTSAutoMixinTool.Bound", new IMAGE_BRUSH(TEXT("AutoMixinBound"), Icon40x40));
	Style->Set("PuerTSAutoMixinTool.PreMixinError", new IMAGE_BRUSH(TEXT("AutoMixinPreMixinError"), Icon20x20));
	Style->Set("PuerTSAutoMixinTool.NoBlueprint", new IMAGE_BRUSH(TEXT("AutoMixinNoBlueprint"), Icon20x20));
	return Style;
}

void FPuerTSToolStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FPuerTSToolStyle::Get()
{
	return *StyleInstance;
}
