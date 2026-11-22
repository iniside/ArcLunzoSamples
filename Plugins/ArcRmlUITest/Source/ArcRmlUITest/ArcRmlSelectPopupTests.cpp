// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"

namespace ArcRmlSelectPopupTestsPrivate
{
	const TCHAR* const Fixture = TEXT("Form/SelectPopup.rml");
	const TCHAR* const Sheet = TEXT("SelectPopup.rcssc");

	// Chrome 156's popup shows at most 20 rows (VENDORED 231 review fixes, long.html).
	constexpr int32 MaxRows = 20;
	// The open list's local 1 px border (WidgetDropDown constructor).
	constexpr float ListBorder = 1.0f;
	// Scroll offsets may land on whole pixels; the regressions these tests guard are 2 px (twice the list border) off.
	constexpr float ScrollTolerance = 0.75f;

	const TCHAR* const HighlightFill = TEXT("#1967d2ff");
	const TCHAR* const HighlightText = TEXT("#ffffffff");

	float Bottom(const FArcRmlTestLayoutElement& Element)
	{
		return Element.Rect.Position.Y + Element.Rect.Size.Y;
	}

	FString DescribeRect(const FArcRmlTestLayoutElement* Element)
	{
		return Element ? FString::Printf(TEXT("(%.1f,%.1f %.1fx%.1f scroll %.1f)"), Element->Rect.Position.X, Element->Rect.Position.Y, Element->Rect.Size.X,
			Element->Rect.Size.Y, Element->Scroll.Y)
			: FString(TEXT("<none>"));
	}
}

