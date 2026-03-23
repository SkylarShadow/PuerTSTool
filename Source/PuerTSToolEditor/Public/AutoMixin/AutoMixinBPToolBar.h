#pragma once
#include "AutoMixinEditorTool.h"
#include "Android/AndroidPlatformCompilerSetup.h"

class FAutoMixinBPToolBar :FAutoMixinEditorTool
{
public:
	virtual void Initialize() override;
	virtual void Uninitialize() override;
	
protected:
	void RegisterButton();
	void ButtonPressed();
	
	static UBlueprint* GetActiveBlueprint();
};
