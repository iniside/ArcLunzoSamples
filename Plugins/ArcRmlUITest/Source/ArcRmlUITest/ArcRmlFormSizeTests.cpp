// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace ArcRmlFormSizeTestsPrivate
{
	const TCHAR* const SizesFixture = TEXT("Form/FormSizes.rml");
	const TCHAR* const SizesSheet = TEXT("FormSizes.rcssc");
	const TCHAR* const OracleFixture = TEXT("Form/FormOracleProbe.rml");
	const TCHAR* const OracleSheet = TEXT("FormOracleProbe.rcssc");
	const TCHAR* const OracleModel = TEXT("fo");
	const TCHAR* const NoFontFixture = TEXT("Form/FormNoFont.rml");
	const TCHAR* const NoFontSheet = TEXT("FormNoFont.rcssc");
	const TCHAR* const LongOption = TEXT("A much longer option label here");

	// Widths and heights equal Chrome's: a width within half a pixel (the dump rounds), a height exactly. Rows the fork misses
	// by a known gap carry their own width tolerance or the fork's height.
	constexpr float ExactWidthTolerance = 0.5f;
	constexpr float WidthTolerance = 1.0f;
	constexpr float HeightTolerance = 0.01f;
	constexpr float TopTolerance = 0.5f;
	constexpr float SameTolerance = 0.01f;

	// Chrome 156 border boxes of FormSizes (rml-forms step 4, VENDORED 225; chrome_layout_compare.py chrome --tol 0 on the
	// test's 1920x1080 dump, html_sha256 137abce31c91...). fs-select-pretty came after that dump and is checked against
	// fs-select-long by PrettyPrintedOptions_SizeAsTheirCollapsedText instead. Y is each control's top since controls sit on
	// Chrome's text baseline (rml-forms step 5, VENDORED 226; chrome --tol 0.5 on the test's dump, html_sha256 7cf2a8614678...,
	// 2026-11-06): every row as high as Chrome's, a select 1 below its row top, a checkbox 3. The rows hold one control each, x 0.
	struct FChromeBox
	{
		const TCHAR* Id;
		float Y;
		float Width;
		float Height;
		float Tolerance;
	};

	const FChromeBox ChromeBoxes[] = {
		{TEXT("fs-text"), 0.0f, 177.0f, 23.0f, ExactWidthTolerance},
		{TEXT("fs-text-size"), 27.0f, 97.0f, 23.0f, ExactWidthTolerance},
		{TEXT("fs-text-size3"), 54.0f, 41.0f, 23.0f, ExactWidthTolerance},
		{TEXT("fs-email"), 81.0f, 177.0f, 23.0f, ExactWidthTolerance},
		{TEXT("fs-password"), 108.0f, 177.0f, 23.0f, ExactWidthTolerance},
		{TEXT("fs-text-big"), 135.0f, 261.0f, 32.0f, ExactWidthTolerance},
		{TEXT("fs-ta"), 171.0f, 181.0f, 42.0f, ExactWidthTolerance},
		{TEXT("fs-ta-rc"), 222.0f, 261.0f, 78.0f, ExactWidthTolerance},
		{TEXT("fs-select-short"), 310.0f, 39.0f, 21.0f, ExactWidthTolerance},
		{TEXT("fs-select-long"), 336.0f, 102.0f, 21.0f, ExactWidthTolerance},
		{TEXT("fs-select-i"), 362.0f, 36.0f, 21.0f, ExactWidthTolerance},
		{TEXT("fs-select-w"), 388.0f, 116.0f, 21.0f, ExactWidthTolerance},
		// Chrome 227; the fork's option text is 226 (known gap of step 4).
		{TEXT("fs-select-longer"), 414.0f, 227.0f, 21.0f, WidthTolerance},
		{TEXT("fs-select-label"), 440.0f, 167.0f, 21.0f, ExactWidthTolerance},
		{TEXT("fs-select-big"), 465.0f, 142.0f, 30.0f, ExactWidthTolerance},
		// Fork pin, known gap: y 501 and height 19 are the fork's, Chrome's are y 502 and height 17; pinned so a change shows.
		{TEXT("fs-select-none"), 501.0f, 82.0f, 19.0f, ExactWidthTolerance},
		{TEXT("fs-checkbox"), 527.0f, 13.0f, 13.0f, ExactWidthTolerance},
		{TEXT("fs-radio"), 552.0f, 13.0f, 13.0f, ExactWidthTolerance},
		// Space Grotesk text inputs: the 'x' advances 9.488 (16 px) and 14.232 (24 px) stay unrounded, 18 px rounds up.
		{TEXT("fs-text-16"), 574.0f, 209.0f, 27.0f, ExactWidthTolerance},
		{TEXT("fs-text-16-size10"), 605.0f, 114.0f, 27.0f, ExactWidthTolerance},
		{TEXT("fs-text-18"), 636.0f, 240.0f, 29.0f, ExactWidthTolerance},
		{TEXT("fs-text-24"), 669.0f, 310.0f, 37.0f, ExactWidthTolerance},
		// JetBrains Mono textareas: the 14.4 px advance at 24 px stays unrounded, the others round up.
		{TEXT("fs-ta-16"), 710.0f, 221.0f, 48.0f, ExactWidthTolerance},
		{TEXT("fs-ta-18"), 767.0f, 241.0f, 52.0f, ExactWidthTolerance},
		{TEXT("fs-ta-big"), 828.0f, 261.0f, 58.0f, ExactWidthTolerance},
		{TEXT("fs-ta-24"), 895.0f, 309.0f, 68.0f, ExactWidthTolerance},
	};

	FString DescribeBox(const TArray<FArcRmlTestLayoutElement>& Layout, const FChromeBox& Box)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Box.Id);
		if (!Element)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Box.Id);
		}
		const bool bWidthOff = FMath::Abs(Element->Rect.Size.X - Box.Width) > Box.Tolerance;
		const bool bHeightOff = FMath::Abs(Element->Rect.Size.Y - Box.Height) > HeightTolerance;
		const bool bTopOff = FMath::Abs(Element->Rect.Position.Y - Box.Y) > TopTolerance;
		if (bWidthOff || bHeightOff || bTopOff)
		{
			return FString::Printf(TEXT("[%s %.3f x %.3f at y %.3f, expected %g x %g at y %g (width within %g, y within %g)] "), Box.Id, Element->Rect.Size.X,
				Element->Rect.Size.Y, Element->Rect.Position.Y, Box.Width, Box.Height, Box.Y, Box.Tolerance, TopTolerance);
		}
		return FString();
	}

	FVector2f GetSize(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* Id)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Id);
		return Element ? Element->Rect.Size : FVector2f(-1.0f, -1.0f);
	}

	// A row of the FormOracleProbe preview model's rows list ({ "v", "name" }).
	FArcRmlValue MakeRow(const FString& Value, const FString& Name)
	{
		FArcRmlValue Row;
		Row.Kind = EArcRmlValueKind::Struct;
		Row.MemberNames = {TEXT("v"), TEXT("name")};
		Row.Children.Add(ArcRmlUITestHelpers::MakeString(Value));
		Row.Children.Add(ArcRmlUITestHelpers::MakeString(Name));
		return Row;
	}
}

