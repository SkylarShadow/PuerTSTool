// Fill out your copyright notice in the Description page of Project Settings.


#include "TSSubsystem.h"
#include <Kismet/GameplayStatics.h>
#include <PuertsModule.h>

#include "PuerTSToolLogChannels.h"
#include "SourceFileWatcher.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 6
constexpr EInternalObjectFlags AutoMixinAsyncObjectFlags = EInternalObjectFlags_AsyncLoading | EInternalObjectFlags::Async;
#else
constexpr EInternalObjectFlags AutoMixinAsyncObjectFlags = EInternalObjectFlags::AsyncLoading | EInternalObjectFlags::Async;
#endif

DECLARE_STATS_GROUP(TEXT("TSSubsystem"), STATGROUP_TSSubsystem, STATCAT_Advanced);

void UTSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 自动Mixin监听需要尽早启动：
	// TS侧注册路径后，后续新加载出的UClass会通过NotifyUObjectCreated进入自动绑定流程。
	StartAutoMixinListen();

	//-----------------------------------PuertsModule.cpp中添加此段代码,提供外部接口访问JsEnv-----------------------------------
	//virtual puerts::FJsEnv* GetJsEnv()
	//{
	//	return JsEnv.Get();
	//}
	//-----------------------------------PuertsModule.cpp中添加此段代码,提供外部接口访问JsEnv-----------------------------------
	//要启用下方的构造JsEnv的方式，需要在对应PuertsModule添加上述函数:
	puerts::FJsEnv* GameScript = IPuertsModule::Get().GetJsEnv();


	//TSharedPtr<puerts::FJsEnv> GameScript;
	//GameScript = MakeShared<puerts::FJsEnv>();

#if WITH_EDITOR

	std::function<void(const FString&)> SourceLoadedCallback = nullptr;

	SourceFileWatcher = MakeShared<PUERTS_NAMESPACE::FSourceFileWatcher>(
		[this](const FString& InPath)
		{
			HotReloadJavaScriptEnv(InPath);
		});
	SourceLoadedCallback = [this](const FString& InPath)
	{
		if (SourceFileWatcher.IsValid())
		{
			SourceFileWatcher->OnSourceLoaded(InPath);
		}
	};
#endif
	
	if (GameScript != nullptr)
	{
		TArray<TPair<FString, UObject*>> Arguments;
		Arguments.Add(TPair<FString, UObject*>(TEXT("GameInstance"), UGameplayStatics::GetGameInstance(this)));
		if (GetGameInstance()->IsDedicatedServerInstance())
		{

		}
		else
		{
			GameScript->Start("QuickStart", Arguments);
		}
	}
	UE_LOG(LogPuerTSTool, Log, TEXT("UTSSubsystem Initialize"));
}

void UTSSubsystem::Deinitialize()
{
	// Subsystem销毁前先移除全局UObject监听，避免后续对象创建回调到已释放的Subsystem。
	StopAutoMixinListen();

	Super::Deinitialize();

	TArray<FTSEventData> eventData;
	this->CallTSEvent(FName("ReleaseTS"), eventData);
	m_mapEvent.Empty();
	
	m_arrCacheClass.Empty();
	m_arrCacheObejct.Empty();

	// 自动Mixin的Native状态不依赖TS侧ReleaseTS是否执行成功，这里主动清理路径、弱引用和候选队列。
	ClearAutoMixinClasses();
}

bool UTSSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{
		TArray<UClass*> ChildClasses;
		GetDerivedClasses(GetClass(), ChildClasses, false);

		// Only create an instance if there is no override implementation defined elsewhere
		return ChildClasses.Num() == 0;
	}

	return false;
}

UTSSubsystem* UTSSubsystem::Get(const UObject* WorldContextObject)
{
	UGameInstance* pIns = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (IsValid(pIns))
	{
		return pIns->GetSubsystem<UTSSubsystem>();
	}

	return nullptr;
}

void UTSSubsystem::PassTSFunctionAsEvent(FName _strEventName, FTSCallBack _Callback)
{
	if (!m_mapEvent.Contains(_strEventName))
	{
		m_mapEvent.Add(_strEventName, _Callback);
	}
	else
	{
		UE_LOG(LogPuerTSTool, Warning, TEXT("JS Function is already register!---------->%s"), *_strEventName.ToString());
	}
}

void UTSSubsystem::CallTSEvent(FName _strEventName, TArray<FTSEventData> _EventData)
{
	if (m_mapEvent.Contains(_strEventName))
	{
		m_mapEvent[_strEventName].ExecuteIfBound(_strEventName, _EventData);
	}
	else
	{
		UE_LOG(LogPuerTSTool, Warning, TEXT("JS Function is not register!---------->%s"), *_strEventName.ToString());
	}
}

void UTSSubsystem::CallTSFunction(FName _strEventName, TArray<FTSEventData> _EventData)
{
	this->OnTSFunction.ExecuteIfBound(_strEventName, _EventData);
}

