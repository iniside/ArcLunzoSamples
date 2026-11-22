// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParitySelectorTestsPrivate
{
	struct FChromeBox
	{
		const TCHAR* Id;
		float Y;
		float Height;
	};

	struct FChromeColour
	{
		const TCHAR* Id;
		FIntVector4 Chrome;
	};

	const FChromeBox ChromeSpaceBoxes[] = {
		{TEXT("space-a"), 0.0f, 10.0f},
		{TEXT("space-b"), 26.0f, 10.0f},
		{TEXT("space-c"), 52.0f, 10.0f},
		{TEXT("space-half-a"), 62.0f, 10.0f},
		{TEXT("space-half-b"), 78.0f, 10.0f},
		{TEXT("space-half-c"), 94.0f, 10.0f},
	};

	const FChromeBox ChromeDivideBoxes[] = {
		{TEXT("divide-a"), 104.0f, 11.0f},
		{TEXT("divide-b"), 115.0f, 11.0f},
		{TEXT("divide-c"), 126.0f, 10.0f},
	};

	const FChromeColour ChromeDivideBorderColours[] = {
		{TEXT("divide-a"), FIntVector4(0, 170, 85, 255)},
		{TEXT("divide-b"), FIntVector4(0, 170, 85, 255)},
	};

	const FIntVector4 ChromeGrey(128, 128, 128, 255);
	const FIntVector4 ChromeBrand(51, 102, 204, 255);
	const FIntVector4 ChromeAccent(204, 51, 102, 255);

	const FChromeColour ChromeHoveredBackgrounds[] = {
		{TEXT("group-target"), ChromeBrand},
		{TEXT("group-idle-target"), ChromeGrey},
	};

	const FChromeColour ChromeUnhoveredBackgrounds[] = {
		{TEXT("group-target"), ChromeGrey},
		{TEXT("group-idle-target"), ChromeGrey},
	};

	const FChromeColour ChromeOpenedPeerBackgrounds[] = {
		{TEXT("peer-target"), ChromeAccent},
		{TEXT("peer-gap"), ChromeGrey},
		{TEXT("peer-far-target"), ChromeAccent},
		{TEXT("peer-off-gap"), ChromeGrey},
		{TEXT("peer-off-far-target"), ChromeGrey},
	};

	const FChromeColour ChromeToggledPeerBackgrounds[] = {
		{TEXT("peer-target"), ChromeGrey},
		{TEXT("peer-gap"), ChromeGrey},
		{TEXT("peer-far-target"), ChromeGrey},
		{TEXT("peer-off-gap"), ChromeGrey},
		{TEXT("peer-off-far-target"), ChromeAccent},
	};

	const FChromeColour ChromeSpecificityBackgrounds[] = {
		{TEXT("where-p"), FIntVector4(0, 255, 0, 255)},
		{TEXT("is-span"), FIntVector4(0, 0, 255, 255)},
		{TEXT("nest-n3"), FIntVector4(255, 128, 0, 255)},
		{TEXT("nest-n1"), ChromeGrey},
		{TEXT("forgiving"), FIntVector4(0, 128, 128, 255)},
		{TEXT("esc-plain"), FIntVector4(160, 82, 45, 255)},
		{TEXT("esc-md"), ChromeGrey},
	};

	const FChromeColour ChromeNestedBorderColours[] = {
		{TEXT("nest-n3"), FIntVector4(128, 0, 255, 255)},
		{TEXT("where-p"), FIntVector4(255, 0, 170, 255)},
	};

	constexpr float BoxTolerance = 0.5f;
	constexpr int32 ColourTolerance = 1;
	const FVector2f AwayPoint(1000.0f, 700.0f);

	FString IsWherePath()
	{
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Selectors/IsWhereTest.rml"));
	}

	FString PeerCheckedPath()
	{
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Selectors/PeerCheckedTest.rml"));
	}

	void DropCompiledFixtures()
	{
		FArcRmlTestView::DropCompiledDocument(IsWherePath());
		FArcRmlTestView::DropCompiledDocument(PeerCheckedPath());
		const FString Dir = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/Selectors"));
		IFileManager::Get().Delete(*FPaths::Combine(Dir, TEXT("IsWhere.rcssc")), false, false, true);
		IFileManager::Get().Delete(*FPaths::Combine(Dir, TEXT("PeerChecked.rcssc")), false, false, true);
		FArcRmlTestView::ClearDocumentCaches();
	}

	bool ParseHex(const FString& Text, FIntVector4& OutColour)
	{
		if (Text.Len() != 9 || !Text.StartsWith(TEXT("#")))
		{
			return false;
		}
		OutColour = FIntVector4(FParse::HexNumber(*Text.Mid(1, 2)), FParse::HexNumber(*Text.Mid(3, 2)), FParse::HexNumber(*Text.Mid(5, 2)),
			FParse::HexNumber(*Text.Mid(7, 2)));
		return true;
	}

	FString DescribeBoxMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FChromeBox> Boxes)
	{
		FString Failures;
		for (const FChromeBox& Box : Boxes)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Box.Id);
			if (!Element)
			{
				Failures += FString::Printf(TEXT("[%s missing] "), Box.Id);
				continue;
			}
			if (FMath::Abs(Element->Rect.Position.Y - Box.Y) > BoxTolerance || FMath::Abs(Element->Rect.Size.Y - Box.Height) > BoxTolerance)
			{
				Failures += FString::Printf(TEXT("[%s y %.2f height %.2f, Chrome y %.2f height %.2f] "), Box.Id, Element->Rect.Position.Y, Element->Rect.Size.Y,
					Box.Y, Box.Height);
			}
		}
		return Failures;
	}

	FString DescribeColourMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FChromeColour> Colours, bool bBorderTop)
	{
		FString Failures;
		for (const FChromeColour& Expected : Colours)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Expected.Id);
			FIntVector4 Colour(-1, -1, -1, -1);
			const FString Text = Element ? (bBorderTop ? Element->BorderTopColor : Element->BackgroundColor) : FString();
			if (!ParseHex(Text, Colour))
			{
				Failures += FString::Printf(TEXT("[%s has no colour] "), Expected.Id);
				continue;
			}
			const FIntVector4 Difference = Colour - Expected.Chrome;
			if (FMath::Abs(Difference.X) > ColourTolerance || FMath::Abs(Difference.Y) > ColourTolerance || FMath::Abs(Difference.Z) > ColourTolerance
				|| FMath::Abs(Difference.W) > ColourTolerance)
			{
				Failures += FString::Printf(TEXT("[%s is %s, Chrome %d,%d,%d,%d] "), Expected.Id, *Text, Expected.Chrome.X, Expected.Chrome.Y, Expected.Chrome.Z,
					Expected.Chrome.W);
			}
		}
		return Failures;
	}
}

