// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JsEnv.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/UObjectArray.h"
#include "TSSubsystem.generated.h"

namespace puerts
{
	class FSourceFileWatcher;
}

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
	// 监听所有UObject创建。Listening模式依赖这里捕获新加载出来的UClass或新实例。
	, public FUObjectArray::FUObjectCreateListener
	, public FUObjectArray::FUObjectDeleteListener
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
	
	
	#if WITH_EDITOR
		TSharedPtr<PUERTS_NAMESPACE::FSourceFileWatcher> SourceFileWatcher;
	
		void HotReloadJavaScriptEnv(const FString& Path);
	#endif
	
	//缓存蓝图类到m_arrCacheClass
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void CacheClass(const UClass* _UClass);

	//缓存蓝图对象到m_arrCacheObejct
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void CacheObject(const UObject* _UObject);

	//从m_arrCacheClass移除蓝图类
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void RemoveClass(const UClass* _UClass);

	//从m_arrCacheObejct移除蓝图对象
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem")
	void RemoveObject(const UObject* _UObject);

	
public:
	
	// FUObjectArray Listener Interface
	
	// UObject创建入口：
	// 如果ObjectBase本身是UClass，尝试按该UClass路径mixin。
	// 如果ObjectBase是普通对象，取Object->GetClass()尝试mixin，覆盖“实例化时才需要绑定”的场景。
	virtual void NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index) override;

	// 当前自动mixin不需要在删除时做反向处理。UClass被GC后，m_autoMixedClasses里的弱引用会自然失效。
	virtual void NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index) override;

	// UObject系统关闭时移除监听，避免Subsystem析构后仍被GUObjectArray回调。
	virtual void OnUObjectArrayShutdown() override;
	
	// FUObjectArray Listener Interface
	
	
	// TS侧@mixin(..., EMixinMode.Listening)调用。
	// 只注册蓝图生成类路径，例如"/Game/Tests/BP_TestPuer.BP_TestPuer_C"。
	// 不LoadObject、不持有UClass，让UClass生命周期继续由UE控制。
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem|AutoMixin")
	void RegisterAutoMixinClass(const FString& ClassPath);

	// 清理自动Mixin注册信息。通常在TS环境释放或Subsystem销毁时调用。
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem|AutoMixin")
	void ClearAutoMixinClasses();
	
	// 如果UClass在TS注册路径前已经加载，NotifyUObjectCreated不会再触发。
	// 调用该函数会检查已加载类，命中m_autoMixinClassPaths后回调TS执行mixin。
	UFUNCTION(BlueprintCallable, Category = "TSSubsystem|AutoMixin")
	void RefreshAutoMixinLoadedClasses();

private:
	
	// 自动Mixin路径列表，仅保存路径，不持有UClass。
	// 这是监听模式的“mixin列表”：NotifyUObjectCreated发现UClass后用GetPathName()与这里匹配。
	TSet<FString> m_autoMixinClassPaths; //TODO: 如果查到FString开销大用FName代替？

	// 已Mixin的UClass弱引用，用于防止同一个活着的UClass重复mixin。
	// 使用弱引用是为了不阻止UClass GC；同路径UClass重新加载成新对象后，可以再次mixin。
	TSet<TWeakObjectPtr<UClass>> m_autoMixedClasses; //TODO:flush or refresh 进行清理？

	// 异步加载中创建的UClass可能还没PostLoad/CDO未初始化，不能立刻mixin。
	// 先放入候选队列，等待OnAsyncLoadingFlushUpdate里确认ready后再处理。
	FCriticalSection m_autoMixinCandidatesLock;
	TArray<FWeakObjectPtr> m_autoMixinCandidates;
	FDelegateHandle m_autoMixinAsyncFlushHandle;
	bool m_bAutoMixinListening = false;

	
	// 注册/移除GUObjectArray监听，以及异步加载flush监听。
	void StartAutoMixinListen();
	void StopAutoMixinListen();

	// 异步加载flush时重试候选对象，避免在RF_NeedPostLoad或AsyncLoading阶段mixin。
	void OnAutoMixinAsyncLoadingFlushUpdate();

	// 从任意UObject提取UClass并进入自动mixin判断。
	void TryAutoMixin(UObject* Object);

	// 自动mixin主判断：
	// 路径命中m_autoMixinClassPaths、Class ready、且未绑定过时，CallTSEvent("ApplyAutoMixin")。
	void TryAutoMixinClass(UClass* Class);

	// 将暂时不能处理的对象加入候选队列。
	void AddAutoMixinCandidate(UObject* Object);

	// 判断UClass是否已经脱离异步加载/重编译临时类/CDO初始化阶段，可以安全执行blueprint.mixin。
	bool IsAutoMixinClassReady(UClass* Class) const;
};