FText UTSSubsystem::FormatText(const FText& _txtFormat, const TArray<FText>& _arrArgs)
{
	FFormatOrderedArguments args;
	for (int32 i = 0; i < _arrArgs.Num(); i++)
	{
		args.Add(_arrArgs[i]);
	}

	FText txtResult = FText::Format(_txtFormat, args);
	return txtResult;
}

bool UTSSubsystem::IsDS()
{
	return GetGameInstance()->IsDedicatedServerInstance();
}

#if WITH_EDITOR
void UTSSubsystem::HotReloadJavaScriptEnv(const FString& Path)
{
	puerts::FJsEnv* JsEnv = IPuertsModule::Get().GetJsEnv();
	if (JsEnv!=nullptr)
	{
		TArray<uint8> Source;
		if (FFileHelper::LoadFileToArray(Source, *Path))
		{
			JsEnv->ReloadSource(Path, puerts::PString(reinterpret_cast<const char*>(Source.GetData()), Source.Num()));
			UE_LOG(LogPuerTSToolEditor, Display, TEXT("hot reload file success for %s"), *Path);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("read file fail for %s"), *Path);
		}
	}
}
#endif

void UTSSubsystem::CacheClass(const UClass* _UClass)
{
	if (!this->m_arrCacheClass.Contains(_UClass))
	{
		this->m_arrCacheClass.Add(_UClass);
	}
}

void UTSSubsystem::CacheObject(const UObject* _UObject)
{
	if (!this->m_arrCacheObejct.Contains(_UObject))
	{
		this->m_arrCacheObejct.Add(_UObject);
	}
}

void UTSSubsystem::RemoveClass(const UClass* _UClass)
{
	if (this->m_arrCacheClass.Contains(_UClass))
	{
		this->m_arrCacheClass.Remove(_UClass);
	}
}

void UTSSubsystem::RemoveObject(const UObject* _UObject)
{
	if (this->m_arrCacheObejct.Contains(_UObject))
	{
		this->m_arrCacheObejct.Remove(_UObject);
	}
}

//DECLARE_CYCLE_STAT(TEXT("AutoMixin NotifyUObjectCreated"), STAT_AutoMixin_Create, STATGROUP_TSSubsystem);

//TODO: 做过滤
void UTSSubsystem::NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index)
{
	//SCOPE_CYCLE_COUNTER(STAT_AutoMixin_Create);
	// 自动Mixin入口：
	// ObjectBase可能就是新加载出的UClass。也可能是普通实例，此时通过Object->GetClass()检查它的类是否需要Mixin
	UObject* Object = (UObject*)ObjectBase;
	TryAutoMixin(Object);
}

void UTSSubsystem::NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index)
{

}

void UTSSubsystem::OnUObjectArrayShutdown()
{
	// UObject系统关闭时也要移除监听，避免全局回调残留。
	StopAutoMixinListen();
}

void UTSSubsystem::RegisterAutoMixinClass(const FString& ClassPath)
{
	if (!ClassPath.IsEmpty())
	{
		// 监听模式只保存路径字符串，不主动LoadObject，也不持有UClass。
		// UClass生命周期仍由UE资源加载、场景切换和GC控制。
		m_autoMixinClassPaths.Add(ClassPath);
	}
}

void UTSSubsystem::ClearAutoMixinClasses()
{
	// 清理自动Mixin注册路径、已绑定弱引用和异步加载候选对象。
	m_autoMixinClassPaths.Empty();
	m_autoMixedClasses.Empty();

	FScopeLock Lock(&m_autoMixinCandidatesLock);
	m_autoMixinCandidates.Empty();
}

void UTSSubsystem::RefreshAutoMixinLoadedClasses()
{
	// 如果TS注册路径前目标UClass已经加载完成，NotifyUObjectCreated不会再次触发，所以这里按路径查找已加载UClass并主动尝试Mixin。
	for (const FString& ClassPath : m_autoMixinClassPaths)
	{
		if (UClass* Class = FindObject<UClass>(nullptr, *ClassPath))
		{
			TryAutoMixinClass(Class);
		}
	}
}

void UTSSubsystem::StartAutoMixinListen()
{
	if (m_bAutoMixinListening)
	{
		return;
	}

	// 监听所有UObject创建，用于发现新加载出的UClass或新实例。
	// 异步加载flush回调用于处理“创建时类还没PostLoad完成”的延迟绑定。
	GUObjectArray.AddUObjectCreateListener(this);
	GUObjectArray.AddUObjectDeleteListener(this);
	m_autoMixinAsyncFlushHandle = FCoreDelegates::OnAsyncLoadingFlushUpdate.AddUObject(
		this,
		&UTSSubsystem::OnAutoMixinAsyncLoadingFlushUpdate
	);

	m_bAutoMixinListening = true;
}

void UTSSubsystem::StopAutoMixinListen()
{
	if (!m_bAutoMixinListening)
	{
		return;
	}

	// StartAutoMixinListen里注册的监听必须成对移除。
	GUObjectArray.RemoveUObjectCreateListener(this);
	GUObjectArray.RemoveUObjectDeleteListener(this);
	FCoreDelegates::OnAsyncLoadingFlushUpdate.Remove(m_autoMixinAsyncFlushHandle);

	m_bAutoMixinListening = false;
}

