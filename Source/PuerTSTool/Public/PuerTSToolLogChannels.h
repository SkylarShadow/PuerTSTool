// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

class UObject;

PUERTSTOOL_API DECLARE_LOG_CATEGORY_EXTERN(LogPuerTSTool, Log, All);
PUERTSTOOL_API DECLARE_LOG_CATEGORY_EXTERN(LogPuerTSToolEditor, Log, All);

PUERTSTOOL_API FString GetClientServerContextString(UObject* ContextObject);