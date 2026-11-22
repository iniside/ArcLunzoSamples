// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlKeptTabTestHelpers.h"
#include "CQTest.h"
#include "InputCoreTypes.h"

namespace ArcRmlTabIsolationTestsPrivate
{
	using namespace ArcRmlKeptTabTestHelpers;

	const TCHAR* const PixFrame = TEXT("Tabs/KeepPixFrame.rml");
	const TCHAR* const PixTab = TEXT("Tabs/KeepPixTab.rml");
	const TCHAR* const PixAlone = TEXT("Tabs/KeepPixAlone.rml");

	constexpr int32 Tolerance = 2;
	constexpr int32 EmbedLeft = 30;
	constexpr int32 EmbedTop = 40;
	const FIntPoint EmbedSize(400, 240);

	struct FChromePixel
	{
		const TCHAR* Name = TEXT("");
		int32 X = 0;
		int32 Y = 0;
		FColor Chrome;
	};

	const FChromePixel ChromePixels[] = {
		{TEXT("box first pixel"), 10, 0, FColor(48, 80, 160, 255)},
		{TEXT("box centre"), 70, 20, FColor(48, 80, 160, 255)},
		{TEXT("box last pixel"), 129, 39, FColor(48, 80, 160, 255)},
		{TEXT("right of the box"), 130, 20, FColor(255, 255, 255, 255)},
		{TEXT("gap under the box"), 70, 40, FColor(255, 255, 255, 255)},
		{TEXT("fallback fill first row"), 70, 50, FColor(48, 160, 80, 255)},
		{TEXT("fallback fill last row"), 70, 79, FColor(48, 160, 80, 255)},
		{TEXT("gap under the fill"), 70, 80, FColor(255, 255, 255, 255)},
		{TEXT("border corner"), 10, 124, FColor(160, 48, 32, 255)},
		{TEXT("border left"), 13, 138, FColor(160, 48, 32, 255)},
		{TEXT("border inner left"), 14, 138, FColor(224, 192, 64, 255)},
		{TEXT("border inner right"), 113, 138, FColor(224, 192, 64, 255)},
		{TEXT("border right"), 114, 138, FColor(160, 48, 32, 255)},
		{TEXT("right of the border"), 118, 138, FColor(255, 255, 255, 255)},
		{TEXT("border top"), 60, 127, FColor(160, 48, 32, 255)},
		{TEXT("border inner top"), 60, 128, FColor(224, 192, 64, 255)},
		{TEXT("border inner bottom"), 60, 147, FColor(224, 192, 64, 255)},
		{TEXT("border bottom"), 60, 151, FColor(160, 48, 32, 255)},
		{TEXT("under the border"), 60, 152, FColor(255, 255, 255, 255)},
	};

	bool IsNear(const FColor& Measured, const FColor& Expected)
	{
		return FMath::Abs(Measured.R - Expected.R) <= Tolerance && FMath::Abs(Measured.G - Expected.G) <= Tolerance && FMath::Abs(Measured.B - Expected.B) <= Tolerance
			&& FMath::Abs(Measured.A - Expected.A) <= Tolerance;
	}

