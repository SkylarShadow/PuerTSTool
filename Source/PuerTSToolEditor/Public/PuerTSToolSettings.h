#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PuerTSToolSettings.generated.h"

UENUM()
enum class EPuertsCodeEditorType : uint8
{
	/** Visual Studio Code */
	VSCode,

	/** 其他编辑器（需手动指定可执行文件名） */
	Custom
};
    
USTRUCT(BlueprintType)
struct FPuerTSTemplateMapping
{
	GENERATED_BODY()

	// 蓝图父类（支持子类匹配）
	UPROPERTY(EditAnywhere, Config)
	TSoftClassPtr<UObject> BaseClass;

	// 模板文件名
	UPROPERTY(EditAnywhere, Config)
	FString TemplateName;
};

UCLASS(config = PuerTSTool, defaultconfig, meta = (DisplayName = "PuerTSTool"))
class PUERTSTOOLEDITOR_API UPuerTSToolSettings : public UObject
{
    GENERATED_BODY()

public:
    UPuerTSToolSettings();
    FString GetCodeEditorCommand() const;

    // 获取默认设置实例
    static const UPuerTSToolSettings* Get() { return GetDefault<UPuerTSToolSettings>(); }
	
	FString DefaultTemplateName = "BpTemplate.ts";
	
	// 类 -> 模板映射
	UPROPERTY(EditAnywhere, Config, Category = "PuerTSTool|AutoMixin", meta = (DisplayName = "Template Map"))
	TArray<FPuerTSTemplateMapping> TemplateMappings;
	
	UPROPERTY(EditAnywhere, Config, Category = "PuerTSTool|AutoMixin", meta = (DisplayName = "Template File Directory"))
	FString TemplateDir;
	
	//项目目录的TS文件夹
	UPROPERTY(VisibleAnywhere, Config, Category = "PuerTSTool|AutoMixin", meta = (DisplayName = "TypeScript Directory"))
	FString TypeScriptDir;

	//import mixin的ts文件
	UPROPERTY(VisibleAnywhere, Config, Category = "PuerTSTool|AutoMixin", meta = (DisplayName = "Import Mixin File Name"))
	FString ImportMixinFileName;
	
	// DoNotOverwritePaths 使用规则：
	// 1. 相对于 PuertsFrameworkPath
	// 2. 支持两种写法：
	//    - 文件：App.ts
	//    - 目录：Framework/ 或 Framework
	// 3. 自动匹配子目录
	UPROPERTY(EditAnywhere, Config, Category = "PuerTSTool|AutoMixin", meta = (DisplayName = "Framework Override WhiteList"))
	TArray<FString> DoNotOverwritePaths;
	
	UPROPERTY(EditAnywhere, Config, Category = "PuerTSTool|CodeEditor", meta = (DisplayName = "TypeScript Code Editor Type"))
	EPuertsCodeEditorType PuertsCodeEditorType = EPuertsCodeEditorType::VSCode;
	
	UPROPERTY(EditAnywhere, Config, Category = "PuerTSTool|CodeEditor", meta = (DisplayName = "Open Code Editor After Generate TypeScript"))
	bool bOpenCodeEditor = true;
	
	UPROPERTY(EditAnywhere, config,Category = "PuerTSTool|CodeEditor",meta = (EditCondition = "PuertsCodeEditorType == EPuertsCodeEditorType::Custom", DisplayName ="Custom Code Editor Executable Path"))
	FString CustomEditorExecutable;
	
	/*//相对于插件目录的TS文件夹(框架文件夹)
	UPROPERTY(EditAnywhere, Config, Category = "TypeScript", meta = (DisplayName = "Puerts Framework Path"))
	FString PuertsFrameworkPath;*/

	
	// 发现不生效
	/*UPROPERTY(EditAnywhere, Config, Category = "TypeScript", meta = (DisplayName = "Overwrite Files When Deploying PuerTS Framework"))
	bool bOverwriteAllTSFilesWhenDeploy = false; */
	

};