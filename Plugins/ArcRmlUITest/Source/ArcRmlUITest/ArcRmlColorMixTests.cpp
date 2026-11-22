// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

namespace ArcRmlColorMixTestsPrivate
{
	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
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

	FString ToHex(const FIntVector4& Colour)
	{
		return FString::Printf(TEXT("#%02x%02x%02x%02x"), Colour.X, Colour.Y, Colour.Z, Colour.W);
	}

	bool IsNear(const FIntVector4& A, const FIntVector4& B)
	{
		return FMath::Abs(A.X - B.X) <= 1 && FMath::Abs(A.Y - B.Y) <= 1 && FMath::Abs(A.Z - B.Z) <= 1 && FMath::Abs(A.W - B.W) <= 1;
	}

	enum class EColourSlot : uint8
	{
		Color,
		Background,
		BorderTop,
	};

	const FString& GetSlot(const FArcRmlTestLayoutElement& Element, EColourSlot Slot)
	{
		switch (Slot)
		{
		case EColourSlot::Background:
			return Element.BackgroundColor;
		case EColourSlot::BorderTop:
			return Element.BorderTopColor;
		default:
			return Element.Color;
		}
	}
}

TEST_CLASS(ArcRmlColorMixTests, "ArcRmlUI.ColorMix")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlColorMixTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
	}

	bool SettleFrames()
	{
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			FPlatformProcess::Sleep(0.05f);
			if (!View.Flush())
			{
				return false;
			}
		}
		return true;
	}

	bool OpenFrozen()
	{
		ArcRmlColorMixTestsPrivate::RunConsole(TEXT("ArcRml.Clock 1000"));
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Color/ColorMixTest.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && SettleFrames() && View.GetProbe().bHasDocument;
	}

	bool Advance(double Seconds)
	{
		ArcRmlColorMixTestsPrivate::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.3f"), Seconds));
		return SettleFrames();
	}

	FIntVector4 ReadColour(const TCHAR* DomId, ArcRmlColorMixTestsPrivate::EColourSlot Slot)
	{
		using namespace ArcRmlColorMixTestsPrivate;
		FIntVector4 Colour(-1, -1, -1, -1);
		TArray<FArcRmlTestLayoutElement> Elements;
		if (!View.ParseLayout(Elements))
		{
			return Colour;
		}
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		if (Element)
		{
			ParseHex(GetSlot(*Element, Slot), Colour);
		}
		return Colour;
	}

	void ExpectColour(const TCHAR* Moment, const TCHAR* DomId, ArcRmlColorMixTestsPrivate::EColourSlot Slot, const FIntVector4& Expected)
	{
		using namespace ArcRmlColorMixTestsPrivate;
		const FIntVector4 Colour = ReadColour(DomId, Slot);
		ASSERT_THAT(IsTrue(IsNear(Colour, Expected), FString::Printf(TEXT("%s %s slot %d is %s, Chrome %s; %s"), Moment, DomId, static_cast<int32>(Slot),
			*ToHex(Colour), *ToHex(Expected), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	void ExpectExactColour(const TCHAR* Moment, const TCHAR* DomId, const FIntVector4& Expected)
	{
		using namespace ArcRmlColorMixTestsPrivate;
		const FIntVector4 Colour = ReadColour(DomId, EColourSlot::Background);
		ASSERT_THAT(IsTrue(Colour == Expected, FString::Printf(TEXT("%s %s background is %s, Chrome %s; %s"), Moment, DomId, *ToHex(Colour), *ToHex(Expected),
			*ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	void ExpectSiteColour(const TCHAR* DomId, bool bOnText, const TCHAR* Site, const FIntVector4& Expected)
	{
		using namespace ArcRmlColorMixTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Elements;
		const FArcRmlTestLayoutElement* Element = View.ParseLayout(Elements) ? ArcRmlUITestHelpers::FindByDomId(Elements, DomId) : nullptr;
		if (Element && bOnText)
		{
			const FString OwnerPath = Element->Path;
			Element = Elements.FindByPredicate([&OwnerPath](const FArcRmlTestLayoutElement& Candidate)
				{ return Candidate.Parent == OwnerPath && Candidate.Kind == TEXT("text"); });
		}
		FIntVector4 Colour(-1, -1, -1, -1);
		const FString* Hex = Element ? Element->SiteColours.Find(Site) : nullptr;
		if (Hex)
		{
			ParseHex(*Hex, Colour);
		}
		ASSERT_THAT(IsTrue(IsNear(Colour, Expected), FString::Printf(TEXT("%s %s draws %s, Chrome %s; %s"), DomId, Site, *ToHex(Colour), *ToHex(Expected),
			*ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	TEST_METHOD(CurrentColorInsideColorMix_ResolvesAgainstTheElementColour)
	{
		using namespace ArcRmlColorMixTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("color-mix")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("red"), TEXT("mix-bg"), EColourSlot::Background, FIntVector4(128, 0, 128, 255));
		ExpectColour(TEXT("red"), TEXT("mix-bg"), EColourSlot::BorderTop, FIntVector4(255, 0, 0, 64));
		ExpectColour(TEXT("red"), TEXT("mix-color"), EColourSlot::Color, FIntVector4(255, 191, 191, 255));
		ExpectColour(TEXT("red"), TEXT("mix-color"), EColourSlot::Background, FIntVector4(128, 96, 96, 255));
		ExpectColour(TEXT("red"), TEXT("mix-inline"), EColourSlot::Background, FIntVector4(128, 128, 0, 255));
	}

	TEST_METHOD(CurrentColorInsideColorMix_FollowsAColourChange)
	{
		using namespace ArcRmlColorMixTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetRootClass(TEXT("blue"), true);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ExpectColour(TEXT("blue"), TEXT("mix-bg"), EColourSlot::Background, FIntVector4(0, 0, 255, 255));
		ExpectColour(TEXT("blue"), TEXT("mix-bg"), EColourSlot::BorderTop, FIntVector4(0, 0, 255, 64));
		ExpectColour(TEXT("blue"), TEXT("mix-color"), EColourSlot::Color, FIntVector4(191, 191, 255, 255));
		ExpectColour(TEXT("blue"), TEXT("mix-color"), EColourSlot::Background, FIntVector4(96, 96, 128, 255));
		ExpectColour(TEXT("blue"), TEXT("mix-inline"), EColourSlot::Background, FIntVector4(0, 128, 128, 255));
	}

	TEST_METHOD(CurrentColorInsideColorMix_ResolvesAgainstTheElementColourAtEveryOtherSite)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSiteColour(TEXT("site-caret"), false, TEXT("caret"), FIntVector4(255, 0, 0, 128));
		ExpectSiteColour(TEXT("site-box-shadow"), false, TEXT("box_shadow"), FIntVector4(255, 0, 0, 128));
		ExpectSiteColour(TEXT("site-gradient"), false, TEXT("gradient_stop"), FIntVector4(255, 128, 128, 255));
		ExpectSiteColour(TEXT("site-drop-shadow"), false, TEXT("drop_shadow"), FIntVector4(128, 0, 0, 255));
		ExpectSiteColour(TEXT("site-text-shadow"), true, TEXT("text_shadow"), FIntVector4(128, 0, 0, 255));
		ExpectSiteColour(TEXT("site-font-effect"), true, TEXT("font_effect"), FIntVector4(128, 0, 0, 255));
		ExpectSiteColour(TEXT("site-text-stroke"), true, TEXT("text_stroke"), FIntVector4(128, 0, 0, 255));
	}

	TEST_METHOD(CurrentColorInsideColorMix_ResolvesTheScrollbarTrackAgainstTheElementColour)
	{
		using namespace ArcRmlColorMixTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Scroller = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("site-scrollbar"));
		ASSERT_THAT(IsNotNull(Scroller));
		const FArcRmlTestLayoutElement* Track = Elements.FindByPredicate([Scroller](const FArcRmlTestLayoutElement& Element)
			{ return Element.Parent == Scroller->Path && Element.Tag == TEXT("scrollbarvertical"); });
		FIntVector4 Colour(-1, -1, -1, -1);
		if (Track)
		{
			ParseHex(Track->BackgroundColor, Colour);
		}
		ASSERT_THAT(IsTrue(IsNear(Colour, FIntVector4(64, 0, 0, 255)), FString::Printf(TEXT("scrollbar track is %s, Chrome #400000ff; %s"), *ToHex(Colour),
			*ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	TEST_METHOD(ColorMixWithAnOutOfGamutOklchOperand_Is70_229_117AsInChrome)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectExactColour(TEXT("static"), TEXT("mix-oklch"), FIntVector4(70, 229, 117, 255));
	}

	TEST_METHOD(OklchKeyframeFromACompiledStyleSheet_Is0_46_93Halfway)
	{
		const FString SheetBinary = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Color/ColorMixTest.rcssc"));
		IFileManager::Get().Delete(*SheetBinary, false, false, true);
		FArcRmlTestView::ClearDocumentCaches();
		ASSERT_THAT(IsTrue(OpenFrozen(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*SheetBinary), TEXT("the style sheet was not compiled at open")));
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ExpectExactColour(TEXT("halfway"), TEXT("oklch-keyframes"), FIntVector4(0, 46, 93, 255));
	}

	TEST_METHOD(TransitionToAnOklchColour_InterpolatesInOklabTo91_42_34Halfway)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectExactColour(TEXT("before"), TEXT("oklch-transition"), FIntVector4(0, 0, 0, 255));
		View.SetRootClass(TEXT("blue"), true);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ExpectExactColour(TEXT("halfway (sRGB would be #773b33ff)"), TEXT("oklch-transition"), FIntVector4(91, 42, 34, 255));
		ASSERT_THAT(IsTrue(Advance(3.0)));
		ExpectExactColour(TEXT("after"), TEXT("oklch-transition"), FIntVector4(237, 118, 101, 255));
	}

	TEST_METHOD(TransitionToAColorMix_InterpolatesInOklabTo45_45_99HalfwayAndEndsAtTheResolvedMix)
	{
		using namespace ArcRmlColorMixTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("before"), TEXT("mix-transition"), EColourSlot::Background, FIntVector4(0, 0, 0, 255));
		View.SetRootClass(TEXT("blue"), true);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ExpectExactColour(TEXT("halfway (sRGB would be #404080ff)"), TEXT("mix-transition"), FIntVector4(45, 45, 99, 255));
		ASSERT_THAT(IsTrue(Advance(3.0)));
		ExpectColour(TEXT("after"), TEXT("mix-transition"), EColourSlot::Background, FIntVector4(128, 128, 255, 255));
	}
};