	FString DescribeChromeMismatches(const FArcRmlTestView& View, FIntPoint Origin)
	{
		TArray<FString> Parts;
		for (const FChromePixel& Pixel : ChromePixels)
		{
			const FColor Measured = View.GetPixel(Origin.X + Pixel.X, Origin.Y + Pixel.Y);
			if (!IsNear(Measured, Pixel.Chrome))
			{
				Parts.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d chrome %d,%d,%d"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G, Measured.B, Pixel.Chrome.R,
					Pixel.Chrome.G, Pixel.Chrome.B));
			}
		}
		return FString::Join(Parts, TEXT(" | "));
	}

	TArray<FColor> ReadRegion(const FArcRmlTestView& View, FIntPoint Origin, FIntPoint Size)
	{
		TArray<FColor> Region;
		Region.Reserve(Size.X * Size.Y);
		for (int32 Y = 0; Y < Size.Y; ++Y)
		{
			for (int32 X = 0; X < Size.X; ++X)
			{
				Region.Add(View.GetPixel(Origin.X + X, Origin.Y + Y));
			}
		}
		return Region;
	}

	FString DescribeFirstDifference(const TArray<FColor>& A, const TArray<FColor>& B, int32 Width, int32& OutDifferent)
	{
		OutDifferent = 0;
		FString First;
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index] != B[Index])
			{
				if (OutDifferent == 0)
				{
					First = FString::Printf(TEXT("first at (%d,%d): embedded %d,%d,%d alone %d,%d,%d"), Index % Width, Index / Width, A[Index].R, A[Index].G, A[Index].B, B[Index].R,
						B[Index].G, B[Index].B);
				}
				++OutDifferent;
			}
		}
		return First;
	}

	bool IsRect(const FBox2f& Bounds, const FArcRmlTestLayoutElement& Element)
	{
		return FMath::Abs(Bounds.Min.X - Element.Rect.Position.X) <= 0.5f && FMath::Abs(Bounds.Min.Y - Element.Rect.Position.Y) <= 0.5f
			&& FMath::Abs(Bounds.Max.X - (Element.Rect.Position.X + Element.Rect.Size.X)) <= 0.5f && FMath::Abs(Bounds.Max.Y - (Element.Rect.Position.Y + Element.Rect.Size.Y)) <= 0.5f;
	}

	int32 CountGradientsOver(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour, const FArcRmlTestLayoutElement& Element)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw* Draw : FindFlatDraws(Draws, Colour))
		{
			Count += IsRect(Draw->Bounds, Element) ? 1 : 0;
		}
		return Count;
	}
}

