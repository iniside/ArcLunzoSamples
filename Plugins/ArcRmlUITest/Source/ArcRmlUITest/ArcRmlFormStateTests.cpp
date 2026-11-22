// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

namespace ArcRmlFormStateTestsPrivate
{
	const TCHAR* const Fixture = TEXT("Form/FormStates.rml");
	const TCHAR* const Sheet = TEXT("FormStates.rcssc");
	const TCHAR* const ModelName = TEXT("fs");
	// FormStates is taller than the 720 px test view; Chrome was measured at 1920x1080.
	const FIntPoint PageSize(1920, 1080);

	// FormStates.rcss: each indicator span takes its colour when the .ctl before it in the cell matches the state.
	const TCHAR* const IndicatorOff = TEXT("#000000ff");
	const TCHAR* const EnabledOn = TEXT("#008000ff");
	const TCHAR* const DisabledOn = TEXT("#008001ff");
	const TCHAR* const RequiredOn = TEXT("#008002ff");
	const TCHAR* const OptionalOn = TEXT("#008003ff");
	const TCHAR* const ReadOnlyOn = TEXT("#008004ff");
	const TCHAR* const ReadWriteOn = TEXT("#008005ff");
	const TCHAR* const ReadOnlyBackground = TEXT("#faf0e6ff");
	const TCHAR* const ReadWriteBackground = TEXT("#e6f0faff");

	const TCHAR* const PlaceholderRed = TEXT("#c80000ff");
	const TCHAR* const PlaceholderFocusBlue = TEXT("#0000c8ff");
	const TCHAR* const PlaceholderFontGreen = TEXT("#007800ff");
	const TCHAR* const TailwindPlaceholderRed = TEXT("#ef4444ff");
	const TCHAR* const UserAgentPlaceholderGrey = TEXT("#757575ff");
	const TCHAR* const TailwindRequiredBlue = TEXT("#2563ebff");
	const TCHAR* const PlaceholderText = TEXT("Your name");

	struct FStates
	{
		bool bEnabled;
		bool bDisabled;
		bool bRequired;
		bool bOptional;
		bool bReadOnly;
		bool bReadWrite;
	};

	const FStates EnOptRw = {true, false, false, true, false, true};
	const FStates EnReqRw = {true, false, true, false, false, true};
	const FStates EnOptRo = {true, false, false, true, true, false};
	const FStates EnReqRo = {true, false, true, false, true, false};
	const FStates DisOptRo = {false, true, false, true, true, false};
	const FStates DisReqRo = {false, true, true, false, true, false};
	const FStates EnOpt = {true, false, false, true, false, false};
	const FStates EnReq = {true, false, true, false, false, false};
	const FStates DisOpt = {false, true, false, true, false, false};

	struct FStateCase
	{
		const TCHAR* Cell;
		const TCHAR* Control;
		FStates States;
	};

	// Chrome 156, chrome_layout_compare.py chrome --computed color,background-color on the test's FormStates dump (rml-forms step 6,
	// VENDORED 227): every :enabled, :disabled, :required, :optional indicator below equals Chrome's, and :read-only / :read-write of
	// every text field. Chrome also matches :read-only on the non-text controls (select, button, submit, checkbox, radio, range); the
	// fork matches neither there (plugin CLAUDE.md, the plan's 1% case), so their .ro indicator is expected off.
	const FStateCase LoadCases[] = {
		{TEXT("fs-cell-text"), TEXT("fs-text"), EnOptRw},
		{TEXT("fs-cell-text-req"), TEXT("fs-text-req"), EnReqRw},
		{TEXT("fs-cell-text-ro"), TEXT("fs-text-ro"), EnOptRo},
		{TEXT("fs-cell-text-off"), TEXT("fs-text-off"), DisOptRo},
		{TEXT("fs-cell-text-ro-req"), TEXT("fs-text-ro-req"), EnReqRo},
		{TEXT("fs-cell-email"), TEXT("fs-email"), EnReqRw},
		{TEXT("fs-cell-password"), TEXT("fs-password"), EnOptRo},
		{TEXT("fs-cell-ta"), TEXT("fs-ta"), EnOptRw},
		{TEXT("fs-cell-ta-req"), TEXT("fs-ta-req"), EnReqRw},
		{TEXT("fs-cell-ta-ro"), TEXT("fs-ta-ro"), EnOptRo},
		{TEXT("fs-cell-ta-off"), TEXT("fs-ta-off"), DisOptRo},
		{TEXT("fs-cell-select"), TEXT("fs-select"), EnOpt},
		{TEXT("fs-cell-select-req"), TEXT("fs-select-req"), EnReq},
		{TEXT("fs-cell-select-off"), TEXT("fs-select-off"), DisOpt},
		{TEXT("fs-cell-button-off"), TEXT("fs-button-off"), DisOpt},
		{TEXT("fs-cell-checkbox"), TEXT("fs-checkbox"), EnOpt},
		{TEXT("fs-cell-checkbox-req"), TEXT("fs-checkbox-req"), EnReq},
		{TEXT("fs-cell-radio-req"), TEXT("fs-radio-req"), EnReq},
		{TEXT("fs-cell-bound"), TEXT("fs-bound"), EnOptRw},
		{TEXT("fs-cell-ta-bound"), TEXT("fs-ta-bound"), EnOptRw},
		{TEXT("fs-cell-kind"), TEXT("fs-kind"), EnReqRw},
	};

	// A button, a submit and a range with required are :optional, not :required, in Chrome (Blink's IsOptionalFormControl). Their .ro is
	// expected off by the fork's rule; Chrome matches :read-only on them.
	const FStateCase RequiredIgnoredCases[] = {
		{TEXT("fs-cell-button"), TEXT("fs-button"), EnOpt},
		{TEXT("fs-cell-submit"), TEXT("fs-submit"), EnOpt},
		{TEXT("fs-cell-range-req"), TEXT("fs-range-req"), EnOpt},
	};

