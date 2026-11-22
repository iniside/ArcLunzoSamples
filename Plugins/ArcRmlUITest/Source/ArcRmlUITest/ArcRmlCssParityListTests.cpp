// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/ScopeLock.h"

namespace ArcRmlCssParityListTestsPrivate
{
	struct FTextStartCase
	{
		const TCHAR* Id;
		float X;
	};

	struct FHeightCase
	{
		const TCHAR* Id;
		float Height;
	};

	struct FMarkerTextCase
	{
		const TCHAR* Id;
		const TCHAR* Text;
		float Right;
		float Width;
	};

	struct FInsideMarkerTextCase
	{
		const TCHAR* Id;
		const TCHAR* Text;
		float X;
		float Width;
	};

	struct FInkCase
	{
		const TCHAR* Name;
		FIntRect Region;
		FIntRect Ink;
		double Coverage;
		FColor Colour;
	};

	// Chrome 156, measure.mjs on markers.html, the page rml2html.py writes from ListMarkersTest (Saved/ArcRml/Step9cChrome/markers.json).
	const FTextStartCase ChromeTextStarts[] = {
		{TEXT("a1"), 100.0f},
		{TEXT("a2"), 125.0f},
		{TEXT("a5"), 100.0f},
		{TEXT("a8"), 100.0f},
		{TEXT("b1"), 400.0f},
		{TEXT("b10"), 400.0f},
		{TEXT("d1"), 417.5625f},
		{TEXT("d10"), 427.4844f},
		{TEXT("c1"), 722.0f},
		{TEXT("c2"), 717.5625f},
		{TEXT("c3"), 743.0f},
		{TEXT("c4"), 700.0f},
		{TEXT("c9"), 722.0f},
		{TEXT("n2a"), 1040.0f},
		{TEXT("n3a"), 1080.0f},
		{TEXT("k2a"), 1000.0f},
	};

	// Chrome's li border-box heights: the line box grows with the font and the line-height, and an inside marker's text wraps to three lines.
	const FHeightCase ChromeItemHeights[] = {
		{TEXT("a1"), 21.0f},
		{TEXT("a5"), 31.0f},
		{TEXT("a6"), 40.0f},
		{TEXT("a7"), 16.0f},
		{TEXT("a8"), 61.0f},
		{TEXT("c1"), 63.0f},
		{TEXT("c3"), 40.0f},
		{TEXT("c4"), 40.0f},
		{TEXT("n1a"), 63.0f},
	};

	// An outside text marker ends at the li's border edge; widths are Chrome's tabular-nums advances of the same strings (the w* spans).
	const FMarkerTextCase ChromeOutsideTexts[] = {
		{TEXT("b1"), TEXT("1. "), 400.0f, 17.5625f},
		{TEXT("b2"), TEXT("2. "), 400.0f, 17.5625f},
		{TEXT("b10"), TEXT("10. "), 400.0f, 27.4844f},
		{TEXT("b11"), TEXT("11. "), 400.0f, 27.4844f},
		{TEXT("k3a"), TEXT("3. "), 1000.0f, 17.5625f},
		{TEXT("k3b"), TEXT("10. "), 1000.0f, 27.4844f},
		{TEXT("k3c"), TEXT("11. "), 1000.0f, 27.4844f},
		{TEXT("k4a"), TEXT("3. "), 1000.0f, 17.5625f},
		{TEXT("k4c"), TEXT("1. "), 1000.0f, 17.5625f},
		{TEXT("k5d"), TEXT("IV. "), 1000.0f, 20.2188f},
		{TEXT("k6a"), TEXT("01. "), 1000.0f, 27.4844f},
	};

	const FInsideMarkerTextCase ChromeInsideTexts[] = {
		{TEXT("c2"), TEXT("1. "), 700.0f, 17.5625f},
		{TEXT("d10"), TEXT("10. "), 400.0f, 27.4844f},
	};

	// Chrome 156 with body.edited (markers_edited.json): #u1 square, #o1 lower-alpha, #u9 outside, #k2 'list-style: decimal inside'.
	const FTextStartCase ChromeEditedTextStarts[] = {
		{TEXT("a1"), 100.0f},
		{TEXT("b1"), 400.0f},
		{TEXT("c1"), 700.0f},
		{TEXT("k2a"), 1017.5625f},
	};

	const FMarkerTextCase ChromeEditedOutsideTexts[] = {
		{TEXT("b1"), TEXT("a. "), 400.0f, 16.8438f},
		{TEXT("b11"), TEXT("k. "), 400.0f, 16.25f},
	};

