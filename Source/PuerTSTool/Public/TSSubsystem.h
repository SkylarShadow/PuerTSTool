// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JsEnv.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TSSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FTSEventData
{
	GENERATED_BODY()

	FTSEventData()
		:ObjectData(nullptr)
		, Int32Data(0)
		, FloatData(0)
	{
	}

public:
	UPROPERTY(BlueprintReadWrite, Category = "TSEventData")
	FString DataKey;

	UPROPERTY(BlueprintReadWrite, Category = "TSEventData")
	TObjectPtr<UObject> ObjectData;

	UPROPERTY(BlueprintReadWrite, Category = "TSEventData")
	int32 Int32Data;

	UPROPERTY(BlueprintReadWrite, Category = "TSEventData")
	float FloatData;

	UPROPERTY(BlueprintReadWrite, Category = "TSEventData")
	FString StringData;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FTSCallBack, FName, EventName, TArray<FTSEventData>, EventData);

/**
 * TypeScript管理器
 */
UCLASS()
class PUERTSTOOL_API UTSSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TMap<FName, FTSCallBack> m_mapEvent;

	//缓存类引用，防止TS引用被释放掉
	UPROPERTY()
	TArray<const UClass*> m_arrCacheClass;

	//缓存对象引用，防止TS引用被释放掉
	UPROPERTY()
	TArray<const UObject*> m_arrCacheObejct;

public:
	//给TS绑定反射方法，不要在其他地方调用
	UPROPERTY(BlueprintReadOnly, Category = "TSSubsystem")
	FTSCallBack OnTSFunction;

public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

	void Deinitialize() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** 获取单例 */
	UFUNCTION(BlueprintPure, Category = "TSSubsystem")
	static UTSSubsystem* Get(const UObject* WorldContextObject);

	//绑定TS回调方法，在TS里面调用，单播绑定，不能绑定多个方法
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void PassTSFunctionAsEvent(FName _strEventName, FTSCallBack _Callback);

	//调用TS事件，在蓝图或C++里面调用
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void CallTSEvent(FName _strEventName, TArray<FTSEventData> _EventData);

	//调用TS方法，通过注入TS语句直接调用，没有编译检查，慎用！
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void CallTSFunction(FName _strEventName, TArray<FTSEventData> _EventData);

	//格式化文本
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	static FText FormatText(const FText& _txtFormat, const TArray<FText>& _arrArgs);

	//是否是DS服务器
	UFUNCTION(BlueprintPure, Category = "TSSubsystem")
	bool IsDS();

	//缓存蓝图类
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void CacheClass(const UClass* _UClass);

	//缓存蓝图对象
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void CacheObject(const UObject* _UObject);

	//移除蓝图类
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void RemoveClass(const UClass* _UClass);

	//移除蓝图对象
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void RemoveObject(const UObject* _UObject);
};