	const FArcRmlTestLayoutElement* FindChild(const TArray<FArcRmlTestLayoutElement>& Layout, const FString& ParentPath, const TCHAR* Tag, const TCHAR* Class)
	{
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Parent != ParentPath || Element.Tag != Tag)
			{
				continue;
			}
			TArray<FString> Classes;
			Element.Classes.ParseIntoArrayWS(Classes);
			if (FCString::Strlen(Class) == 0 || Classes.Contains(Class))
			{
				return &Element;
			}
		}
		return nullptr;
	}

	FString DescribeIndicator(const TArray<FArcRmlTestLayoutElement>& Layout, const FArcRmlTestLayoutElement& Cell, const TCHAR* Control, const TCHAR* Class,
		bool bOn, const TCHAR* OnColour)
	{
		const FArcRmlTestLayoutElement* Indicator = FindChild(Layout, Cell.Path, TEXT("span"), Class);
		if (!Indicator)
		{
			return FString::Printf(TEXT("[%s: no .%s indicator] "), Control, Class);
		}
		const TCHAR* Expected = bOn ? OnColour : IndicatorOff;
		if (Indicator->Color != Expected)
		{
			return FString::Printf(TEXT("[%s .%s %s, expected %s (%s)] "), Control, Class, *Indicator->Color, Expected, bOn ? TEXT("matches") : TEXT("does not match"));
		}
		return FString();
	}

	FString DescribeStates(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* CellId, const TCHAR* Control, const FStates& States)
	{
		const FArcRmlTestLayoutElement* Cell = ArcRmlUITestHelpers::FindByDomId(Layout, CellId);
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Control);
		if (!Cell || !Element)
		{
			return FString::Printf(TEXT("[%s or %s is not in the layout] "), CellId, Control);
		}
		FString Mismatches;
		Mismatches += DescribeIndicator(Layout, *Cell, Control, TEXT("en"), States.bEnabled, EnabledOn);
		Mismatches += DescribeIndicator(Layout, *Cell, Control, TEXT("dis"), States.bDisabled, DisabledOn);
		Mismatches += DescribeIndicator(Layout, *Cell, Control, TEXT("req"), States.bRequired, RequiredOn);
		Mismatches += DescribeIndicator(Layout, *Cell, Control, TEXT("opt"), States.bOptional, OptionalOn);
		Mismatches += DescribeIndicator(Layout, *Cell, Control, TEXT("ro"), States.bReadOnly, ReadOnlyOn);
		Mismatches += DescribeIndicator(Layout, *Cell, Control, TEXT("rw"), States.bReadWrite, ReadWriteOn);
		if (States.bReadOnly && Element->BackgroundColor != ReadOnlyBackground)
		{
			Mismatches += FString::Printf(TEXT("[%s background %s, .ctl:read-only %s] "), Control, *Element->BackgroundColor, ReadOnlyBackground);
		}
		if (States.bReadWrite && Element->BackgroundColor != ReadWriteBackground)
		{
			Mismatches += FString::Printf(TEXT("[%s background %s, .ctl:read-write %s] "), Control, *Element->BackgroundColor, ReadWriteBackground);
		}
		if (!States.bReadOnly && !States.bReadWrite && (Element->BackgroundColor == ReadOnlyBackground || Element->BackgroundColor == ReadWriteBackground))
		{
			Mismatches += FString::Printf(TEXT("[%s background %s from a .ctl:read-only or :read-write rule] "), Control, *Element->BackgroundColor);
		}
		return Mismatches;
	}

	FString DescribeCases(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FStateCase> Cases)
	{
		FString Mismatches;
		for (const FStateCase& Case : Cases)
		{
			Mismatches += DescribeStates(Layout, Case.Cell, Case.Control, Case.States);
		}
		return Mismatches;
	}

	// The ::placeholder part of a text field: a non-DOM child of the field with the tag 'placeholder', holding one text node.
	const FArcRmlTestLayoutElement* FindPlaceholder(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* Field)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Field);
		return Element ? FindChild(Layout, Element->Path, TEXT("placeholder"), TEXT("")) : nullptr;
	}

	// The text the placeholder part lays out: its text node's lines; empty while the field has a value.
	FString GetPlaceholderLines(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* Field)
	{
		const FArcRmlTestLayoutElement* Part = FindPlaceholder(Layout, Field);
		if (!Part)
		{
			return FString(TEXT("<no placeholder part>"));
		}
		FString Text;
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Parent == Part->Path && Element.Kind == TEXT("text"))
			{
				for (const FArcRmlTestTextLine& Line : Element.Lines)
				{
					Text += Text.IsEmpty() ? Line.Text : TEXT(" ") + Line.Text;
				}
			}
		}
		return Text;
	}

	// The first line the placeholder part lays out; Width < 0 when it has none.
	FArcRmlTestTextLine GetPlaceholderFirstLine(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* Field)
	{
		FArcRmlTestTextLine First;
		First.Width = -1.0f;
		const FArcRmlTestLayoutElement* Part = FindPlaceholder(Layout, Field);
		if (!Part)
		{
			return First;
		}
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Parent == Part->Path && Element.Kind == TEXT("text") && !Element.Lines.IsEmpty())
			{
				return Element.Lines[0];
			}
		}
		return First;
	}

	float GetMaxQuadHeight(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		float Height = 0.0f;
		for (const FArcRmlTestDrawQuad& Quad : ArcRmlUITestHelpers::GetGlyphQuads(Draws, Colour))
		{
			Height = FMath::Max(Height, Quad.Height);
		}
		return Height;
	}

	FString DescribePart(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* Field, const TCHAR* Colour)
	{
		const FArcRmlTestLayoutElement* Part = FindPlaceholder(Layout, Field);
		if (!Part)
		{
			return FString::Printf(TEXT("[%s has no placeholder part] "), Field);
		}
		if (Part->Color != Colour)
		{
			return FString::Printf(TEXT("[%s::placeholder colour %s, expected %s] "), Field, *Part->Color, Colour);
		}
		return FString();
	}

	// Distinct premultiplied colours of the textured (glyph) draws, for failure messages.
	FString DescribeGlyphColours(const TArray<FArcRmlTestDraw>& Draws)
	{
		TArray<FString> Colours;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bTextured)
			{
				Colours.AddUnique(FString::Printf(TEXT("%d,%d,%d,%d"), Draw.FirstColourPremultiplied.R, Draw.FirstColourPremultiplied.G,
					Draw.FirstColourPremultiplied.B, Draw.FirstColourPremultiplied.A));
			}
		}
		return FString::Join(Colours, TEXT(" | "));
	}

	int32 CountGlyphDraws(const TArray<FArcRmlTestDraw>& Draws, const FColor& Premultiplied, int32 Tolerance)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			const FColor& Colour = Draw.FirstColourPremultiplied;
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bTextured && FMath::Abs(Colour.R - Premultiplied.R) <= Tolerance
				&& FMath::Abs(Colour.G - Premultiplied.G) <= Tolerance && FMath::Abs(Colour.B - Premultiplied.B) <= Tolerance
				&& FMath::Abs(Colour.A - Premultiplied.A) <= Tolerance)
			{
				++Count;
			}
		}
		return Count;
	}

	FString GetDumpDir(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), Name));
	}

	FModifierKeysState MakeCtrl()
	{
		return FModifierKeysState(false, false, true, false, false, false, false, false, false);
	}

	FModifierKeysState MakeShift()
	{
		return FModifierKeysState(true, false, false, false, false, false, false, false, false);
	}

	class FVerifyLogCapture final : public FOutputDevice
	{
	public:
		explicit FVerifyLogCapture(const TCHAR* InCheck, const TCHAR* InCleanMarker)
			: Check(InCheck)
			, CleanMarker(InCleanMarker)
		{
		}

		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category != FName(TEXT("LogArcRmlUI")) || !FCString::Strstr(Text, Check))
			{
				return;
			}
			FScopeLock Lock(&Mutex);
			if (const TCHAR* Clean = FCString::Strstr(Text, CleanMarker))
			{
				CleanPasses++;
				VerifiedFrames += FCString::Atoi(Clean + FCString::Strlen(CleanMarker));
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

		int32 Num()
		{
			FScopeLock Lock(&Mutex);
			return Lines.Num();
		}

		int32 NumCleanPasses()
		{
			FScopeLock Lock(&Mutex);
			return CleanPasses;
		}

		// Frames counted by the clean summaries; the render verify summary reads "mismatches 0 (<frames> frames, ...)".
		int64 NumVerifiedFrames()
		{
			FScopeLock Lock(&Mutex);
			return VerifiedFrames;
		}

	private:
		const TCHAR* Check;
		const TCHAR* CleanMarker;
		FCriticalSection Mutex;
		TArray<FString> Lines;
		int32 CleanPasses = 0;
		int64 VerifiedFrames = 0;
	};

	// Shared open, click and model helpers of the FormStates tests.
	class FFormStatesPage
	{
	public:
		FArcRmlTestView View;
		TArray<FArcRmlTestLayoutElement> Layout;

		bool Open(bool bDump)
		{
			const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
			FArcRmlTestView::DropCompiledDocument(Path);
			IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
			if (!(View.Open(Path, PageSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
			{
				return false;
			}
			if (bDump)
			{
				View.DumpLayout(GetDumpDir(FPaths::GetBaseFilename(Path)));
			}
			return View.Flush() && View.Flush() && View.ParseLayout(Layout);
		}

		bool Refresh()
		{
			return View.FlushUntilSettled() && View.Flush() && View.ParseLayout(Layout);
		}

		bool SetBool(const TCHAR* Field, bool bValue)
		{
			View.SetModelValue(ArcRmlFormStateTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeBool(bValue));
			return Refresh();
		}

		bool SetString(const TCHAR* Field, const TCHAR* Value)
		{
			View.SetModelValue(ArcRmlFormStateTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeString(Value));
			return Refresh();
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
			return Refresh();
		}

		bool Press(const FKey& Key, const FModifierKeysState& Modifiers = FModifierKeysState())
		{
			View.PressKey(Key, Modifiers);
			return Refresh();
		}

		bool Type(const TCHAR* Text)
		{
			View.TypeText(Text);
			return Refresh();
		}

		bool Ime(EArcRmlInputType Type, const TCHAR* Text, int32 Start, int32 End)
		{
			FArcRmlInputEvent Event;
			Event.Type = Type;
			Event.Text = Text;
			Event.RangeStart = Start;
			Event.RangeEnd = End;
			Event.Generation = ++ImeGeneration;
			View.RawInput(MoveTemp(Event));
			return Refresh();
		}

		FString GetFieldText() const
		{
			return View.GetSnapshot().TextField.bActive ? View.GetSnapshot().TextField.Text : FString(TEXT("<no active text field>"));
		}

	private:
		uint64 ImeGeneration = 0;
	};
}

// :enabled, :disabled, :required, :optional, :read-only and :read-write (rml-forms step 6, VENDORED 227) at load and when their
// attributes change through data-attrif-* and data-attr-type, read from FormStates' indicator colours.
TEST_CLASS(ArcRmlFormStateTests, "ArcRmlUI.Form.States")
{
	ArcRmlFormStateTestsPrivate::FFormStatesPage Page;

	AFTER_EACH()
	{
		Page.View.Close();
	}

	TEST_METHOD(EveryControl_MatchesItsStatesAtLoad)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(true), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		const FString Mismatches = DescribeCases(Page.Layout, LoadCases);
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("each control's indicators and read-only / read-write background follow its disabled, required and readonly attributes")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(ButtonSubmitAndRangeWithRequired_AreOptionalNotRequired)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		const FString Mismatches = DescribeCases(Page.Layout, RequiredIgnoredCases);
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("a button, a submit and a range ignore required: :enabled and :optional, not :required")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(BoundAttributes_SetAndRemoved_MoveTheStatesOfATextInputAndATextArea)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		struct FStep
		{
			const TCHAR* Field;
			bool bValue;
			FStates States;
		};
		const FStep Steps[] = {
			{TEXT("req"), true, EnReqRw},
			{TEXT("ro"), true, EnReqRo},
			{TEXT("off"), true, DisReqRo},
			{TEXT("req"), false, DisOptRo},
			{TEXT("off"), false, EnOptRo},
			{TEXT("ro"), false, EnOptRw},
		};
		FString Mismatches;
		for (const FStep& Step : Steps)
		{
			ASSERT_THAT(IsTrue(Page.SetBool(Step.Field, Step.bValue)));
			const FString StepMismatches = DescribeStates(Page.Layout, TEXT("fs-cell-bound"), TEXT("fs-bound"), Step.States)
				+ DescribeStates(Page.Layout, TEXT("fs-cell-ta-bound"), TEXT("fs-ta-bound"), Step.States);
			if (!StepMismatches.IsEmpty())
			{
				Mismatches += FString::Printf(TEXT("{%s = %s: %s} "), Step.Field, Step.bValue ? TEXT("true") : TEXT("false"), *StepMismatches);
			}
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("each data-attrif-required / readonly / disabled set and removal moves the states at once")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(TypeChange_FromTextToRangeWithRequired_DropsRequiredAndReadWrite)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		ASSERT_THAT(IsTrue(DescribeStates(Page.Layout, TEXT("fs-cell-kind"), TEXT("fs-kind"), EnReqRw).IsEmpty(), TEXT("a required text input is :required and :read-write")));
		ASSERT_THAT(IsTrue(Page.SetString(TEXT("kind"), TEXT("range"))));
		// :read-only off on the range is the fork's rule; Chrome matches it on every non-editable control.
		const FString Range = DescribeStates(Page.Layout, TEXT("fs-cell-kind"), TEXT("fs-kind"), EnOpt);
		ASSERT_THAT(IsTrue(Range.IsEmpty(), FString(TEXT("as a range the required input is :optional and neither :read-only nor :read-write")) + TEXT(": ") + Range));
		ASSERT_THAT(IsTrue(Page.SetString(TEXT("kind"), TEXT("text"))));
		const FString Text = DescribeStates(Page.Layout, TEXT("fs-cell-kind"), TEXT("fs-kind"), EnReqRw);
		ASSERT_THAT(IsTrue(Text.IsEmpty(), FString(TEXT("back to text it is :required and :read-write again")) + TEXT(": ") + Text));
	}

	TEST_METHOD(TailwindRequiredVariant_AppliesOnlyToTheRequiredField)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		const FArcRmlTestLayoutElement* Required = ArcRmlUITestHelpers::FindByDomId(Page.Layout, TEXT("fs-tw-req"));
		const FArcRmlTestLayoutElement* Optional = ArcRmlUITestHelpers::FindByDomId(Page.Layout, TEXT("fs-tw-opt"));
		ASSERT_THAT(IsTrue(Required && Optional));
		ASSERT_THAT(AreEqual(FString(TailwindRequiredBlue), Required->Color, TEXT(".required\\:text-blue-600:required colours the required input")));
		ASSERT_THAT(IsTrue(Optional->Color != TailwindRequiredBlue, FString::Printf(TEXT("the input without required keeps its colour, got %s"), *Optional->Color)));
	}
};

// readonly (rml-forms step 6, VENDORED 227): every edit of a readonly text input or textarea is ignored, while the same edit changes a
// read-write field of the same page; selection, Ctrl+C and focus stay.
TEST_CLASS(ArcRmlFormReadOnlyTests, "ArcRmlUI.Form.ReadOnly")
{
	ArcRmlFormStateTestsPrivate::FFormStatesPage Page;
	FString SavedClipboard;

	BEFORE_EACH()
	{
		FPlatformApplicationMisc::ClipboardPaste(SavedClipboard);
	}

	AFTER_EACH()
	{
		Page.View.Close();
		FPlatformApplicationMisc::ClipboardCopy(*SavedClipboard);
	}

	struct FEdit
	{
		const TCHAR* Name;
		const TCHAR* ReadWriteResult;
	};

	bool ApplyEdit(int32 Index)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		switch (Index)
		{
		case 0:
			return Page.Type(TEXT("x"));
		case 1:
			return Page.Press(EKeys::BackSpace);
		case 2:
			return Page.Press(EKeys::Home) && Page.Press(EKeys::Delete);
		case 3:
			return Page.Press(EKeys::A, MakeCtrl()) && Page.Press(EKeys::X, MakeCtrl());
		case 4:
			FPlatformApplicationMisc::ClipboardCopy(TEXT("zz"));
			return Page.Press(EKeys::V, MakeCtrl());
		case 5:
			return Page.Ime(EArcRmlInputType::ImeSetText, TEXT("zz"), 0, 2);
		default:
			return false;
		}
	}

	// Opens the page, focuses Field with its caret at the end and applies one edit; returns the field's value after it.
	FString EditField(const TCHAR* Field, int32 Index)
	{
		Page.View.Close();
		if (!Page.Open(false) || !Page.Click(Field) || !Page.Press(EKeys::End))
		{
			return FString(TEXT("<open or focus failed>"));
		}
		const FString Focus = ArcRmlUITestHelpers::GetFocusDomId(Page.View);
		if (Focus != Field)
		{
			return FString::Printf(TEXT("<focus is '%s'>"), *Focus);
		}
		if (!ApplyEdit(Index))
		{
			return FString(TEXT("<edit not flushed>"));
		}
		const FString Text = Page.GetFieldText();
		Page.View.Close();
		return Text;
	}

	FString DescribeEdits(const TCHAR* ReadWriteField, const TCHAR* ReadOnlyField)
	{
		const FEdit Edits[] = {
			{TEXT("textinput 'x'"), TEXT("abx")},
			{TEXT("Backspace"), TEXT("a")},
			{TEXT("Home, Delete"), TEXT("b")},
			{TEXT("Ctrl+A, Ctrl+X"), TEXT("")},
			{TEXT("Ctrl+V 'zz'"), TEXT("abzz")},
			{TEXT("IME SetText 'zz' over 0..2"), TEXT("zz")},
		};
		FString Mismatches;
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Edits)); ++Index)
		{
			const FEdit& Edit = Edits[Index];
			const FString ReadWrite = EditField(ReadWriteField, Index);
			if (ReadWrite != Edit.ReadWriteResult)
			{
				Mismatches += FString::Printf(TEXT("[%s on read-write %s gave '%s', expected '%s': the edit does not reach the field] "), Edit.Name, ReadWriteField,
					*ReadWrite.ReplaceCharWithEscapedChar(), *FString(Edit.ReadWriteResult).ReplaceCharWithEscapedChar());
			}
			const FString ReadOnly = EditField(ReadOnlyField, Index);
			if (ReadOnly != TEXT("ab"))
			{
				Mismatches += FString::Printf(TEXT("[%s on readonly %s gave '%s', expected 'ab'] "), Edit.Name, ReadOnlyField, *ReadOnly.ReplaceCharWithEscapedChar());
			}
		}
		return Mismatches;
	}

	TEST_METHOD(ReadOnlyTextInput_IgnoresTypingDeleteCutPasteAndIme)
	{
		const FString Mismatches = DescribeEdits(TEXT("fs-text"), TEXT("fs-text-ro"));
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("a readonly text input keeps 'ab' through every edit that changes the read-write one")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(ReadOnlyTextArea_IgnoresTypingDeleteCutPasteAndIme)
	{
		const FString Mismatches = DescribeEdits(TEXT("fs-ta"), TEXT("fs-ta-ro"));
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("a readonly textarea keeps 'ab' through every edit that changes the read-write one")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(ReadOnlyTextInput_KeepsFocusSelectionAndCopy)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		ASSERT_THAT(IsTrue(Page.Click(TEXT("fs-text-ro"))));
		ASSERT_THAT(AreEqual(FString(TEXT("fs-text-ro")), ArcRmlUITestHelpers::GetFocusDomId(Page.View), TEXT("a click focuses the readonly input")));
		ASSERT_THAT(IsTrue(Page.View.GetSnapshot().TextField.bActive, TEXT("the focused readonly input is the active text field")));
		ASSERT_THAT(IsTrue(Page.Press(EKeys::A, MakeCtrl())));
		const FArcRmlTextFieldShadow& All = Page.View.GetSnapshot().TextField;
		ASSERT_THAT(IsTrue(All.SelectionStart == 0 && All.SelectionEnd == 2, FString::Printf(TEXT("Ctrl+A selects 0..2, got %d..%d"), All.SelectionStart, All.SelectionEnd)));
		const int32 CopiesBefore = Page.View.GetClipboardCopies().Num();
		ASSERT_THAT(IsTrue(Page.Press(EKeys::C, MakeCtrl())));
		const TArray<FString>& Copies = Page.View.GetClipboardCopies();
		ASSERT_THAT(IsTrue(Copies.Num() == CopiesBefore + 1 && Copies.Last() == TEXT("ab"),
			FString::Printf(TEXT("Ctrl+C copies 'ab': %d copies, last '%s'"), Copies.Num() - CopiesBefore, Copies.IsEmpty() ? TEXT("") : *Copies.Last())));
		const FArcRmlTextFieldShadow& Copied = Page.View.GetSnapshot().TextField;
		ASSERT_THAT(IsTrue(Copied.SelectionStart == 0 && Copied.SelectionEnd == 2 && Copied.Text == TEXT("ab"),
			FString::Printf(TEXT("Ctrl+C keeps the selection and value: %d..%d '%s'"), Copied.SelectionStart, Copied.SelectionEnd, *Copied.Text)));
		ASSERT_THAT(IsTrue(Page.Press(EKeys::End) && Page.Press(EKeys::Left, MakeShift())));
		const FArcRmlTextFieldShadow& Shifted = Page.View.GetSnapshot().TextField;
		ASSERT_THAT(IsTrue(FMath::Min(Shifted.SelectionStart, Shifted.SelectionEnd) == 1 && FMath::Max(Shifted.SelectionStart, Shifted.SelectionEnd) == 2,
			FString::Printf(TEXT("End, Shift+Left selects 1..2, got %d..%d"), Shifted.SelectionStart, Shifted.SelectionEnd)));
		ASSERT_THAT(AreEqual(FString(TEXT("fs-text-ro")), ArcRmlUITestHelpers::GetFocusDomId(Page.View), TEXT("the readonly input keeps focus through the keys")));
	}
};

