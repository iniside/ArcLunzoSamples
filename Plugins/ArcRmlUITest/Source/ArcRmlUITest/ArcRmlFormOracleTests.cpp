// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace ArcRmlFormOracleTestsPrivate
{
	const TCHAR* const Fixture = TEXT("Form/FormOracleProbe.rml");
	const TCHAR* const DumpName = TEXT("FormOracleProbe");
	// DefaultGame.ini GenericFontFamilies: sans-serif is Space Grotesk (also the fallback), monospace JetBrains Mono.
	const TCHAR* const ControlFamilyRule = TEXT(":where(input, select, button) { font-family: \"Space Grotesk\"; }");
	const TCHAR* const TextAreaFamilyRule = TEXT(":where(textarea) { font-family: \"JetBrains Mono\", \"Space Grotesk\"; }");
	const TCHAR* const DefaultsStyle = TEXT("<style data-arc-source=\"rmlui-defaults\">");

	FString GetDumpDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("FormOracle")));
	}

	// The html between the open tag of the element with DomId and its closing tag; empty when either is missing.
	FString GetInnerHtml(const FString& Html, const TCHAR* DomId, const TCHAR* Tag)
	{
		const int32 IdAt = Html.Find(FString::Printf(TEXT(" id=\"%s\""), DomId), ESearchCase::CaseSensitive);
		if (IdAt == INDEX_NONE)
		{
			return FString();
		}
		const int32 OpenEnd = Html.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, IdAt);
		const int32 CloseAt = Html.Find(FString::Printf(TEXT("</%s>"), Tag), ESearchCase::CaseSensitive, ESearchDir::FromStart, IdAt);
		if (OpenEnd == INDEX_NONE || CloseAt == INDEX_NONE || CloseAt < OpenEnd)
		{
			return FString();
		}
		return Html.Mid(OpenEnd + 1, CloseAt - OpenEnd - 1);
	}

	FString GetStyleBlock(const FString& Html, const FString& OpenTag)
	{
		const int32 OpenAt = Html.Find(OpenTag, ESearchCase::CaseSensitive);
		if (OpenAt == INDEX_NONE)
		{
			return FString();
		}
		const int32 CloseAt = Html.Find(TEXT("</style>"), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenAt);
		return CloseAt == INDEX_NONE ? FString() : Html.Mid(OpenAt, CloseAt - OpenAt);
	}

	struct FJsonNode
	{
		FString Id;
		FString Parent;
		FString Tag;
		FString DomId;
		bool bDom = true;
	};

	bool ReadNodes(const FString& Json, TArray<FJsonNode>& OutNodes)
	{
		TSharedPtr<FJsonObject> Root;
		const TArray<TSharedPtr<FJsonValue>>* Elements = nullptr;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid() || !Root->TryGetArrayField(TEXT("elements"), Elements))
		{
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Elements)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value->TryGetObject(Object))
			{
				return false;
			}
			FJsonNode& Node = OutNodes.AddDefaulted_GetRef();
			(*Object)->TryGetStringField(TEXT("id"), Node.Id);
			(*Object)->TryGetStringField(TEXT("parent"), Node.Parent);
			(*Object)->TryGetStringField(TEXT("tag"), Node.Tag);
			(*Object)->TryGetStringField(TEXT("dom_id"), Node.DomId);
			(*Object)->TryGetBoolField(TEXT("dom"), Node.bDom);
		}
		return true;
	}

	// Each child of DomId must have bExpectedDom; a control without children is a mismatch, as the check would prove nothing.
	FString DescribeChildDom(const TArray<FJsonNode>& Nodes, const TCHAR* DomId, bool bExpectedDom)
	{
		const FJsonNode* Owner = Nodes.FindByPredicate([DomId](const FJsonNode& Node) -> bool
		{
			return Node.DomId == DomId;
		});
		if (!Owner)
		{
			return FString::Printf(TEXT("[%s is not in the dump json] "), DomId);
		}
		FString Mismatches;
		int32 NumChildren = 0;
		for (const FJsonNode& Node : Nodes)
		{
			if (Node.Parent != Owner->Id)
			{
				continue;
			}
			++NumChildren;
			if (Node.bDom != bExpectedDom)
			{
				Mismatches += FString::Printf(TEXT("[%s child %s (%s) has dom:%s, expected dom:%s] "), DomId, *Node.Id, *Node.Tag,
					Node.bDom ? TEXT("true") : TEXT("false"), bExpectedDom ? TEXT("true") : TEXT("false"));
			}
		}
		if (NumChildren == 0)
		{
			Mismatches += FString::Printf(TEXT("[%s has no child in the dump json, the fork's value text is gone] "), DomId);
		}
		return Mismatches;
	}
}

