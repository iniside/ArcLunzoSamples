// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace ArcRmlRangePartTestsPrivate
{
	const TCHAR* const Fixture = TEXT("Form/RangePartSelectorsTest.rml");
	const TCHAR* const ModelName = TEXT("rp");
	const TCHAR* const Red = TEXT("#ff0000ff");
	const TCHAR* const Blue = TEXT("#0000ffff");
	const TCHAR* const Green = TEXT("#008000ff");
	const TCHAR* const Orange = TEXT("#ffa500ff");
	const TCHAR* const DefaultTrack = TEXT("#efefefff");
	const TCHAR* const DefaultAccent = TEXT("#0075ffff");
	constexpr float Tolerance = 0.5f;

	struct FRangeParts
	{
		const FArcRmlTestLayoutElement* Track = nullptr;
		const FArcRmlTestLayoutElement* Thumb = nullptr;
		const FArcRmlTestLayoutElement* Progress = nullptr;
	};

	const FArcRmlTestLayoutElement* FindChild(const TArray<FArcRmlTestLayoutElement>& Layout, const FString& ParentPath, const TCHAR* Tag)
	{
		return Layout.FindByPredicate([&ParentPath, Tag](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Parent == ParentPath && Element.Tag == Tag;
		});
	}

	FRangeParts FindParts(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* DomId)
	{
		FRangeParts Parts;
		const FArcRmlTestLayoutElement* Input = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Input)
		{
			return Parts;
		}
		Parts.Track = FindChild(Layout, Input->Path, TEXT("slidertrack"));
		Parts.Thumb = FindChild(Layout, Input->Path, TEXT("sliderbar"));
		Parts.Progress = Parts.Track ? FindChild(Layout, Parts.Track->Path, TEXT("sliderprogress")) : nullptr;
		return Parts;
	}

	bool Near(float Value, float Expected)
	{
		return FMath::Abs(Value - Expected) <= Tolerance;
	}

	FString DescribeStyled(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* DomId, float ThumbWidth)
	{
		const FRangeParts Parts = FindParts(Layout, DomId);
		if (!Parts.Track || !Parts.Thumb)
		{
			return FString::Printf(TEXT("[%s has no track or thumb] "), DomId);
		}
		FString Mismatches;
		if (!Near(Parts.Track->Rect.Size.Y, 6.0f) || Parts.Track->BackgroundColor != Blue)
		{
			Mismatches += FString::Printf(TEXT("[%s track is %.2f high %s, expected the part rule's 6 %s] "), DomId, Parts.Track->Rect.Size.Y,
				*Parts.Track->BackgroundColor, Blue);
		}
		if (!Near(Parts.Thumb->Rect.Size.X, ThumbWidth) || !Near(Parts.Thumb->Rect.Size.Y, 16.0f) || Parts.Thumb->BackgroundColor != Green)
		{
			Mismatches += FString::Printf(TEXT("[%s thumb is %.2fx%.2f %s, expected the part rule's %.2fx16 %s] "), DomId, Parts.Thumb->Rect.Size.X,
				Parts.Thumb->Rect.Size.Y, *Parts.Thumb->BackgroundColor, ThumbWidth, Green);
		}
		return Mismatches;
	}

	FString DescribeSelectedColours(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* DomId, bool bStyled)
	{
		const FRangeParts Parts = FindParts(Layout, DomId);
		if (!Parts.Track || !Parts.Thumb)
		{
			return FString::Printf(TEXT("[%s has no track or thumb] "), DomId);
		}
		if ((Parts.Track->BackgroundColor == Blue) == bStyled && (Parts.Thumb->BackgroundColor == Green) == bStyled)
		{
			return FString();
		}
		return FString::Printf(TEXT("[%s track %s, thumb %s, expected the part rules' %s and %s %s] "), DomId, *Parts.Track->BackgroundColor,
			*Parts.Thumb->BackgroundColor, Blue, Green, bStyled ? TEXT("to apply") : TEXT("not to apply"));
	}

	// Chrome's default slider (the widget's local properties): an 8 px #efefef track, a 16x16 thumb and an 8 px progress in the
	// auto accent. Every colour and the track's height differ from the part rules' (blue 6 px track, green thumb) and the tag rules'
	// (red, 3 px, 40 px wide, hidden).
	FString DescribeDefaultLook(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* DomId)
	{
		const FRangeParts Parts = FindParts(Layout, DomId);
		if (!Parts.Track || !Parts.Thumb || !Parts.Progress)
		{
			return FString::Printf(TEXT("[%s has no track, thumb or progress] "), DomId);
		}
		FString Mismatches;
		if (!Near(Parts.Track->Rect.Size.Y, 8.0f) || Parts.Track->BackgroundColor != DefaultTrack || !Parts.Track->bVisible)
		{
			Mismatches += FString::Printf(TEXT("[%s track is %.2f high %s%s, expected the default look's visible 8 %s] "), DomId, Parts.Track->Rect.Size.Y,
				*Parts.Track->BackgroundColor, Parts.Track->bVisible ? TEXT("") : TEXT(" hidden"), DefaultTrack);
		}
		if (!Near(Parts.Thumb->Rect.Size.X, 16.0f) || !Near(Parts.Thumb->Rect.Size.Y, 16.0f) || Parts.Thumb->BackgroundColor != DefaultAccent
			|| !Parts.Thumb->bVisible)
		{
			Mismatches += FString::Printf(TEXT("[%s thumb is %.2fx%.2f %s%s, expected the default look's visible 16x16 %s] "), DomId, Parts.Thumb->Rect.Size.X,
				Parts.Thumb->Rect.Size.Y, *Parts.Thumb->BackgroundColor, Parts.Thumb->bVisible ? TEXT("") : TEXT(" hidden"), DefaultAccent);
		}
		if (!Near(Parts.Progress->Rect.Size.Y, 8.0f) || Parts.Progress->BackgroundColor != DefaultAccent || !Parts.Progress->bVisible)
		{
			Mismatches += FString::Printf(TEXT("[%s progress is %.2f high %s%s, expected the default look's visible 8 %s] "), DomId, Parts.Progress->Rect.Size.Y,
				*Parts.Progress->BackgroundColor, Parts.Progress->bVisible ? TEXT("") : TEXT(" hidden"), DefaultAccent);
		}
		return Mismatches;
	}

	const FArcRmlInspectorSnapshot* FindLastSnapshot(const FArcRmlTestView& View)
	{
		const TArray<FArcRmlInspectorAnswer>& Answers = View.GetInspectorAnswers();
		for (int32 Index = Answers.Num() - 1; Index >= 0; --Index)
		{
			if (Answers[Index].Snapshot.IsValid())
			{
				return Answers[Index].Snapshot.Get();
			}
		}
		return nullptr;
	}
}

