// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityRootTestsPrivate
{
	struct FBoxCase
	{
		const TCHAR* Id;
		float X;
		float Y;
		float Width;
		float Height;
	};

	const FBoxCase ChromeRemBox = {TEXT("rem-box"), 0.0f, 0.0f, 100.0f, 40.0f};
	const FBoxCase ChromeLineHeightText = {TEXT("lh-text"), 0.0f, 40.0f, 1280.0f, 15.0f};
	const FBoxCase ChromeDarkParagraph = {TEXT("dark-p"), 0.0f, 55.0f, 1280.0f, 15.0f};
	const FBoxCase ChromeRootChild = {TEXT("root-child"), 0.0f, 70.0f, 70.0f, 10.0f};

	const TCHAR* const ChromeDarkColour = TEXT("#ff0000ff");
	const TCHAR* const ChromeTextColour = TEXT("#000000ff");
	constexpr float Tolerance = 1.0f;

	FString DescribeMismatch(const FArcRmlTestLayoutElement* Element, const FBoxCase& Case)
	{
		if (!Element)
		{
			return FString::Printf(TEXT("%s is not in the layout"), Case.Id);
		}
		const FArcRmlInspectorRect& Rect = Element->Rect;
		if (FMath::Abs(Rect.Position.X - Case.X) > Tolerance || FMath::Abs(Rect.Position.Y - Case.Y) > Tolerance
			|| FMath::Abs(Rect.Size.X - Case.Width) > Tolerance || FMath::Abs(Rect.Size.Y - Case.Height) > Tolerance)
		{
			return FString::Printf(TEXT("%s is %.2f,%.2f %.2fx%.2f, Chrome %.0f,%.0f %.0fx%.0f"), Case.Id, Rect.Position.X, Rect.Position.Y, Rect.Size.X, Rect.Size.Y,
				Case.X, Case.Y, Case.Width, Case.Height);
		}
		return FString();
	}

	bool HasClass(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* Path, const TCHAR* Class)
	{
		const FArcRmlTestLayoutElement* Element = Layout.FindByPredicate([Path](const FArcRmlTestLayoutElement& Candidate) -> bool
		{
			return Candidate.Path == Path;
		});
		TArray<FString> Classes;
		if (Element)
		{
			Element->Classes.ParseIntoArrayWS(Classes);
		}
		return Classes.Contains(Class);
	}
}

TEST_CLASS(ArcRmlCssParityRootTests, "ArcRmlUI.CssParity.Root")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	BEFORE_EACH()
	{
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Root/HtmlRootTest.rml")))
			&& View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(RemWithAnHtmlFontSizeRule_ReadsTheHtmlElementAsChrome)
	{
		using namespace ArcRmlCssParityRootTestsPrivate;
		ASSERT_THAT(AreEqual(FString(), DescribeMismatch(ArcRmlUITestHelpers::FindByDomId(Layout, ChromeRemBox.Id), ChromeRemBox)));
	}

	TEST_METHOD(RootSelector_MatchesTheHtmlParentOfBodyAsChrome)
	{
		using namespace ArcRmlCssParityRootTestsPrivate;
		ASSERT_THAT(AreEqual(FString(), DescribeMismatch(ArcRmlUITestHelpers::FindByDomId(Layout, ChromeRootChild.Id), ChromeRootChild)));
	}

	TEST_METHOD(HtmlAndHostLineHeightRule_IsInheritedByTextAsChrome)
	{
		using namespace ArcRmlCssParityRootTestsPrivate;
		ASSERT_THAT(AreEqual(FString(), DescribeMismatch(ArcRmlUITestHelpers::FindByDomId(Layout, ChromeLineHeightText.Id), ChromeLineHeightText)));
	}

	TEST_METHOD(ClassOnTheRmlRoot_MatchesADescendantRuleAsChrome)
	{
		using namespace ArcRmlCssParityRootTestsPrivate;
		const FArcRmlTestLayoutElement* Paragraph = ArcRmlUITestHelpers::FindByDomId(Layout, ChromeDarkParagraph.Id);
		ASSERT_THAT(AreEqual(FString(), DescribeMismatch(Paragraph, ChromeDarkParagraph)));
		ASSERT_THAT(IsTrue(HasClass(Layout, TEXT("0"), TEXT("dark")), TEXT("the html element (path 0) does not carry the rml root's class")));
		ASSERT_THAT(IsFalse(HasClass(Layout, TEXT("0.0"), TEXT("dark")), TEXT("the body element (path 0.0) carries the rml root's class")));
		ASSERT_THAT(IsTrue(Paragraph->Color.Equals(ChromeDarkColour, ESearchCase::IgnoreCase),
			FString::Printf(TEXT("the .dark p text is %s, Chrome %s"), *Paragraph->Color, ChromeDarkColour)));
		const FArcRmlTestLayoutElement* Plain = ArcRmlUITestHelpers::FindByDomId(Layout, ChromeLineHeightText.Id);
		ASSERT_THAT(IsNotNull(Plain));
		ASSERT_THAT(IsTrue(Plain->Color.Equals(ChromeTextColour, ESearchCase::IgnoreCase),
			FString::Printf(TEXT("the div outside the .dark p rule is %s, Chrome %s"), *Plain->Color, ChromeTextColour)));
	}
};