	// Chrome 156 measure.mjs on tailwind.html, from ListTailwindTest (Saved/ArcRml/Step9cChrome/tailwind.json).
	const FTextStartCase ChromeTailwindTextStarts[] = {
		{TEXT("tw0a"), 100.0f},
		{TEXT("tw1a"), 122.0f},
		{TEXT("tw1b"), 122.0f},
		{TEXT("tw2a"), 124.0f},
		{TEXT("tw3a"), 100.0f},
		{TEXT("tw4a"), 120.0f},
		{TEXT("tw5a"), 117.5625f},
	};

	const FMarkerTextCase ChromeTailwindOutsideTexts[] = {
		{TEXT("tw2a"), TEXT("1. "), 124.0f, 17.5625f},
		{TEXT("tw2b"), TEXT("2. "), 124.0f, 17.5625f},
	};

	const FColor Black(0, 0, 0, 255);
	const FColor Red(0xcc, 0x33, 0x00, 255);

	FIntRect MakeRect(int32 X0, int32 Y0, int32 X1, int32 Y1)
	{
		return FIntRect(X0, Y0, X1, Y1);
	}

	FIntRect MakeInk(int32 X, int32 Y, int32 Width, int32 Height)
	{
		return FIntRect(X, Y, X + Width, Y + Height);
	}

	// Chrome 156 screenshot markers.png: the ink box (any channel 6 below white) and the coverage of the ink colour in each marker's region.
	const FInkCase ChromeSymbolInks[] = {
		{TEXT("disc 16"), MakeRect(60, 10, 100, 31), MakeInk(83, 19, 5, 5), 19.88, Black},
		{TEXT("circle 16"), MakeRect(60, 70, 100, 91), MakeInk(82, 78, 7, 7), 13.68, Black},
		{TEXT("square 16"), MakeRect(60, 100, 100, 121), MakeInk(83, 109, 5, 5), 25.0, Black},
		{TEXT("disc 24"), MakeRect(50, 130, 100, 161), MakeInk(77, 142, 8, 8), 52.16, Black},
		{TEXT("circle 32"), MakeRect(50, 170, 100, 210), MakeInk(72, 185, 12, 12), 26.9, Black},
		{TEXT("disc 12"), MakeRect(60, 220, 100, 236), MakeInk(85, 226, 4, 4), 12.13, Black},
		{TEXT("square 48"), MakeRect(40, 250, 100, 311), MakeInk(62, 274, 16, 16), 256.0, Black},
		{TEXT("nested circle"), MakeRect(1000, 31, 1040, 52), MakeInk(1022, 39, 7, 7), 13.68, Black},
		{TEXT("nested square"), MakeRect(1040, 52, 1080, 73), MakeInk(1063, 61, 5, 5), 25.0, Black},
		{TEXT("red li disc"), MakeRect(960, 120, 1000, 141), MakeInk(983, 129, 5, 5), 19.9, Red},
		{TEXT("24px li disc"), MakeRect(960, 141, 1000, 172), MakeInk(977, 153, 8, 8), 52.16, Black},
		{TEXT("inside disc"), MakeRect(700, 10, 715, 31), MakeInk(700, 19, 5, 5), 19.88, Black},
		{TEXT("inside disc 32"), MakeRect(700, 120, 735, 160), MakeInk(700, 136, 10, 10), 77.04, Black},
		{TEXT("inside disc before collapsed spaces"), MakeRect(700, 290, 715, 311), MakeInk(700, 299, 5, 5), 19.88, Black},
		{TEXT("disc at line-height 40"), MakeRect(660, 170, 700, 210), MakeInk(683, 188, 5, 5), 19.88, Black},
		{TEXT("disc of li > p"), MakeRect(660, 230, 700, 251), MakeInk(683, 239, 5, 5), 19.88, Black},
		{TEXT("disc of li > flex"), MakeRect(660, 260, 700, 281), MakeInk(683, 269, 5, 5), 19.88, Black},
	};

	const FInkCase ChromeTextInks[] = {
		{TEXT("outside 1."), MakeRect(360, 10, 400, 31), MakeInk(383, 14, 13, 13), 31.96, Black},
		{TEXT("outside 10."), MakeRect(360, 199, 400, 220), MakeInk(373, 203, 23, 13), 68.02, Black},
		{TEXT("inside 1."), MakeRect(700, 80, 717, 101), MakeInk(700, 84, 14, 13), 32.13, Black},
		{TEXT("inside 10."), MakeRect(400, 489, 427, 510), MakeInk(400, 493, 24, 13), 67.93, Black},
	};