// The select popup's review fixes (rml-forms step 9, VENDORED 231 "Review fixes"): the 20-row cap, the scroll to the selected row on
// open (Center) and to the highlight (Nearest) inside the list's 1 px border, and the highlight putting the option's own local
// colours back when it leaves. Driven through the view's pointer and key input; read from the layout dump.
TEST_CLASS(ArcRmlSelectPopupTests, "ArcRmlUI.Form.SelectPopup")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;
	FString Mismatches;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		using namespace ArcRmlSelectPopupTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && Settle();
	}

	bool Settle()
	{
		return View.FlushUntilSettled() && View.Flush() && View.ParseLayout(Layout);
	}

	bool OpenList(const TCHAR* DomId)
	{
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Select)
		{
			return false;
		}
		const FVector2f Centre = Select->Rect.Position + Select->Rect.Size * 0.5f;
		View.MouseMove(Centre);
		View.MouseClick(Centre);
		return Settle() && View.GetSnapshot().bSelectListOpen;
	}

	bool Press(const FKey& Key, int32 Times = 1)
	{
		for (int32 Index = 0; Index < Times; ++Index)
		{
			View.PressKey(Key);
			if (!Settle())
			{
				return false;
			}
		}
		return true;
	}

	const FArcRmlTestLayoutElement* FindSelectBox(const TCHAR* DomId) const
	{
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Select)
		{
			return nullptr;
		}
		const FString PartPrefix = Select->Path + TEXT("~");
		return Layout.FindByPredicate([&PartPrefix](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Path.StartsWith(PartPrefix) && Element.Tag == TEXT("selectbox");
		});
	}

	// The list's options in document order.
	TArray<const FArcRmlTestLayoutElement*> GetOptions(const FArcRmlTestLayoutElement& Box) const
	{
		TArray<const FArcRmlTestLayoutElement*> Options;
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Tag == TEXT("option") && Element.Parent == Box.Path)
			{
				Options.Add(&Element);
			}
		}
		return Options;
	}

	// A visible scrollbar track of the open list (the list scrolls).
	bool HasScrollTrack(const FArcRmlTestLayoutElement& Box) const
	{
		const FString BoxPath = Box.Path;
		return Layout.ContainsByPredicate([&BoxPath](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Path.StartsWith(BoxPath) && Element.Path != BoxPath && Element.Tag == TEXT("slidertrack") && Element.bVisible;
		});
	}

	void ExpectColours(const TCHAR* Step, const FArcRmlTestLayoutElement* Option, const TCHAR* Name, const FString& Colour, const FString& Background)
	{
		if (!Option)
		{
			Mismatches += FString::Printf(TEXT("[%s: no %s] "), Step, Name);
			return;
		}
		if (Option->Color != Colour || Option->BackgroundColor != Background)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s color %s background %s, expected %s on %s] "), Step, Name, *Option->Color, *Option->BackgroundColor, *Colour,
				*Background);
		}
	}

	// 20 options open unscrolled with every row shown; 21 open 20 rows high (the 21st option's offset from the first, inside the
	// 1 px border) and scroll, as Chrome's popup.
	TEST_METHOD(TwentyRowsFit_TwentyOneScrollAtTwentyRowsHigh)
	{
		using namespace ArcRmlSelectPopupTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(OpenList(TEXT("spp-20")), TEXT("the click did not open spp-20's list")));
		const FArcRmlTestLayoutElement* Box = FindSelectBox(TEXT("spp-20"));
		ASSERT_THAT(IsNotNull(Box));
		TArray<const FArcRmlTestLayoutElement*> Options = GetOptions(*Box);
		ASSERT_THAT(AreEqual(MaxRows, Options.Num()));
		const float AllRows = Bottom(*Options.Last()) - Options[0]->Rect.Position.Y;
		if (HasScrollTrack(*Box) || !FMath::IsNearlyEqual(Box->Rect.Size.Y, AllRows + 2.0f * ListBorder, 0.5f) || !FMath::IsNearlyZero(Box->Scroll.Y, 0.5f))
		{
			Mismatches += FString::Printf(TEXT("[20 options: list %s, scrollbar %s, expected every row (%.1f) plus the border and no scrollbar] "), *DescribeRect(Box),
				HasScrollTrack(*Box) ? TEXT("shown") : TEXT("none"), AllRows);
		}
		ASSERT_THAT(IsTrue(Press(EKeys::Escape) && !View.GetSnapshot().bSelectListOpen, TEXT("Escape did not close spp-20's list")));

		ASSERT_THAT(IsTrue(OpenList(TEXT("spp-21")), TEXT("the click did not open spp-21's list")));
		Box = FindSelectBox(TEXT("spp-21"));
		ASSERT_THAT(IsNotNull(Box));
		Options = GetOptions(*Box);
		ASSERT_THAT(AreEqual(MaxRows + 1, Options.Num()));
		ASSERT_THAT(IsTrue(FMath::IsNearlyZero(Box->Scroll.Y, 0.5f), *FString::Printf(TEXT("spp-21 opened scrolled: %s"), *DescribeRect(Box))));
		const float TwentyRows = Options[MaxRows]->Rect.Position.Y - Options[0]->Rect.Position.Y;
		if (!HasScrollTrack(*Box) || !FMath::IsNearlyEqual(Box->Rect.Size.Y, TwentyRows + 2.0f * ListBorder, 0.5f)
			|| Bottom(*Options.Last()) <= Bottom(*Box) - ListBorder + 0.5f)
		{
			Mismatches += FString::Printf(TEXT("[21 options: list %s, scrollbar %s, 21st option bottom %.1f; expected 20 rows (%.1f) plus the border, a scrollbar and the 21st cut] "),
				*DescribeRect(Box), HasScrollTrack(*Box) ? TEXT("shown") : TEXT("none"), Bottom(*Options.Last()), TwentyRows);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// spp-long (60 options, Item 40 selected) opens with Item 40 centred in the list's padding box (Center); Down to Item 51 brings
	// its bottom to the padding box's bottom and Up to Item 26 its top to the padding box's top (Nearest). Element::ScrollIntoView
	// counted the list's 1 px border twice, which put each 2 px off; none of these scrolls reaches the scroll range's ends.
	TEST_METHOD(LongList_OpenCentresTheSelectedRow_AndTheHighlightLandsAtThePaddingEdges)
	{
		using namespace ArcRmlSelectPopupTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(OpenList(TEXT("spp-long")), TEXT("the click did not open spp-long's list")));
		const FArcRmlTestLayoutElement* Box = FindSelectBox(TEXT("spp-long"));
		ASSERT_THAT(IsNotNull(Box));
		TArray<const FArcRmlTestLayoutElement*> Options = GetOptions(*Box);
		ASSERT_THAT(AreEqual(60, Options.Num()));
		const float PaddingTop = Box->Rect.Position.Y + ListBorder;
		const float PaddingBottom = Bottom(*Box) - ListBorder;
		const float SelectedCentre = Options[39]->Rect.Position.Y + Options[39]->Rect.Size.Y * 0.5f;
		if (Box->Scroll.Y <= 0.0f || !FMath::IsNearlyEqual(SelectedCentre, (PaddingTop + PaddingBottom) * 0.5f, ScrollTolerance))
		{
			Mismatches += FString::Printf(TEXT("[open: Item 40 centre %.1f, list %s, expected the padding box centre %.1f] "), SelectedCentre, *DescribeRect(Box),
				(PaddingTop + PaddingBottom) * 0.5f);
		}
		const float ScrollRange = Bottom(*Options.Last()) - Options[0]->Rect.Position.Y - (PaddingBottom - PaddingTop);

		ASSERT_THAT(IsTrue(Press(EKeys::Down, 11)));
		Box = FindSelectBox(TEXT("spp-long"));
		ASSERT_THAT(IsNotNull(Box));
		Options = GetOptions(*Box);
		ASSERT_THAT(AreEqual(60, Options.Num()));
		if (Box->Scroll.Y >= ScrollRange - 1.0f || !FMath::IsNearlyEqual(Bottom(*Options[50]), Bottom(*Box) - ListBorder, ScrollTolerance))
		{
			Mismatches += FString::Printf(TEXT("[Down to Item 51: its bottom %.1f, list %s, expected the padding box bottom %.1f below the scroll range end %.1f] "),
				Bottom(*Options[50]), *DescribeRect(Box), Bottom(*Box) - ListBorder, ScrollRange);
		}

		ASSERT_THAT(IsTrue(Press(EKeys::Up, 25)));
		Box = FindSelectBox(TEXT("spp-long"));
		ASSERT_THAT(IsNotNull(Box));
		Options = GetOptions(*Box);
		ASSERT_THAT(AreEqual(60, Options.Num()));
		if (Box->Scroll.Y <= 1.0f || !FMath::IsNearlyEqual(Options[25]->Rect.Position.Y, Box->Rect.Position.Y + ListBorder, ScrollTolerance))
		{
			Mismatches += FString::Printf(TEXT("[Up to Item 26: its top %.1f, list %s, expected the padding box top %.1f] "), Options[25]->Rect.Position.Y,
				*DescribeRect(Box), Box->Rect.Position.Y + ListBorder);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// spp-hl: Red has an inline color and background-color, Blue a data-style-color bound to ink, Apple and Plain none. The
	// highlight paints each white on rgb(25, 103, 210) while on it and puts the option's own local colours back (or none) when it
	// moves on, so the author colours survive.
	TEST_METHOD(HighlightMovingOn_GivesInlineAndBoundColoursBack)
	{
		using namespace ArcRmlSelectPopupTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("spp-hl"));
		ASSERT_THAT(IsNotNull(Select));
		const FString SelectColour = Select->Color;
		ASSERT_THAT(IsTrue(OpenList(TEXT("spp-hl")), TEXT("the click did not open spp-hl's list")));

		const TCHAR* const Names[] = {TEXT("Apple"), TEXT("Red"), TEXT("Blue"), TEXT("Plain")};
		const FString OwnColour[] = {SelectColour, TEXT("#ff0000ff"), TEXT("#0000ffff"), SelectColour};
		const FString OwnBackground[] = {TEXT("#00000000"), TEXT("#ffff00ff"), TEXT("#00000000"), TEXT("#00000000")};
		for (int32 Highlight = 0; Highlight < 4; ++Highlight)
		{
			if (Highlight > 0)
			{
				ASSERT_THAT(IsTrue(Press(EKeys::Down)));
			}
			const FString Step = FString::Printf(TEXT("highlight on %s"), Names[Highlight]);
			const FArcRmlTestLayoutElement* Box = FindSelectBox(TEXT("spp-hl"));
			ASSERT_THAT(IsNotNull(Box));
			const TArray<const FArcRmlTestLayoutElement*> Options = GetOptions(*Box);
			ASSERT_THAT(AreEqual(4, Options.Num()));
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const bool bHighlighted = Index == Highlight;
				ExpectColours(*Step, Options[Index], Names[Index], bHighlighted ? FString(HighlightText) : OwnColour[Index],
					bHighlighted ? FString(HighlightFill) : OwnBackground[Index]);
			}
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};
