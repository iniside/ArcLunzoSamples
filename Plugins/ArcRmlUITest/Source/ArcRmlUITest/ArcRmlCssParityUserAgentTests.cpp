// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityUserAgentTestsPrivate
{
	struct FBoxCase
	{
		const TCHAR* Id;
		float X;
		float Y;
		float Width;
		float Height;
	};

	const FBoxCase ChromeDefaultBoxes[] = {
		{TEXT("ua-h1"), 8.0f, 21.4375f, 1264.0f, 40.0f},
		{TEXT("ua-h2"), 8.0f, 82.875f, 1264.0f, 31.0f},
		{TEXT("ua-h6"), 8.0f, 138.8438f, 1264.0f, 14.0f},
		{TEXT("ua-p"), 8.0f, 177.8125f, 1264.0f, 21.0f},
		{TEXT("ua-ul"), 8.0f, 214.8125f, 1264.0f, 21.0f},
		{TEXT("ua-li"), 48.0f, 214.8125f, 1224.0f, 21.0f},
		{TEXT("ua-ol"), 8.0f, 251.8125f, 1264.0f, 42.0f},
		{TEXT("ua-ol-li"), 48.0f, 251.8125f, 1224.0f, 42.0f},
		{TEXT("ua-nested-ul"), 48.0f, 272.8125f, 1224.0f, 21.0f},
		{TEXT("ua-nested-li"), 88.0f, 272.8125f, 1184.0f, 21.0f},
		{TEXT("ua-after"), 8.0f, 355.8125f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeTableBoxes[] = {
		{TEXT("ua-table"), 8.0f, 309.8125f, 41.0312f, 46.0f},
		{TEXT("ua-th"), 8.0f, 309.8125f, 41.0312f, 23.0f},
		{TEXT("ua-td"), 8.0f, 332.8125f, 41.0312f, 23.0f},
	};

	const FBoxCase ChromeShownToggle = {TEXT("ua-toggle"), 8.0f, 376.8125f, 1264.0f, 21.0f};

	const FBoxCase ChromePreflightBoxes[] = {
		{TEXT("pf-ul"), 8.0f, 16.0f, 1264.0f, 21.0f},
		{TEXT("pf-li"), 8.0f, 16.0f, 1264.0f, 21.0f},
		{TEXT("pf-p"), 8.0f, 53.0f, 1264.0f, 21.0f},
		{TEXT("pf-after"), 8.0f, 90.0f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeAloneBoxes[] = {
		{TEXT("uat-top"), 8.0f, 8.0f, 1264.0f, 21.0f},
		{TEXT("uat-h2"), 8.0f, 48.9062f, 1264.0f, 31.0f},
		{TEXT("uat-p"), 8.0f, 99.8125f, 1264.0f, 21.0f},
	};

	const FVector2f EmbedOrigin(100.0f, 50.0f);
	constexpr float EmbedWidth = 600.0f;
	constexpr float AloneWidth = 1280.0f;
	constexpr float Tolerance = 1.0f;

	const TCHAR* const DumpedFixtures[] = {
		TEXT("CssParity/UserAgent/UserAgentTest.rml"),
		TEXT("CssParity/UserAgent/UserAgentPreflightTest.rml"),
		TEXT("CssParity/UserAgent/UserAgentAloneTest.rml"),
		TEXT("Drag/DragUserAgentTest.rml"),
	};

	FString DescribeMismatch(const TArray<FArcRmlTestLayoutElement>& Layout, const FBoxCase& Case)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
		if (!Element)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
		}
		const FArcRmlInspectorRect& Rect = Element->Rect;
		if (FMath::Abs(Rect.Position.X - Case.X) > Tolerance || FMath::Abs(Rect.Position.Y - Case.Y) > Tolerance
			|| FMath::Abs(Rect.Size.X - Case.Width) > Tolerance || FMath::Abs(Rect.Size.Y - Case.Height) > Tolerance)
		{
			return FString::Printf(TEXT("[%s is %.2f,%.2f %.2fx%.2f, Chrome %.2f,%.2f %.2fx%.2f] "), Case.Id, Rect.Position.X, Rect.Position.Y, Rect.Size.X,
				Rect.Size.Y, Case.X, Case.Y, Case.Width, Case.Height);
		}
		return FString();
	}

	template <int32 N>
	FString DescribeMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, const FBoxCase (&Cases)[N])
	{
		FString Mismatches;
		for (const FBoxCase& Case : Cases)
		{
			Mismatches += DescribeMismatch(Layout, Case);
		}
		return Mismatches;
	}

	FString DescribeHidden(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* Id)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Id);
		if (!Element)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Id);
		}
		if (Element->bVisible || Element->Rect.Size.X != 0.0f || Element->Rect.Size.Y != 0.0f)
		{
			return FString::Printf(TEXT("[%s is shown with %.2fx%.2f, Chrome display: none] "), Id, Element->Rect.Size.X, Element->Rect.Size.Y);
		}
		return FString();
	}
}