// Chrome's layout dump has no slider part boxes, so the part checks are fork-internal; the @supports boxes are Chrome's.
TEST_CLASS(ArcRmlCssParityRangePartTests, "ArcRmlUI.CssParity.RangeParts")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		using namespace ArcRmlRangePartTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), TEXT("RangePartSelectors.rcssc")), false, true, true);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("RangeParts"))));
		return View.Flush() && View.Flush();
	}

	TEST_METHOD(PartRules_ApplyOnlyWhenTheInputComputesAppearanceNone)
	{
		using namespace ArcRmlRangePartTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		FString Mismatches;
		Mismatches += DescribeStyled(Layout, TEXT("rp-none"), 16.0f);
		Mismatches += DescribeStyled(Layout, TEXT("rp-wk"), 16.0f);
		Mismatches += DescribeStyled(Layout, TEXT("rp-later-none"), 16.0f);
		Mismatches += DescribeStyled(Layout, TEXT("rp-id"), 24.0f);
		Mismatches += DescribeDefaultLook(Layout, TEXT("rp-auto"));
		Mismatches += DescribeDefaultLook(Layout, TEXT("rp-later-auto"));
		Mismatches += DescribeDefaultLook(Layout, TEXT("rp-bound"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(OrdinarySelectors_NoLongerReachRangeParts)
	{
		using namespace ArcRmlRangePartTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		FString Mismatches = DescribeDefaultLook(Layout, TEXT("rp-plain"));
		const FRangeParts Parts = FindParts(Layout, TEXT("rp-none"));
		if (!Parts.Progress || Parts.Progress->BackgroundColor == Red || !Near(Parts.Progress->Rect.Size.Y, 6.0f))
		{
			Mismatches += FString::Printf(TEXT("[rp-none progress is %s, expected no rule: the track's 6 high, not %s] "),
				Parts.Progress ? *FString::Printf(TEXT("%.2f high %s"), Parts.Progress->Rect.Size.Y, *Parts.Progress->BackgroundColor) : TEXT("missing"), Red);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(AppearanceChangedThroughABinding_ReselectsThePartRules)
	{
		using namespace ArcRmlRangePartTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("look")), ArcRmlUITestHelpers::MakeString(TEXT("none")));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		Mismatches += DescribeSelectedColours(Layout, TEXT("rp-bound"), true);
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("look")), ArcRmlUITestHelpers::MakeString(TEXT("auto")));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		Mismatches += DescribeSelectedColours(Layout, TEXT("rp-bound"), false);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(HoveredThumb_TakesTheThumbHoverRule)
	{
		using namespace ArcRmlRangePartTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		const FRangeParts Before = FindParts(Layout, TEXT("rp-none"));
		ASSERT_THAT(IsTrue(Before.Thumb != nullptr && Before.Track != nullptr, TEXT("rp-none has no thumb or track")));
		const FVector2f Centre = Before.Thumb->Rect.Position + Before.Thumb->Rect.Size * 0.5f;
		const FVector2f TrackOnly(Before.Track->Rect.Position.X + 4.0f, Before.Track->Rect.Position.Y + Before.Track->Rect.Size.Y * 0.5f);
		ASSERT_THAT(IsTrue(TrackOnly.X < Before.Thumb->Rect.Position.X, TEXT("the track point is not left of the thumb")));
		FString Mismatches;
		View.MouseMove(TrackOnly);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		const FRangeParts OverTrack = FindParts(Layout, TEXT("rp-none"));
		if (!OverTrack.Thumb || OverTrack.Thumb->BackgroundColor != Green)
		{
			Mismatches += FString::Printf(TEXT("[thumb with the pointer on the track away from it is %s, expected the unhovered %s: the input is hovered, the thumb is not] "),
				OverTrack.Thumb ? *OverTrack.Thumb->BackgroundColor : TEXT("missing"), Green);
		}
		View.MouseMove(Centre);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		const FRangeParts After = FindParts(Layout, TEXT("rp-none"));
		if (!After.Thumb || After.Thumb->BackgroundColor != Orange)
		{
			Mismatches += FString::Printf(TEXT("[hovered thumb is %s, expected %s] "), After.Thumb ? *After.Thumb->BackgroundColor : TEXT("missing"), Orange);
		}
		if (!After.Track || After.Track->BackgroundColor != Blue)
		{
			Mismatches += FString::Printf(TEXT("[track under the hovered thumb is %s, expected %s] "), After.Track ? *After.Track->BackgroundColor : TEXT("missing"), Blue);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(AccentColorAndAppearance_ParseAsChrome)
	{
		using namespace ArcRmlRangePartTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		FString Mismatches;
		for (const TCHAR* DomId : {TEXT("sup-accent-auto"), TEXT("sup-accent-color"), TEXT("sup-appearance"), TEXT("sup-wk-appearance"), TEXT("sup-thumb"),
				 TEXT("sup-track"), TEXT("sup-thumb-hover")})
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
			if (!Element || !Near(Element->Rect.Size.Y, 10.0f))
			{
				Mismatches += FString::Printf(TEXT("[%s is %s high, Chrome 10: its @supports condition is true] "), DomId,
					Element ? *FString::Printf(TEXT("%.2f"), Element->Rect.Size.Y) : TEXT("missing"));
			}
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// rp-bound's appearance follows the look binding after the first layout; the parts must be laid out again with the sizes of
	// the newly selected look (the part rules: a 6 px track with the 16x16 thumb at its top, centred on 50 of a 200 px input).
	TEST_METHOD(AppearanceChangedThroughABinding_LaysThePartsOutAgain)
	{
		using namespace ArcRmlRangePartTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		FString Mismatches = DescribeDefaultLook(Layout, TEXT("rp-bound"));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("look")), ArcRmlUITestHelpers::MakeString(TEXT("none")));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		Mismatches += DescribeStyled(Layout, TEXT("rp-bound"), 16.0f);
		const FArcRmlTestLayoutElement* Input = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("rp-bound"));
		const FRangeParts Parts = FindParts(Layout, TEXT("rp-bound"));
		if (Input && Parts.Track && Parts.Thumb && Parts.Progress)
		{
			const float TrackTop = Input->Rect.Position.Y + (Input->Rect.Size.Y - 6.0f) * 0.5f;
			const float ThumbLeft = Input->Rect.Position.X + (Input->Rect.Size.X - 16.0f) * 0.5f;
			if (!Near(Parts.Track->Rect.Position.Y, TrackTop) || !Near(Parts.Thumb->Rect.Position.Y, TrackTop) || !Near(Parts.Thumb->Rect.Position.X, ThumbLeft)
				|| !Near(Parts.Progress->Rect.Size.Y, 6.0f))
			{
				Mismatches += FString::Printf(TEXT("[rp-bound with appearance none: track top %.2f, thumb %.2f,%.2f, progress %.2f high; expected track and thumb top %.2f, thumb left %.2f, progress 6] "),
					Parts.Track->Rect.Position.Y, Parts.Thumb->Rect.Position.X, Parts.Thumb->Rect.Position.Y, Parts.Progress->Rect.Size.Y, TrackTop, ThumbLeft);
			}
		}
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("look")), ArcRmlUITestHelpers::MakeString(TEXT("auto")));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		Mismatches += DescribeDefaultLook(Layout, TEXT("rp-bound"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(LayoutDump_CallsThePartsSlider)
	{
		using namespace ArcRmlRangePartTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		FString Mismatches;
		for (const TCHAR* DomId : {TEXT("rp-none"), TEXT("rp-auto")})
		{
			const FRangeParts Parts = FindParts(Layout, DomId);
			for (const FArcRmlTestLayoutElement* Part : {Parts.Track, Parts.Thumb, Parts.Progress})
			{
				if (!Part || Part->Kind != TEXT("slider"))
				{
					Mismatches += FString::Printf(TEXT("[%s part %s is kind '%s', expected 'slider'] "), DomId, Part ? *Part->Tag : TEXT("missing"),
						Part ? *Part->Kind : TEXT(""));
				}
			}
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// #app-child is a div under an appearance: auto parent; #app-probe inside it takes appearance: inherit, which reads
	// #app-child's resolved (specified) appearance. Chrome computes none for both (Saved/ArcRml/RangePartsChrome/computed.json),
	// so the part rules apply to #app-probe; were appearance inherited, #app-child would resolve to auto and they would not.
	// #app-direct-probe inherits straight from an auto parent and shows the default look, so the inherit probe can see auto.
	// #acc-child keeps the user-agent appearance: auto and draws the default look in the accent-color: red it inherits from .acc.
	TEST_METHOD(AccentColor_IsInheritedAndAppearanceIsNot)
	{
		using namespace ArcRmlRangePartTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		FString Mismatches = DescribeStyled(Layout, TEXT("app-probe"), 16.0f);
		Mismatches += DescribeDefaultLook(Layout, TEXT("app-direct-probe"));
		const FRangeParts Accent = FindParts(Layout, TEXT("acc-child"));
		if (!Accent.Progress || !Accent.Thumb || Accent.Progress->BackgroundColor != Red || Accent.Thumb->BackgroundColor != Red)
		{
			Mismatches += FString::Printf(TEXT("[#acc-child progress %s, thumb %s, expected the inherited accent %s] "),
				Accent.Progress ? *Accent.Progress->BackgroundColor : TEXT("missing"), Accent.Thumb ? *Accent.Thumb->BackgroundColor : TEXT("missing"), Red);
		}
		View.InspectEdit(TEXT("#acc-child"), FString(), FString());
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot));
		bool bAccentInherited = false;
		for (const FArcRmlInspectorRule& Rule : Snapshot->Rules)
		{
			if (!Rule.InheritedFrom.Contains(TEXT("acc-parent")))
			{
				continue;
			}
			for (const FArcRmlInspectorProperty& Property : Rule.Properties)
			{
				bAccentInherited |= Property.Name == TEXT("accent-color");
			}
		}
		if (!bAccentInherited)
		{
			Mismatches += TEXT("[accent-color of #acc-parent is not inherited by #acc-child] ");
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};
