// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"

namespace ArcRmlFormLookTestsPrivate
{
	const TCHAR* const LookFixture = TEXT("Form/FormLookPixel.rml");
	const TCHAR* const LookSheet = TEXT("FormLookPixel.rcssc");
	const TCHAR* const OpenFixture = TEXT("Form/SelectOpenPixel.rml");
	const TCHAR* const OpenSheet = TEXT("SelectOpenPixel.rcssc");
	const TCHAR* const PartFixture = TEXT("Form/SelectPartRules.rml");
	const TCHAR* const PartSheet = TEXT("SelectPartRules.rcssc");
	const TCHAR* const PopupFixture = TEXT("Form/SelectPopupPixel.rml");
	const TCHAR* const PopupSheet = TEXT("SelectPopupPixel.rcssc");

	// The range's Pixels.RangeDefault tolerance.
	constexpr int32 PixelTolerance = 2;

	// One point of a Chrome screenshot and the point of the fork's readback it stands for: both are equal for every control but the
	// 26 px checkbox and radio, which Chrome lays out 13 px further left (the appearance: none ones before them are 0 x 0 there,
	// 13 x 13 here), and the open list's rows, which are 25 px high in Chrome and 17 px here (VENDORED 231 "Not done").
	struct FChromePixel
	{
		const TCHAR* Name;
		int32 X;
		int32 Y;
		int32 ChromeX;
		int32 ChromeY;
		FColor Chrome;
	};

	bool OpenFresh(FArcRmlTestView& View, const TCHAR* Fixture, const TCHAR* Sheet)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	// The fixture's .html and .layout.json for chrome_layout_compare.py and the CDP shot of the open list.
	bool DumpForChrome(FArcRmlTestView& View, const TCHAR* Name)
	{
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), Name)));
		return View.Flush() && View.Flush();
	}

	// Clicks the select's centre with the pointer resting there, as the CDP click does in Chrome.
	bool OpenList(FArcRmlTestView& View, const TCHAR* DomId)
	{
		TArray<FArcRmlTestLayoutElement> Layout;
		if (!View.ParseLayout(Layout))
		{
			return false;
		}
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Select)
		{
			return false;
		}
		const FVector2f Centre = Select->Rect.Position + Select->Rect.Size * 0.5f;
		View.MouseMove(Centre);
		View.MouseClick(Centre);
		return View.FlushUntilSettled() && View.Flush() && View.GetSnapshot().bSelectListOpen;
	}

	FString DescribeMismatches(const FArcRmlTestView& View, TConstArrayView<FChromePixel> Expected)
	{
		FString Mismatches;
		for (const FChromePixel& Pixel : Expected)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (FMath::Abs(Measured.R - Pixel.Chrome.R) > PixelTolerance || FMath::Abs(Measured.G - Pixel.Chrome.G) > PixelTolerance
				|| FMath::Abs(Measured.B - Pixel.Chrome.B) > PixelTolerance || FMath::Abs(Measured.A - Pixel.Chrome.A) > PixelTolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s (%d,%d): %d,%d,%d,%d, Chrome (%d,%d) %d,%d,%d,%d] "), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G,
					Measured.B, Measured.A, Pixel.ChromeX, Pixel.ChromeY, Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B, Pixel.Chrome.A);
			}
		}
		return Mismatches;
	}

	// The select's internal part with this tag, found under the select's non-DOM ('~') path.
	const FArcRmlTestLayoutElement* FindPart(const TArray<FArcRmlTestLayoutElement>& Layout, const FArcRmlTestLayoutElement& Select, const TCHAR* Tag)
	{
		const FString PartPrefix = Select.Path + TEXT("~");
		const FString PartTag = Tag;
		return Layout.FindByPredicate([&PartPrefix, &PartTag](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Path.StartsWith(PartPrefix) && Element.Tag == PartTag;
		});
	}

	const FArcRmlTestLayoutElement* FindOption(const TArray<FArcRmlTestLayoutElement>& Layout, const FArcRmlTestLayoutElement& Box, int32 Index)
	{
		int32 Seen = 0;
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Tag == TEXT("option") && Element.Parent == Box.Path)
			{
				if (Seen == Index)
				{
					return &Element;
				}
				++Seen;
			}
		}
		return nullptr;
	}

	FString DescribeRect(const FArcRmlTestLayoutElement* Element)
	{
		return Element ? FString::Printf(TEXT("(%.1f,%.1f %.1fx%.1f)"), Element->Rect.Position.X, Element->Rect.Position.Y, Element->Rect.Size.X, Element->Rect.Size.Y)
			: FString(TEXT("<none>"));
	}
}

