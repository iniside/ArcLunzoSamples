// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlCssParityInitialTestsPrivate
{
	struct FBoxCase
	{
		const TCHAR* Id;
		float X;
		float Y;
		float Width;
		float Height;
	};

	const FBoxCase ChromeBoxes[] = {
		{TEXT("text"), 0.0f, 0.0f, 1280.0f, 21.0f},
		{TEXT("rem-pad"), 0.0f, 21.0f, 132.0f, 42.0f},
		{TEXT("rem-inner"), 16.0f, 37.0f, 100.0f, 10.0f},
		{TEXT("em-box"), 0.0f, 63.0f, 32.0f, 16.0f},
		{TEXT("rem-box"), 0.0f, 79.0f, 48.0f, 8.0f},
	};

	const FBoxCase ChromeKeywordChild = {TEXT("kw-child"), 0.0f, 87.0f, 32.0f, 16.0f};

	const TCHAR* const ChromeTextColour = TEXT("#000000ff");
	const TCHAR* const ChromeParentColour = TEXT("#ff0000ff");
	constexpr float Tolerance = 1.0f;
}

TEST_CLASS(ArcRmlCssParityInitialTests, "ArcRmlUI.CssParity.Initial")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	BEFORE_EACH()
	{
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Initial/InitialValuesTest.rml")))
			&& View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(UnstyledText_IsBlackAsChrome)
	{
		using namespace ArcRmlCssParityInitialTestsPrivate;
		const FArcRmlTestLayoutElement* Text = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("text"));
		ASSERT_THAT(IsNotNull(Text));
		ASSERT_THAT(IsTrue(Text->Color.Equals(ChromeTextColour, ESearchCase::IgnoreCase),
			FString::Printf(TEXT("unstyled text is %s, Chrome %s"), *Text->Color, ChromeTextColour)));
		const FArcRmlTestLayoutElement* TextNode = Layout.FindByPredicate([Text](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Parent == Text->Path && Element.Kind == TEXT("text");
		});
		ASSERT_THAT(IsNotNull(TextNode, TEXT("the unstyled div has no text node")));
		ASSERT_THAT(IsTrue(TextNode->Color.Equals(ChromeTextColour, ESearchCase::IgnoreCase),
			FString::Printf(TEXT("the unstyled text node is %s, Chrome %s"), *TextNode->Color, ChromeTextColour)));
	}

	TEST_METHOD(RemAndEmWithNoFontSizeRule_UseTheSixteenPixelRootAsChrome)
	{
		using namespace ArcRmlCssParityInitialTestsPrivate;
		FString Failures;
		for (const FBoxCase& Case : ChromeBoxes)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
			if (!Element)
			{
				Failures += FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
				continue;
			}
			const FArcRmlInspectorRect& Rect = Element->Rect;
			if (FMath::Abs(Rect.Position.X - Case.X) > Tolerance || FMath::Abs(Rect.Position.Y - Case.Y) > Tolerance
				|| FMath::Abs(Rect.Size.X - Case.Width) > Tolerance || FMath::Abs(Rect.Size.Y - Case.Height) > Tolerance)
			{
				Failures += FString::Printf(TEXT("[%s is %.2f,%.2f %.2fx%.2f, Chrome %.0f,%.0f %.0fx%.0f] "), Case.Id, Rect.Position.X, Rect.Position.Y,
					Rect.Size.X, Rect.Size.Y, Case.X, Case.Y, Case.Width, Case.Height);
			}
		}
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(InitialKeywordOverInheritedValues_ResetsColourAndFontSizeAsChrome)
	{
		using namespace ArcRmlCssParityInitialTestsPrivate;
		const FArcRmlTestLayoutElement* Parent = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("kw-parent"));
		const FArcRmlTestLayoutElement* Child = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("kw-child"));
		ASSERT_THAT(IsNotNull(Parent));
		ASSERT_THAT(IsNotNull(Child));
		ASSERT_THAT(IsTrue(Parent->Color.Equals(ChromeParentColour, ESearchCase::IgnoreCase),
			FString::Printf(TEXT("the parent is %s, Chrome %s"), *Parent->Color, ChromeParentColour)));
		ASSERT_THAT(IsTrue(Child->Color.Equals(ChromeTextColour, ESearchCase::IgnoreCase),
			FString::Printf(TEXT("color: initial under a red parent is %s, Chrome %s"), *Child->Color, ChromeTextColour)));
		const FArcRmlInspectorRect& Rect = Child->Rect;
		ASSERT_THAT(IsTrue(FMath::Abs(Rect.Position.X - ChromeKeywordChild.X) <= Tolerance && FMath::Abs(Rect.Position.Y - ChromeKeywordChild.Y) <= Tolerance
			&& FMath::Abs(Rect.Size.X - ChromeKeywordChild.Width) <= Tolerance && FMath::Abs(Rect.Size.Y - ChromeKeywordChild.Height) <= Tolerance,
			FString::Printf(TEXT("kw-child is %.2f,%.2f %.2fx%.2f with font-size: initial under a 10px parent, Chrome %.0f,%.0f %.0fx%.0f"), Rect.Position.X, Rect.Position.Y,
				Rect.Size.X, Rect.Size.Y, ChromeKeywordChild.X, ChromeKeywordChild.Y, ChromeKeywordChild.Width, ChromeKeywordChild.Height)));
	}
};
