// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TSToolBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class PUERTSTOOL_API UTSToolBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/**
	 * 获取文件所在目录路径
	 */
	UFUNCTION(BlueprintPure, Category="TSTool|Misc|StringTools")
	static FString GetDirectoryName(const FString& Path);

	/**
	 * 合并路径
	 */
	UFUNCTION(BlueprintPure, Category="TSTool|Misc|StringTools")
	static FString CombinePath(const FString& Directory, const FString& File);

	/**
	 * 检查文件是否存在
	 */
	UFUNCTION(BlueprintPure, Category="TSTool|Misc|StringTools")
	static bool FileExists(const FString& Path);

	/**
	 * 读取文件所有文本内容
	 */
	UFUNCTION(BlueprintCallable, Category="TSTool|Misc|StringTools")
	static FString ReadAllText(const FString& Path);
};