TEST_CLASS(ArcRmlTabIsolationTests, "ArcRmlUI.TabIsolation")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.Clock live"));
	}

	bool Read()
	{
		return View.Flush() && View.ParseLayout(Elements);
	}

	bool OpenFrame(bool bPad)
	{
		return Assert.IsTrue(ArcRmlKeptTabTestHelpers::OpenFrame(View, bPad), ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Read();
	}

	bool Visit(const TCHAR* Relative)
	{
		return Assert.IsTrue(ArcRmlKeptTabTestHelpers::Visit(View, Relative), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View) + ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Read();
	}

	bool Advance(double Seconds)
	{
		ArcRmlKeptTabTestHelpers::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.3f"), Seconds));
		return ArcRmlKeptTabTestHelpers::SettleFrames(View) && Read();
	}

	float WidthOf(const TCHAR* DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? Element->Rect.Size.X : -1.0f;
	}

	TEST_METHOD(EmbeddedTab_GivesTheSamePixelsAsTheTabAloneAtTheEmbedSize)
	{
		using namespace ArcRmlTabIsolationTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(PixFrame));
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(PixTab));
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(PixAlone));

		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(PixFrame)), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(PixTab)), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(ArcRmlUITestHelpers::MakePath(PixTab), View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Read()));
		const FArcRmlTestLayoutElement* Embed = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("pf-embed"));
		ASSERT_THAT(IsNotNull(Embed));
		ASSERT_THAT(IsTrue(Embed->Rect.Position.X == EmbedLeft && Embed->Rect.Position.Y == EmbedTop && Embed->Rect.Size.X == EmbedSize.X && Embed->Rect.Size.Y == EmbedSize.Y,
			TEXT("the arc-embed box is not where the fixture puts it")));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		const TArray<FColor> Embedded = ReadRegion(View, FIntPoint(EmbedLeft, EmbedTop), EmbedSize);
		const FString EmbeddedMismatches = DescribeChromeMismatches(View, FIntPoint(EmbedLeft, EmbedTop));
		View.Close();

		ASSERT_THAT(IsTrue(View.Open(ArcRmlUITestHelpers::MakePath(PixAlone), EmbedSize, 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		ASSERT_THAT(IsTrue(View.GetPixelSize() == EmbedSize, View.GetPixelSize().ToString()));
		const TArray<FColor> Alone = ReadRegion(View, FIntPoint::ZeroValue, EmbedSize);

		ASSERT_THAT(AreEqual(FString(), DescribeChromeMismatches(View, FIntPoint::ZeroValue), TEXT("the tab alone differs from Chrome")));
		ASSERT_THAT(AreEqual(FString(), EmbeddedMismatches, TEXT("the embedded tab differs from Chrome's rendering of the tab alone")));
		int32 Different = 0;
		const FString First = DescribeFirstDifference(Embedded, Alone, EmbedSize.X, Different);
		ASSERT_THAT(AreEqual(0, Different, FString::Printf(TEXT("the embedded tab differs from the tab alone in %d pixels, %s"), Different, *First)));
	}

	TEST_METHOD(TabHtmlRules_ApplyAtTheTabRootAsTheTabAloneInChrome)
	{
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("Tabs/HtmlRootFrame.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(TEXT("Tabs/HtmlRootTab.rml"))), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && Read(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		const FArcRmlTestLayoutElement* TabBox = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ht-box"));
		const FArcRmlTestLayoutElement* TabText = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ht-text"));
		const FArcRmlTestLayoutElement* FrameBox = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("hf-box"));
		ASSERT_THAT(IsTrue(TabBox && TabText && FrameBox, TEXT("a fixture element is not in the layout")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(100.0f, TabBox->Rect.Size.X, 1.0f) && FMath::IsNearlyEqual(40.0f, TabBox->Rect.Size.Y, 1.0f),
			FString::Printf(TEXT("the tab's 5rem x 2rem box is %.2fx%.2f, Chrome 100x40 for the tab alone with its html at 20px"), TabBox->Rect.Size.X, TabBox->Rect.Size.Y)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(30.0f, TabText->Rect.Size.Y, 1.0f),
			FString::Printf(TEXT("the tab's text line is %.2f px high, Chrome 30 for the tab's html line-height 1.5 at 20px"), TabText->Rect.Size.Y)));
		ASSERT_THAT(IsTrue(TabText->Color.Equals(TEXT("#0000c8ff"), ESearchCase::IgnoreCase),
			FString::Printf(TEXT("the tab's text is %s, Chrome #0000c8ff from the tab's html rule"), *TabText->Color)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(50.0f, FrameBox->Rect.Size.X, 1.0f) && FMath::IsNearlyEqual(20.0f, FrameBox->Rect.Size.Y, 1.0f),
			FString::Printf(TEXT("the frame's 5rem x 2rem box is %.2fx%.2f, Chrome 50x20 for the frame's html at 10px"), FrameBox->Rect.Size.X, FrameBox->Rect.Size.Y)));
		ASSERT_THAT(IsTrue(FrameBox->Color.Equals(TEXT("#c80000ff"), ESearchCase::IgnoreCase),
			FString::Printf(TEXT("the frame's box is %s, Chrome #c80000ff from the frame's html rule"), *FrameBox->Color)));
	}

	TEST_METHOD(SameKeyframesName_EachKeptTabAndTheFrameUseTheirOwn)
	{
		using namespace ArcRmlTabIsolationTestsPrivate;
		RunConsole(TEXT("ArcRml.Clock 1000"));
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ASSERT_THAT(IsNear(20.0f, WidthOf(TEXT("kf-same")), 1.0f, TEXT("the frame's @keyframes kSame, 10 to 30 px over 4 s, is 20 px after 2 s")));

		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ASSERT_THAT(IsNear(200.0f, WidthOf(TEXT("ka-anim")), 1.0f, TEXT("tab A's @keyframes kSame, 100 to 300 px over 4 s, is 200 px after 2 s")));
		ASSERT_THAT(IsNear(30.0f, WidthOf(TEXT("kf-same")), 1.0f, TEXT("the frame's animation ended at the end of its own @keyframes")));

		ASSERT_THAT(IsTrue(Visit(TabB)));
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ASSERT_THAT(IsNear(100.0f, WidthOf(TEXT("kb-anim")), 1.0f, TEXT("tab B's @keyframes kSame, 50 to 150 px over 4 s, is 100 px after 2 s, not tab A's or the frame's")));

		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ASSERT_THAT(IsNear(200.0f, WidthOf(TEXT("ka-anim")), 1.0f, TEXT("tab A uses another tab's @keyframes after it was shown again")));
		ASSERT_THAT(IsNear(30.0f, WidthOf(TEXT("kf-same")), 1.0f, TEXT("a tab's @keyframes replaced the frame's")));
	}

	TEST_METHOD(SameDecoratorName_EachKeptTabAndTheFrameUseTheirOwn)
	{
		using namespace ArcRmlTabIsolationTestsPrivate;
		const FColor Red(255, 0, 0, 255);
		const FColor Blue(0, 0, 255, 255);
		const FColor Green(0, 255, 0, 255);
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(Visit(TabB)));

		const FArcRmlTestLayoutElement* FrameDeco = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("kf-deco"));
		ASSERT_THAT(IsNotNull(FrameDeco));
		const FArcRmlTestLayoutElement* DecoB = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("kb-deco"));
		ASSERT_THAT(IsNotNull(DecoB));
		ASSERT_THAT(AreEqual(1, CountGradientsOver(View.GetDraws(), Blue, *DecoB), TEXT("tab B's decorator kdSame is not its own blue gradient")));
		ASSERT_THAT(AreEqual(0, CountGradientsOver(View.GetDraws(), Red, *DecoB), TEXT("tab A's decorator kdSame draws on tab B's element")));
		ASSERT_THAT(AreEqual(0, CountGradientsOver(View.GetDraws(), Green, *DecoB), TEXT("the frame's decorator kdSame draws on tab B's element")));
		ASSERT_THAT(AreEqual(1, CountGradientsOver(View.GetDraws(), Green, *FrameDeco), TEXT("the frame's decorator kdSame is not its own green gradient")));
		ASSERT_THAT(AreEqual(0, CountGradientsOver(View.GetDraws(), Blue, *FrameDeco), TEXT("a tab's decorator kdSame replaced the frame's")));

		ASSERT_THAT(IsTrue(Visit(TabA)));
		const FArcRmlTestLayoutElement* DecoA = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-deco"));
		ASSERT_THAT(IsNotNull(DecoA));
		ASSERT_THAT(AreEqual(1, CountGradientsOver(View.GetDraws(), Red, *DecoA), TEXT("tab A's decorator kdSame is not its own red gradient")));
		ASSERT_THAT(AreEqual(0, CountGradientsOver(View.GetDraws(), Blue, *DecoA), TEXT("tab B's decorator kdSame draws on tab A's element")));
		ASSERT_THAT(AreEqual(0, CountGradientsOver(View.GetDraws(), Green, *DecoA), TEXT("the frame's decorator kdSame draws on tab A's element")));
	}

	TEST_METHOD(IdInATabAndInTheFrame_ResolveSeparately)
	{
		using namespace ArcRmlTabIsolationTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrame(true)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		TArray<const FArcRmlTestLayoutElement*> Dup = FindAll(Elements, TEXT("dup"));
		ASSERT_THAT(AreEqual(2, Dup.Num()));
		ASSERT_THAT(IsNear(111.0f, Dup[0]->Rect.Size.X, 0.5f, TEXT("the frame's #dup rule")));
		ASSERT_THAT(IsNear(222.0f, Dup[1]->Rect.Size.X, 0.5f, TEXT("tab A's #dup rule")));
		ASSERT_THAT(IsTrue(Dup[0]->Path != Dup[1]->Path));

		ASSERT_THAT(AreEqual(FString(TEXT("ka-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(View.Flush()));
		View.PressKey(EKeys::Gamepad_DPad_Right);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("nid")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("nav-right: #nid in tab A did not move focus")));
		const FArcRmlTestLayoutElement* Focused = ArcRmlUITestHelpers::FindByPath(Elements, View.GetProbe().NavFocusPath);
		ASSERT_THAT(IsNotNull(Focused));
		ASSERT_THAT(IsNear(92.0f, Focused->Rect.Size.X, 0.5f, TEXT("nav-right: #nid in tab A went to another scope's #nid (frame 91 px, tab A 92 px, tab B 93 px)")));

		ASSERT_THAT(IsTrue(Visit(TabB)));
		Dup = FindAll(Elements, TEXT("dup"));
		ASSERT_THAT(AreEqual(3, Dup.Num()));
		ASSERT_THAT(IsNear(111.0f, Dup[0]->Rect.Size.X, 0.5f, TEXT("the frame's #dup rule")));
		ASSERT_THAT(IsNear(333.0f, Dup[2]->Rect.Size.X, 0.5f, TEXT("tab B's #dup rule")));
	}

	TEST_METHOD(FrameIdLookup_NeverFindsATabElement)
	{
		using namespace ArcRmlTabIsolationTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrame(true)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(AreEqual(1, FindAll(Elements, TEXT("ka-only-btn")).Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("ka-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.ClickElement(TEXT("#kf-btn"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		View.PressKey(EKeys::Gamepad_DPad_Right);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		const FArcRmlTestLayoutElement* Reached = ArcRmlUITestHelpers::FindByPath(Elements, View.GetProbe().NavFocusPath);
		ASSERT_THAT(IsNotNull(Reached, TEXT("nav-right: #nid in the frame moved no focus, so the probe is not valid")));
		ASSERT_THAT(IsNear(91.0f, Reached->Rect.Size.X, 0.5f, TEXT("nav-right: #nid in the frame went to a tab's #nid")));
		View.ClickElement(TEXT("#kf-btn2"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("kf-btn2")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		View.PressKey(EKeys::Gamepad_DPad_Right);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreNotEqual(FString(TEXT("ka-only-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("nav-right: #ka-only-btn in the frame found an element of tab A")));
	}

	TEST_METHOD(TabContentPastTheEmbedBox_IsClippedAndNotHit)
	{
		using namespace ArcRmlTabIsolationTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		const FArcRmlTestLayoutElement* Embed = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("kf-embed"));
		const FArcRmlTestLayoutElement* Over = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-over"));
		ASSERT_THAT(IsNotNull(Embed));
		ASSERT_THAT(IsNotNull(Over));
		const float EmbedBottom = Embed->Rect.Position.Y + Embed->Rect.Size.Y;
		ASSERT_THAT(IsTrue(Over->Rect.Position.Y + 10.0f < EmbedBottom && Over->Rect.Position.Y + Over->Rect.Size.Y > EmbedBottom + 40.0f,
			TEXT("the fixture element does not straddle the bottom of the arc-embed box")));
		const float X = Over->Rect.Position.X + 40.0f;
		ASSERT_THAT(IsTrue(View.IsInteractiveAt(FVector2f(X, EmbedBottom - 5.0f)), TEXT("the part of the element inside the box is not hit, so the probe is not valid")));
		ASSERT_THAT(IsFalse(View.IsInteractiveAt(FVector2f(X, EmbedBottom + 20.0f)), TEXT("the part of the element past the arc-embed box is hit")));

		const TArray<const FArcRmlTestDraw*> Fills = FindFlatDraws(View.GetDraws(), FColor(170, 0, 119, 255));
		ASSERT_THAT(AreEqual(1, Fills.Num()));
		ASSERT_THAT(IsTrue(Fills[0]->Scissor.IsSet() && Fills[0]->Scissor->Max.Y <= EmbedBottom + 0.5f,
			FString::Printf(TEXT("the element past the box is not clipped: scissor %s, box bottom %.1f"), Fills[0]->Scissor.IsSet() ? *Fills[0]->Scissor->ToString() : TEXT("none"), EmbedBottom)));
	}
};