	// Each item's digits, from start, value and reversed: '3.' and '1.' differ in coverage, '10.' and '1.' in width, roman numerals in width.
	const FInkCase ChromeOrdinalInks[] = {
		{TEXT("start=3 first, 3."), MakeRect(960, 210, 1000, 231), MakeInk(983, 214, 13, 13), 39.47, Black},
		{TEXT("value=10, 10."), MakeRect(960, 231, 1000, 252), MakeInk(973, 235, 23, 13), 68.02, Black},
		{TEXT("after value=10, 11."), MakeRect(960, 252, 1000, 273), MakeInk(973, 256, 23, 13), 60.2, Black},
		{TEXT("reversed of 3, first 3."), MakeRect(960, 290, 1000, 311), MakeInk(983, 294, 13, 13), 39.47, Black},
		{TEXT("reversed of 3, second 2."), MakeRect(960, 311, 1000, 332), MakeInk(983, 315, 13, 13), 37.65, Black},
		{TEXT("reversed of 3, last 1."), MakeRect(960, 332, 1000, 353), MakeInk(983, 336, 13, 13), 31.96, Black},
		{TEXT("upper-roman I."), MakeRect(960, 370, 1000, 391), MakeInk(989, 374, 7, 13), 17.84, Black},
		{TEXT("upper-roman II."), MakeRect(960, 391, 1000, 412), MakeInk(985, 395, 11, 13), 31.95, Black},
		{TEXT("upper-roman III."), MakeRect(960, 412, 1000, 433), MakeInk(981, 416, 15, 13), 46.07, Black},
		{TEXT("upper-roman IV."), MakeRect(960, 433, 1000, 454), MakeInk(980, 437, 16, 13), 46.72, Black},
		{TEXT("decimal-leading-zero 01."), MakeRect(960, 470, 1000, 491), MakeInk(973, 474, 23, 13), 68.02, Black},
		{TEXT("reversed of 10, first 10."), MakeRect(960, 500, 1000, 521), MakeInk(973, 504, 23, 13), 68.02, Black},
		{TEXT("reversed of 10, second 9."), MakeRect(960, 521, 1000, 542), MakeInk(983, 525, 13, 13), 42.23, Black},
		{TEXT("reversed of 10, last 1."), MakeRect(960, 689, 1000, 710), MakeInk(983, 693, 13, 13), 31.96, Black},
	};

	// Chrome 156 screenshot markers_edited.png: body.edited turns #u1 square, #o1 lower-alpha, #u9 outside, #k2 'decimal inside'.
	const FInkCase ChromeEditedInks[] = {
		{TEXT("disc to square"), MakeRect(60, 10, 100, 31), MakeInk(83, 19, 5, 5), 25.0, Black},
		{TEXT("decimal to lower-alpha, a."), MakeRect(360, 10, 400, 31), MakeInk(383, 18, 13, 9), 32.86, Black},
		{TEXT("decimal to lower-alpha, k."), MakeRect(360, 220, 400, 241), MakeInk(384, 224, 12, 13), 30.92, Black},
		{TEXT("inside to outside disc"), MakeRect(660, 10, 700, 31), MakeInk(683, 19, 5, 5), 19.88, Black},
		{TEXT("none to decimal inside"), MakeRect(1000, 180, 1018, 201), MakeInk(1000, 184, 14, 13), 32.13, Black},
	};

	// The red item's bullet centre in markers.png is (204, 51, 0): the item's colour, not black.
	constexpr int32 RedBulletX = 985;
	constexpr int32 RedBulletY = 131;

	// Chrome 156 screenshot tailwind.png.
	const FInkCase ChromeTailwindInks[] = {
		{TEXT("preflight list"), MakeRect(60, 10, 100, 31), FIntRect(), 0.0, Black},
		{TEXT("list-disc list-inside"), MakeRect(100, 80, 115, 101), MakeInk(100, 89, 5, 5), 19.88, Black},
		{TEXT("list-decimal pl-6"), MakeRect(100, 150, 124, 171), MakeInk(107, 154, 13, 13), 31.96, Black},
		{TEXT("list-disc list-outside pl-5"), MakeRect(100, 260, 120, 281), MakeInk(103, 269, 5, 5), 19.88, Black},
		{TEXT("list-decimal list-inside"), MakeRect(100, 330, 117, 351), MakeInk(100, 334, 14, 13), 32.13, Black},
	};

	const FInkCase ChromeNoneInk = {TEXT("list-style-type none"), MakeRect(960, 180, 1000, 201), FIntRect(), 0.0, Black};

	constexpr float Tolerance = 0.5f;
	constexpr int32 TextInkTolerance = 1;
	constexpr double SymbolCoverageTolerance = 0.1;
	constexpr double TextCoverageTolerance = 0.15;