// Form-control intrinsic sizes (rml-forms step 4, VENDORED 225): every FormSizes box against Chrome, the select following
// option adds, removals and appearance changes, and checkbox and radio at a dp ratio other than 1.
TEST_CLASS(ArcRmlFormSizeTests, "ArcRmlUI.Form.Sizes")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Relative, const TCHAR* Sheet, float DpRatio = 1.0f)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(Relative);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path, DpRatio) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		// The dump is the input of chrome_layout_compare.py chrome --tol 0, which gave the expected values.
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), FPaths::GetBaseFilename(Path))));
		return View.Flush() && View.Flush() && View.ParseLayout(Layout);
	}

	bool Settle()
	{
		Layout.Reset();
		return View.FlushUntilSettled() && View.Flush() && View.ParseLayout(Layout);
	}

	void ApplyOp(EArcRmlListOpType Type, int32 Index, FArcRmlValue&& Value = FArcRmlValue())
	{
		TArray<FArcRmlListOp> Ops;
		FArcRmlListOp& Op = Ops.AddDefaulted_GetRef();
		Op.Type = Type;
		Op.Index = Index;
		Op.Value = MoveTemp(Value);
		View.ApplyListOps(ArcRmlFormSizeTestsPrivate::OracleModel, ArcRmlUITestHelpers::MakeFieldPath(TEXT("rows")), MoveTemp(Ops));
	}

	TEST_METHOD(EveryControl_TakesChromesBorderBox)
	{
		using namespace ArcRmlFormSizeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(SizesFixture, SizesSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const FChromeBox& Box : ChromeBoxes)
		{
			Mismatches += DescribeBox(Layout, Box);
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("text inputs, textareas, selects, checkbox and radio take Chrome's border boxes and tops")) + TEXT(": ") + Mismatches));
	}

	// At dp ratio 2 every length doubles: checkbox and radio 26 x 26, the bare textarea 2 x 181 and the "cd" select 2 x 21
	// high, its width within 2 px of 2 x 39 (the option text is measured at the doubled size). The textarea is pinned to the
	// fork's 363, Chrome 362, known gap: the JetBrains Mono 'x' at 2 x 13.3333 px comes out a hair above 16, which max(x,
	// round(x)) keeps, so ceil(20 x avg) is 321 instead of 320.
	TEST_METHOD(CheckboxAndRadio_Are13PxTimesTheDpRatio)
	{
		using namespace ArcRmlFormSizeTestsPrivate;
		constexpr float DpRatio = 2.0f;
		ASSERT_THAT(IsTrue(OpenFixture(SizesFixture, SizesSheet, DpRatio), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const TCHAR* Id : {TEXT("fs-checkbox"), TEXT("fs-radio")})
		{
			const FVector2f Size = GetSize(Layout, Id);
			if (FMath::Abs(Size.X - 13.0f * DpRatio) > SameTolerance || FMath::Abs(Size.Y - 13.0f * DpRatio) > SameTolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s %.3f x %.3f, expected %g x %g] "), Id, Size.X, Size.Y, 13.0f * DpRatio, 13.0f * DpRatio);
			}
		}
		const FVector2f TextArea = GetSize(Layout, TEXT("fs-ta"));
		constexpr float ForkTextAreaWidth = 181.0f * DpRatio + 1.0f;
		if (FMath::Abs(TextArea.X - ForkTextAreaWidth) > SameTolerance)
		{
			Mismatches += FString::Printf(TEXT("[fs-ta %.3f wide, expected the fork's %g (Chrome %g)] "), TextArea.X, ForkTextAreaWidth, 181.0f * DpRatio);
		}
		const FVector2f Select = GetSize(Layout, TEXT("fs-select-short"));
		if (FMath::Abs(Select.Y - 21.0f * DpRatio) > SameTolerance || FMath::Abs(Select.X - 39.0f * DpRatio) > 2.0f)
		{
			Mismatches += FString::Printf(TEXT("[fs-select-short %.3f x %.3f, expected %g (within 2) x %g] "), Select.X, Select.Y, 39.0f * DpRatio, 21.0f * DpRatio);
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("checkbox, radio, textarea and select sizes scale by the dp ratio 2")) + TEXT(": ") + Mismatches));
	}

	// The data-for select of FormOracleProbe holds Ash and Birch; an added option wider than both widens it to Chrome's box of a
	// select with that option (fs-select-longer), and each removal narrows it to its widest remaining option. The option is
	// added under the select's selectbox, whose own structural dirtying already reaches the select, so this stays green without
	// the select's OnChildAdd / OnChildRemove DirtyLayout.
	TEST_METHOD(DataForSelect_ResizesWhenAnOptionIsAddedOrRemoved)
	{
		using namespace ArcRmlFormSizeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(OracleFixture, OracleSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Loaded = GetSize(Layout, TEXT("fo-select-for"));
		ASSERT_THAT(IsTrue(Loaded.X > 0.0f && Loaded.X < 150.0f, FString::Printf(TEXT("the loaded Ash / Birch select is %.3f wide"), Loaded.X)));

		ApplyOp(EArcRmlListOpType::Insert, 2, MakeRow(TEXT("r3"), LongOption));
		ASSERT_THAT(IsTrue(Settle()));
		const FVector2f Added = GetSize(Layout, TEXT("fo-select-for"));
		ASSERT_THAT(IsTrue(FMath::Abs(Added.X - 227.0f) <= WidthTolerance,
			FString::Printf(TEXT("an added '%s' option widens the select to Chrome's 227 px; it is %.3f (loaded %.3f)"), LongOption, Added.X, Loaded.X)));
		ASSERT_THAT(IsTrue(FMath::Abs(Added.Y - Loaded.Y) <= SameTolerance, TEXT("an added option keeps the select one line high")));

		ApplyOp(EArcRmlListOpType::Remove, 2);
		ASSERT_THAT(IsTrue(Settle()));
		const FVector2f Removed = GetSize(Layout, TEXT("fo-select-for"));
		ASSERT_THAT(IsTrue(FMath::Abs(Removed.X - Loaded.X) <= SameTolerance,
			FString::Printf(TEXT("removing the long option narrows the select back to %.3f; it is %.3f"), Loaded.X, Removed.X)));

		ApplyOp(EArcRmlListOpType::Remove, 1);
		ASSERT_THAT(IsTrue(Settle()));
		const FVector2f AshOnly = GetSize(Layout, TEXT("fo-select-for"));
		ASSERT_THAT(IsTrue(AshOnly.X > 0.0f && AshOnly.X < Loaded.X - 1.0f,
			FString::Printf(TEXT("removing Birch, the widest option, narrows the select to Ash; it is %.3f, with Birch %.3f"), AshOnly.X, Loaded.X)));
	}

	// FormNoFont names only an unloaded family on its text input and textarea, so both have no font face (FontProvider returns
	// none: the family is not generic and nothing appends the fallback family). Their content width then holds no text width:
	// 0 for the input, the 15 px scrollbar alone for the textarea. Without the null-face guard in InputTypeText and
	// ElementFormControlTextArea::GetIntrinsicDimensions the first layout reads the metrics of a null face handle.
	TEST_METHOD(TextFields_WithoutALoadedFace_TakeNoTextWidth)
	{
		using namespace ArcRmlFormSizeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(NoFontFixture, NoFontSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Input = GetSize(Layout, TEXT("fnf-text"));
		const FVector2f TextArea = GetSize(Layout, TEXT("fnf-ta"));
		// Input: 2 px padding and a 2 px border per side. Textarea: 2 px padding and a 1 px border per side.
		constexpr float InputFrame = 2.0f * (2.0f + 2.0f);
		constexpr float TextAreaFrame = 2.0f * (2.0f + 1.0f);
		ASSERT_THAT(IsTrue(FMath::Abs(Input.X - InputFrame) <= SameTolerance,
			FString::Printf(TEXT("a text input with no font face is %g wide (padding and border, no text); it is %.3f x %.3f"), InputFrame, Input.X, Input.Y)));
		ASSERT_THAT(IsTrue(FMath::Abs(TextArea.X - (15.0f + TextAreaFrame)) <= SameTolerance,
			FString::Printf(TEXT("a textarea with no font face is %g wide (the 15 px scrollbar, padding and border); it is %.3f x %.3f"), 15.0f + TextAreaFrame, TextArea.X, TextArea.Y)));
	}

	// A bound option text that grows widens the select as an added option does.
	TEST_METHOD(DataForSelect_ResizesWhenAnOptionTextChanges)
	{
		using namespace ArcRmlFormSizeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(OracleFixture, OracleSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Loaded = GetSize(Layout, TEXT("fo-select-for"));
		View.SetModelValue(OracleModel, ArcRmlUITestHelpers::MakeRowFieldPath(TEXT("rows"), 1, TEXT("name")), ArcRmlUITestHelpers::MakeString(LongOption));
		ASSERT_THAT(IsTrue(Settle()));
		const FVector2f Renamed = GetSize(Layout, TEXT("fo-select-for"));
		ASSERT_THAT(IsTrue(FMath::Abs(Renamed.X - 227.0f) <= WidthTolerance,
			FString::Printf(TEXT("renaming Birch to '%s' widens the select to Chrome's 227 px; it is %.3f (loaded %.3f)"), LongOption, Renamed.X, Loaded.X)));
	}

	// fs-select-long and fs-select-none hold the same options; an inline appearance change on one makes it the other's size in
	// Chrome (82 px without the 20 px of the native look, 102 px with it), and the height drops the 2 px.
	TEST_METHOD(Select_ResizesWhenItsAppearanceSwitches)
	{
		using namespace ArcRmlFormSizeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(SizesFixture, SizesSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Auto = GetSize(Layout, TEXT("fs-select-long"));
		const FVector2f None = GetSize(Layout, TEXT("fs-select-none"));
		ASSERT_THAT(IsTrue(Auto.X - None.X > 10.0f, FString::Printf(TEXT("the loaded auto select (%.3f) is wider than the none one (%.3f)"), Auto.X, None.X)));
		ASSERT_THAT(IsTrue(FMath::Abs((Auto.Y - None.Y) - 2.0f) <= SameTolerance,
			FString::Printf(TEXT("the loaded auto select is 2 px higher than the none one; it is %.3f against %.3f"), Auto.Y, None.Y)));

		View.InspectEdit(TEXT("#fs-select-long"), TEXT("appearance"), TEXT("none"));
		ASSERT_THAT(IsTrue(Settle()));
		const FVector2f ToNone = GetSize(Layout, TEXT("fs-select-long"));
		ASSERT_THAT(IsTrue(FMath::Abs(ToNone.X - 82.0f) <= WidthTolerance && FMath::Abs(ToNone.X - None.X) <= SameTolerance
				&& FMath::Abs(ToNone.Y - None.Y) <= SameTolerance,
			FString::Printf(TEXT("appearance: none makes the select %.3f x %.3f; the loaded none select is %.3f x %.3f, Chrome 82 wide"), ToNone.X, ToNone.Y, None.X, None.Y)));

		View.InspectEdit(TEXT("#fs-select-long"), TEXT("appearance"), TEXT("auto"));
		ASSERT_THAT(IsTrue(Settle()));
		const FVector2f BackToAuto = GetSize(Layout, TEXT("fs-select-long"));
		ASSERT_THAT(IsTrue(FMath::Abs(BackToAuto.X - Auto.X) <= SameTolerance && FMath::Abs(BackToAuto.Y - Auto.Y) <= SameTolerance,
			FString::Printf(TEXT("appearance: auto again gives back %.3f x %.3f; it is %.3f x %.3f"), Auto.X, Auto.Y, BackToAuto.X, BackToAuto.Y)));
	}

	// fs-select-pretty holds fs-select-long's options pretty-printed, each text between a newline and indentation; the select
	// sizes by the stripped and collapsed text, so it takes fs-select-long's 102 x 21.
	TEST_METHOD(PrettyPrintedOptions_SizeAsTheirCollapsedText)
	{
		using namespace ArcRmlFormSizeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(SizesFixture, SizesSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Pretty = GetSize(Layout, TEXT("fs-select-pretty"));
		const FVector2f Long = GetSize(Layout, TEXT("fs-select-long"));
		ASSERT_THAT(IsTrue(FMath::Abs(Pretty.X - 102.0f) <= ExactWidthTolerance && FMath::Abs(Pretty.X - Long.X) <= SameTolerance
				&& FMath::Abs(Pretty.Y - Long.Y) <= SameTolerance,
			FString::Printf(TEXT("the pretty-printed select is %.3f x %.3f; fs-select-long is %.3f x %.3f, Chrome 102 wide"), Pretty.X, Pretty.Y, Long.X, Long.Y)));
	}
};
