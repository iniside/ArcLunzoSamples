// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Dom/JsonObject.h"
#include "InputCoreTypes.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace ArcRmlCssParityLineBreakTestsPrivate
{
	struct FChromeLine
	{
		const TCHAR* Text;
		float Width;
	};

	struct FChromeBox
	{
		const TCHAR* DomId;
		float BoxWidth;
		const FChromeLine* Lines;
		int32 NumLines;
	};

	constexpr float WidthTolerance = 0.05f;
	constexpr float CaretTolerance = 1.0f;

	const FChromeLine KernedLines[] = {
		{TEXT("AVATAR WAVE Tokyo Yacht Taffy"), 241.266f},
		{TEXT("LAVA AWAY Typography VAT YAW"), 246.219f},
		{TEXT("Wavy Toy AVOW Tyre Vault Yoyo"), 241.531f},
		{TEXT("TAV AT AV To Ya"), 114.75f},
	};

	const FChromeLine VatNarrowLines[] = {{TEXT("to"), 16.969f}, {TEXT("VAT"), 27.375f}, {TEXT("YAW"), 32.219f}};
	const FChromeLine VatWideLines[] = {{TEXT("to VAT"), 47.812f}, {TEXT("YAW"), 32.219f}};
	const FChromeLine LavaNarrowLines[] = {{TEXT("a"), 9.203f}, {TEXT("LAVA"), 36.938f}, {TEXT("AWAY"), 41.875f}};
	const FChromeLine LavaWideLines[] = {{TEXT("a LAVA"), 50.25f}, {TEXT("AWAY"), 41.875f}};

	const FChromeBox Sweeps[] = {
		{TEXT("vat-0"), 46.5f, VatNarrowLines, UE_ARRAY_COUNT(VatNarrowLines)},
		{TEXT("vat-1"), 48.5f, VatWideLines, UE_ARRAY_COUNT(VatWideLines)},
		{TEXT("lava-0"), 49.5f, LavaNarrowLines, UE_ARRAY_COUNT(LavaNarrowLines)},
		{TEXT("lava-1"), 51.5f, LavaWideLines, UE_ARRAY_COUNT(LavaWideLines)},
	};

	const FChromeLine FitYaLines[] = {{TEXT("VAT VAT AWAY"), 103.578f}};
	const FChromeLine FitTokyoLines[] = {{TEXT("VAT AWAY Yes"), 102.703f}};

	const FChromeLine MonoWrapLines[] = {{TEXT("a->b!=c=>d="), 92.406f}, {TEXT("==e|>f<=g>="), 92.406f}, {TEXT("h--i++j::k"), 84.0f}};

	const FChromeLine ProbeMonoLines[] = {
		{TEXT("a -> b != c => d"), 134.406f},
		{TEXT("=== e |> f <= g"), 126.0f},
		{TEXT(">= h -- i ++ j ::"), 142.812f},
		{TEXT("k"), 8.406f},
	};

	const FChromeLine ProbeLongLines[] = {
		{TEXT("The quick brown fox jumps over the lazy dog while the wizard quietly"), 499.891f},
		{TEXT("brews a potion of vexing jumbled flasks. Pack my box with five dozen"), 498.75f},
		{TEXT("liquor jugs, then sphinx of black quartz, judge my vow. A wavy Tokyo"), 493.219f},
		{TEXT("yacht ran away to Taffy Vault at a VAT office, where AVATAR wave"), 464.953f},
		{TEXT("typography was tested by careful readers. The quick brown fox jumps"), 508.453f},
		{TEXT("over the lazy dog while the wizard quietly brews a potion of vexing"), 483.859f},
		{TEXT("jumbled flasks. Pack my box with five dozen liquor jugs, then sphinx of"), 508.781f},
		{TEXT("black quartz, judge my vow. A wavy Tokyo yacht ran away to Taffy Vault"), 516.141f},
		{TEXT("at a VAT office, where AVATAR wave typography was tested by careful"), 502.156f},
		{TEXT("readers. The quick brown fox jumps over the lazy dog while the wizard"), 506.594f},
		{TEXT("quietly brews a potion of vexing jumbled flasks. Pack my box with five"), 505.344f},
		{TEXT("dozen liquor jugs, then sphinx of black quartz, judge my vow. A wavy"), 494.766f},
		{TEXT("Tokyo yacht ran away to Taffy Vault at a VAT office, where AVATAR wave"), 511.906f},
		{TEXT("typography was tested by careful readers."), 310.938f},
	};

	const FChromeLine LongWordLines[] = {
		{TEXT("Antidisestablishmentariani"), 193.531f},
		{TEXT("smAntidisestablishmentari"), 192.844f},
		{TEXT("anismAntidisestablishment"), 196.438f},
		{TEXT("arianism"), 60.266f},
	};

	const float HugeWordWidths[] = {899.688f, 897.359f, 891.453f, 895.016f, 896.797f, 896.797f, 898.047f, 897.578f, 896.234f, 899.766f, 891.016f, 899.688f, 897.359f, 891.453f, 895.016f, 896.797f, 896.797f, 73.578f};

	const FColor InputGlyphColour(0x10, 0xa0, 0xd0, 0xff);
	const FColor DivGlyphColour(0xd0, 0x10, 0xa0, 0xff);

	constexpr float WidthOfO = 12.375f;
	constexpr float WidthOfOffi = 31.641f;
	constexpr float WidthOfOffice = 55.453f;
	constexpr float WidthOfA = 11.5f;
	constexpr float WidthOfAb = 24.391f;

	const FColor KernedColour(0xd0, 0xa0, 0x20, 0xff);
	const FColor LigatureCaretColour(0xff, 0x00, 0x00, 0xff);
	const FColor BidiCaretColour(0xff, 0xff, 0x00, 0xff);
	const FColor TextAreaCaretColour(0x00, 0xff, 0x00, 0xff);
	const FColor SelectionColour(0xff, 0x80, 0x00, 0xff);

	FString DescribeLines(const TArray<FArcRmlTestTextLine>& Lines)
	{
		TArray<FString> Parts;
		for (const FArcRmlTestTextLine& Line : Lines)
		{
			Parts.Add(FString::Printf(TEXT("'%s' %.3f"), *Line.Text, Line.Width));
		}
		return FString::Join(Parts, TEXT(" | "));
	}

	const FArcRmlTestDraw* FindFlat(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
			{
				return &Draw;
			}
		}
		return nullptr;
	}

	int32 CountQuads(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour, bool bOutlinedOnly)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
			{
				for (const FArcRmlTestDrawQuad& Quad : Draw.Quads)
				{
					Count += (!bOutlinedOnly || (Quad.Width > 0.0f && Quad.Height > 0.0f)) ? 1 : 0;
				}
			}
		}
		return Count;
	}
}