// The Chrome oracle page for form controls (rml-forms step 1): ArcRmlLayoutDump.cpp writes a select's options, drops a button
// input's value text and maps the control families. The dump is read back from disk, as chrome_layout_compare.py reads it.
TEST_CLASS(ArcRmlFormOracleTests, "ArcRmlUI.Form.Oracle")
{
	FArcRmlTestView View;
	FString Html;
	FString Json;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenAndDump()
	{
		using namespace ArcRmlFormOracleTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), TEXT("FormOracleProbe.rcssc")), false, true, true);
		const FString Dir = GetDumpDir();
		IFileManager::Get().DeleteDirectory(*Dir, false, true);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		View.DumpLayout(Dir);
		return View.Flush() && View.Flush()
			&& FFileHelper::LoadFileToString(Html, *FPaths::Combine(Dir, FString(DumpName) + TEXT(".html")))
			&& FFileHelper::LoadFileToString(Json, *FPaths::Combine(Dir, FString(DumpName) + TEXT(".layout.json")));
	}

	TEST_METHOD(Select_WritesItsOptionsWithValueSelectedDisabledAndText)
	{
		using namespace ArcRmlFormOracleTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAndDump(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		const FString Options = GetInnerHtml(Html, TEXT("fo-select"), TEXT("select"));
		const FString Expected = TEXT("<option value=\"a\">Apple</option><option value=\"b\" selected>Banana split</option>")
			TEXT("<option value=\"c\" disabled>Cherry</option>");
		if (Options != Expected)
		{
			Mismatches += FString::Printf(TEXT("[fo-select holds '%s', expected '%s'] "), *Options, *Expected);
		}
		const FString Short = GetInnerHtml(Html, TEXT("fo-select-short"), TEXT("select"));
		// The widget marks the option it shows: with none marked it selects the first and gives it the attribute.
		if (Short != TEXT("<option value=\"x\" selected>cd</option>"))
		{
			Mismatches += FString::Printf(TEXT("[fo-select-short holds '%s', expected its one option cd, selected by the widget] "), *Short);
		}
		// The selectbox also holds the hidden data-for template and the whitespace between the multi-line tags.
		const FString ForRows = GetInnerHtml(Html, TEXT("fo-select-for"), TEXT("select"));
		const FString ExpectedForRows = TEXT("<option value=\"r1\" selected>Ash</option><option value=\"r2\">Birch</option>");
		if (ForRows != ExpectedForRows)
		{
			Mismatches += FString::Printf(TEXT("[fo-select-for holds '%s', expected the two preview rows '%s'] "), *ForRows, *ExpectedForRows);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(SelectOptions_CarryNoArcId)
	{
		ASSERT_THAT(IsTrue(OpenAndDump(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		int32 NumOptions = 0;
		for (int32 At = Html.Find(TEXT("<option"), ESearchCase::CaseSensitive); At != INDEX_NONE;
			At = Html.Find(TEXT("<option"), ESearchCase::CaseSensitive, ESearchDir::FromStart, At + 1))
		{
			++NumOptions;
			const int32 EndAt = Html.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, At);
			const FString StartTag = EndAt == INDEX_NONE ? Html.Mid(At) : Html.Mid(At, EndAt - At + 1);
			if (StartTag.Contains(TEXT("data-arc-id"), ESearchCase::CaseSensitive))
			{
				Mismatches += FString::Printf(TEXT("[the option %s carries data-arc-id; a closed select's options have no box in Chrome] "), *StartTag);
			}
		}
		// fo-select 3, fo-select-short 1, fo-select-for 2.
		if (NumOptions != 6)
		{
			Mismatches += FString::Printf(TEXT("[the dump html has %d option start tags, expected 6] "), NumOptions);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(ButtonInputs_HaveNoValueTextInTheHtml)
	{
		using namespace ArcRmlFormOracleTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAndDump(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const TCHAR* Text : {TEXT("-->Save"), TEXT("-->Send"), TEXT("-->Clear")})
		{
			if (Html.Contains(Text, ESearchCase::CaseSensitive))
			{
				Mismatches += FString::Printf(TEXT("[the dump html has the button input's value text %s] "), Text);
			}
		}
		if (Html.Contains(TEXT("</input>"), ESearchCase::CaseSensitive))
		{
			Mismatches += TEXT("[the dump html closes an input, it wrote children into it] ");
		}
		for (const TCHAR* Value : {TEXT("value=\"Save\""), TEXT("value=\"Send\""), TEXT("value=\"Clear\"")})
		{
			if (!Html.Contains(Value, ESearchCase::CaseSensitive))
			{
				Mismatches += FString::Printf(TEXT("[the dump html lost the button input's %s] "), Value);
			}
		}
		const FString Button = GetInnerHtml(Html, TEXT("fo-button"), TEXT("button"));
		if (!Button.EndsWith(TEXT("-->Go")))
		{
			Mismatches += FString::Printf(TEXT("[the button element holds '%s', expected its text Go] "), *Button);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(ButtonInputValueText_IsNotDomInTheJson)
	{
		using namespace ArcRmlFormOracleTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAndDump(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FJsonNode> Nodes;
		ASSERT_THAT(IsTrue(ReadNodes(Json, Nodes), TEXT("the dump json does not parse")));
		FString Mismatches;
		Mismatches += DescribeChildDom(Nodes, TEXT("fo-ibutton"), false);
		Mismatches += DescribeChildDom(Nodes, TEXT("fo-submit"), false);
		Mismatches += DescribeChildDom(Nodes, TEXT("fo-button"), true);
		// A reset input is a button input (VENDORED 224): its value text is dom:false like a submit's.
		Mismatches += DescribeChildDom(Nodes, TEXT("fo-reset"), false);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(ControlFamilies_MapToTheConfiguredFamilies)
	{
		using namespace ArcRmlFormOracleTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAndDump(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		const FString Defaults = GetStyleBlock(Html, DefaultsStyle);
		for (const TCHAR* Rule : {ControlFamilyRule, TextAreaFamilyRule})
		{
			if (!Defaults.Contains(Rule, ESearchCase::CaseSensitive))
			{
				Mismatches += FString::Printf(TEXT("[the rmlui-defaults style has no '%s'] "), Rule);
			}
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches, Defaults));
	}
};