TEST_CLASS(ArcRmlCssParityBodyHeightTests, "ArcRmlUI.CssParity.BodyHeight")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	FString DescribePath(const TCHAR* Path, float Width, float Height)
	{
		const FArcRmlTestLayoutElement* Element = Layout.FindByPredicate([Path](const FArcRmlTestLayoutElement& Candidate) -> bool
		{
			return Candidate.Path == Path;
		});
		if (!Element)
		{
			return FString::Printf(TEXT("path %s is not in the layout"), Path);
		}
		const FArcRmlInspectorRect& Rect = Element->Rect;
		if (!FMath::IsNearlyZero(Rect.Position.X, 1.0f) || !FMath::IsNearlyZero(Rect.Position.Y, 1.0f) || !FMath::IsNearlyEqual(Rect.Size.X, Width, 1.0f)
			|| !FMath::IsNearlyEqual(Rect.Size.Y, Height, 1.0f))
		{
			return FString::Printf(TEXT("path %s is %.2f,%.2f %.2fx%.2f, Chrome 0,0 %.0fx%.0f"), Path, Rect.Position.X, Rect.Position.Y, Rect.Size.X, Rect.Size.Y, Width, Height);
		}
		return FString();
	}

	bool OpenFixture(const TCHAR* Relative)
	{
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(Relative)) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout);
	}

	TEST_METHOD(BodyPercentHeightUnderAnAutoHtml_IsItsContentAsChrome)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/Root/BodyHeightTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribePath(TEXT("0"), 1280.0f, 40.0f), TEXT("Chrome: html is as high as its content")));
		ASSERT_THAT(AreEqual(FString(), DescribePath(TEXT("0.0"), 1280.0f, 40.0f), TEXT("Chrome: body height 100% of an auto-height html is auto")));
	}

	TEST_METHOD(BodyPercentHeightUnderAFullHeightHtml_FillsTheViewportAsChrome)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/Root/BodyHeightHtmlTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribePath(TEXT("0"), 1280.0f, 720.0f), TEXT("Chrome: html height 100% is the viewport height")));
		ASSERT_THAT(AreEqual(FString(), DescribePath(TEXT("0.0"), 1280.0f, 720.0f), TEXT("Chrome: body height 100% of that html fills the viewport")));
	}

	TEST_METHOD(ReferencePage_LeavesTheHtmlHeightToItsContent)
	{
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("CssParityBodyHeightDump")));
		IFileManager::Get().DeleteDirectory(*Dir, false, true);
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/Root/BodyHeightTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.DumpLayout(Dir);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		FString Html;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(Html, *FPaths::Combine(Dir, TEXT("BodyHeightTest.html"))), TEXT("the layout dump wrote no html")));
		TArray<FString> Lines;
		Html.ParseIntoArrayLines(Lines);
		const FString* Reset = Lines.FindByPredicate([](const FString& Line) -> bool
		{
			return Line.StartsWith(TEXT("html { margin: 0;"));
		});
		ASSERT_THAT(IsNotNull(Reset, TEXT("the reference page has no html reset rule")));
		ASSERT_THAT(IsTrue(Reset->Contains(TEXT("width: 1280px")), *Reset));
		ASSERT_THAT(IsFalse(Reset->Contains(TEXT("height")), FString::Printf(TEXT("the html reset forces a height, Chrome's html is as high as its content: %s"), **Reset)));
	}
};