// Default look of the form controls (rml-forms step 9, VENDORED 231) against Chrome 156's screenshots in
// Saved/ArcRml/Step9Look/chrome/ of the fixtures' layout dumps (Saved/Automation/Tmp/ArcRmlUITest/<fixture>/, written by each test):
// FormLookPixel.page.png (FormLookPixel.html sha256 d306f053...), SelectOpenPixel.open.page.png (SelectOpenPixel.html 49e90220...)
// and SelectPopupPixel.dark.png / .semi.png (the dump with Red's inline style put back, which the dump drops: 272fd464...), each
// list opened by a CDP click. Every page is rgb(180, 200, 220), so a white fill that is not drawn shows the page. Points are inside a
// fill or on a 1 px straight border line or a chevron or tick pixel that Chrome draws solid; Chrome's anti-aliased edges (radio
// rings, corners) are left out, as the fork's meshes are hard-edged.
TEST_CLASS(ArcRmlFormLookPixelTests, "ArcRmlUI.Pixels.FormLook")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(DefaultLook_EveryControl_MatchesChromePixels)
	{
		using namespace ArcRmlFormLookTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFresh(View, LookFixture, LookSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(DumpForChrome(View, TEXT("FormLookPixel"))));
		ASSERT_THAT(IsTrue(View.RenderPixels()));

		const FColor Border(118, 118, 118, 255);
		const FColor White(255, 255, 255, 255);
		const FColor Black(0, 0, 0, 255);
		const FColor AuthorFill(255, 240, 200, 255);
		const FColor AuthorBorder(200, 0, 0, 255);
		const FColor Ink(0, 120, 0, 255);
		const FColor ButtonFill(239, 239, 239, 255);
		const FColor ButtonFace(240, 240, 240, 255);
		const FColor Accent(0, 117, 255, 255);
		const FColor AuthorAccent(220, 30, 60, 255);
		const FColor Page(180, 200, 220, 255);
		const FChromePixel Expected[] = {
			// Text fields: the native box; appearance: none paints the computed 2 px inset border; an author box replaces the look.
			{TEXT("fl-text border"), 10, 17, 10, 17, Border},
			{TEXT("fl-text fill"), 100, 17, 100, 17, White},
			{TEXT("fl-ph fill"), 300, 17, 300, 17, White},
			{TEXT("fl-text-none inset top"), 450, 6, 450, 6, FColor(33, 33, 33, 255)},
			{TEXT("fl-text-none inset right"), 563, 12, 563, 12, FColor(202, 202, 202, 255)},
			{TEXT("fl-text-author fill"), 660, 16, 660, 16, AuthorFill},
			{TEXT("fl-text-author border"), 577, 10, 577, 10, AuthorBorder},
			// Textareas.
			{TEXT("fl-ta border"), 10, 70, 10, 70, Border},
			{TEXT("fl-ta fill"), 100, 70, 100, 70, White},
			{TEXT("fl-ta-none computed border"), 250, 52, 250, 52, Border},
			// Selects: box and chevron; none draws no chevron; an author box keeps the chevron; the chevron takes the select's color.
			{TEXT("fl-sel fill"), 50, 121, 50, 121, White},
			{TEXT("fl-sel border"), 10, 128, 10, 128, Border},
			{TEXT("fl-sel chevron left arm"), 99, 126, 99, 126, Black},
			{TEXT("fl-sel chevron tip"), 102, 129, 102, 129, Black},
			{TEXT("fl-sel chevron right arm"), 105, 127, 105, 127, Black},
			{TEXT("fl-sel-none computed border"), 130, 118, 130, 118, Border},
			{TEXT("fl-sel-author fill"), 260, 121, 260, 121, AuthorFill},
			{TEXT("fl-sel-author border"), 218, 128, 218, 128, AuthorBorder},
			{TEXT("fl-sel-author chevron left arm"), 307, 126, 307, 126, Black},
			{TEXT("fl-sel-author chevron tip"), 310, 129, 310, 129, Black},
			{TEXT("fl-sel-color fill"), 360, 121, 360, 121, White},
			{TEXT("fl-sel-color chevron left arm"), 378, 126, 378, 126, Ink},
			{TEXT("fl-sel-color chevron tip"), 381, 129, 381, 129, Ink},
			// Buttons: the native box filled 239; appearance: none shows the user-agent ButtonFace 240 and the computed outset border.
			{TEXT("fl-btn fill"), 12, 175, 12, 175, ButtonFill},
			{TEXT("fl-btn border"), 10, 175, 10, 175, Border},
			{TEXT("fl-ibutton fill"), 70, 175, 70, 175, ButtonFill},
			{TEXT("fl-submit fill"), 120, 166, 120, 166, ButtonFill},
			{TEXT("fl-reset fill"), 178, 166, 178, 166, ButtonFill},
			{TEXT("fl-ibutton border top"), 85, 164, 85, 164, Border},
			{TEXT("fl-submit border top"), 137, 164, 137, 164, Border},
			{TEXT("fl-reset border top"), 198, 164, 198, 164, Border},
			{TEXT("fl-btn-none user-agent ButtonFace"), 238, 168, 238, 168, ButtonFace},
			{TEXT("fl-btn-none outset top"), 240, 164, 240, 164, FColor(168, 168, 168, 255)},
			{TEXT("fl-btn-none outset right"), 282, 170, 282, 170, FColor(84, 84, 84, 255)},
			{TEXT("fl-btn-author fill"), 300, 170, 300, 170, AuthorFill},
			{TEXT("fl-btn-author border"), 296, 170, 296, 170, AuthorBorder},
			// Checkboxes: unchecked grey border on white, checked accent fill with a white tick, accent-color, none draws nothing.
			{TEXT("fl-cb border"), 14, 219, 14, 219, Border},
			{TEXT("fl-cb fill"), 20, 219, 20, 219, White},
			{TEXT("fl-cb-on fill"), 48, 224, 48, 224, Accent},
			{TEXT("fl-cb-on tick"), 52, 220, 52, 220, White},
			{TEXT("fl-cb-accent fill"), 80, 224, 80, 224, AuthorAccent},
			{TEXT("fl-cb-none draws nothing inside its box"), 116, 219, 116, 219, Page},
			{TEXT("fl-cb-big fill"), 146, 215, 133, 215, Accent},
			{TEXT("fl-cb-big tick short arm"), 152, 229, 139, 229, White},
			{TEXT("fl-cb-big tick long arm"), 160, 220, 147, 220, White},
			// Radios: checked accent dot inside a white gap, accent-color, none draws nothing (the rings are anti-aliased in Chrome).
			{TEXT("fl-rd fill"), 21, 265, 21, 265, White},
			{TEXT("fl-rd-on dot"), 54, 265, 54, 265, Accent},
			{TEXT("fl-rd-on gap between ring and dot"), 49, 265, 49, 265, White},
			{TEXT("fl-rd-accent dot"), 87, 265, 87, 265, AuthorAccent},
			{TEXT("fl-rd-none draws nothing inside its box"), 120, 265, 120, 265, Page},
			{TEXT("fl-rd-big dot"), 160, 271, 147, 271, Accent},
			{TEXT("fl-rd-big gap between ring and dot"), 150, 272, 137, 272, White},
		};
		FString Mismatches = DescribeMismatches(View, MakeArrayView(Expected));
		// fl-sel-none draws no chevron: where fl-sel's chevron arms are, it shows the same colour as its own box beside them. Chrome
		// fills that box white (background-color: field), the fork leaves it transparent under appearance: none (VENDORED 231 "Not
		// done"), so this compares within the readback instead of against Chrome.
		const FColor NoneBox = View.GetPixel(185, 121);
		for (const FIntPoint& Arm : {FIntPoint(195, 126), FIntPoint(198, 129)})
		{
			const FColor Measured = View.GetPixel(Arm.X, Arm.Y);
			if (FMath::Abs(Measured.R - NoneBox.R) > PixelTolerance || FMath::Abs(Measured.G - NoneBox.G) > PixelTolerance || FMath::Abs(Measured.B - NoneBox.B) > PixelTolerance)
			{
				Mismatches += FString::Printf(TEXT("[fl-sel-none chevron (%d,%d): %d,%d,%d, its box (185,121) %d,%d,%d] "), Arm.X, Arm.Y, Measured.R, Measured.G, Measured.B, NoneBox.R,
					NoneBox.G, NoneBox.B);
			}
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// so-sel opened by a click on it, the pointer resting on the closed box: a white list with a 1 px grey border, Apple (selected)
	// in Chrome's popup highlight, Cherry in its author yellow (select option[value=c]), the other rows white.
	TEST_METHOD(SelectOpen_ListBorderHighlightAndOptionColours_MatchChromePixels)
	{
		using namespace ArcRmlFormLookTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFresh(View, OpenFixture, OpenSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(DumpForChrome(View, TEXT("SelectOpenPixel"))));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("so-sel"));
		ASSERT_THAT(IsNotNull(Select));
		const FVector2f Centre = Select->Rect.Position + Select->Rect.Size * 0.5f;
		View.MouseMove(Centre);
		View.MouseClick(Centre);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.Flush()));
		ASSERT_THAT(IsTrue(View.GetSnapshot().bSelectListOpen, TEXT("the click did not open so-sel's list")));
		ASSERT_THAT(IsTrue(View.RenderPixels()));

		const FColor Border(118, 118, 118, 255);
		const FColor White(255, 255, 255, 255);
		const FChromePixel Expected[] = {
			{TEXT("Apple highlighted"), 100, 40, 100, 40, FColor(25, 103, 210, 255)},
			{TEXT("list left border"), 10, 45, 10, 45, Border},
			{TEXT("list right border"), 111, 45, 111, 45, Border},
			{TEXT("list bottom border"), 60, 101, 60, 133, Border},
			{TEXT("Banana row not highlighted"), 100, 58, 100, 70, White},
			{TEXT("Cherry author background-color"), 100, 75, 100, 95, FColor(255, 255, 0, 255)},
			{TEXT("Date row not highlighted"), 100, 92, 100, 120, White},
		};
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(View, MakeArrayView(Expected))));
	}

	// SelectPopupPixel on its rgb(180, 200, 220) page, each list opened by a click on its select with the pointer resting on the
	// closed box (VENDORED 231 review fixes): spx-dark (color #fff, background-color #222) opens a list filled rgb(34, 34, 34) in the
	// grey border, Banana (selected) in the highlight, its options in white and Red in its inline red; spx-semi (background-color
	// rgba(255, 0, 0, 0.5)) opens filled with that colour over white, rgb(255, 127, 127), not over the page.
	TEST_METHOD(SelectOpen_DarkAndSemiTransparentLists_MatchChromePixels)
	{
		using namespace ArcRmlFormLookTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFresh(View, PopupFixture, PopupSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(DumpForChrome(View, TEXT("SelectPopupPixel"))));
		ASSERT_THAT(IsTrue(OpenList(View, TEXT("spx-dark")), TEXT("the click did not open spx-dark's list")));
		ASSERT_THAT(IsTrue(View.RenderPixels()));

		const FColor Border(118, 118, 118, 255);
		const FColor Highlight(25, 103, 210, 255);
		const FColor DarkFill(34, 34, 34, 255);
		const FChromePixel Dark[] = {
			{TEXT("dark Apple row fill"), 70, 40, 70, 44, DarkFill},
			{TEXT("dark Banana highlighted"), 70, 57, 70, 69, Highlight},
			{TEXT("dark Cherry row fill"), 70, 74, 70, 94, DarkFill},
			{TEXT("dark Elder row fill"), 70, 108, 70, 144, DarkFill},
			{TEXT("dark list left border"), 10, 45, 10, 45, Border},
			{TEXT("dark list right border"), 80, 45, 80, 45, Border},
			{TEXT("dark list bottom border"), 40, 117, 40, 157, Border},
		};
		FString Mismatches = DescribeMismatches(View, MakeArrayView(Dark));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("spx-dark"));
		ASSERT_THAT(IsNotNull(Select));
		const FArcRmlTestLayoutElement* Box = FindPart(Layout, *Select, TEXT("selectbox"));
		ASSERT_THAT(IsNotNull(Box));
		const FArcRmlTestLayoutElement* Apple = FindOption(Layout, *Box, 0);
		const FArcRmlTestLayoutElement* Red = FindOption(Layout, *Box, 3);
		ASSERT_THAT(IsTrue(Apple && Red, TEXT("spx-dark's open list does not hold its options")));
		if (Apple->Color != TEXT("#ffffffff") || Red->Color != TEXT("#ff0000ff"))
		{
			Mismatches += FString::Printf(TEXT("[dark option text: Apple %s (expected the select's #ffffffff), Red %s (expected its inline #ff0000ff)] "), *Apple->Color, *Red->Color);
		}

		View.PressKey(EKeys::Escape);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.Flush() && !View.GetSnapshot().bSelectListOpen, TEXT("Escape did not close spx-dark's list")));
		ASSERT_THAT(IsTrue(OpenList(View, TEXT("spx-semi")), TEXT("the click did not open spx-semi's list")));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		const FColor SemiFill(255, 127, 127, 255);
		const FChromePixel Semi[] = {
			{TEXT("semi Apple row fill"), 340, 40, 340, 44, SemiFill},
			{TEXT("semi Banana highlighted"), 340, 57, 340, 69, Highlight},
			{TEXT("semi Cherry row fill"), 340, 74, 340, 94, SemiFill},
			{TEXT("semi list left border"), 281, 45, 281, 45, Border},
			{TEXT("semi list right border"), 351, 45, 351, 45, Border},
			{TEXT("semi list bottom border"), 310, 83, 310, 107, Border},
		};
		Mismatches += DescribeMismatches(View, MakeArrayView(Semi));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};