	const FArcRmlTestLayoutElement* FindMarker(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* ItemId)
	{
		const FArcRmlTestLayoutElement* Item = ArcRmlUITestHelpers::FindByDomId(Layout, ItemId);
		if (!Item)
		{
			return nullptr;
		}
		const FString MarkerPath = Item->Path + TEXT("::marker");
		return Layout.FindByPredicate([&MarkerPath](const FArcRmlTestLayoutElement& Element) { return Element.Path == MarkerPath; });
	}

	const FArcRmlTestTextLine* FindMarkerLine(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* ItemId)
	{
		const FArcRmlTestLayoutElement* Marker = FindMarker(Layout, ItemId);
		if (!Marker)
		{
			return nullptr;
		}
		const FArcRmlTestLayoutElement* Text = Layout.FindByPredicate(
			[Marker](const FArcRmlTestLayoutElement& Element) { return Element.Parent == Marker->Path && !Element.Lines.IsEmpty(); });
		return Text ? &Text->Lines[0] : nullptr;
	}

	template <int32 N>
	FString DescribeTextStarts(const TArray<FArcRmlTestLayoutElement>& Layout, const FTextStartCase (&Cases)[N])
	{
		FString Mismatches;
		for (const FTextStartCase& Case : Cases)
		{
			const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Layout, Case.Id);
			if (Lines.IsEmpty())
			{
				Mismatches += FString::Printf(TEXT("[%s has no text line] "), Case.Id);
			}
			else if (FMath::Abs(Lines[0].X - Case.X) > Tolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s text starts at %.4f, Chrome %.4f] "), Case.Id, Lines[0].X, Case.X);
			}
		}
		return Mismatches;
	}

	template <int32 N>
	FString DescribeItemHeights(const TArray<FArcRmlTestLayoutElement>& Layout, const FHeightCase (&Cases)[N])
	{
		FString Mismatches;
		for (const FHeightCase& Case : Cases)
		{
			const FArcRmlTestLayoutElement* Item = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
			if (!Item)
			{
				Mismatches += FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
			}
			else if (FMath::Abs(Item->Rect.Size.Y - Case.Height) > Tolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s is %.2f high, Chrome %.2f] "), Case.Id, Item->Rect.Size.Y, Case.Height);
			}
		}
		return Mismatches;
	}

	template <int32 N>
	FString DescribeOutsideTexts(const TArray<FArcRmlTestLayoutElement>& Layout, const FMarkerTextCase (&Cases)[N])
	{
		FString Mismatches;
		for (const FMarkerTextCase& Case : Cases)
		{
			const FArcRmlTestTextLine* Line = FindMarkerLine(Layout, Case.Id);
			if (!Line)
			{
				Mismatches += FString::Printf(TEXT("[%s has no marker text] "), Case.Id);
				continue;
			}
			if (Line->Text != Case.Text)
			{
				Mismatches += FString::Printf(TEXT("[%s marker '%s', Chrome '%s'] "), Case.Id, *Line->Text, Case.Text);
			}
			if (FMath::Abs(Line->X + Line->Width - Case.Right) > Tolerance || FMath::Abs(Line->Width - Case.Width) > Tolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s marker text x %.4f width %.4f, Chrome ends at %.4f, width %.4f] "), Case.Id, Line->X, Line->Width, Case.Right,
					Case.Width);
			}
		}
		return Mismatches;
	}

	template <int32 N>
	FString DescribeInsideTexts(const TArray<FArcRmlTestLayoutElement>& Layout, const FInsideMarkerTextCase (&Cases)[N])
	{
		FString Mismatches;
		for (const FInsideMarkerTextCase& Case : Cases)
		{
			const FArcRmlTestTextLine* Line = FindMarkerLine(Layout, Case.Id);
			if (!Line)
			{
				Mismatches += FString::Printf(TEXT("[%s has no marker text] "), Case.Id);
			}
			else if (Line->Text != Case.Text || FMath::Abs(Line->X - Case.X) > Tolerance || FMath::Abs(Line->Width - Case.Width) > Tolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s marker '%s' x %.4f width %.4f, Chrome '%s' x %.4f width %.4f] "), Case.Id, *Line->Text, Line->X, Line->Width,
					Case.Text, Case.X, Case.Width);
			}
		}
		return Mismatches;
	}

	FIntRect FindInk(const FArcRmlTestView& View, const FIntRect& Region)
	{
		FIntRect Ink(MAX_int32, MAX_int32, MIN_int32, MIN_int32);
		for (int32 Y = Region.Min.Y; Y < Region.Max.Y; ++Y)
		{
			for (int32 X = Region.Min.X; X < Region.Max.X; ++X)
			{
				const FColor Pixel = View.GetPixel(X, Y);
				if (255 - FMath::Min3(Pixel.R, Pixel.G, Pixel.B) >= 6)
				{
					Ink.Min = Ink.Min.ComponentMin(FIntPoint(X, Y));
					Ink.Max = Ink.Max.ComponentMax(FIntPoint(X + 1, Y + 1));
				}
			}
		}
		return Ink.Min.X == MAX_int32 ? FIntRect() : Ink;
	}

	FString DescribeInk(const FArcRmlTestView& View, const FInkCase& Case, int32 InkTolerance, double CoverageTolerance)
	{
		const FIntRect Ink = FindInk(View, Case.Region);
		const double Coverage = View.SumCoverage(Case.Region, Case.Colour);
		const bool bInkMatches = FMath::Abs(Ink.Min.X - Case.Ink.Min.X) <= InkTolerance && FMath::Abs(Ink.Min.Y - Case.Ink.Min.Y) <= InkTolerance
			&& FMath::Abs(Ink.Max.X - Case.Ink.Max.X) <= InkTolerance && FMath::Abs(Ink.Max.Y - Case.Ink.Max.Y) <= InkTolerance;
		const bool bCoverageMatches = FMath::Abs(Coverage - Case.Coverage) <= FMath::Max(Case.Coverage * CoverageTolerance, 0.5);
		if (bInkMatches && bCoverageMatches)
		{
			return FString();
		}
		return FString::Printf(TEXT("[%s ink %d,%d %dx%d coverage %.2f, Chrome %d,%d %dx%d coverage %.2f] "), Case.Name, Ink.Min.X, Ink.Min.Y, Ink.Width(), Ink.Height(),
			Coverage, Case.Ink.Min.X, Case.Ink.Min.Y, Case.Ink.Width(), Case.Ink.Height(), Case.Coverage);
	}

	template <int32 N>
	FString DescribeInks(const FArcRmlTestView& View, const FInkCase (&Cases)[N], int32 InkTolerance, double CoverageTolerance)
	{
		FString Mismatches;
		for (const FInkCase& Case : Cases)
		{
			Mismatches += DescribeInk(View, Case, InkTolerance, CoverageTolerance);
		}
		return Mismatches;
	}

	class FLayoutVerifyLogCapture final : public FOutputDevice
	{
	public:
		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category != FName(TEXT("LogArcRmlUI")) || !FCString::Strstr(Text, TEXT("layout verify")))
			{
				return;
			}
			FScopeLock Lock(&Mutex);
			if (FCString::Strstr(Text, TEXT("mismatches 0,")))
			{
				if (FCString::Strstr(Text, TEXT("layout verify '")))
				{
					CleanPasses++;
					if (const TCHAR* Reused = FCString::Strstr(Text, TEXT("reused boxes ")))
					{
						ReusedBoxes += FCString::Atoi(Reused + FCString::Strlen(TEXT("reused boxes ")));
					}
				}
			}
			else
			{
				Lines.Add(Text);
			}
		}

		virtual bool CanBeUsedOnAnyThread() const override
		{
			return true;
		}

		FString Describe()
		{
			FScopeLock Lock(&Mutex);
			return FString::Join(Lines, TEXT(" | "));
		}

		int32 NumCleanPasses()
		{
			FScopeLock Lock(&Mutex);
			return CleanPasses;
		}

		int32 NumReusedBoxes()
		{
			FScopeLock Lock(&Mutex);
			return ReusedBoxes;
		}

		void ResetReusedBoxes()
		{
			FScopeLock Lock(&Mutex);
			ReusedBoxes = 0;
		}

	private:
		FCriticalSection Mutex;
		TArray<FString> Lines;
		int32 CleanPasses = 0;
		int32 ReusedBoxes = 0;
	};

	FArcRmlValue MakeRow(const FString& Name)
	{
		FArcRmlValue Row;
		Row.Kind = EArcRmlValueKind::Struct;
		Row.MemberNames = {TEXT("Name")};
		Row.Children.Add(ArcRmlUITestHelpers::MakeString(Name));
		return Row;
	}

	bool Matches(const FString& Mismatches, const TCHAR* What, FString& OutFailure)
	{
		OutFailure = FString::Printf(TEXT("%s: %s"), What, *Mismatches);
		return Mismatches.IsEmpty();
	}

	struct FDataRow
	{
		const TCHAR* Name;
		const TCHAR* Marker;
	};

	// The rows are keyed by name, so each row keeps its li across inserts and removals and an existing li must change its own marker.
	FString DescribeDataMarkers(const TArray<FArcRmlTestLayoutElement>& Layout, std::initializer_list<FDataRow> Rows, float ChromeWidth,
		std::initializer_list<const TCHAR*> Removed = {})
	{
		FString Mismatches;
		for (const FDataRow& Row : Rows)
		{
			const FString Id = FString::Printf(TEXT("row-%s"), Row.Name);
			const FArcRmlTestTextLine* Line = FindMarkerLine(Layout, *Id);
			if (!Line)
			{
				Mismatches += FString::Printf(TEXT("[%s has no marker text] "), *Id);
			}
			else if (Line->Text != Row.Marker || FMath::Abs(Line->X + Line->Width - 100.0f) > Tolerance
				|| (ChromeWidth > 0.0f && FMath::Abs(Line->Width - ChromeWidth) > Tolerance))
			{
				Mismatches += FString::Printf(TEXT("[%s marker '%s' ends at %.4f width %.4f, expected '%s' ending at 100] "), *Id, *Line->Text, Line->X + Line->Width,
					Line->Width, Row.Marker);
			}
		}
		for (const TCHAR* Name : Removed)
		{
			if (ArcRmlUITestHelpers::FindByDomId(Layout, *FString::Printf(TEXT("row-%s"), Name)))
			{
				Mismatches += FString::Printf(TEXT("[row-%s is still in the list] "), Name);
			}
		}
		for (const TCHAR* Kept : {TEXT("du0"), TEXT("du1")})
		{
			const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Layout, Kept);
			if (!FindMarker(Layout, Kept) || Lines.IsEmpty() || FMath::Abs(Lines[0].X - 100.0f) > Tolerance)
			{
				Mismatches += FString::Printf(TEXT("[the untouched list's %s, laid out again from its record, lost its disc or moved its text] "), Kept);
			}
		}
		return Mismatches;
	}
}

