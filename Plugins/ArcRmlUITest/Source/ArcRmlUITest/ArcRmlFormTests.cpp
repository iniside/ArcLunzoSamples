// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"

namespace ArcRmlFormTestsPrivate
{
	const TCHAR* const DefaultsFixture = TEXT("Form/FormDefaults.rml");
	const TCHAR* const DefaultsSheet = TEXT("FormDefaults.rcssc");
	const TCHAR* const OriginFixture = TEXT("Form/FormUserAgentOrigin.rml");
	const TCHAR* const OriginSheet = TEXT("FormUserAgentOrigin.rcssc");
	const TCHAR* const OutsetFixture = TEXT("CssParity/UserAgent/BorderOutsetTest.rml");
	const TCHAR* const OutsetSheet = TEXT("BorderOutset.rcssc");
	// StyleSheetUserAgent::Initialise names the parsed user-agent sheet's source.
	const TCHAR* const UserAgentFile = TEXT("user-agent.css");

	constexpr float ControlFontSize = 13.3333f;
	constexpr float SizeTolerance = 0.01f;
	constexpr float EdgeTolerance = 0.01f;
	constexpr float WidthTolerance = 0.05f;
	constexpr int32 PixelTolerance = 2;

	struct FEdges
	{
		float Top;
		float Right;
		float Bottom;
		float Left;
	};

	// Chrome 156 computed values of the bare controls of FormDefaults (rml-forms step 2, VENDORED 222 and 223:
	// chrome --computed on the headless dump, cursor measured 2026-11-05; every value below equals Chrome's). An empty BorderColour is not compared
	// (checkbox, radio and range have no border). Text-align left is the initial value Chrome computes as start.
	struct FControlCase
	{
		const TCHAR* Id;
		const TCHAR* Family;
		FEdges Padding;
		float BorderWidth;
		const TCHAR* BorderStyle;
		const TCHAR* BorderColour;
		FEdges Margin;
		const TCHAR* BoxSizing;
		const TCHAR* TextAlign;
		const TCHAR* Cursor;
	};

	const FEdges TextPadding = {1.0f, 2.0f, 1.0f, 2.0f};
	const FEdges ButtonPadding = {1.0f, 6.0f, 1.0f, 6.0f};
	const FEdges NoEdges = {0.0f, 0.0f, 0.0f, 0.0f};
	const FEdges TextAreaPadding = {2.0f, 2.0f, 2.0f, 2.0f};
	const FEdges CheckboxMargin = {3.0f, 3.0f, 3.0f, 4.0f};
	const FEdges RadioMargin = {3.0f, 3.0f, 0.0f, 5.0f};
	const FEdges RangeMargin = {2.0f, 2.0f, 2.0f, 2.0f};