TEST_CLASS(ArcRmlCssParityIsWhereTests, "ArcRmlUI.CssParity.IsWhere")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	BEFORE_EACH()
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		DropCompiledFixtures();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, IsWherePath()) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	bool HoverGroupLead()
	{
		const FArcRmlTestLayoutElement* Target = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("group-lead"));
		if (!Target)
		{
			return false;
		}
		View.MouseMove(Target->Rect.Position + Target->Rect.Size * 0.5f);
		return View.FlushUntilSettled() && View.ParseLayout(Layout);
	}

	TEST_METHOD(SpaceY4AndEscapedSpaceY1Point5_SpaceTheChildrenAsChrome)
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		const FString Failures = DescribeBoxMismatches(Layout, ChromeSpaceBoxes);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(DivideY_BordersEveryChildButTheLastAsChrome)
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		const FString Failures = DescribeBoxMismatches(Layout, ChromeDivideBoxes) + DescribeColourMismatches(Layout, ChromeDivideBorderColours, true);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(GroupHoverOnAnotherChild_ColoursOnlyTheHoveredGroupsTargetAsChrome)
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		ASSERT_THAT(IsTrue(HoverGroupLead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Failures = DescribeColourMismatches(Layout, ChromeHoveredBackgrounds, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(WhereHasZeroSpecificityIsTheMostSpecificArgumentsNestingForgivingListsAndEscapedNotArguments_AsChrome)
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		const FString Failures =
			DescribeColourMismatches(Layout, ChromeSpecificityBackgrounds, false) + DescribeColourMismatches(Layout, ChromeNestedBorderColours, true);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(HoverEnteringAndLeavingAnotherChildOfTheGroupAtRuntime_RestylesTheTargetThroughWhereAsChrome)
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		const FString Before = DescribeColourMismatches(Layout, ChromeUnhoveredBackgrounds, false);
		ASSERT_THAT(IsTrue(Before.IsEmpty(), Before));
		ASSERT_THAT(IsTrue(HoverGroupLead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Hovered = DescribeColourMismatches(Layout, ChromeHoveredBackgrounds, false);
		ASSERT_THAT(IsTrue(Hovered.IsEmpty(), Hovered));
		View.MouseMove(AwayPoint);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Left = DescribeColourMismatches(Layout, ChromeUnhoveredBackgrounds, false);
		ASSERT_THAT(IsTrue(Left.IsEmpty(), Left));
	}

};

TEST_CLASS(ArcRmlCssParityPeerCheckedTests, "ArcRmlUI.CssParity.PeerChecked")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	BEFORE_EACH()
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		DropCompiledFixtures();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, PeerCheckedPath()) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(PeerChecked_ColoursEveryLaterSiblingOfTheCheckedPeerAsChrome)
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		const FString Failures = DescribeColourMismatches(Layout, ChromeOpenedPeerBackgrounds, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CheckboxesToggledAtRuntime_RestyleTheirLaterSiblingsThroughWhereAsChrome)
	{
		using namespace ArcRmlCssParitySelectorTestsPrivate;
		View.ClickElement(TEXT("#peer-off"));
		View.ClickElement(TEXT("#peer"));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Failures = DescribeColourMismatches(Layout, ChromeToggledPeerBackgrounds, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}
};

// Every element is left-to-right. Expected widths come from Chrome on the DirTest fixture.
TEST_CLASS(ArcRmlCssParityDirTests, "ArcRmlUI.CssParity.Dir")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	FString DescribeWidthMismatches(TConstArrayView<TPair<const TCHAR*, float>> Expected)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Selectors/DirTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/Selectors/Dir.rcssc")), false, false, true);
		FArcRmlTestView::ClearDocumentCaches();
		TArray<FArcRmlTestLayoutElement> Layout;
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout)))
		{
			return FString::Printf(TEXT("DirTest did not open: %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View));
		}
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("Dir"))));
		if (!(View.Flush() && View.Flush()))
		{
			return TEXT("DirTest did not write its layout dump");
		}
		FString Failures;
		for (const TPair<const TCHAR*, float>& Entry : Expected)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Entry.Key);
			if (Element == nullptr)
			{
				Failures += FString::Printf(TEXT("[%s missing] "), Entry.Key);
			}
			else if (FMath::Abs(Element->Rect.Size.X - Entry.Value) > 0.5f)
			{
				Failures += FString::Printf(TEXT("[%s width %.2f, Chrome %.2f] "), Entry.Key, Element->Rect.Size.X, Entry.Value);
			}
		}
		return Failures;
	}

	TEST_METHOD(DirLtrMatchesAndDirRtlOrAnotherIdentifierMatchesNothing)
	{
		ASSERT_THAT(AreEqual(FString(), DescribeWidthMismatches({
			{ TEXT("tw-ltr"), 50.0f },
			{ TEXT("tw-rtl"), 40.0f },
			{ TEXT("plain-ltr"), 50.0f },
			{ TEXT("plain-rtl"), 40.0f },
			{ TEXT("upper"), 50.0f },
			{ TEXT("other"), 40.0f },
		})));
	}

	// Dir.rcss line 21 is ".bad:dir(1)", line 22 ".bad-string:dir(\"ltr\")".
	FString DescribeInvalidSelectorMismatches()
	{
		FString Failures;
		const TArray<FArcRmlDiagnostic>& Diagnostics = View.GetViewDiagnostics().Diagnostics;
		for (const int32 Line : { 21, 22 })
		{
			int32 Found = 0;
			for (const FArcRmlDiagnostic& Diagnostic : Diagnostics)
			{
				if (Diagnostic.Kind == EArcRmlDiagnosticKind::InvalidSelector && Diagnostic.Line == Line && Diagnostic.File.EndsWith(TEXT("Dir.rcss")))
				{
					++Found;
				}
			}
			if (Found != 1)
			{
				Failures += FString::Printf(TEXT("[Dir.rcss line %d: %d invalid-selector diagnostics, expected 1] "), Line, Found);
			}
		}
		for (const FArcRmlDiagnostic& Diagnostic : Diagnostics)
		{
			if (Diagnostic.Kind == EArcRmlDiagnosticKind::UnknownPseudoClass && Diagnostic.Message.Contains(TEXT(":dir")))
			{
				Failures += FString::Printf(TEXT("[unknown pseudo-class at line %d: %s] "), Diagnostic.Line, *Diagnostic.Message);
			}
		}
		return Failures;
	}

	TEST_METHOD(DirWithoutAnIdentifier_IsAnInvalidSelector)
	{
		ASSERT_THAT(AreEqual(FString(), DescribeWidthMismatches({
			{ TEXT("bad"), 40.0f },
			{ TEXT("bad-string"), 40.0f },
			{ TEXT("forgive"), 50.0f },
		})));
		ASSERT_THAT(AreEqual(FString(), DescribeInvalidSelectorMismatches(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(DirCountsAsAPseudoClassAndZeroInsideWhere)
	{
		ASSERT_THAT(AreEqual(FString(), DescribeWidthMismatches({
			{ TEXT("spec"), 60.0f },
			{ TEXT("zero"), 50.0f },
		})));
	}
};