TEST_CLASS(ArcRmlCssParityLineBreakTests, "ArcRmlUI.CssParity.TextLineBreak")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Name, float DpRatio = 1.0f)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("CssParity/Text/%s.rml"), Name));
		FArcRmlTestView::DropCompiledDocument(Path);
		return ArcRmlUITestHelpers::OpenView(View, Path, DpRatio) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork && View.ParseLayout(Elements);
	}

	void ExpectLines(const TCHAR* DomId, const ArcRmlCssParityLineBreakTestsPrivate::FChromeLine* Chrome, int32 Count, float BoxWidth = -1.0f, bool bCheckWidths = true)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, DomId);
		ASSERT_THAT(AreEqual(Count, Lines.Num(), FString::Printf(TEXT("%s lines: %s"), DomId, *DescribeLines(Lines))));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FString Text = Lines[Index].Text;
			Text.TrimEndInline();
			ASSERT_THAT(AreEqual(FString(Chrome[Index].Text), Text, FString::Printf(TEXT("%s line %d of: %s"), DomId, Index, *DescribeLines(Lines))));
			ASSERT_THAT(IsTrue(!bCheckWidths || FMath::IsNearlyEqual(Lines[Index].Width, Chrome[Index].Width, WidthTolerance),
				FString::Printf(TEXT("%s line %d is %.4f px, Chrome %.4f; %s"), DomId, Index, Lines[Index].Width, Chrome[Index].Width, *DescribeLines(Lines))));
			if (BoxWidth > 0.0f)
			{
				ASSERT_THAT(IsTrue(Lines[Index].Width <= BoxWidth + 0.001f,
					FString::Printf(TEXT("%s line %d is %.4f px in a %.2f px box; %s"), DomId, Index, Lines[Index].Width, BoxWidth, *DescribeLines(Lines))));
			}
		}
	}

	bool ClickCentre(const TCHAR* DomId)
	{
		const FArcRmlTestLayoutElement* Field = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		if (!Field)
		{
			return false;
		}
		View.MouseClick(FVector2f(Field->Rect.Position.X + Field->Rect.Size.X * 0.5f, Field->Rect.Position.Y + Field->Rect.Size.Y * 0.5f));
		return View.Flush() && View.Flush();
	}

	void PressTimes(const FKey& Key, int32 Count, const FModifierKeysState& Modifiers = FModifierKeysState())
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			View.PressKey(Key, Modifiers);
		}
		View.Flush();
		View.Flush();
	}

	void ExpectCaret(const TCHAR* DomId, const FColor& Colour, float ExpectedOffset, const TCHAR* What)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		const FArcRmlTestLayoutElement* Field = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		const FArcRmlTestDraw* Caret = FindFlat(View.GetDraws(), Colour);
		ASSERT_THAT(IsNotNull(Field, DomId));
		ASSERT_THAT(IsNotNull(Caret, FString::Printf(TEXT("%s: no caret draw"), What)));
		const float Offset = Caret->Bounds.Min.X - Field->Rect.Position.X;
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Offset, ExpectedOffset, CaretTolerance),
			FString::Printf(TEXT("%s: the caret is %.2f px into the text, Chrome %.2f"), What, Offset, ExpectedOffset)));
	}

	bool ReadTextTop(const TCHAR* DomId, float& OutTop)
	{
		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(View.GetProbe().LayoutJson), Root) || !Root.IsValid())
		{
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
		if (!Root->TryGetArrayField(TEXT("elements"), Array))
		{
			return false;
		}
		FString OwnerId;
		for (const TSharedPtr<FJsonValue>& Value : *Array)
		{
			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			FString DomIdValue;
			if (Object->TryGetStringField(TEXT("dom_id"), DomIdValue) && DomIdValue == DomId)
			{
				OwnerId = Object->GetStringField(TEXT("id"));
				break;
			}
		}
		for (const TSharedPtr<FJsonValue>& Value : *Array)
		{
			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			FString Parent;
			FString Kind;
			if (!OwnerId.IsEmpty() && Object->TryGetStringField(TEXT("parent"), Parent) && Parent == OwnerId && Object->TryGetStringField(TEXT("kind"), Kind) && Kind == TEXT("text"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Ascent = nullptr;
				const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
				if (Object->TryGetArrayField(TEXT("ascent"), Ascent) && Ascent->Num() > 0 && Object->TryGetArrayField(TEXT("lines"), Lines) && Lines->Num() > 0)
				{
					OutTop = static_cast<float>((*Lines)[0]->AsObject()->GetNumberField(TEXT("baseline")) - (*Ascent)[0]->AsNumber());
					return true;
				}
			}
		}
		return false;
	}

	void ExpectHeight(const TCHAR* DomId, float Chrome)
	{
		const FArcRmlTestLayoutElement* Box = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Box, DomId));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Box->Rect.Size.Y, Chrome, 0.01f), FString::Printf(TEXT("%s is %.3f px tall, Chrome %.3f"), DomId, Box->Rect.Size.Y, Chrome)));
	}

	float GetBaselineInBox(const TCHAR* BoxId, const FColor& GlyphColour)
	{
		const FArcRmlTestLayoutElement* Box = ArcRmlUITestHelpers::FindByDomId(Elements, BoxId);
		const TArray<FArcRmlTestDrawQuad> Quads = ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), GlyphColour);
		if (!Box || Quads.IsEmpty())
		{
			return -1000.0f;
		}
		return Quads[0].Y + Quads[0].Height - Box->Rect.Position.Y;
	}

	void ExpectBoxInRow(const TCHAR* BoxId, const TCHAR* RowId, float ChromeOffsetY, float ChromeHeight)
	{
		const FArcRmlTestLayoutElement* Box = ArcRmlUITestHelpers::FindByDomId(Elements, BoxId);
		const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, RowId);
		ASSERT_THAT(IsNotNull(Box, BoxId));
		ASSERT_THAT(IsNotNull(Row, RowId));
		const float OffsetY = Box->Rect.Position.Y - Row->Rect.Position.Y;
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(OffsetY, ChromeOffsetY, 0.01f) && FMath::IsNearlyEqual(Box->Rect.Size.Y, ChromeHeight, 0.01f),
			FString::Printf(TEXT("%s sits %.3f px below its line box and is %.3f px tall, Chrome %.3f and %.3f"), BoxId, OffsetY, Box->Rect.Size.Y, ChromeOffsetY,
				ChromeHeight)));
	}

	void ExpectLineCount(const TCHAR* DomId, int32 Chrome)
	{
		ASSERT_THAT(AreEqual(Chrome, ArcRmlUITestHelpers::GetLines(Elements, DomId).Num(), DomId));
	}

	TEST_METHOD(KernedParagraph_BreaksAndWidthsMatchChrome)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectLines(TEXT("kerned"), KernedLines, UE_ARRAY_COUNT(KernedLines));
	}

	TEST_METHOD(BreaksAtUnsafeKerningPairs_MatchChromeAndNeverExceedTheirBox)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (const FChromeBox& Sweep : Sweeps)
		{
			ExpectLines(Sweep.DomId, Sweep.Lines, Sweep.NumLines, Sweep.BoxWidth);
		}
	}

	TEST_METHOD(LineThatFitsOnlyWithKerningAcrossASpace_StaysOnOneLine)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectLines(TEXT("fit-ya"), FitYaLines, UE_ARRAY_COUNT(FitYaLines), 104.0f);
		ExpectLines(TEXT("fit-tokyo"), FitTokyoLines, UE_ARRAY_COUNT(FitTokyoLines), 103.0f);
	}

	TEST_METHOD(LineHeightNormal_MatchesChromeBlinkMetrics)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineMetricsTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectHeight(TEXT("lh15"), 19.0f);
		ExpectHeight(TEXT("lh16"), 21.0f);
	}

	TEST_METHOD(LeadingAboveTheBaseline_IsHalfTheLeadingRoundedDown)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineMetricsTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectHeight(TEXT("jbm9"), 13.5f);
		float TextTop = 0.0f;
		ASSERT_THAT(IsTrue(ReadTextTop(TEXT("jbm9"), TextTop), TEXT("the dump has no text line for jbm9")));
		const FArcRmlTestLayoutElement* Box = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("jbm9"));
		ASSERT_THAT(IsNotNull(Box));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(TextTop - Box->Rect.Position.Y, 0.0f, 0.05f),
			FString::Printf(TEXT("JetBrains Mono 9px with line-height 1.5 puts the text top %.3f px below its box, Chrome 0"), TextTop - Box->Rect.Position.Y)));
	}

	TEST_METHOD(BreakAllLongWord_BreaksIntoChromeLines)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineMetricsTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectLines(TEXT("longword"), LongWordLines, UE_ARRAY_COUNT(LongWordLines), 200.0f);
	}

	TEST_METHOD(BreakAllWithRightAlignment_LeavesTheCollapsedSpaceOutOfTheLine)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineEdgeProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestTextLine> Left = ArcRmlUITestHelpers::GetLines(Elements, TEXT("ba-space"));
		const TArray<FArcRmlTestTextLine> Right = ArcRmlUITestHelpers::GetLines(Elements, TEXT("ba-space-r"));
		ASSERT_THAT(AreEqual(2, Left.Num(), ArcRmlCssParityLineBreakTestsPrivate::DescribeLines(Left)));
		ASSERT_THAT(AreEqual(2, Right.Num(), ArcRmlCssParityLineBreakTestsPrivate::DescribeLines(Right)));
		ASSERT_THAT(AreEqual(FString(TEXT("ab")), Right[0].Text, TEXT("the first line holds no space")));
		ASSERT_THAT(AreEqual(FString(TEXT("cd")), Right[1].Text));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Right[0].Width, 19.516f, 0.05f), FString::Printf(TEXT("ab is %.3f px wide, Chrome 19.516"), Right[0].Width)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Right[0].X, 28.484f, 0.05f), FString::Printf(TEXT("right-aligned ab starts at x %.3f, Chrome 28.484"), Right[0].X)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Right[1].X, 28.125f, 0.05f), FString::Printf(TEXT("right-aligned cd starts at x %.3f, Chrome 28.125"), Right[1].X)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Left[1].Width, 19.875f, 0.05f), FString::Printf(TEXT("cd is %.3f px wide, Chrome 19.875"), Left[1].Width)));
	}

	TEST_METHOD(BreakAllTrailingSpaceThatDoesNotFit_HangsOnTheSameLine)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineEdgeProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("ba-trail"));
		ASSERT_THAT(AreEqual(1, Lines.Num(), ArcRmlCssParityLineBreakTestsPrivate::DescribeLines(Lines)));
		ASSERT_THAT(AreEqual(FString(TEXT("abcdef")), Lines[0].Text, TEXT("no line of its own holds the hanging space")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Lines[0].Width, 55.609f, 0.05f), FString::Printf(TEXT("abcdef is %.3f px wide, Chrome 55.609"), Lines[0].Width)));
	}

	TEST_METHOD(LineStartingAtAnUnsafeBreak_IsShapedWithoutTheHangingSpace)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectLines(TEXT("vat-0"), VatNarrowLines, UE_ARRAY_COUNT(VatNarrowLines), 46.5f);
	}

	TEST_METHOD(FallbackFaceInANormalLine_RaisesTheLineHeightLikeChrome)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineEdgeProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectHeight(TEXT("mix"), 26.0f);
		ExpectHeight(TEXT("plain"), 24.0f);
		ExpectHeight(TEXT("mix-fixed"), 20.0f);
	}

	TEST_METHOD(TextInputBaseline_MatchesADivWithTheSameLineHeight)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineEdgeProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const float DivBaseline = GetBaselineInBox(TEXT("div15"), DivGlyphColour);
		const float InputBaseline = GetBaselineInBox(TEXT("in15"), InputGlyphColour);
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(DivBaseline, 17.0f, 0.01f), FString::Printf(TEXT("the div baseline is %.3f px down, Chrome 17"), DivBaseline)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(InputBaseline, 17.0f, 0.01f), FString::Printf(TEXT("the input baseline is %.3f px down, Chrome 17"), InputBaseline)));
	}

	TEST_METHOD(InlineFlexAndGridBaselines_PlaceBoxesLikeChrome)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineEdgeProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectBoxInRow(TEXT("empty-flex"), TEXT("ib-row"), 11.0f, 2.0f);
		ExpectBoxInRow(TEXT("empty-grid"), TEXT("gr-row"), 11.0f, 2.0f);
		ExpectBoxInRow(TEXT("badge-box"), TEXT("fx-badge"), 2.0f, 18.0f);
		ExpectBoxInRow(TEXT("badge-img"), TEXT("fx-img"), 0.0f, 20.0f);
		ExpectBoxInRow(TEXT("badge-two"), TEXT("fx-two"), 2.0f, 29.0f);
	}

	TEST_METHOD(WinAndTypoMetrics_GiveChromeLineHeights)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineEdgeProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectHeight(TEXT("win-rob20"), 24.0f);
		ExpectHeight(TEXT("win-rob48"), 58.0f);
		ExpectHeight(TEXT("typo-dev20"), 26.0f);
		ExpectHeight(TEXT("typo-dev48"), 63.0f);
	}

	TEST_METHOD(LineFitOnTheSixtyFourthGrid_WrapsLikeChrome)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineEdgeProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectLineCount(TEXT("fit-under"), 2);
		ExpectLineCount(TEXT("fit-48"), 2);
		ExpectLineCount(TEXT("fit-off"), 1);
		ExpectLineCount(TEXT("fit-49"), 1);
		ExpectLineCount(TEXT("fit-exact"), 1);
	}

	TEST_METHOD(FontListReadThroughVar_TakesTheFallbackFaceForAMissingGlyph)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("LineEdgeProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestTextLine> Mono = ArcRmlUITestHelpers::GetLines(Elements, TEXT("vm"));
		const TArray<FArcRmlTestTextLine> Reference = ArcRmlUITestHelpers::GetLines(Elements, TEXT("vm-ref"));
		ASSERT_THAT(AreEqual(1, Mono.Num()));
		ASSERT_THAT(AreEqual(1, Reference.Num()));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Mono[0].Width, 19.609f, 0.05f) && FMath::IsNearlyEqual(Reference[0].Width, 19.609f, 0.05f),
			FString::Printf(TEXT("U+01C4 through var(--font-mono) is %.3f px and in Space Grotesk %.3f px, Chrome 19.609 for both"), Mono[0].Width, Reference[0].Width)));
	}

	TEST_METHOD(EquipmentFooterKeyHints_SitLikeChromeAtDpFourThirds)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("EquipmentFooter"), 4.0f / 3.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		struct FHint
		{
			const TCHAR* Path;
			float OffsetY;
			float Height;
		};
		const FArcRmlTestLayoutElement* Footer = ArcRmlUITestHelpers::FindByPath(Elements, TEXT("0.0.1.9"));
		ASSERT_THAT(IsNotNull(Footer, TEXT("the footer 0.0.1.9 of the TabMenu frame")));
		const FHint Hints[] = {
			{TEXT("0.0.1.9.1.0"), 5.672f, 22.656f},
			{TEXT("0.0.1.9.3.1.0"), 6.656f, 19.0f},
			{TEXT("0.0.1.9.11.0"), 5.672f, 22.656f},
		};
		for (const FHint& Hint : Hints)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByPath(Elements, Hint.Path);
			ASSERT_THAT(IsNotNull(Element, Hint.Path));
			const float OffsetY = Element->Rect.Position.Y - Footer->Rect.Position.Y;
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(OffsetY, Hint.OffsetY, 0.03f) && FMath::IsNearlyEqual(Element->Rect.Size.Y, Hint.Height, 0.03f),
				FString::Printf(TEXT("%s sits %.3f px below the footer and is %.3f px tall, Chrome %.3f and %.3f"), Hint.Path, OffsetY, Element->Rect.Size.Y,
					Hint.OffsetY, Hint.Height)));
		}
	}

	TEST_METHOD(RunBreakProbe_MatchesChromeLines)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakProbe")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectLines(TEXT("kerned"), KernedLines, UE_ARRAY_COUNT(KernedLines), 260.0f);
		ExpectLines(TEXT("mono"), ProbeMonoLines, UE_ARRAY_COUNT(ProbeMonoLines), 150.0f);
		ExpectLines(TEXT("long"), ProbeLongLines, UE_ARRAY_COUNT(ProbeLongLines), 520.0f);
	}

	TEST_METHOD(BreakAllWord_BreaksIntoChromeLineWidths)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("BreakAllWord")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("huge"));
		ASSERT_THAT(AreEqual(UE_ARRAY_COUNT(HugeWordWidths), Lines.Num(), DescribeLines(Lines)));
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Lines[Index].Width, HugeWordWidths[Index], WidthTolerance),
				FString::Printf(TEXT("line %d is %.3f px, Chrome %.3f"), Index, Lines[Index].Width, HugeWordWidths[Index])));
		}
	}

	TEST_METHOD(MonoBreakAllInsideCaltSequences_MatchesChrome)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectLines(TEXT("mono-wrap"), MonoWrapLines, UE_ARRAY_COUNT(MonoWrapLines));
	}

	TEST_METHOD(InputCaretAroundALigature_SitsOnTheClusterBoundary)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ClickCentre(TEXT("in-lig"))));
		PressTimes(EKeys::End, 1);
		ExpectCaret(TEXT("in-lig"), LigatureCaretColour, WidthOfOffice, TEXT("after office"));
		PressTimes(EKeys::Left, 2);
		ExpectCaret(TEXT("in-lig"), LigatureCaretColour, WidthOfOffi, TEXT("after the ffi ligature"));
		PressTimes(EKeys::Left, 2);
		ExpectCaret(TEXT("in-lig"), LigatureCaretColour, WidthOfO + (WidthOfOffi - WidthOfO) / 3.0f, TEXT("inside the ligature after its first f"));
	}

	TEST_METHOD(InputCaretAroundABidiControl_StaysOnTheGraphemeBoundary)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ClickCentre(TEXT("in-bidi"))));
		PressTimes(EKeys::End, 1);
		PressTimes(EKeys::Left, 1);
		ExpectCaret(TEXT("in-bidi"), BidiCaretColour, WidthOfA, TEXT("between the bidi control and b"));
		PressTimes(EKeys::Left, 1);
		ExpectCaret(TEXT("in-bidi"), BidiCaretColour, WidthOfA, TEXT("between a and the bidi control"));
	}

	TEST_METHOD(TextAreaCaretAtTheEndOfALine_SitsAtTheFullWidthOfTheLine)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ClickCentre(TEXT("ta"))));
		PressTimes(EKeys::Up, 1);
		PressTimes(EKeys::Home, 1);
		PressTimes(EKeys::Right, 2);
		ExpectCaret(TEXT("ta"), TextAreaCaretColour, WidthOfAb, TEXT("end of the first textarea line"));
	}

	TEST_METHOD(TextAreaSelectionToTheEndOfALine_CoversTheFullWidthOfTheLine)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ClickCentre(TEXT("ta"))));
		PressTimes(EKeys::Up, 1);
		PressTimes(EKeys::Home, 1);
		PressTimes(EKeys::End, 1, FModifierKeysState(true, false, false, false, false, false, false, false, false));
		const FArcRmlTestDraw* Selection = FindFlat(View.GetDraws(), SelectionColour);
		ASSERT_THAT(IsNotNull(Selection, FString::Printf(TEXT("no selection background draw; selection range %d..%d"), View.GetSnapshot().TextField.SelectionStart,
			View.GetSnapshot().TextField.SelectionEnd)));
		const float Width = Selection->Bounds.Max.X - Selection->Bounds.Min.X;
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Width, WidthOfAb, CaretTolerance),
			FString::Printf(TEXT("the selection is %.2f px wide, Chrome width of ab %.2f"), Width, WidthOfAb)));
	}

