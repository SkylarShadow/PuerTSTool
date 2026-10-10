#include "AutoMixinUtils.h"

#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PuerTSToolLogChannels.h"
#include "PuerTSToolSettings.h"

namespace AutoMixinUtils
{
	namespace
	{
		bool TryBuildImportPathFromObjectPath(const FString& ObjectPath, FString& OutActualPath, FString& OutImportPath)
		{
			FString Lefts;
			FString Rights;
			ObjectPath.Split(TEXT("."), &Lefts, &Rights);

			TArray<FString> PathParts;
			Lefts.ParseIntoArray(PathParts, TEXT("/"), true);
			if (PathParts.IsEmpty())
			{
				return false;
			}

			OutActualPath = PathParts[0] == TEXT("Game") ? Lefts.Mid(5) : Lefts;
			OutImportPath = NormalizeImportPath(OutActualPath.Mid(1));
			return true;
		}
	}

	FString NormalizeImportPath(FString ImportPath)
	{
		ImportPath.TrimStartAndEndInline();
		ImportPath.ReplaceInline(TEXT("\\"), TEXT("/"));
		ImportPath.RemoveFromStart(TEXT("./"));
		ImportPath.RemoveFromStart(TEXT("/"));
		ImportPath.RemoveFromEnd(TEXT(".ts"));
		return ImportPath;
	}

	FString MakeImportKey(const FString& ImportPath)
	{
		return NormalizeImportPath(ImportPath).ToLower();
	}

	bool TryExtractImportPath(const FString& Line, FString& OutImportPath)
	{
		const int32 QuoteStart = Line.Find(TEXT("\"./"));
		const int32 SingleQuoteStart = Line.Find(TEXT("'./"));
		int32 ImportStart = QuoteStart;
		TCHAR QuoteChar = '"';

		if (ImportStart == INDEX_NONE || (SingleQuoteStart != INDEX_NONE && SingleQuoteStart < ImportStart))
		{
			ImportStart = SingleQuoteStart;
			QuoteChar = '\'';
		}

		if (ImportStart == INDEX_NONE)
		{
			return false;
		}

		ImportStart += 1;
		const int32 ImportEnd = Line.Find(FString::Chr(QuoteChar), ESearchCase::CaseSensitive, ESearchDir::FromStart, ImportStart);
		if (ImportEnd == INDEX_NONE)
		{
			return false;
		}

		OutImportPath = NormalizeImportPath(Line.Mid(ImportStart, ImportEnd - ImportStart));
		return !OutImportPath.IsEmpty();
	}

	bool IsCommentedImportLine(const FString& TrimmedLine)
	{
		return TrimmedLine.StartsWith(TEXT("//")) || TrimmedLine.StartsWith(TEXT("/*")) || TrimmedLine.StartsWith(TEXT("*"));
	}

	bool TryBuildTsPathInfo(const UBlueprint* Blueprint, FTsPathInfo& OutInfo)
	{
		if (!Blueprint)
		{
			return false;
		}

		const FString BlueprintPath = Blueprint->GetPathName();
		if (!TryBuildImportPathFromObjectPath(BlueprintPath, OutInfo.ActualPath, OutInfo.ImportPath))
		{
			return false;
		}

		OutInfo.TsFilePath = FPaths::Combine(GetTypeScriptRootPath(), OutInfo.ImportPath + TEXT(".ts"));
		FPaths::NormalizeFilename(OutInfo.TsFilePath);
		return true;
	}

	FString GetTypeScriptRootPath()
	{
		const UPuerTSToolSettings* Settings = GetDefault<UPuerTSToolSettings>();
		FString TypeScriptRootPath = FPaths::Combine(FPaths::ProjectDir(), Settings->TypeScriptDir);
		FPaths::NormalizeFilename(TypeScriptRootPath);
		return TypeScriptRootPath;
	}

