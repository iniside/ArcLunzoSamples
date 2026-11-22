// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlLineBreakTestsPrivate
{
	const TCHAR* const ChipText = TEXT("T3 · Expert");
	constexpr float ChipWidthAtDpOne = 67.11f;
	constexpr float ChipWidthTolerance = 0.05f;

	FString DescribeLines(const TArray<FArcRmlTestTextLine>& Lines)
	{
		TArray<FString> Parts;
		for (const FArcRmlTestTextLine& Line : Lines)
		{
			Parts.Add(FString::Printf(TEXT("[%.2f] '%s'"), Line.Width, *Line.Text));
		}
		return FString::Printf(TEXT("%d lines: %s"), Lines.Num(), *FString::Join(Parts, TEXT(" ")));
	}

	FString FirstWord(const FString& Text)
	{
		FString Word;
		FString Rest;
		const FString Trimmed = Text.TrimStart();
		return Trimmed.Split(TEXT(" "), &Word, &Rest) ? Word : Trimmed.TrimEnd();
	}
}

TEST_CLASS(ArcRmlLineBreakTests, "ArcRmlUI.LineBreak")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenLineBreakTest()
	{
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("Text/LineBreakTest.rml"))) && View.Flush() && View.Flush()
			&& View.GetProbe().bHasDocument && View.ParseLayout(Elements);
	}

	bool ResizeAndRead(float DpRatio)
	{
		View.Resize(ArcRmlUITestHelpers::ViewSize, DpRatio);
		return View.Flush() && View.Flush() && View.ParseLayout(Elements);
	}

	float MeasureNoWrap(const FString& ProbeId, const FString& Field, const FString& Text)
	{
		View.SetModelValue(TEXT("text"), ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeString(Text));
		TArray<FArcRmlTestLayoutElement> ProbeElements;
		if (!View.Flush() || !View.ParseLayout(ProbeElements))
		{
			return -1.0f;
		}
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(ProbeElements, ProbeId);
		return Lines.Num() == 1 ? Lines[0].Width : -1.0f;
	}

	void CheckChipsOnOneLine(float DpRatio)
	{
		const TCHAR* const ChipIds[] = {TEXT("chip-inline"), TEXT("chip-flex"), TEXT("chip-abs")};
		for (const TCHAR* const ChipId : ChipIds)
		{
			const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, ChipId);
			const FString Context = FString::Printf(TEXT("%s at dp %.2f: %s"), ChipId, DpRatio, *ArcRmlLineBreakTestsPrivate::DescribeLines(Lines));
			ASSERT_THAT(AreEqual(1, Lines.Num(), Context));
			ASSERT_THAT(AreEqual(FString(ArcRmlLineBreakTestsPrivate::ChipText), Lines[0].Text.TrimEnd(), Context));
			if (DpRatio == 1.0f)
			{
				ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(ArcRmlLineBreakTestsPrivate::ChipWidthAtDpOne, Lines[0].Width, ArcRmlLineBreakTestsPrivate::ChipWidthTolerance),
					FString::Printf(TEXT("the reproducer string must measure Chrome's %.2f px as one string at dp 1; %s"), ArcRmlLineBreakTestsPrivate::ChipWidthAtDpOne, *Context)));
			}
		}
	}

	TEST_METHOD(ShrinkToFitBox_KeepsTextOnOneLine)
	{
		ASSERT_THAT(IsTrue(OpenLineBreakTest(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		CheckChipsOnOneLine(1.0f);
		ASSERT_THAT(IsTrue(ResizeAndRead(1.25f), TEXT("resize to dp 1.25 failed")));
		CheckChipsOnOneLine(1.25f);
		ASSERT_THAT(IsTrue(ResizeAndRead(1.5f), TEXT("resize to dp 1.5 failed")));
		CheckChipsOnOneLine(1.5f);
	}

	TEST_METHOD(EmptyBoundText_HasNoLines)
	{
		ASSERT_THAT(IsTrue(OpenLineBreakTest(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Owner = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("empty-bound"));
		ASSERT_THAT(IsNotNull(Owner));
		const FArcRmlTestLayoutElement* TextChild = Elements.FindByPredicate([Owner](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Parent == Owner->Path && Element.Kind == TEXT("text");
		});
		ASSERT_THAT(IsNotNull(TextChild, TEXT("the bound text node under empty-bound does not exist")));
		ASSERT_THAT(AreEqual(0, TextChild->Lines.Num(), ArcRmlLineBreakTestsPrivate::DescribeLines(TextChild->Lines)));
	}

	TEST_METHOD(LongParagraphInFlexItem_FillsItsMaxWidthAndBreaksAtIt)
	{
		ASSERT_THAT(IsTrue(OpenLineBreakTest(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Flex = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("para-flex"));
		const FArcRmlTestLayoutElement* Reference = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("para-ref"));
		ASSERT_THAT(IsNotNull(Flex));
		ASSERT_THAT(IsNotNull(Reference));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Flex->ContentSize.X, Reference->ContentSize.X, 0.5f),
			FString::Printf(TEXT("the flex paragraph is %.2f wide, its 440px max-width box %.2f"), Flex->ContentSize.X, Reference->ContentSize.X)));
		const float BoxWidth = Flex->ContentSize.X;
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("para-flex"));
		ASSERT_THAT(IsTrue(Lines.Num() > 1, ArcRmlLineBreakTestsPrivate::DescribeLines(Lines)));

		for (int32 Index = 0; Index + 1 < Lines.Num(); ++Index)
		{
			const FString LineText = Lines[Index].Text.TrimEnd();
			const FString NextWord = ArcRmlLineBreakTestsPrivate::FirstWord(Lines[Index + 1].Text);
			const float LineWidth = MeasureNoWrap(TEXT("probe-line"), TEXT("probeLine"), LineText);
			const float WithNextWord = MeasureNoWrap(TEXT("probe-next"), TEXT("probeNext"), LineText + TEXT(" ") + NextWord);
			const FString Context = FString::Printf(TEXT("line %d '%s' measures %.2f, with '%s' %.2f, box %.1f"), Index, *LineText, LineWidth, *NextWord, WithNextWord,
				BoxWidth);
			ASSERT_THAT(IsTrue(LineWidth > 0 && WithNextWord > 0, Context));
			ASSERT_THAT(IsTrue(LineWidth <= BoxWidth, Context));
			ASSERT_THAT(IsTrue(WithNextWord > BoxWidth, Context));
		}
	}
};