TEST_CLASS(ArcRmlCssParityListTests, "ArcRmlUI.CssParity.Lists")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;
	FString Failure;
	ArcRmlCssParityListTestsPrivate::FLayoutVerifyLogCapture Capture;
	bool bCaptureAttached = false;
	int32 SavedVerify = 0;

	AFTER_EACH()
	{
		View.Close();
		if (bCaptureAttached)
		{
			GLog->RemoveOutputDevice(&Capture);
			bCaptureAttached = false;
			IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify"))->Set(SavedVerify, ECVF_SetByCode);
		}
	}

	bool OpenFixture(const TCHAR* File)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("CssParity/Lists/%s"), File));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ParseLayout(Layout);
	}

	bool Settle()
	{
		Layout.Reset();
		return View.FlushUntilSettled() && View.ParseLayout(Layout);
	}

	TEST_METHOD(ListItems_PlaceTheirTextAndOutsideMarkersAsChrome)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ListMarkersTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Matches(DescribeTextStarts(Layout, ChromeTextStarts) + DescribeItemHeights(Layout, ChromeItemHeights),
			TEXT("an outside marker takes no space; an inside disc is 1px wider than the bullet plus 1em, an inside 'N. ' its advance; leading spaces collapse"), Failure), Failure));
		ASSERT_THAT(IsTrue(Matches(DescribeOutsideTexts(Layout, ChromeOutsideTexts),
			TEXT("an outside 'N. ' ends at the li's border edge with tabular digits; start, value and reversed set the counter"), Failure), Failure));
		ASSERT_THAT(IsTrue(Matches(DescribeInsideTexts(Layout, ChromeInsideTexts), TEXT("an inside 'N. ' starts the first line"), Failure), Failure));
		ASSERT_THAT(IsNull(FindMarker(Layout, TEXT("k2a")), TEXT("list-style-type none creates no marker")));
	}

	TEST_METHOD(RuntimeChange_RebuildsMarkersAsChrome)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ListMarkersTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetRootClass(TEXT("edited"), true);
		ASSERT_THAT(IsTrue(Settle()));
		const FArcRmlTestLayoutElement* Square = FindMarker(Layout, TEXT("a1"));
		ASSERT_THAT(IsTrue(Square && !FindMarkerLine(Layout, TEXT("a1")), TEXT("disc to square keeps a symbol marker without text")));
		ASSERT_THAT(AreEqual(FString(TEXT("square")), Square->ListStyleType, TEXT("the marker computes list-style-type square, as Chrome's #u1")));
		const FArcRmlTestLayoutElement* Alpha = FindMarker(Layout, TEXT("b1"));
		ASSERT_THAT(IsTrue(Alpha && Alpha->ListStyleType == TEXT("lower-alpha"), TEXT("the ol's marker computes lower-alpha, as Chrome's #o1")));
		ASSERT_THAT(IsTrue(Matches(DescribeTextStarts(Layout, ChromeEditedTextStarts) + DescribeOutsideTexts(Layout, ChromeEditedOutsideTexts),
			TEXT("decimal to lower-alpha relabels, inside to outside moves the text to the li's edge, none to 'decimal inside' adds a marker"), Failure), Failure));
		View.SetRootClass(TEXT("edited"), false);
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(IsTrue(Matches(DescribeTextStarts(Layout, ChromeTextStarts) + DescribeOutsideTexts(Layout, ChromeOutsideTexts),
			TEXT("removing the class restores the loaded markers"), Failure), Failure));
		ASSERT_THAT(IsNull(FindMarker(Layout, TEXT("k2a")), TEXT("list-style-type none again removes the marker")));
		const FArcRmlTestLayoutElement* Disc = FindMarker(Layout, TEXT("a1"));
		ASSERT_THAT(IsTrue(Disc && Disc->ListStyleType == TEXT("disc"), TEXT("#u1's marker is a disc again")));
	}

	TEST_METHOD(TailwindPreflightAndListUtilities_MatchChrome)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ListTailwindTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Matches(DescribeTextStarts(Layout, ChromeTailwindTextStarts) + DescribeOutsideTexts(Layout, ChromeTailwindOutsideTexts),
			TEXT("preflight's list-style none drops markers; list-disc, list-decimal, list-inside and list-outside bring them back"), Failure), Failure));
		ASSERT_THAT(IsNull(FindMarker(Layout, TEXT("tw0a")), TEXT("preflight removes the ul's disc")));
		ASSERT_THAT(IsNull(FindMarker(Layout, TEXT("tw3a")), TEXT("list-none has no marker")));
	}

	TEST_METHOD(DataForInsertRemoveAndClassToggle_RenumberWithCleanLayoutVerify)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		IConsoleVariable* Verify = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify"));
		ASSERT_THAT(IsNotNull(Verify));
		SavedVerify = Verify->GetInt();
		Verify->Set(1, ECVF_SetByCode);
		GLog->AddOutputDevice(&Capture);
		bCaptureAttached = true;

		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ListDataTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Matches(DescribeDataMarkers(Layout, {{TEXT("Alpha"), TEXT("1. ")}, {TEXT("Beta"), TEXT("2. ")}, {TEXT("Gamma"), TEXT("3. ")}}, 17.5625f),
			TEXT("the loaded rows"), Failure), Failure));
		GLog->Flush();
		Capture.ResetReusedBoxes();

		TArray<FArcRmlListOp> Insert;
		FArcRmlListOp& InsertOp = Insert.AddDefaulted_GetRef();
		InsertOp.Type = EArcRmlListOpType::Insert;
		InsertOp.Index = 0;
		InsertOp.Value = MakeRow(TEXT("Zero"));
		View.ApplyListOps(TEXT("lst"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Insert));
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(IsTrue(Matches(DescribeDataMarkers(Layout,
			{{TEXT("Zero"), TEXT("1. ")}, {TEXT("Alpha"), TEXT("2. ")}, {TEXT("Beta"), TEXT("3. ")}, {TEXT("Gamma"), TEXT("4. ")}}, 17.5625f),
			TEXT("a row inserted at the front renumbers the li elements that were already there"), Failure), Failure));

		TArray<FArcRmlListOp> Remove;
		FArcRmlListOp& RemoveOp = Remove.AddDefaulted_GetRef();
		RemoveOp.Type = EArcRmlListOpType::Remove;
		RemoveOp.Index = 1;
		View.ApplyListOps(TEXT("lst"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Remove));
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(IsTrue(Matches(DescribeDataMarkers(Layout, {{TEXT("Zero"), TEXT("1. ")}, {TEXT("Beta"), TEXT("2. ")}, {TEXT("Gamma"), TEXT("3. ")}}, 17.5625f,
			{TEXT("Alpha")}), TEXT("a removed row renumbers the li elements after it"), Failure), Failure));

		View.SetModelValue(TEXT("lst"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Alpha")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(IsTrue(Matches(DescribeDataMarkers(Layout, {{TEXT("Zero"), TEXT("A. ")}, {TEXT("Beta"), TEXT("B. ")}, {TEXT("Gamma"), TEXT("C. ")}}, -1.0f),
			TEXT("a class toggled to upper-alpha relabels"), Failure), Failure));
		const FArcRmlTestTextLine* First = FindMarkerLine(Layout, TEXT("row-Zero"));
		ASSERT_THAT(IsTrue(First && FMath::Abs(First->Width - 17.7188f) <= Tolerance, TEXT("'A. ' is Chrome's 17.7188px wide")));

		GLog->Flush();
		ASSERT_THAT(IsTrue(Matches(Capture.Describe(), TEXT("layout verify found no reused box that differs from a full layout"), Failure), Failure));
		ASSERT_THAT(IsTrue(Capture.NumCleanPasses() > 0, TEXT("no layout verify pass was logged")));
		ASSERT_THAT(IsTrue(Capture.NumReusedBoxes() > 0, TEXT("the verified passes after the load reused no box, so layout reuse was not checked")));
	}
};