	const FControlCase ChromeControls[] = {
		{TEXT("fd-text"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-untyped"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-text-size"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-email"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-password"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-search"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("border-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-number"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-placeholder"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-value"), TEXT("sans-serif"), TextPadding, 2.0f, TEXT("inset"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-ta"), TEXT("monospace"), TextAreaPadding, 1.0f, TEXT("solid"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-ta-rc"), TEXT("monospace"), TextAreaPadding, 1.0f, TEXT("solid"), TEXT("#767676ff"), NoEdges, TEXT("content-box"), TEXT("left"), TEXT("text")},
		{TEXT("fd-select"), TEXT("sans-serif"), NoEdges, 1.0f, TEXT("solid"), TEXT("#767676ff"), NoEdges, TEXT("border-box"), TEXT("left"), TEXT("default")},
		{TEXT("fd-button"), TEXT("sans-serif"), ButtonPadding, 2.0f, TEXT("outset"), TEXT("#000000ff"), NoEdges, TEXT("border-box"), TEXT("center"), TEXT("default")},
		{TEXT("fd-ibutton"), TEXT("sans-serif"), ButtonPadding, 2.0f, TEXT("outset"), TEXT("#000000ff"), NoEdges, TEXT("border-box"), TEXT("center"), TEXT("default")},
		{TEXT("fd-submit"), TEXT("sans-serif"), ButtonPadding, 2.0f, TEXT("outset"), TEXT("#000000ff"), NoEdges, TEXT("border-box"), TEXT("center"), TEXT("default")},
		{TEXT("fd-reset"), TEXT("sans-serif"), ButtonPadding, 2.0f, TEXT("outset"), TEXT("#000000ff"), NoEdges, TEXT("border-box"), TEXT("center"), TEXT("default")},
		{TEXT("fd-checkbox"), TEXT("sans-serif"), NoEdges, 0.0f, TEXT("none"), TEXT(""), CheckboxMargin, TEXT("border-box"), TEXT("left"), TEXT("default")},
		{TEXT("fd-checkbox-on"), TEXT("sans-serif"), NoEdges, 0.0f, TEXT("none"), TEXT(""), CheckboxMargin, TEXT("border-box"), TEXT("left"), TEXT("default")},
		{TEXT("fd-radio"), TEXT("sans-serif"), NoEdges, 0.0f, TEXT("none"), TEXT(""), RadioMargin, TEXT("border-box"), TEXT("left"), TEXT("default")},
		{TEXT("fd-radio-on"), TEXT("sans-serif"), NoEdges, 0.0f, TEXT("none"), TEXT(""), RadioMargin, TEXT("border-box"), TEXT("left"), TEXT("default")},
		{TEXT("fd-range"), TEXT("sans-serif"), NoEdges, 0.0f, TEXT("none"), TEXT(""), RangeMargin, TEXT("content-box"), TEXT("left"), TEXT("default")},
	};

	// The value an element's cascade gives one property, and the sheet that declared it; bFound false is the initial value.
	struct FDeclared
	{
		bool bFound = false;
		bool bInherited = false;
		FString Value;
		FString File;
	};

	FDeclared FindDeclared(const FArcRmlInspectorSnapshot& Snapshot, const TCHAR* Name)
	{
		FDeclared Declared;
		for (const FArcRmlInspectorRule& Rule : Snapshot.Rules)
		{
			for (const FArcRmlInspectorProperty& Property : Rule.Properties)
			{
				if (Property.Name != Name)
				{
					continue;
				}
				// The snapshot lists the element's own winning declarations first, then the inherited ones nearest first.
				Declared.bFound = true;
				Declared.bInherited = !Rule.InheritedFrom.IsEmpty();
				Declared.Value = Property.Value;
				Declared.File = Rule.File;
				return Declared;
			}
		}
		return Declared;
	}

	// The element's own declaration must come from the user-agent sheet with Expected; an empty Initial means the
	// property must be declared, otherwise no declaration at all is the initial value Initial.
	FString DescribeUserAgent(const FArcRmlInspectorSnapshot& Snapshot, const TCHAR* Id, const TCHAR* Name, const TCHAR* Expected, const TCHAR* Initial = TEXT(""))
	{
		const FDeclared Declared = FindDeclared(Snapshot, Name);
		if (!Declared.bFound || Declared.bInherited)
		{
			if (FCString::Strlen(Initial) > 0 && FCString::Strcmp(Initial, Expected) == 0 && !Declared.bFound)
			{
				return FString();
			}
			return FString::Printf(TEXT("[%s %s: %s, Chrome %s] "), Id, Name,
				Declared.bFound ? *FString::Printf(TEXT("inherited '%s' from %s"), *Declared.Value, *Declared.File) : TEXT("not declared"), Expected);
		}
		if (Declared.Value != Expected || Declared.File != UserAgentFile)
		{
			return FString::Printf(TEXT("[%s %s: '%s' from '%s', Chrome '%s' from the user-agent sheet] "), Id, Name, *Declared.Value, *Declared.File, Expected);
		}
		return FString();
	}

	FString DescribeEdges(const TCHAR* Id, const TCHAR* Name, const FArcRmlInspectorEdges& Measured, const FEdges& Expected)
	{
		if (FMath::Abs(Measured.Top - Expected.Top) > EdgeTolerance || FMath::Abs(Measured.Right - Expected.Right) > EdgeTolerance
			|| FMath::Abs(Measured.Bottom - Expected.Bottom) > EdgeTolerance || FMath::Abs(Measured.Left - Expected.Left) > EdgeTolerance)
		{
			return FString::Printf(TEXT("[%s %s %g %g %g %g, Chrome %g %g %g %g] "), Id, Name, Measured.Top, Measured.Right, Measured.Bottom, Measured.Left,
				Expected.Top, Expected.Right, Expected.Bottom, Expected.Left);
		}
		return FString();
	}

	// Every text line laid out under the element with DomId, at any depth. The value text of a text input or textarea is
	// widget-internal and not in the layout dump, so only DOM text (a button's) can be measured.
	TArray<FArcRmlTestTextLine> GetDescendantLines(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* DomId)
	{
		TArray<FArcRmlTestTextLine> Lines;
		const FArcRmlTestLayoutElement* Owner = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Owner)
		{
			return Lines;
		}
		const FString Prefix = Owner->Path + TEXT(".");
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Kind == TEXT("text") && Element.Path.StartsWith(Prefix))
			{
				Lines.Append(Element.Lines);
			}
		}
		return Lines;
	}

	float GetFirstLineWidth(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* DomId)
	{
		const TArray<FArcRmlTestTextLine> Lines = GetDescendantLines(Layout, DomId);
		return Lines.IsEmpty() ? -1.0f : Lines[0].Width;
	}

	// The text the closed select shows: the text under its selectvalue part.
	FString GetSelectValueText(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* DomId)
	{
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Select)
		{
			return FString(TEXT("<no select>"));
		}
		const FString& SelectPath = Select->Path;
		const FArcRmlTestLayoutElement* Value = Layout.FindByPredicate([&SelectPath](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Parent == SelectPath && Element.Tag == TEXT("selectvalue");
		});
		if (!Value)
		{
			return FString(TEXT("<no selectvalue>"));
		}
		FString Text;
		const FString Prefix = Value->Path + TEXT(".");
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Kind == TEXT("text") && Element.Path.StartsWith(Prefix))
			{
				Text += Element.Text;
			}
		}
		return Text;
	}

	// Chrome 156 border boxes on FormDefaults (rml-forms step 2, VENDORED 223; layout-oracle.md "Form-control targets", and
	// chrome_layout_compare.py chrome on the test's dump, 2026-11-05). A negative Width checks the height only: the text input and
	// textarea widths are not Chrome's yet (plan step 3 on).
	struct FChromeSize
	{
		const TCHAR* Id;
		float Width;
		float Height;
	};

	constexpr float BoxTolerance = 0.5f;

	const FChromeSize ChromeSizes[] = {
		{TEXT("fd-button"), 46.30f, 23.0f},
		{TEXT("fd-ibutton"), 46.30f, 23.0f},
		{TEXT("fd-submit"), 48.83f, 23.0f},
		{TEXT("fd-inl-button"), 32.56f, 23.0f},
		{TEXT("fd-line-button"), -1.0f, 23.0f},
		// Rows of a control between "ab" and "ef" (rml-forms step 5, VENDORED 226; chrome --tol 0.5 on the test's dump, html_sha256
		// 41fb6adb0fa8..., 2026-11-06): text input 23, select 22, checkbox 21, as Chrome's.
		{TEXT("fd-line-text"), -1.0f, 23.0f},
		{TEXT("fd-line-select"), -1.0f, 22.0f},
		{TEXT("fd-line-checkbox"), -1.0f, 21.0f},
		{TEXT("fd-text"), -1.0f, 23.0f},
		{TEXT("fd-text-size"), -1.0f, 23.0f},
		{TEXT("fd-ta"), -1.0f, 42.0f},
		{TEXT("fd-ta-rc"), -1.0f, 78.0f},
		{TEXT("fd-line-textarea"), -1.0f, 47.0f},
	};

	struct FChromePixel
	{
		const TCHAR* Name;
		int32 X;
		int32 Y;
		FColor Chrome;
	};

	// Chrome 156, chrome_layout_compare.py --screenshot --pixel on the BorderOutsetTest dump (1920x1080, rml-forms step 2,
	// VENDORED 223): outset lightens the top and left sides and darkens the bottom and right, Blink's CalculateInsetOutsetColor.
	// The appearance: none button's inner background is not probed (Chrome 240, fork 255: the native look is plan step 9).
	const FChromePixel ChromeOutsetPixels[] = {
		{TEXT("#3366cc top"), 34, 11, FColor(63, 127, 255, 255)},
		{TEXT("#3366cc bottom"), 34, 36, FColor(30, 60, 120, 255)},
		{TEXT("#3366cc left"), 11, 24, FColor(63, 127, 255, 255)},
		{TEXT("#3366cc right"), 56, 24, FColor(30, 60, 120, 255)},
		{TEXT("#303030 top"), 94, 11, FColor(132, 132, 132, 255)},
		{TEXT("#303030 bottom"), 94, 36, FColor(0, 0, 0, 255)},
		{TEXT("#303030 left"), 71, 24, FColor(132, 132, 132, 255)},
		{TEXT("#303030 right"), 116, 24, FColor(0, 0, 0, 255)},
		{TEXT("#f0f0f0 top"), 154, 11, FColor(240, 240, 240, 255)},
		{TEXT("#f0f0f0 bottom"), 154, 36, FColor(156, 156, 156, 255)},
		{TEXT("#f0f0f0 left"), 131, 24, FColor(240, 240, 240, 255)},
		{TEXT("#f0f0f0 right"), 176, 24, FColor(156, 156, 156, 255)},
		{TEXT("#ffffff top"), 214, 11, FColor(255, 255, 255, 255)},
		{TEXT("#ffffff bottom"), 214, 36, FColor(171, 171, 171, 255)},
		{TEXT("#ffffff left"), 191, 24, FColor(255, 255, 255, 255)},
		{TEXT("#ffffff right"), 236, 24, FColor(171, 171, 171, 255)},
		{TEXT("black top"), 274, 11, FColor(168, 168, 168, 255)},
		{TEXT("black bottom"), 274, 36, FColor(84, 84, 84, 255)},
		{TEXT("black left"), 251, 24, FColor(168, 168, 168, 255)},
		{TEXT("black right"), 296, 24, FColor(84, 84, 84, 255)},
		{TEXT("currentcolor top"), 334, 11, FColor(238, 238, 238, 255)},
		{TEXT("currentcolor bottom"), 334, 36, FColor(154, 154, 154, 255)},
		{TEXT("currentcolor left"), 311, 24, FColor(238, 238, 238, 255)},
		{TEXT("currentcolor right"), 356, 24, FColor(154, 154, 154, 255)},
		{TEXT("outset inset top (outset)"), 394, 11, FColor(63, 127, 255, 255)},
		{TEXT("outset inset bottom (outset)"), 394, 36, FColor(30, 60, 120, 255)},
		{TEXT("outset inset left (inset)"), 371, 24, FColor(30, 60, 120, 255)},
		{TEXT("outset inset right (inset)"), 416, 24, FColor(63, 127, 255, 255)},
		{TEXT("button top"), 40, 61, FColor(168, 168, 168, 255)},
		{TEXT("button bottom"), 40, 89, FColor(84, 84, 84, 255)},
		{TEXT("button left"), 11, 75, FColor(168, 168, 168, 255)},
		{TEXT("button right"), 69, 75, FColor(84, 84, 84, 255)},
	};

	FString GetDumpDir(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), Name));
	}

	bool IsNear(const FColor& Measured, const FColor& Expected)
	{
		return FMath::Abs(Measured.R - Expected.R) <= PixelTolerance && FMath::Abs(Measured.G - Expected.G) <= PixelTolerance
			&& FMath::Abs(Measured.B - Expected.B) <= PixelTolerance && FMath::Abs(Measured.A - Expected.A) <= PixelTolerance;
	}

	// Chrome 156 screenshot of the same dump (2026-11-05): the shared collapsed edge of co-outset-dotted (x 170 to 175) is the
	// second cell's dotted #cc3333, dots centred near y 112, 124, 135 and 146; the first cell's outset #3366cc loses, as dotted
	// outranks outset.
	const FChromePixel ChromeCollapsePixels[] = {
		{TEXT("dotted dot 1"), 173, 112, FColor(204, 51, 51, 255)},
		{TEXT("dotted dot 2"), 172, 124, FColor(204, 51, 51, 255)},
		{TEXT("dotted dot 3"), 173, 135, FColor(204, 51, 51, 255)},
		{TEXT("dotted dot 3 left"), 171, 136, FColor(204, 51, 51, 255)},
		{TEXT("dotted dot 4"), 173, 146, FColor(204, 51, 51, 255)},
	};

	FString DescribePixels(const FArcRmlTestView& View, TConstArrayView<FChromePixel> Pixels)
	{
		TArray<FString> Mismatches;
		for (const FChromePixel& Pixel : Pixels)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (!IsNear(Measured, Pixel.Chrome))
			{
				Mismatches.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d chrome %d,%d,%d"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G, Measured.B,
					Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B));
			}
		}
		return FString::Join(Mismatches, TEXT(" | "));
	}
}