void UTSSubsystem::OnAutoMixinAsyncLoadingFlushUpdate()
{
	TArray<UObject*> LocalCandidates;
	{
		FScopeLock Lock(&m_autoMixinCandidatesLock);
		for (int32 i = m_autoMixinCandidates.Num() - 1; i >= 0; --i)
		{
			FWeakObjectPtr ObjectPtr = m_autoMixinCandidates[i];
			if (!ObjectPtr.IsValid())
			{
				// 候选对象在等待期间已经失效，直接丢弃。
				m_autoMixinCandidates.RemoveAtSwap(i);
				continue;
			}

			UObject* Object = ObjectPtr.Get();
			UClass* Class = Object->IsA<UClass>() ? Cast<UClass>(Object) : Object->GetClass(); //TODO: !Object->IsA<UClass>()则return,不处理实例
			if (!IsAutoMixinClassReady(Class))
			{
				// 类还处于异步加载/PostLoad/CDO初始化阶段，继续留在候选队列等待下一次flush。
				continue;
			}

			LocalCandidates.Add(Object);
			m_autoMixinCandidates.RemoveAtSwap(i);
		}
	}

	for (UObject* Object : LocalCandidates)
	{
		// 离开锁后再执行Mixin判断，避免在锁内触发TS回调或反射修改。
		TryAutoMixin(Object);
	}
}

void UTSSubsystem::TryAutoMixin(UObject* Object)
{
	if (m_autoMixinClassPaths.Num() <= 0 || !IsValid(Object))
	{
		return;
	}

	if (!IsInGameThread())
	{
		// UObject创建可能发生在异步加载线程，不能直接修改反射数据，先放入候选队列。
		AddAutoMixinCandidate(Object);
		return;
	}

	// 如果Object本身是UClass就直接使用；否则取实例所属Class。
	UClass* Class = Object->IsA<UClass>() ? Cast<UClass>(Object) : Object->GetClass();
	TryAutoMixinClass(Class);
}

void UTSSubsystem::TryAutoMixinClass(UClass* Class)
{
	if (m_autoMixinClassPaths.Num() <= 0 || !IsValid(Class))
	{
		return;
	}

	if (!IsInGameThread())
	{
		// blueprint.mixin会修改UFunction/反射数据，必须回到游戏线程处理
		AddAutoMixinCandidate(Class);
		return;
	}

	const FString ClassPath = Class->GetPathName(); //TODO: 尝试减少调用这个的开销，例如非目标类也可以记录下来，避免每个实例重复路径计算。

	if (!m_autoMixinClassPaths.Contains(ClassPath))
	{
		// 该UClass不在TS注册的自动Mixin路径列表中。
		return;
	}

	if (!IsAutoMixinClassReady(Class))
	{
		// 路径匹配，但类尚未准备好，延迟到异步加载flush后再尝试。
		AddAutoMixinCandidate(Class);
		return;
	}

	if (m_autoMixedClasses.Contains(Class))
	{
		// 同一个存活的UClass只Mixin一次。
		// 这里是弱引用记录，不会阻止UClass GC；同路径新UClass加载后仍可再次Mixin。
		return;
	}

	m_autoMixedClasses.Add(Class);

	// 把当前真实UClass传回TS：
	// ObjectData用于blueprint.tojs(ucls)，StringData用于TS侧在autoMixinInfos中查找对应TS类。
	FTSEventData Data;
	Data.DataKey = TEXT("Class");
	Data.ObjectData = Class;
	Data.StringData = ClassPath;

	TArray<FTSEventData> EventData;
	EventData.Add(Data);
	CallTSEvent(FName(TEXT("ApplyAutoMixin")), EventData);
}

void UTSSubsystem::AddAutoMixinCandidate(UObject* Object)
{
	if (!Object)
	{
		return;
	}

	// 候选队列使用弱引用，避免因为等待Mixin而延长UClass/UObject生命周期。
	FScopeLock Lock(&m_autoMixinCandidatesLock);
	m_autoMixinCandidates.AddUnique(Object);
}

bool UTSSubsystem::IsAutoMixinClassReady(UClass* Class) const
{
	if (!IsValid(Class))
	{
		return false;
	}

	if (Class->HasAnyClassFlags(CLASS_NewerVersionExists))
	{
		// 蓝图重编译后旧版本类不再绑定。
		return false;
	}

	if (Class->HasAnyInternalFlags(AutoMixinAsyncObjectFlags)
		|| Class->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
	{
		// 类还在加载/PostLoad阶段，此时修改函数表不安全。
		return false;
	}

	const FString ClassName = Class->GetName();
	if (ClassName.StartsWith(TEXT("SKEL_")) || ClassName.StartsWith(TEXT("REINST_")))
	{
		// 编辑器临时类不参与运行时Mixin。
		return false;
	}

	const UObject* CDO = Class->GetDefaultObject(false);
	if (!CDO || CDO->HasAnyFlags(RF_NeedInitialization))
	{
		// CDO未初始化完成时不执行Mixin。
		return false;
	}

	return true;
}