TEST_CLASS(ArcRmlCssParityUserAgentTests, "ArcRmlUI.CssParity.UserAgent")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Relative)
	{
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(Relative)) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout);
	}

	TEST_METHOD(UnstyledHeadingsParagraphsAndLists_TakeTheHtmlDefaultsAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/UserAgent/UserAgentTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeDefaultBoxes)));
	}

	TEST_METHOD(Table_TakesTheHtmlTableDefaultsAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/UserAgent/UserAgentTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeTableBoxes)));
	}

	TEST_METHOD(HiddenAttributeAndImportantUserAgentRule_BeatAnImportantAuthorRuleAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/UserAgent/UserAgentTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeHidden(Layout, TEXT("ua-hidden")), TEXT("the [hidden] user-agent rule does not hide the element")));
		ASSERT_THAT(AreEqual(FString(), DescribeHidden(Layout, TEXT("ua-hidden-input")),
			TEXT("an author display: block !important beats the user-agent input[type=hidden] display: none !important")));
		ASSERT_THAT(AreEqual(FString(), DescribeHidden(Layout, TEXT("ua-toggle")), TEXT("the bound hidden attribute does not hide the element")));
	}

	TEST_METHOD(ImportantUserAgentRule_BeatsAnImportantInlineStyleAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/UserAgent/UserAgentTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeHidden(Layout, TEXT("ua-hidden-inline")),
			TEXT("an inline display: block !important beats the user-agent input[type=hidden] display: none !important")));
	}

	TEST_METHOD(HiddenAttributeRemovedAtRuntime_ShowsTheElementAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/UserAgent/UserAgentTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeHidden(Layout, TEXT("ua-toggle"))));
		View.SetModelValue(TEXT("ua"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("hideToggle")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Layout)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatch(Layout, ChromeShownToggle), TEXT("removing the hidden attribute does not restyle the element")));
	}

	TEST_METHOD(PreflightPaddingReset_BeatsTheUserAgentListPaddingAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/UserAgent/UserAgentPreflightTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromePreflightBoxes)));
	}

	TEST_METHOD(DocumentWithoutStyleSheets_TakesTheUserAgentDefaultsAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/UserAgent/UserAgentAloneTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeAloneBoxes)));
	}

	TEST_METHOD(Tab_TakesTheUserAgentDefaultsAsOpenedAlone)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("CssParity/UserAgent/UserAgentFrame.rml"))),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(TEXT("CssParity/UserAgent/UserAgentTab.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout)));
		FString Mismatches;
		for (const FBoxCase& Alone : ChromeAloneBoxes)
		{
			const FBoxCase InTab = {Alone.Id, EmbedOrigin.X + Alone.X, EmbedOrigin.Y + Alone.Y, Alone.Width - (AloneWidth - EmbedWidth), Alone.Height};
			Mismatches += DescribeMismatch(Layout, InTab);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(ReferencePage_LeavesChromeItsOwnUserAgentRules)
	{
		using namespace ArcRmlCssParityUserAgentTestsPrivate;
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("CssParityUserAgentDump")));
		IFileManager::Get().DeleteDirectory(*Dir, false, true);
		for (const TCHAR* Fixture : DumpedFixtures)
		{
			ASSERT_THAT(IsTrue(OpenFixture(Fixture), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			View.DumpLayout(Dir);
			ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
			FString Html;
			const FString Name = FPaths::GetBaseFilename(Fixture);
			ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(Html, *FPaths::Combine(Dir, Name + TEXT(".html"))), *FString::Printf(TEXT("the layout dump wrote no html for %s"), *Name)));
			ASSERT_THAT(IsFalse(Html.Contains(TEXT("all: unset")), *FString::Printf(TEXT("the reference page of %s still resets Chrome's user-agent rules"), *Name)));
			View.Close();
		}
	}
};