// The ::placeholder part (rml-forms step 6, VENDORED 227): author ::placeholder, :focus::placeholder and Tailwind placeholder: rules
// reach the part's colour, opacity and font; tag rules do not; the part shows only while the value is empty.
TEST_CLASS(ArcRmlFormPlaceholderTests, "ArcRmlUI.Form.Placeholder")
{
	ArcRmlFormStateTestsPrivate::FFormStatesPage Page;

	AFTER_EACH()
	{
		Page.View.Close();
	}

	TEST_METHOD(PlaceholderRules_ReachThePartsColourAndFont)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		FString Mismatches;
		Mismatches += DescribePart(Page.Layout, TEXT("fs-ph"), PlaceholderRed);
		Mismatches += DescribePart(Page.Layout, TEXT("fs-ph-ta"), PlaceholderRed);
		Mismatches += DescribePart(Page.Layout, TEXT("fs-ph-font"), PlaceholderFontGreen);
		Mismatches += DescribePart(Page.Layout, TEXT("fs-ph-tw"), TailwindPlaceholderRed);
		// With no author rule the part takes the user-agent ::placeholder grey rgb(117, 117, 117), as Chrome (VENDORED 231); the red and
		// green parts above show that rule sits below every author ::placeholder rule.
		const FArcRmlTestLayoutElement* Plain = ArcRmlUITestHelpers::FindByDomId(Page.Layout, TEXT("fs-ph-plain"));
		const FArcRmlTestLayoutElement* PlainPart = FindPlaceholder(Page.Layout, TEXT("fs-ph-plain"));
		if (!Plain || !PlainPart || PlainPart->Color != UserAgentPlaceholderGrey || Plain->Color == UserAgentPlaceholderGrey)
		{
			Mismatches += FString::Printf(TEXT("[fs-ph-plain::placeholder %s in a %s field, expected the user-agent %s] "),
				PlainPart ? *PlainPart->Color : TEXT("<none>"), Plain ? *Plain->Color : TEXT("<none>"), UserAgentPlaceholderGrey);
		}
		const FArcRmlTestLayoutElement* Font = ArcRmlUITestHelpers::FindByDomId(Page.Layout, TEXT("fs-ph-font"));
		const FArcRmlTestLayoutElement* FontPart = FindPlaceholder(Page.Layout, TEXT("fs-ph-font"));
		if (!Font || !FontPart || FMath::Abs(FontPart->FontSize - 20.0f) > 0.01f || FMath::Abs(Font->FontSize - 20.0f) < 0.01f)
		{
			Mismatches += FString::Printf(TEXT("[fs-ph-font::placeholder font-size %.3f in a %.3f field, expected the rule's 20 in the user-agent 13.333] "),
				FontPart ? FontPart->FontSize : -1.0f, Font ? Font->FontSize : -1.0f);
		}
		for (const TCHAR* Field : {TEXT("fs-ph"), TEXT("fs-ph-plain"), TEXT("fs-ph-font")})
		{
			const FString Lines = GetPlaceholderLines(Page.Layout, Field);
			if (Lines != PlaceholderText)
			{
				Mismatches += FString::Printf(TEXT("[%s::placeholder lays out '%s', expected '%s'] "), Field, *Lines, PlaceholderText);
			}
		}
		const TArray<FArcRmlTestDrawQuad> FontQuads = ArcRmlUITestHelpers::GetGlyphQuads(Page.View.GetDraws(), FColor(0, 120, 0, 255));
		TArray<FString> QuadWidths;
		int32 FontGlyphs = 0;
		for (const FArcRmlTestDrawQuad& Quad : FontQuads)
		{
			QuadWidths.Add(FString::Printf(TEXT("%.2fx%.2f"), Quad.Width, Quad.Height));
			FontGlyphs += Quad.Width > 0.0f ? 1 : 0;
		}
		TestRunner->AddInfo(FString::Printf(TEXT("fs-ph-font::placeholder green quads: %s"), *FString::Join(QuadWidths, TEXT(" "))));
		// The space of "Your name" adds an empty quad (Rml FontFaceLayer), so only quads with a width are glyphs.
		if (FontGlyphs != 8)
		{
			Mismatches += FString::Printf(TEXT("[fs-ph-font::placeholder draws %d green glyphs, expected the 8 of 'Your name'; quads %s; glyph colours %s] "), FontGlyphs,
				*FString::Join(QuadWidths, TEXT(" ")), *DescribeGlyphColours(Page.View.GetDraws()));
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("::placeholder rules set the part's colour and font; the 'placeholder' tag rule does not reach it")) + TEXT(": ") + Mismatches));
	}

	// .ph::placeholder is rgb(200, 0, 0) at opacity 0.5: the glyphs draw at half alpha, premultiplied; no glyph draws the opaque red.
	TEST_METHOD(PlaceholderOpacity_DrawsTheGlyphsAtHalfAlpha)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		const TArray<FArcRmlTestDraw>& Draws = Page.View.GetDraws();
		const int32 Half = CountGlyphDraws(Draws, FColor(100, 0, 0, 128), 1);
		const int32 Opaque = CountGlyphDraws(Draws, FColor(200, 0, 0, 255), 1);
		ASSERT_THAT(IsTrue(Half > 0 && Opaque == 0, FString::Printf(TEXT("the .ph placeholder glyphs draw red at half alpha: %d half, %d opaque draws; glyph colours %s"), Half,
			Opaque, *DescribeGlyphColours(Draws))));
	}

	// .ph-font:focus::placeholder changes only the part's font-size (20 to 24 px): the part must lay its text out again at the new size,
	// so the line widens and the drawn glyphs grow.
	TEST_METHOD(FocusFontRule_LaysThePlaceholderOutAgainAtTheNewSize)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		const FColor Green(0, 120, 0, 255);
		const FArcRmlTestTextLine Before = GetPlaceholderFirstLine(Page.Layout, TEXT("fs-ph-font"));
		const float HeightBefore = GetMaxQuadHeight(Page.View.GetDraws(), Green);
		ASSERT_THAT(IsTrue(Before.Width > 0.0f && HeightBefore > 0.0f, FString::Printf(TEXT("the 20 px placeholder is laid out and drawn: width %.2f, glyph height %.2f"),
			Before.Width, HeightBefore)));
		ASSERT_THAT(IsTrue(Page.Click(TEXT("fs-ph-font"))));
		ASSERT_THAT(AreEqual(FString(TEXT("fs-ph-font")), ArcRmlUITestHelpers::GetFocusDomId(Page.View)));
		const FArcRmlTestTextLine After = GetPlaceholderFirstLine(Page.Layout, TEXT("fs-ph-font"));
		const float HeightAfter = GetMaxQuadHeight(Page.View.GetDraws(), Green);
		ASSERT_THAT(IsTrue(After.Width > Before.Width * 1.1f && HeightAfter > HeightBefore,
			FString::Printf(TEXT("at 24 px the line widens from %.2f (expected about %.2f) and the glyphs grow from %.2f: width %.2f, glyph height %.2f"),
				Before.Width, Before.Width * 1.2f, HeightBefore, After.Width, HeightAfter)));
	}

	// fs-ph-center has text-align: center, inherited by the part: its line sits in the middle of the width the placeholder is laid out
	// in, the content box less the 1 px caret (FormatPlaceholder).
	TEST_METHOD(CentredField_CentresThePlaceholderLine)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		const FArcRmlTestLayoutElement* Field = ArcRmlUITestHelpers::FindByDomId(Page.Layout, TEXT("fs-ph-center"));
		ASSERT_THAT(IsNotNull(Field));
		const FArcRmlTestTextLine Line = GetPlaceholderFirstLine(Page.Layout, TEXT("fs-ph-center"));
		ASSERT_THAT(IsTrue(Line.Width > 0.0f && Line.Text == TEXT("Mid"), FString::Printf(TEXT("the placeholder lays out 'Mid', got '%s' %.2f wide"), *Line.Text, Line.Width)));
		const float ContentLeft = Field->Rect.Position.X + (Field->Rect.Size.X - Field->ContentSize.X) * 0.5f;
		const float Expected = ContentLeft + (Field->ContentSize.X - 1.0f - Line.Width) * 0.5f;
		ASSERT_THAT(IsTrue(FMath::Abs(Line.X - Expected) <= 1.0f && Line.X - ContentLeft > 5.0f,
			FString::Printf(TEXT("'Mid' starts at %.2f, centred %.2f (content left %.2f, width %.2f, line %.2f)"), Line.X, Expected, ContentLeft, Field->ContentSize.X,
				Line.Width)));
	}

	TEST_METHOD(FocusPlaceholderRule_AppliesWhileTheFieldIsFocused)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		ASSERT_THAT(IsTrue(Page.Click(TEXT("fs-ph"))));
		ASSERT_THAT(AreEqual(FString(TEXT("fs-ph")), ArcRmlUITestHelpers::GetFocusDomId(Page.View)));
		FString Mismatches = DescribePart(Page.Layout, TEXT("fs-ph"), PlaceholderFocusBlue) + DescribePart(Page.Layout, TEXT("fs-ph-ta"), PlaceholderRed);
		const int32 FocusedHalf = CountGlyphDraws(Page.View.GetDraws(), FColor(0, 0, 100, 128), 1);
		if (FocusedHalf == 0)
		{
			Mismatches += FString::Printf(TEXT("[no blue half-alpha placeholder glyphs while focused; glyph colours %s] "), *DescribeGlyphColours(Page.View.GetDraws()));
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT(".ph:focus::placeholder colours only the focused field's placeholder and keeps its opacity")) + TEXT(": ") + Mismatches));
		ASSERT_THAT(IsTrue(Page.Press(EKeys::Tab)));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::GetFocusDomId(Page.View) != TEXT("fs-ph"), TEXT("Tab moves focus off fs-ph")));
		const FString Blurred = DescribePart(Page.Layout, TEXT("fs-ph"), PlaceholderRed);
		ASSERT_THAT(IsTrue(Blurred.IsEmpty(), FString(TEXT("after the blur the placeholder takes .ph::placeholder again")) + TEXT(": ") + Blurred));
	}

	TEST_METHOD(TypingHidesThePlaceholderAndClearingShowsIt)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		ASSERT_THAT(AreEqual(FString(), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-value")), TEXT("a field with a value shows no placeholder")));
		ASSERT_THAT(IsTrue(Page.Click(TEXT("fs-ph")) && Page.Type(TEXT("x"))));
		ASSERT_THAT(AreEqual(FString(TEXT("x")), Page.GetFieldText()));
		ASSERT_THAT(AreEqual(FString(), GetPlaceholderLines(Page.Layout, TEXT("fs-ph")), TEXT("typing hides the placeholder")));
		ASSERT_THAT(IsTrue(Page.Press(EKeys::BackSpace)));
		ASSERT_THAT(AreEqual(FString(), Page.GetFieldText()));
		ASSERT_THAT(AreEqual(FString(PlaceholderText), GetPlaceholderLines(Page.Layout, TEXT("fs-ph")), TEXT("clearing the value shows the placeholder again")));
	}

	TEST_METHOD(BoundValueHidesThePlaceholderAndClearingShowsIt)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		ASSERT_THAT(AreEqual(FString(PlaceholderText), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-bound")), TEXT("the empty bound field shows its placeholder")));
		ASSERT_THAT(IsTrue(Page.SetString(TEXT("name"), TEXT("ab"))));
		ASSERT_THAT(AreEqual(FString(), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-bound")), TEXT("a bound value hides the placeholder")));
		ASSERT_THAT(IsTrue(Page.SetString(TEXT("name"), TEXT(""))));
		ASSERT_THAT(AreEqual(FString(PlaceholderText), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-bound")), TEXT("an empty bound value shows the placeholder again")));
	}
};

// Draw cost of frames that replay FormStates (rml-forms step 6): the first draw, the :focus::placeholder restyle and the typed value
// that hides the placeholder. The numbers are written to the test log and recorded in VENDORED 227.
TEST_CLASS(ArcRmlFormStateDrawTests, "ArcRmlUI.Pixels.FormStateDraws")
{
	ArcRmlFormStateTestsPrivate::FFormStatesPage Page;

	AFTER_EACH()
	{
		Page.View.Close();
	}

	bool RecordNewFrame(TFunctionRef<void()> Change)
	{
		const uint64 Before = Page.View.GetProbe().FrameSerial;
		Change();
		for (int32 Attempt = 0; Attempt < 50 && Page.View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!Page.View.FlushUntilSettled())
			{
				return false;
			}
		}
		return Page.View.GetProbe().FrameSerial > Before;
	}

	bool DrawContent(const TCHAR* Step, int32 GeometryDrawCalls, int32 GeometryRhiCommands)
	{
		if (!Page.View.DrawFrame())
		{
			TestRunner->AddError(FString::Printf(TEXT("FormStates %s: the frame was not drawn"), Step));
			return false;
		}
		TestRunner->AddInfo(FString::Printf(TEXT("FormStateDraws %s: %d draw calls, %d RHI commands (geometry %d draw calls, %d RHI commands)"), Step,
			Page.View.GetDrawGpuDrawCalls(), Page.View.GetDrawGpuRhiCommands(), Page.View.GetReplayDrawCalls(), Page.View.GetReplayRhiCommands()));
		if (Page.View.GetReplayDrawCalls() != GeometryDrawCalls || Page.View.GetReplayRhiCommands() != GeometryRhiCommands)
		{
			TestRunner->AddError(FString::Printf(TEXT("FormStates %s: the frame's geometry took %d draw calls and %d RHI commands, expected %d and %d"), Step,
				Page.View.GetReplayDrawCalls(), Page.View.GetReplayRhiCommands(), GeometryDrawCalls, GeometryRhiCommands));
		}
		return true;
	}

	TEST_METHOD(FirstDrawFocusRestyleAndTyping)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		Page.View.SetDestination(FIntRect(FIntPoint::ZeroValue, ArcRmlFormStateTestsPrivate::PageSize));
		ASSERT_THAT(IsTrue(DrawContent(TEXT("first draw"), 1, 8)));
		const FVector2f Field = Page.GetCentre(TEXT("fs-ph"));
		ASSERT_THAT(IsTrue(RecordNewFrame([this, Field]() { Page.View.MouseClick(Field); }), TEXT("the focus click recorded no frame")));
		ASSERT_THAT(IsTrue(Page.Refresh()));
		ASSERT_THAT(AreEqual(FString(TEXT("fs-ph")), ArcRmlUITestHelpers::GetFocusDomId(Page.View), TEXT("the click focuses fs-ph before the restyle frame is pinned")));
		ASSERT_THAT(IsTrue(DrawContent(TEXT("focus restyle"), 1, 8)));
		ASSERT_THAT(IsTrue(RecordNewFrame([this]() { Page.View.TypeText(TEXT("x")); }), TEXT("typing recorded no frame")));
		ASSERT_THAT(IsTrue(Page.Refresh()));
		ASSERT_THAT(AreEqual(FString(TEXT("x")), Page.GetFieldText(), TEXT("fs-ph holds the typed 'x' before the typed frame is pinned")));
		ASSERT_THAT(IsTrue(DrawContent(TEXT("typed value"), 1, 8)));
	}
};

// ArcRml.LayoutVerify 1 over the placeholder's show and hide (typing, clearing, binding) and the :focus::placeholder restyle.
TEST_CLASS(ArcRmlFormStateLayoutVerifyTests, "ArcRmlUI.LayoutVerify.FormStates")
{
	ArcRmlFormStateTestsPrivate::FFormStatesPage Page;
	ArcRmlFormStateTestsPrivate::FVerifyLogCapture Capture{TEXT("layout verify"), TEXT("mismatches 0,")};
	int32 SavedVerify = 0;
	bool bCaptureAttached = false;

	BEFORE_EACH()
	{
		IConsoleVariable* Verify = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify"));
		ASSERT_THAT(IsNotNull(Verify));
		SavedVerify = Verify->GetInt();
		Verify->Set(1, ECVF_SetByCode);
		GLog->AddOutputDevice(&Capture);
		bCaptureAttached = true;
	}

	AFTER_EACH()
	{
		Page.View.Close();
		if (bCaptureAttached)
		{
			GLog->RemoveOutputDevice(&Capture);
			bCaptureAttached = false;
		}
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify"))->Set(SavedVerify, ECVF_SetByCode);
	}

	bool ExpectClean(const TCHAR* Moment, int32& CleanBefore)
	{
		GLog->Flush();
		const bool bClean = Assert.AreEqual(0, Capture.Num(), FString::Printf(TEXT("%s: layout verify reported %s"), Moment, *Capture.Describe()))
			&& Assert.IsTrue(Capture.NumCleanPasses() > CleanBefore, FString::Printf(TEXT("%s: no layout verify pass ran"), Moment));
		CleanBefore = Capture.NumCleanPasses();
		return bClean;
	}

	TEST_METHOD(PlaceholderShowHideAndFocusRestyle_VerifyWithNoMismatch)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		GLog->Flush();
		int32 Clean = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(Page.Click(TEXT("fs-ph-font"))));
		const FArcRmlTestLayoutElement* FocusedPart = FindPlaceholder(Page.Layout, TEXT("fs-ph-font"));
		ASSERT_THAT(IsTrue(FocusedPart && FMath::Abs(FocusedPart->FontSize - 24.0f) < 0.01f,
			FString::Printf(TEXT(".ph-font:focus::placeholder sets the part's font-size 24, got %.3f"), FocusedPart ? FocusedPart->FontSize : -1.0f)));
		ASSERT_THAT(AreEqual(FString(PlaceholderText), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-font")), TEXT("the focused 24 px placeholder lays out its text")));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT(":focus::placeholder font-size restyle"), Clean)));
		ASSERT_THAT(IsTrue(Page.Type(TEXT("x"))));
		ASSERT_THAT(AreEqual(FString(), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-font"))));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT("typed, placeholder hidden"), Clean)));
		ASSERT_THAT(IsTrue(Page.Press(EKeys::BackSpace)));
		ASSERT_THAT(AreEqual(FString(PlaceholderText), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-font"))));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT("cleared, placeholder shown"), Clean)));
		ASSERT_THAT(IsTrue(Page.SetString(TEXT("name"), TEXT("ab"))));
		ASSERT_THAT(AreEqual(FString(), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-bound"))));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT("bound value, placeholder hidden"), Clean)));
		ASSERT_THAT(IsTrue(Page.SetString(TEXT("name"), TEXT(""))));
		ASSERT_THAT(AreEqual(FString(PlaceholderText), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-bound"))));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT("bound value cleared, placeholder shown"), Clean)));
		ASSERT_THAT(IsTrue(Page.Click(TEXT("fs-ph"))));
		ASSERT_THAT(IsTrue(DescribePart(Page.Layout, TEXT("fs-ph"), PlaceholderFocusBlue).IsEmpty()));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT(":focus::placeholder restyle"), Clean)));
	}
};

// ArcRml.RenderVerify 1 over the same placeholder show, hide and :focus::placeholder restyle: the retained frame equals a full
// re-record of every element.
TEST_CLASS(ArcRmlFormStateRenderVerifyTests, "ArcRmlUI.RenderVerify.FormStates")
{
	ArcRmlFormStateTestsPrivate::FFormStatesPage Page;
	ArcRmlFormStateTestsPrivate::FVerifyLogCapture Capture{TEXT("render verify"), TEXT("mismatches 0 (")};
	int32 SavedVerify = 0;
	int32 SavedPerfLog = 0;
	float SavedPerfLogInterval = 0.0f;
	int64 FramesMark = 0;
	bool bCaptureAttached = false;

	BEFORE_EACH()
	{
		IConsoleVariable* Verify = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.RenderVerify"));
		IConsoleVariable* PerfLog = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PerfLog"));
		ASSERT_THAT(IsNotNull(Verify));
		ASSERT_THAT(IsNotNull(PerfLog));
		SavedVerify = Verify->GetInt();
		IConsoleVariable* PerfLogInterval = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PerfLogInterval"));
		ASSERT_THAT(IsNotNull(PerfLogInterval));
		SavedPerfLog = PerfLog->GetInt();
		SavedPerfLogInterval = PerfLogInterval->GetFloat();
		Verify->Set(1, ECVF_SetByCode);
		PerfLog->Set(0, ECVF_SetByCode);
		// The view's render verify summary (frames verified, mismatches) is written once per interval, whatever ArcRml.PerfLog is.
		PerfLogInterval->Set(0.1f, ECVF_SetByCode);
		GLog->AddOutputDevice(&Capture);
		bCaptureAttached = true;
	}

	AFTER_EACH()
	{
		Page.View.Close();
		if (bCaptureAttached)
		{
			GLog->RemoveOutputDevice(&Capture);
			bCaptureAttached = false;
		}
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.RenderVerify"))->Set(SavedVerify, ECVF_SetByCode);
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PerfLog"))->Set(SavedPerfLog, ECVF_SetByCode);
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PerfLogInterval"))->Set(SavedPerfLogInterval, ECVF_SetByCode);
	}

	// Waits for a render verify summary that counts frames verified since the last mark: a step whose frames ran without the verify
	// (the cvar not applied) never gets one.
	bool WaitForVerifiedFrames()
	{
		const double Deadline = FPlatformTime::Seconds() + 5.0;
		GLog->Flush();
		while (Capture.NumVerifiedFrames() <= FramesMark && FPlatformTime::Seconds() < Deadline)
		{
			FPlatformProcess::Sleep(0.05f);
			Page.View.Flush();
			GLog->Flush();
		}
		return Capture.NumVerifiedFrames() > FramesMark;
	}

	bool ExpectClean(const TCHAR* Moment)
	{
		const bool bVerified = WaitForVerifiedFrames();
		const int64 Frames = Capture.NumVerifiedFrames() - FramesMark;
		FramesMark = Capture.NumVerifiedFrames();
		const int64 Rerecorded = Page.View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRerecorded);
		const bool bClean = Assert.AreEqual(0, Capture.Num(), FString::Printf(TEXT("%s: render verify reported %s"), Moment, *Capture.Describe()))
			&& Assert.IsTrue(Rerecorded >= 1, FString::Printf(TEXT("%s: no element was re-recorded, so the verify compared nothing changed"), Moment))
			&& Assert.IsTrue(bVerified, FString::Printf(TEXT("%s: no render verify summary counted a frame (%lld frames)"), Moment, Frames));
		Page.View.ResetPerfCounters();
		return bClean;
	}

	TEST_METHOD(PlaceholderShowHideAndFocusRestyle_VerifyWithNoMismatch)
	{
		using namespace ArcRmlFormStateTestsPrivate;
		ASSERT_THAT(IsTrue(Page.Open(false) && Page.View.ResetPerfCounters(), ArcRmlUITestHelpers::DescribeDiagnostics(Page.View)));
		ASSERT_THAT(IsTrue(WaitForVerifiedFrames(), TEXT("the open's frames ran no render verify")));
		FramesMark = Capture.NumVerifiedFrames();
		ASSERT_THAT(IsTrue(Page.Click(TEXT("fs-ph"))));
		ASSERT_THAT(IsTrue(DescribePart(Page.Layout, TEXT("fs-ph"), PlaceholderFocusBlue).IsEmpty()));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT(":focus::placeholder restyle"))));
		ASSERT_THAT(IsTrue(Page.Type(TEXT("x"))));
		ASSERT_THAT(AreEqual(FString(), GetPlaceholderLines(Page.Layout, TEXT("fs-ph"))));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT("typed, placeholder hidden"))));
		ASSERT_THAT(IsTrue(Page.Press(EKeys::BackSpace)));
		ASSERT_THAT(AreEqual(FString(PlaceholderText), GetPlaceholderLines(Page.Layout, TEXT("fs-ph"))));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT("cleared, placeholder shown"))));
		ASSERT_THAT(IsTrue(Page.SetString(TEXT("name"), TEXT("ab"))));
		ASSERT_THAT(AreEqual(FString(), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-bound"))));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT("bound value, placeholder hidden"))));
		ASSERT_THAT(IsTrue(Page.SetString(TEXT("name"), TEXT(""))));
		ASSERT_THAT(AreEqual(FString(PlaceholderText), GetPlaceholderLines(Page.Layout, TEXT("fs-ph-bound"))));
		ASSERT_THAT(IsTrue(ExpectClean(TEXT("bound value cleared, placeholder shown"))));
	}
};