	FString GetPreMixinFilePath()
	{
		const UPuerTSToolSettings* Settings = GetDefault<UPuerTSToolSettings>();
		FString PreMixinFilePath = FPaths::Combine(GetTypeScriptRootPath(), Settings->ImportMixinFileName);
		FPaths::NormalizeFilename(PreMixinFilePath);
		return PreMixinFilePath;
	}

	bool LoadPreMixinImports(FPreMixinImportInfo& OutInfo)
	{
		FString PreMixinContent;
		if (!FFileHelper::LoadFileToString(PreMixinContent, *GetPreMixinFilePath()))
		{
			return false;
		}

		TArray<FString> Lines;
		PreMixinContent.ParseIntoArrayLines(Lines, false);
		for (FString Line : Lines)
		{
			Line.TrimStartAndEndInline();
			if (!Line.Contains(TEXT("import")) || !Line.Contains(TEXT("./")))
			{
				continue;
			}

			FString ImportPath;
			if (!TryExtractImportPath(Line, ImportPath))
			{
				continue;
			}

			const FString ImportKey = MakeImportKey(ImportPath);
			OutInfo.DisplayPaths.FindOrAdd(ImportKey, ImportPath);

			if (IsCommentedImportLine(Line))
			{
				OutInfo.CommentedImports.Add(ImportKey);
			}
			else
			{
				OutInfo.ActiveImports.Add(ImportKey);
			}
		}

		return true;
	}

	void EnsurePreMixinImport(const FString& PreMixinFilePath, const FString& ImportPath)
	{
		const FString NormalizedImportPath = NormalizeImportPath(ImportPath);
		if (NormalizedImportPath.IsEmpty())
		{
			return;
		}

		const FString ImportKey = MakeImportKey(NormalizedImportPath);
		const FString ImportStatement = TEXT("import \"./") + NormalizedImportPath + TEXT("\";");

		FString PreMixinContent;
		FFileHelper::LoadFileToString(PreMixinContent, *PreMixinFilePath);

		TArray<FString> Lines;
		PreMixinContent.ParseIntoArrayLines(Lines, false);

		bool bHasActiveImport = false;
		int32 FirstCommentedIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			FString TrimmedLine = Lines[Index];
			TrimmedLine.TrimStartAndEndInline();
			if (!TrimmedLine.Contains(TEXT("import")) || !TrimmedLine.Contains(TEXT("./")))
			{
				continue;
			}

			FString ExistingImportPath;
			if (!TryExtractImportPath(TrimmedLine, ExistingImportPath) || MakeImportKey(ExistingImportPath) != ImportKey)
			{
				continue;
			}

			if (IsCommentedImportLine(TrimmedLine))
			{
				if (FirstCommentedIndex == INDEX_NONE)
				{
					FirstCommentedIndex = Index;
				}
			}
			else
			{
				bHasActiveImport = true;
				break;
			}
		}

		if (bHasActiveImport)
		{
			return;
		}