// Internal select parts (VENDORED 231; replaces DropDownValueFormattedAsItsOwnRoot_KeepsTheBlockRole): SelectPartRules.rcss gives
// selectvalue, selectarrow and selectbox tag rules that would move, resize, pad and recolour them; none applies. The value sits in
// flow at the select's content-box left inside its 1 px user-agent border, the arrow stays inside the select, the open list's first
// option starts inside its 1 px border, and only the option rule colours Banana.
TEST_CLASS(ArcRmlSelectPartTests, "ArcRmlUI.Form.SelectParts")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(AuthorPartRules_DoNotReachTheInternalParts_AndOptionRulesStillDo)
	{
		using namespace ArcRmlFormLookTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFresh(View, PartFixture, PartSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("sp-sel"));
		ASSERT_THAT(IsNotNull(Select));
		const FArcRmlTestLayoutElement* Value = FindPart(Layout, *Select, TEXT("selectvalue"));
		const FArcRmlTestLayoutElement* Arrow = FindPart(Layout, *Select, TEXT("selectarrow"));
		ASSERT_THAT(IsNotNull(Value, TEXT("the select has no selectvalue")));
		ASSERT_THAT(IsNotNull(Arrow, TEXT("the select has no selectarrow")));

		FString Mismatches;
		constexpr float SelectBorder = 1.0f;
		constexpr float Tolerance = 0.5f;
		const float SelectBottom = Select->Rect.Position.Y + Select->Rect.Size.Y;
		if (FMath::Abs(Value->Rect.Position.X - (Select->Rect.Position.X + SelectBorder)) > Tolerance || Value->Rect.Position.Y < Select->Rect.Position.Y
			|| Value->Rect.Position.Y + Value->Rect.Size.Y > SelectBottom + Tolerance)
		{
			Mismatches += FString::Printf(TEXT("[selectvalue %s, expected in flow at the content-box left of the select %s] "), *DescribeRect(Value), *DescribeRect(Select));
		}
		if (Value->Color != Select->Color)
		{
			Mismatches += FString::Printf(TEXT("[selectvalue colour %s, expected the select's %s] "), *Value->Color, *Select->Color);
		}
		// The widget places the arrow over the select's content box (WidgetDropDown::OnLayout, TOP_RIGHT), out of flow: the rule's
		// 40 x 40 block would stick out below the select.
		if (Arrow->Rect.Position.Y + Arrow->Rect.Size.Y > SelectBottom + Tolerance || FMath::IsNearlyEqual(Arrow->Rect.Size.Y, 40.0f, Tolerance))
		{
			Mismatches += FString::Printf(TEXT("[selectarrow %s, expected inside the select %s, not the rule's 40 x 40] "), *DescribeRect(Arrow), *DescribeRect(Select));
		}

		const FVector2f Centre = Select->Rect.Position + Select->Rect.Size * 0.5f;
		View.MouseMove(Centre);
		View.MouseClick(Centre);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.Flush() && View.ParseLayout(Layout)));
		ASSERT_THAT(IsTrue(View.GetSnapshot().bSelectListOpen, TEXT("the click did not open sp-sel's list")));
		Select = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("sp-sel"));
		ASSERT_THAT(IsNotNull(Select));
		const FArcRmlTestLayoutElement* Box = FindPart(Layout, *Select, TEXT("selectbox"));
		ASSERT_THAT(IsNotNull(Box, TEXT("the select has no selectbox")));
		const FArcRmlTestLayoutElement* Apple = FindOption(Layout, *Box, 0);
		const FArcRmlTestLayoutElement* Banana = FindOption(Layout, *Box, 1);
		const FArcRmlTestLayoutElement* Cherry = FindOption(Layout, *Box, 2);
		ASSERT_THAT(IsTrue(Apple && Banana && Cherry, TEXT("the open list does not hold three options")));
		if (FMath::Abs(Apple->Rect.Position.X - (Box->Rect.Position.X + SelectBorder)) > Tolerance)
		{
			Mismatches += FString::Printf(TEXT("[Apple %s, expected at the list's left %s plus its 1 px border] "), *DescribeRect(Apple), *DescribeRect(Box));
		}
		if (Banana->Color != TEXT("#008000ff"))
		{
			Mismatches += FString::Printf(TEXT("[Banana colour %s, expected the option rule's #008000ff] "), *Banana->Color);
		}
		if (Cherry->Color != Select->Color)
		{
			Mismatches += FString::Printf(TEXT("[Cherry colour %s, expected the select's %s, not the selectbox rule's blue] "), *Cherry->Color, *Select->Color);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};
