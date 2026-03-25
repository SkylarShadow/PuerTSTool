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

TSharedRef< FSlateStyleSet > FPuerTSToolStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("PuerTSToolStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("PuerTSTool")->GetBaseDir() / TEXT("Resources"));

	Style->Set("PuerTSAutoMixinTool.PluginAction", new IMAGE_BRUSH(TEXT("AutoMixinIcon40"), Icon20x20));
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