		if (FirstCommentedIndex != INDEX_NONE)
		{
			Lines[FirstCommentedIndex] = ImportStatement;
			PreMixinContent = FString::Join(Lines, TEXT("\n")) + TEXT("\n");
		}
		else
		{
			if (!PreMixinContent.IsEmpty() && !PreMixinContent.EndsWith(TEXT("\n")))
			{
				PreMixinContent += TEXT("\n");
			}
			PreMixinContent += ImportStatement + TEXT("\n");
		}

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PreMixinFilePath), true);
		if (FFileHelper::SaveStringToFile(PreMixinContent, *PreMixinFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
		{
			UE_LOG(LogPuerTSToolEditor, Log, TEXT("PreMixin.ts更新成功: %s"), *NormalizedImportPath);
		}
	}

	FString StripTsComments(const FString& Content)
	{
		FString Result;
		TArray<FString> Lines;
		Content.ParseIntoArrayLines(Lines, false);

		bool bInBlockComment = false;
		for (FString Line : Lines)
		{
			FString CodeLine;
			for (int32 Index = 0; Index < Line.Len(); ++Index)
			{
				if (bInBlockComment)
				{
					if (Line.Mid(Index, 2) == TEXT("*/"))
					{
						bInBlockComment = false;
						++Index;
					}
					continue;
				}

				if (Line.Mid(Index, 2) == TEXT("//"))
				{
					break;
				}

				if (Line.Mid(Index, 2) == TEXT("/*"))
				{
					bInBlockComment = true;
					++Index;
					continue;
				}

				CodeLine.AppendChar(Line[Index]);
			}

			Result += CodeLine + TEXT("\n");
		}

		return Result;
	}

	bool LoadTsCodeWithoutComments(const FString& TsFilePath, FString& OutCode)
	{
		FString Content;
		if (!FFileHelper::LoadFileToString(Content, *TsFilePath))
		{
			return false;
		}

		OutCode = StripTsComments(Content);
		return true;
	}

	bool IsMixinTsCode(const FString& CodeWithoutComments)
	{
		return CodeWithoutComments.Contains(TEXT("@mixin"));
	}

	bool TryExtractMixinAssetPathFromCode(const FString& CodeWithoutComments, FString& OutAssetPath)
	{
		int32 SearchIndex = 0;
		int32 EqualIndex = INDEX_NONE;
		while (true)
		{
			const int32 ConstIndex = CodeWithoutComments.Find(TEXT("const"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchIndex);
			if (ConstIndex == INDEX_NONE)
			{
				return false;
			}

			// 只接受 const AssetPath =，避免把 SomeAssetPath 或对象属性误识别为配置项。
			const int32 ConstEnd = ConstIndex + FCString::Strlen(TEXT("const"));
			int32 NameStart = ConstEnd;
			while (NameStart < CodeWithoutComments.Len() && FChar::IsWhitespace(CodeWithoutComments[NameStart]))
			{
				++NameStart;
			}
			if (NameStart == ConstEnd)
			{
				SearchIndex = ConstEnd;
				continue;
			}

			const FString AssetPathName = TEXT("AssetPath");
			if (CodeWithoutComments.Mid(NameStart, AssetPathName.Len()) != AssetPathName)
			{
				SearchIndex = ConstIndex + FCString::Strlen(TEXT("const"));
				continue;
			}

			int32 AfterName = NameStart + AssetPathName.Len();
			while (AfterName < CodeWithoutComments.Len() && FChar::IsWhitespace(CodeWithoutComments[AfterName]))
			{
				++AfterName;
			}

			if (AfterName >= CodeWithoutComments.Len() || CodeWithoutComments[AfterName] != TEXT('='))
			{
				SearchIndex = ConstIndex + FCString::Strlen(TEXT("const"));
				continue;
			}

			EqualIndex = AfterName;
			break;
		}

		const int32 DoubleQuoteIndex = CodeWithoutComments.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, EqualIndex);
		const int32 SingleQuoteIndex = CodeWithoutComments.Find(TEXT("'"), ESearchCase::CaseSensitive, ESearchDir::FromStart, EqualIndex);
		int32 QuoteStart = DoubleQuoteIndex;
		TCHAR QuoteChar = '"';
		if (QuoteStart == INDEX_NONE || (SingleQuoteIndex != INDEX_NONE && SingleQuoteIndex < QuoteStart))
		{
			QuoteStart = SingleQuoteIndex;
			QuoteChar = '\'';
		}

		if (QuoteStart == INDEX_NONE)
		{
			return false;
		}

		const int32 ValueStart = QuoteStart + 1;
		const int32 ValueEnd = CodeWithoutComments.Find(FString::Chr(QuoteChar), ESearchCase::CaseSensitive, ESearchDir::FromStart, ValueStart);
		if (ValueEnd == INDEX_NONE)
		{
			return false;
		}

		OutAssetPath = CodeWithoutComments.Mid(ValueStart, ValueEnd - ValueStart);
		OutAssetPath.TrimStartAndEndInline();
		return !OutAssetPath.IsEmpty();
	}

	bool TryExtractMixinBlueprintTypeFromCode(const FString& CodeWithoutComments, FString& OutBlueprintType)
	{
		int32 SearchIndex = 0;
		while (true)
		{
			const int32 InterfaceIndex = CodeWithoutComments.Find(TEXT("export interface"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchIndex);
			if (InterfaceIndex == INDEX_NONE)
			{
				return false;
			}

			const int32 InterfaceEnd = CodeWithoutComments.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, InterfaceIndex);
			const int32 ExtendsIndex = CodeWithoutComments.Find(TEXT("extends"), ESearchCase::CaseSensitive, ESearchDir::FromStart, InterfaceIndex);
			if (ExtendsIndex != INDEX_NONE && (InterfaceEnd == INDEX_NONE || ExtendsIndex < InterfaceEnd))
			{
				int32 TypeStart = ExtendsIndex + FCString::Strlen(TEXT("extends"));
				while (TypeStart < CodeWithoutComments.Len() && FChar::IsWhitespace(CodeWithoutComments[TypeStart]))
				{
					++TypeStart;
				}

				int32 TypeEnd = TypeStart;
				while (TypeEnd < CodeWithoutComments.Len()
					&& !FChar::IsWhitespace(CodeWithoutComments[TypeEnd])
					&& CodeWithoutComments[TypeEnd] != TEXT('{')
					&& CodeWithoutComments[TypeEnd] != TEXT(',')
					&& CodeWithoutComments[TypeEnd] != TEXT('>'))
				{
					++TypeEnd;
				}

				OutBlueprintType = CodeWithoutComments.Mid(TypeStart, TypeEnd - TypeStart);
				OutBlueprintType.TrimStartAndEndInline();
				return !OutBlueprintType.IsEmpty();
			}

			SearchIndex = InterfaceIndex + FCString::Strlen(TEXT("export interface"));
		}
	}

	bool TryParseAssetPath(const FString& AssetPath, FAssetPathInfo& OutInfo)
	{
		FString NormalizedAssetPath = AssetPath;
		NormalizedAssetPath.TrimStartAndEndInline();
		NormalizedAssetPath.ReplaceInline(TEXT("\\"), TEXT("/"));

		const int32 DotIndex = NormalizedAssetPath.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromStart);
		if (DotIndex == INDEX_NONE)
		{
			return false;
		}

		const FString PackagePath = NormalizedAssetPath.Left(DotIndex);
		if (PackagePath.Len() <= 1 || !PackagePath.StartsWith(TEXT("/")))
		{
			return false;
		}

		FString PackageLookupPath = NormalizedAssetPath;
		PackageLookupPath.RemoveFromEnd(TEXT("_C"));
		const FString BlueprintPackageName = FPackageName::ObjectPathToPackageName(PackageLookupPath);
		if (BlueprintPackageName.IsEmpty())
		{
			return false;
		}

		OutInfo.BlueprintPackageName = BlueprintPackageName;
		FString ActualPath;
		if (!TryBuildImportPathFromObjectPath(NormalizedAssetPath, ActualPath, OutInfo.ImportPath))
		{
			return false;
		}
		OutInfo.BlueprintType = TEXT("UE") + NormalizedAssetPath.Replace(TEXT("/"), TEXT("."));
		return !OutInfo.ImportPath.IsEmpty() && !OutInfo.BlueprintType.IsEmpty();
	}

	FString MakeRelativeImportPathForTsFile(const FString& TsFilePath, const FString& TypeScriptRootPath)
	{
		FString RelativePath = TsFilePath;
		FPaths::NormalizeFilename(RelativePath);
		FString RootPath = TypeScriptRootPath / TEXT("");
		FPaths::NormalizeFilename(RootPath);
		FPaths::MakePathRelativeTo(RelativePath, *RootPath);
		RelativePath.RemoveFromEnd(TEXT(".ts"));
		return NormalizeImportPath(RelativePath);
	}

	void AddIssue(TArray<FString>& Issues, const FString& Issue)
	{
		if (!Issues.Contains(Issue))
		{
			Issues.Add(Issue);
		}
	}
}