// The user-agent form-control rules (rml-forms step 2, VENDORED 222 and 223) on bare controls, read through the inspector's
// cascade (value and source sheet of each declaration), the box model and the layout dump.
TEST_CLASS(ArcRmlFormDefaultsTests, "ArcRmlUI.Form.Defaults")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Relative, const TCHAR* Sheet)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(Relative);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		// Each open dumps the fixture for chrome_layout_compare.py (Chrome's boxes and computed values are the expected ones).
		View.DumpLayout(ArcRmlFormTestsPrivate::GetDumpDir(FPaths::GetBaseFilename(Path)));
		return View.Flush() && View.Flush() && View.ParseLayout(Layout);
	}

	TSharedPtr<const FArcRmlInspectorSnapshot, ESPMode::ThreadSafe> Inspect(const TCHAR* DomId)
	{
		View.ClearInspectorAnswers();
		View.InspectEdit(FString::Printf(TEXT("#%s"), DomId), FString(), FString());
		if (!View.Flush())
		{
			return nullptr;
		}
		const TArray<FArcRmlInspectorAnswer>& Answers = View.GetInspectorAnswers();
		for (int32 Index = Answers.Num() - 1; Index >= 0; --Index)
		{
			const TSharedPtr<const FArcRmlInspectorSnapshot, ESPMode::ThreadSafe>& Snapshot = Answers[Index].Snapshot;
			if (Snapshot.IsValid() && Snapshot->Nodes.IsValidIndex(Snapshot->Selected) && Snapshot->Nodes[Snapshot->Selected].Id == DomId)
			{
				return Snapshot;
			}
		}
		return nullptr;
	}

	FString DescribeControl(const ArcRmlFormTestsPrivate::FControlCase& Case)
	{
		using namespace ArcRmlFormTestsPrivate;
		const TSharedPtr<const FArcRmlInspectorSnapshot, ESPMode::ThreadSafe> Snapshot = Inspect(Case.Id);
		if (!Snapshot.IsValid() || !Snapshot->SelectedBox.bValid)
		{
			return FString::Printf(TEXT("[%s: no inspector snapshot with a box] "), Case.Id);
		}
		FString Mismatches;
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("display"), TEXT("inline-block"));
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("font-family"), Case.Family);
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("font-size"), TEXT("13.333px"));
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("border-top-style"), Case.BorderStyle);
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("border-right-style"), Case.BorderStyle);
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("box-sizing"), Case.BoxSizing, TEXT("content-box"));
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("text-align"), Case.TextAlign, TEXT("left"));
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("appearance"), TEXT("auto"));
		Mismatches += DescribeUserAgent(*Snapshot, Case.Id, TEXT("cursor"), Case.Cursor);
		Mismatches += DescribeEdges(Case.Id, TEXT("padding"), Snapshot->SelectedBox.PaddingEdges, Case.Padding);
		Mismatches += DescribeEdges(Case.Id, TEXT("border"), Snapshot->SelectedBox.BorderEdges,
			FEdges{Case.BorderWidth, Case.BorderWidth, Case.BorderWidth, Case.BorderWidth});
		Mismatches += DescribeEdges(Case.Id, TEXT("margin"), Snapshot->SelectedBox.MarginEdges, Case.Margin);

		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
		if (!Element)
		{
			Mismatches += FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
			return Mismatches;
		}
		if (FMath::Abs(Element->FontSize - ControlFontSize) > SizeTolerance)
		{
			Mismatches += FString::Printf(TEXT("[%s computed font-size %.4f, Chrome 13.3333] "), Case.Id, Element->FontSize);
		}
		if (FCString::Strlen(Case.BorderColour) > 0 && Element->BorderTopColor != Case.BorderColour)
		{
			Mismatches += FString::Printf(TEXT("[%s border-top-color %s, Chrome %s] "), Case.Id, *Element->BorderTopColor, Case.BorderColour);
		}
		return Mismatches;
	}

	FString DescribeNavAuto(const TCHAR* DomId)
	{
		const TSharedPtr<const FArcRmlInspectorSnapshot, ESPMode::ThreadSafe> Snapshot = Inspect(DomId);
		if (!Snapshot.IsValid())
		{
			return FString::Printf(TEXT("[%s: no inspector snapshot] "), DomId);
		}
		FString Mismatches;
		for (const TCHAR* Name : {TEXT("nav-up"), TEXT("nav-right"), TEXT("nav-down"), TEXT("nav-left")})
		{
			Mismatches += ArcRmlFormTestsPrivate::DescribeUserAgent(*Snapshot, DomId, Name, TEXT("auto"));
		}
		return Mismatches;
	}

	FVector2f GetCentre(const TCHAR* DomId)
	{
		View.ParseLayout(Layout);
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		return Element ? Element->Rect.Position + Element->Rect.Size * 0.5f : FVector2f(-1.0f, -1.0f);
	}

	bool Click(const TCHAR* DomId)
	{
		View.MouseClick(GetCentre(DomId));
		return View.FlushUntilSettled() && View.Flush();
	}

	bool PressDown()
	{
		View.PressKey(EKeys::Down);
		return View.Flush() && View.Flush() && View.ParseLayout(Layout);
	}

	TEST_METHOD(BareControls_TakeChromesUserAgentValues)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DefaultsFixture, DefaultsSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const FControlCase& Case : ChromeControls)
		{
			Mismatches += DescribeControl(Case);
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("each bare control takes Chrome's display, 13.3333px font, family, padding, border, margin, box-sizing, text-align, appearance and cursor from the user-agent sheet")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(BareRadio_IsRoundedByTheUserAgentSheet)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DefaultsFixture, DefaultsSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const TCHAR* Id : {TEXT("fd-radio"), TEXT("fd-radio-on")})
		{
			const TSharedPtr<const FArcRmlInspectorSnapshot, ESPMode::ThreadSafe> Snapshot = Inspect(Id);
			if (!Snapshot.IsValid())
			{
				Mismatches += FString::Printf(TEXT("[%s: no inspector snapshot] "), Id);
				continue;
			}
			Mismatches += DescribeUserAgent(*Snapshot, Id, TEXT("border-top-left-radius"), TEXT("50%"));
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("a radio's border-top-left-radius is Chrome's computed 50%, from the user-agent sheet")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(ButtonAndFixedHeightBoxes_TakeChromesSize)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DefaultsFixture, DefaultsSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const FChromeSize& Size : ChromeSizes)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Size.Id);
			if (!Element)
			{
				Mismatches += FString::Printf(TEXT("[%s is not in the layout] "), Size.Id);
			}
			else if ((Size.Width >= 0.0f && FMath::Abs(Element->Rect.Size.X - Size.Width) > BoxTolerance)
				|| FMath::Abs(Element->Rect.Size.Y - Size.Height) > BoxTolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s %.3f x %.3f, Chrome %g x %g] "), Size.Id, Element->Rect.Size.X, Element->Rect.Size.Y, Size.Width, Size.Height);
			}
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("the button boxes, the control heights and the button and textarea rows are Chrome's border boxes")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(ClosedSelectTextInputAndCheckbox_AreNavStops)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DefaultsFixture, DefaultsSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		Mismatches += DescribeNavAuto(TEXT("fd-select"));
		Mismatches += DescribeNavAuto(TEXT("fd-text"));
		Mismatches += DescribeNavAuto(TEXT("fd-checkbox"));
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("the user-agent sheet gives every control nav: auto (owner decision 2)")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(DownOnAFocusedTextInput_MovesFocusToTheControlBelow)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DefaultsFixture, DefaultsSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("fd-text"))));
		ASSERT_THAT(AreEqual(FString(TEXT("fd-text")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("a click focuses the text input")));
		ASSERT_THAT(IsTrue(PressDown()));
		ASSERT_THAT(AreEqual(FString(TEXT("fd-untyped")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("Down on a single-line field moves focus to the input below")));
	}

	TEST_METHOD(DownOnAFocusedClosedSelect_MovesFocusAndKeepsTheValue)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DefaultsFixture, DefaultsSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		// Tab from the textarea above focuses the select without opening its list.
		ASSERT_THAT(IsTrue(Click(TEXT("fd-ta-rc"))));
		View.PressKey(EKeys::Tab);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("fd-select")), ArcRmlUITestHelpers::GetFocusDomId(View),
			FString::Printf(TEXT("Tab from fd-ta-rc focuses the closed select; focus is '%s'"), *ArcRmlUITestHelpers::GetFocusDomId(View))));
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		ASSERT_THAT(AreEqual(FString(TEXT("Apple")), GetSelectValueText(Layout, TEXT("fd-select")), TEXT("the select shows its first option")));
		ASSERT_THAT(IsTrue(PressDown()));
		const FString Focus = ArcRmlUITestHelpers::GetFocusDomId(View);
		ASSERT_THAT(IsTrue(!Focus.IsEmpty() && Focus != TEXT("fd-select"), FString::Printf(TEXT("Down on a closed select moves focus (owner decision 2); focus is '%s'"), *Focus)));
		ASSERT_THAT(AreEqual(FString(TEXT("Apple")), GetSelectValueText(Layout, TEXT("fd-select")), TEXT("Down on a closed select does not step its value")));
	}

	TEST_METHOD(DownOnAFocusedCheckbox_MovesFocusToARadioBelow)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DefaultsFixture, DefaultsSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("fd-checkbox"))));
		ASSERT_THAT(AreEqual(FString(TEXT("fd-checkbox")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("a click focuses the checkbox")));
		ASSERT_THAT(IsTrue(PressDown()));
		const FString Focus = ArcRmlUITestHelpers::GetFocusDomId(View);
		ASSERT_THAT(IsTrue(Focus.StartsWith(TEXT("fd-radio")), FString::Printf(TEXT("Down on a checkbox moves focus to the radio row below; focus is '%s'"), *Focus)));
	}

	TEST_METHOD(AuthorRule_BeatsTheUserAgentRuleAndLeavesTheRest)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(OriginFixture, OriginSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TSharedPtr<const FArcRmlInspectorSnapshot, ESPMode::ThreadSafe> Snapshot = Inspect(TEXT("fu-author"));
		ASSERT_THAT(IsTrue(Snapshot.IsValid() && Snapshot->SelectedBox.bValid, TEXT("no inspector snapshot of fu-author")));
		FString Mismatches;
		const TCHAR* const AuthorFile = TEXT("/ArcRmlUITest/UI/Rml/Test/Form/FormUserAgentOrigin.rcss");
		for (const TCHAR* Name : {TEXT("display"), TEXT("padding-top"), TEXT("border-top-color"), TEXT("margin-top"), TEXT("font-size")})
		{
			const FDeclared Declared = FindDeclared(*Snapshot, Name);
			if (!Declared.bFound || Declared.bInherited || Declared.File != AuthorFile)
			{
				Mismatches += FString::Printf(TEXT("[fu-author %s from '%s', expected the author sheet] "), Name, *Declared.File);
			}
		}
		Mismatches += DescribeUserAgent(*Snapshot, TEXT("fu-author"), TEXT("border-top-style"), TEXT("inset"));
		Mismatches += DescribeUserAgent(*Snapshot, TEXT("fu-author"), TEXT("font-family"), TEXT("sans-serif"));
		Mismatches += DescribeEdges(TEXT("fu-author"), TEXT("padding"), Snapshot->SelectedBox.PaddingEdges, FEdges{5.0f, 7.0f, 5.0f, 7.0f});
		Mismatches += DescribeEdges(TEXT("fu-author"), TEXT("border"), Snapshot->SelectedBox.BorderEdges, FEdges{2.0f, 2.0f, 2.0f, 2.0f});
		Mismatches += DescribeEdges(TEXT("fu-author"), TEXT("margin"), Snapshot->SelectedBox.MarginEdges, FEdges{6.0f, 6.0f, 6.0f, 6.0f});
		const FArcRmlTestLayoutElement* Author = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("fu-author"));
		if (!Author || Author->BorderTopColor != TEXT("#ff0000ff") || FMath::Abs(Author->FontSize - 20.0f) > SizeTolerance)
		{
			Mismatches += FString::Printf(TEXT("[fu-author border-top-color %s font-size %.4f, expected the author #ff0000ff and 20] "),
				Author ? *Author->BorderTopColor : TEXT("<none>"), Author ? Author->FontSize : -1.0f);
		}

		const TSharedPtr<const FArcRmlInspectorSnapshot, ESPMode::ThreadSafe> Button = Inspect(TEXT("fu-author-button"));
		ASSERT_THAT(IsTrue(Button.IsValid() && Button->SelectedBox.bValid, TEXT("no inspector snapshot of fu-author-button")));
		const FDeclared Align = FindDeclared(*Button, TEXT("text-align"));
		const FDeclared Style = FindDeclared(*Button, TEXT("border-top-style"));
		if (Align.Value != TEXT("left") || Align.File != AuthorFile || Style.Value != TEXT("solid") || Style.File != AuthorFile)
		{
			Mismatches += FString::Printf(TEXT("[fu-author-button text-align '%s' from '%s', border-top-style '%s' from '%s', expected the author left and solid] "),
				*Align.Value, *Align.File, *Style.Value, *Style.File);
		}
		Mismatches += DescribeUserAgent(*Button, TEXT("fu-author-button"), TEXT("box-sizing"), TEXT("border-box"));
		Mismatches += DescribeEdges(TEXT("fu-author-button"), TEXT("padding"), Button->SelectedBox.PaddingEdges, FEdges{1.0f, 6.0f, 1.0f, 6.0f});
		Mismatches += DescribeEdges(TEXT("fu-author-button"), TEXT("border"), Button->SelectedBox.BorderEdges, FEdges{2.0f, 2.0f, 2.0f, 2.0f});
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("an author declaration wins over the user-agent form rule; undeclared properties keep the user-agent values")) + TEXT(": ") + Mismatches));
	}

	// Guards the VENDORED 205 rule input[type=hidden] { display: none !important } against the step-2 form rules; it is not proof
	// of step 2.
	TEST_METHOD(HiddenInput_StaysDisplayNone)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(OriginFixture, OriginSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const TCHAR* Id : {TEXT("fu-hidden"), TEXT("fu-hidden-author")})
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Id);
			if (!Element)
			{
				Mismatches += FString::Printf(TEXT("[%s is not in the layout] "), Id);
			}
			else if (Element->bVisible || Element->Rect.Size.X != 0.0f || Element->Rect.Size.Y != 0.0f)
			{
				Mismatches += FString::Printf(TEXT("[%s is shown with %.2fx%.2f, Chrome display: none] "), Id, Element->Rect.Size.X, Element->Rect.Size.Y);
			}
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("input[type=hidden] { display: none !important } beats the input rule and a normal author display")) + TEXT(": ") + Mismatches));
	}

	// The body is JetBrains Mono, so a button that lost the user-agent family draws in monospace; the textarea sits in a Space
	// Grotesk row, so one that lost the user-agent monospace sizes its columns in Space Grotesk. The textarea's text is
	// widget-internal and not in the dump, so its family shows through its width, Chrome's ceil(avg x 20) + 15
	// (ElementFormControlTextArea::GetIntrinsicDimensions) plus 2px padding and 1px border on each side, avg being the advance
	// of 'x' rounded only when that rounds it up, max(x, round(x)). In monospace every advance is one, so the reference row of
	// 20 'm' gives it. At 13.3333px both families give 8 px (JetBrains Mono 0.6 em is 8, Space Grotesk 0.593 em rounds up to
	// it), so the family is read at an inline 75px, where they give 45 and 44.47: the textarea must take the JetBrains Mono
	// width, and Space Grotesk set inline another one.
	TEST_METHOD(ButtonAndTextAreaText_ResolveToTheConfiguredFamilies)
	{
		using namespace ArcRmlFormTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(OriginFixture, OriginSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const float Sans = GetFirstLineWidth(Layout, TEXT("fu-ref-sans"));
		const float Mono = GetFirstLineWidth(Layout, TEXT("fu-ref-mono"));
		ASSERT_THAT(IsTrue(Sans > 0.0f && Mono > 0.0f && FMath::Abs(Sans - Mono) > 0.5f,
			FString::Printf(TEXT("the two reference families give 'cd' different widths: sans %.3f, mono %.3f"), Sans, Mono)));
		FString Mismatches;
		const float Button = GetFirstLineWidth(Layout, TEXT("fu-button"));
		if (FMath::Abs(Button - Sans) > WidthTolerance)
		{
			Mismatches += FString::Printf(TEXT("[fu-button 'cd' is %.3f wide, Space Grotesk at 13.3333px %.3f, the JetBrains Mono body %.3f] "), Button, Sans, Mono);
		}
		const float MonoColumns = GetFirstLineWidth(Layout, TEXT("fu-ref-mono-m"));
		const float MonoAdvance = MonoColumns / 20.0f;
		const float MonoAverage = FMath::Max(MonoAdvance, FMath::RoundHalfFromZero(MonoAdvance));
		const float MonoTextArea = FMath::CeilToFloat(MonoAverage * 20.0f) + 15.0f + 6.0f;
		const FArcRmlTestLayoutElement* TextArea = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("fu-ta"));
		const float TextAreaWidth = TextArea ? TextArea->Rect.Size.X : -1.0f;
		if (MonoColumns <= 0.0f || FMath::Abs(TextAreaWidth - MonoTextArea) > WidthTolerance)
		{
			Mismatches += FString::Printf(TEXT("[fu-ta is %.3f wide, 20 JetBrains Mono columns (avg %g) plus 15 plus 6 is %.3f] "), TextAreaWidth, MonoAverage, MonoTextArea);
		}

		constexpr float ProbeFontSize = 75.0f;
		View.InspectEdit(TEXT("#fu-ta"), TEXT("font-size"), TEXT("75px"));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.Flush() && View.ParseLayout(Layout)));
		const float MonoProbeAdvance = MonoAdvance * ProbeFontSize / ControlFontSize;
		const float MonoProbeAverage = FMath::Max(MonoProbeAdvance, FMath::RoundHalfFromZero(MonoProbeAdvance));
		const float MonoProbeTextArea = FMath::CeilToFloat(MonoProbeAverage * 20.0f) + 15.0f + 6.0f;
		const FArcRmlTestLayoutElement* ProbeTextArea = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("fu-ta"));
		const float ProbeWidth = ProbeTextArea ? ProbeTextArea->Rect.Size.X : -1.0f;
		if (FMath::Abs(ProbeWidth - MonoProbeTextArea) > WidthTolerance)
		{
			Mismatches += FString::Printf(TEXT("[fu-ta at 75px is %.3f wide, 20 JetBrains Mono columns (avg %g) plus 15 plus 6 is %.3f] "), ProbeWidth, MonoProbeAverage, MonoProbeTextArea);
		}

		View.InspectEdit(TEXT("#fu-ta"), TEXT("font-family"), TEXT("\"Space Grotesk\""));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.Flush() && View.ParseLayout(Layout)));
		const FArcRmlTestLayoutElement* SansTextArea = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("fu-ta"));
		const float SansWidth = SansTextArea ? SansTextArea->Rect.Size.X : -1.0f;
		if (SansWidth <= 0.0f || FMath::Abs(SansWidth - ProbeWidth) <= 0.5f)
		{
			Mismatches += FString::Printf(TEXT("[fu-ta at 75px is %.3f wide in Space Grotesk and %.3f in the user-agent monospace: the width does not tell the families apart] "),
				SansWidth, ProbeWidth);
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("button text is the configured sans-serif and textarea text the configured monospace at 13.3333px, not the parent's family")) + TEXT(": ") + Mismatches));
	}
};

TEST_CLASS(ArcRmlBorderOutsetPixelTests, "ArcRmlUI.Pixels.BorderOutset")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(OutsetBorders_PaintChromesLightTopAndDarkBottomAndLoseToDotted)
	{
		using namespace ArcRmlFormTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(OutsetFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), OutsetSheet), false, true, true);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.DumpLayout(GetDumpDir(FPaths::GetBaseFilename(Path)));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.RenderPixels()));
		const FString PixelMismatches = DescribePixels(View, ChromeOutsetPixels);
		ASSERT_THAT(IsTrue(PixelMismatches.IsEmpty(), FString(TEXT("outset lightens top and left and darkens bottom and right (inset swapped); currentcolor starts from rgb(238,238,238); the user-agent button border is 2px outset black")) + TEXT(": ") + PixelMismatches));
		const FString CollapseMismatches = DescribePixels(View, ChromeCollapsePixels);
		ASSERT_THAT(IsTrue(CollapseMismatches.IsEmpty(), FString(TEXT("on an equally wide collapsed edge the later cell's dotted border beats the first cell's outset")) + TEXT(": ") + CollapseMismatches));
	}
};
