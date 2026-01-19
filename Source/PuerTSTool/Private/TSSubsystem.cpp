// Fill out your copyright notice in the Description page of Project Settings.


#include "TSSubsystem.h"
#include <Kismet/GameplayStatics.h>
#include <PuertsModule.h>

void UTSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

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
	UE_LOG(LogTemp, Warning, TEXT("UTSSubsystem Initialize"));
}

void UTSSubsystem::Deinitialize()
{
	Super::Deinitialize();

	TArray<FTSEventData> eventData;
	this->CallTSEvent(FName("DisposeTS"), eventData);
	m_mapEvent.Empty();
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
		UE_LOG(LogTemp, Warning, TEXT("JS Function is already register!---------->%s"), *_strEventName.ToString());
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
		UE_LOG(LogTemp, Warning, TEXT("JS Function is not register!---------->%s"), *_strEventName.ToString());
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
