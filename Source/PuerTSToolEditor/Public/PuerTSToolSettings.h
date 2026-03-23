#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PuerTSToolSettings.generated.h"

UENUM()
enum class EPuertsMixinEditorType : uint8
{
	/** Visual Studio Code */
	VSCode,

	/** 其他基于 VSCode 的编辑器（需手动指定可执行文件名） */
	Custom
};
    
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "PuerTSTool"))
class PUERTSTOOLEDITOR_API UPuerTSToolSettings : public UObject
{
    GENERATED_BODY()

public:
    UPuerTSToolSettings();

    // 获取默认设置实例
    static const UPuerTSToolSettings* Get() { return GetDefault<UPuerTSToolSettings>(); }

	/** 常量设置 */
	UPROPERTY(EditAnywhere, Config, Category = "TypeScript", meta = (DisplayName = "Template File Name"))
	FString TemplateName;

    
	UPROPERTY(VisibleAnywhere, Config, Category = "TypeScript", meta = (DisplayName = "TypeScript Directory"))
	FString TypeScriptDir;

	UPROPERTY(EditAnywhere, Config, Category = "TypeScript", meta = (DisplayName = "Puerts Framework Path"))
	FString PuertsFrameworkPath;

	UPROPERTY(EditAnywhere, Config, Category = "TypeScript", meta = (DisplayName = "Auto Import File Name"))
	FString AutoImportName;


};