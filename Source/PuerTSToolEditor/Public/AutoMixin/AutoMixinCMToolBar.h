#pragma once
#include "AutoMixinEditorTool.h"

class FAutoMixinCMToolBar:FAutoMixinEditorTool
{
public:
	virtual void Initialize() override;
	virtual void Uninitialize() override;
	
protected:
	void RegisterContextMenuButton() const;
	static void ContextMenuButtonPressed(const TArray<FAssetData>& SelectedAssets);
};
