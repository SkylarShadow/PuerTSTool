#include "PuerTSToolSettings.h"

UPuerTSToolSettings::UPuerTSToolSettings()
{
	// TypeScript defaults
	TemplateName = TEXT("MixinTemplate.ts"); //TODO: 不同的Cpp/蓝图类做对应模板
	TypeScriptDir = TEXT("TypeScript"); 
	PuertsFrameworkPath = TEXT("PuerTSTool/Resources");
	AutoImportName = TEXT("PreMixin.ts");
}