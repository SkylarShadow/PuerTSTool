// Fill out your copyright notice in the Description page of Project Settings.


#include "TSToolBlueprintFunctionLibrary.h"


#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FString UTSToolBlueprintFunctionLibrary::GetDirectoryName(const FString& Path)
{
	return FPaths::GetPath(Path);
}

FString UTSToolBlueprintFunctionLibrary::CombinePath(const FString& Directory, const FString& File)
{
	return FPaths::Combine(Directory, File);
}

bool UTSToolBlueprintFunctionLibrary::FileExists(const FString& Path)
{
	return FPaths::FileExists(Path);
}

FString UTSToolBlueprintFunctionLibrary::ReadAllText(const FString& Path)
{
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("File not exists, Path:%s"), *Path);
		return FString();
	}
	FString Result;
	if (FFileHelper::LoadFileToString(Result, *Path))
	{
		return Result;
	}
	UE_LOG(LogTemp, Error, TEXT("Read file failed, Path:%s"), *Path);
	return FString();
}