#if WITH_EDITOR
	TEST_METHOD(FontReload_KeepsDrawingTheSameGlyphsAtTheSameWidths)
	{
		using namespace ArcRmlCssParityLineBreakTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RunBreakTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const int32 Before = CountQuads(View.GetDraws(), KernedColour, true);
		ASSERT_THAT(IsTrue(Before > 50, FString::Printf(TEXT("the paragraph draws %d glyphs before the reload"), Before)));
		FArcRmlTestView::ReloadFonts({TEXT("/ArcRmlUITest/UI/Rml/Fonts/SpaceGrotesk-Regular.ttf")});
		int32 After = 0;
		for (int32 Round = 0; Round < 40 && After != Before; ++Round)
		{
			ASSERT_THAT(IsTrue(View.Flush()));
			FPlatformProcess::Sleep(0.02f);
			After = CountQuads(View.GetDraws(), KernedColour, true);
		}
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Elements)));
		After = CountQuads(View.GetDraws(), KernedColour, true);
		ASSERT_THAT(AreEqual(Before, After, TEXT("glyphs drawn after the font reload")));
		ExpectLines(TEXT("kerned"), KernedLines, UE_ARRAY_COUNT(KernedLines));
	}
#endif

	TEST_METHOD(ZeroFontSize_RecordsNoGlyphQuads)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ScaleTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FColor ZeroColour(0x20, 0xa0, 0xd0, 0xff);
		ASSERT_THAT(AreEqual(0, ArcRmlCssParityLineBreakTestsPrivate::CountQuads(View.GetDraws(), ZeroColour, false), TEXT("quads of font-size: 0 text")));
	}
};