TEST_CLASS(ArcRmlListMarkerPixelTests, "ArcRmlUI.Pixels.ListMarkers")
{
	FArcRmlTestView View;
	FString Failure;

	AFTER_EACH()
	{
		View.Close();
	}

	bool Render(const TCHAR* File)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("CssParity/Lists/%s"), File));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.RenderPixels();
	}

	TEST_METHOD(SymbolMarkers_DrawChromesBulletsInTheItemColour)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(Render(TEXT("ListMarkersTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Matches(DescribeInks(View, ChromeSymbolInks, 0, SymbolCoverageTolerance),
			TEXT("disc, circle and square: the bullet is round(ascent)*2/3 wide, pixel-snapped, outside 7px plus its width left of the border edge"), Failure), Failure));
		ASSERT_THAT(IsTrue(Matches(DescribeInk(View, ChromeNoneInk, 0, SymbolCoverageTolerance), TEXT("none draws nothing"), Failure), Failure));
		const FColor Centre = View.GetPixel(RedBulletX, RedBulletY);
		ASSERT_THAT(IsTrue(FMath::Abs(Centre.R - 204) <= 2 && FMath::Abs(Centre.G - 51) <= 2 && Centre.B <= 2,
			FString::Printf(TEXT("the red item's bullet centre is %d,%d,%d, Chrome 204,51,0"), Centre.R, Centre.G, Centre.B)));
	}

	TEST_METHOD(RuntimeChange_RedrawsChromesMarkers)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(Render(TEXT("ListMarkersTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetRootClass(TEXT("edited"), true);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.RenderPixels()));
		ASSERT_THAT(IsTrue(Matches(DescribeInks(View, ChromeEditedInks, TextInkTolerance, SymbolCoverageTolerance),
			TEXT("a square is a full 5px quad, lower-alpha draws letters, an outside disc sits left of the li"), Failure), Failure));
	}

	TEST_METHOD(OrdinalMarkers_DrawChromesNumbersForStartValueAndReversed)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(Render(TEXT("ListMarkersTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Matches(DescribeInks(View, ChromeOrdinalInks, TextInkTolerance, TextCoverageTolerance),
			TEXT("start=3, value=10, reversed, upper-roman and decimal-leading-zero draw Chrome's digits on each row"), Failure), Failure));
	}

	TEST_METHOD(TenItemLists_DrawInAsManyCallsAsWithoutMarkers)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(Render(TEXT("ListDrawTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const int32 WithMarkers = View.GetReplayDrawCalls();
		const int32 CommandsWithMarkers = View.GetReplayCommands();
		View.SetRootClass(TEXT("plain"), true);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.RenderPixels()));
		const int32 WithoutMarkers = View.GetReplayDrawCalls();
		const int32 CommandsWithoutMarkers = View.GetReplayCommands();
		ASSERT_THAT(IsTrue(CommandsWithMarkers > CommandsWithoutMarkers,
			FString::Printf(TEXT("the ten bullets record no command of their own (%d with, %d without)"), CommandsWithMarkers, CommandsWithoutMarkers)));
		ASSERT_THAT(AreEqual(WithoutMarkers, WithMarkers,
			FString::Printf(TEXT("a 10-item ul and ol draw in %d calls, without markers %d: the bullets merge with the text"), WithMarkers, WithoutMarkers)));
	}

	TEST_METHOD(TextMarkers_DrawChromesNumbersOutsideAndInside)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(Render(TEXT("ListMarkersTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Matches(DescribeInks(View, ChromeTextInks, TextInkTolerance, TextCoverageTolerance),
			TEXT("'1.' and '10.' end at the same place outside; inside they start the line"), Failure), Failure));
	}

	TEST_METHOD(TailwindListUtilities_DrawChromesMarkers)
	{
		using namespace ArcRmlCssParityListTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(Render(TEXT("ListTailwindTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Matches(DescribeInks(View, ChromeTailwindInks, TextInkTolerance, SymbolCoverageTolerance),
			TEXT("preflight hides markers, the utilities draw disc and decimal inside and outside the padding"), Failure), Failure));
	}
};
