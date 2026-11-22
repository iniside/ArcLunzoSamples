// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"

namespace ArcRmlFormEventTestsPrivate
{
	const TCHAR* const Fixture = TEXT("Form/FormEvents.rml");
	const TCHAR* const Sheet = TEXT("FormEvents.rcssc");
	const TCHAR* const ModelName = TEXT("fe");

	FModifierKeysState MakeCtrl()
	{
		return FModifierKeysState(false, false, true, false, false, false, false, false, false);
	}

	FString FormatArgument(const FArcRmlValue& Value)
	{
		switch (Value.Kind)
		{
		case EArcRmlValueKind::String:
			return FString::Printf(TEXT("'%s'"), *Value.String.ReplaceCharWithEscapedChar());
		case EArcRmlValueKind::Bool:
			return Value.bBool ? TEXT("true") : TEXT("false");
		case EArcRmlValueKind::Int:
			return FString::Printf(TEXT("%lld"), Value.Int);
		case EArcRmlValueKind::Float:
			return FString::Printf(TEXT("%g"), Value.Float);
		case EArcRmlValueKind::Empty:
			return TEXT("<empty>");
		default:
			return FString::Printf(TEXT("<kind %d>"), static_cast<int32>(Value.Kind));
		}
	}

	// FormEvents.rml: every control calls In / Ch (fe-text's row Up) with its tag first, then ev.value and, on checkboxes and radios,
	// ev.checked; the textarea's In also passes ev.linebreak. One call reads "In 'abx'" or "Ch 'yes' true". fe-text's Bl is not listed.
	FString DescribeEvents(const FArcRmlTestView& View, const TCHAR* Tag)
	{
		TArray<FString> Parts;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			const bool bFormEvent = Call.FunctionName == TEXT("In") || Call.FunctionName == TEXT("Ch") || Call.FunctionName == TEXT("Up");
			if (!bFormEvent || Call.Arguments.Num() == 0 || Call.Arguments[0].String != Tag)
			{
				continue;
			}
			FString Part = Call.FunctionName;
			for (int32 Index = 1; Index < Call.Arguments.Num(); ++Index)
			{
				Part += TEXT(" ") + FormatArgument(Call.Arguments[Index]);
			}
			Parts.Add(Part);
		}
		return FString::Join(Parts, TEXT(", "));
	}

	// The names of every call with the given tag, fe-text's Bl included, in dispatch order: "In, Ch, Up, Bl".
	FString DescribeCallOrder(const FArcRmlTestView& View, const TCHAR* Tag)
	{
		TArray<FString> Names;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			if (Call.Arguments.Num() > 0 && Call.Arguments[0].String == Tag)
			{
				Names.Add(Call.FunctionName);
			}
		}
		return FString::Join(Names, TEXT(", "));
	}
}

// Form-control input and change events (rml-forms step 7, VENDORED 228; the plan's event case table, owner decisions 3 and 4), driven
// through the view's input path: pointer clicks, keys, typed text, the host clipboard, IME SetText and the pad accept sequence.
TEST_CLASS(ArcRmlFormEventTests, "ArcRmlUI.Form.Events")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;
	FString Mismatches;
	FString SavedClipboard;
	uint64 ImeGeneration = 0;

	BEFORE_EACH()
	{
		FPlatformApplicationMisc::ClipboardPaste(SavedClipboard);
	}

	AFTER_EACH()
	{
		View.Close();
		FPlatformApplicationMisc::ClipboardCopy(*SavedClipboard);
	}

	bool OpenFixture()
	{
		using namespace ArcRmlFormEventTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		ImeGeneration = 0;
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && Settle();
	}

	bool Settle()
	{
		return View.FlushUntilSettled() && View.Flush() && View.ParseLayout(Layout);
	}

	FVector2f Centre(const TCHAR* DomId)
	{
		View.ParseLayout(Layout);
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		return Element ? Element->Rect.Position + Element->Rect.Size * 0.5f : FVector2f(-1.0f, -1.0f);
	}

	bool ClickAt(const FVector2f& Point)
	{
		View.MouseMove(Point);
		View.MouseClick(Point);
		return Settle();
	}

	bool Click(const TCHAR* DomId)
	{
		return ClickAt(Centre(DomId));
	}

	bool Press(const FKey& Key, const FModifierKeysState& Modifiers = FModifierKeysState())
	{
		View.PressKey(Key, Modifiers);
		return Settle();
	}

	bool Type(const TCHAR* Text)
	{
		View.TypeText(Text);
		return Settle();
	}

	bool ImeSetText(const TCHAR* Text, int32 Start, int32 End)
	{
		FArcRmlInputEvent Event;
		Event.Type = EArcRmlInputType::ImeSetText;
		Event.Text = Text;
		Event.RangeStart = Start;
		Event.RangeEnd = End;
		Event.Generation = ++ImeGeneration;
		View.RawInput(MoveTemp(Event));
		return Settle();
	}

	// The host's pad accept: the analog-cursor press around the accept key, as a pad's face button arrives.
	bool PadAccept()
	{
		const FKey Accept = EKeys::Virtual_Gamepad_Accept.GetVirtualKey();
		View.GamepadAcceptDown();
		View.KeyDown(Accept);
		View.KeyUp(Accept);
		View.GamepadAcceptUp();
		return Settle();
	}

	bool SetModelString(const TCHAR* Field, const TCHAR* Value)
	{
		View.SetModelValue(ArcRmlFormEventTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeString(Value));
		return Settle();
	}

	bool SetModelBool(const TCHAR* Field, bool bValue)
	{
		View.SetModelValue(ArcRmlFormEventTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeBool(bValue));
		return Settle();
	}

	// Focuses a text field by a click and puts its caret at the end.
	bool FocusField(const TCHAR* DomId)
	{
		return Click(DomId) && Press(EKeys::End);
	}

	bool Blur()
	{
		return Click(TEXT("fe-after"));
	}

	void ExpectEvents(const TCHAR* Step, const TCHAR* Tag, const TCHAR* Expected)
	{
		const FString Measured = ArcRmlFormEventTestsPrivate::DescribeEvents(View, Tag);
		if (Measured != Expected)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s fired '%s', expected '%s'] "), Step, Tag, *Measured, Expected);
		}
	}

	void ExpectFocus(const TCHAR* Step, const TCHAR* DomId)
	{
		const FString Focus = ArcRmlUITestHelpers::GetFocusDomId(View);
		if (Focus != DomId)
		{
			Mismatches += FString::Printf(TEXT("[%s: focus on '%s', expected '%s'] "), Step, *Focus, DomId);
		}
	}

	void ExpectFieldText(const TCHAR* Step, const TCHAR* Expected)
	{
		const FArcRmlTextFieldShadow& Field = View.GetSnapshot().TextField;
		const FString Text = Field.bActive ? Field.Text : FString(TEXT("<no active text field>"));
		if (Text != Expected)
		{
			Mismatches += FString::Printf(TEXT("[%s: the focused field holds '%s', expected '%s'] "), Step, *Text.ReplaceCharWithEscapedChar(),
				*FString(Expected).ReplaceCharWithEscapedChar());
		}
	}

	FString GetSelectValueText(const TCHAR* DomId)
	{
		View.ParseLayout(Layout);
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

	void ExpectSelectText(const TCHAR* Step, const TCHAR* DomId, const TCHAR* Expected)
	{
		const FString Text = GetSelectValueText(DomId);
		if (Text != Expected)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s shows '%s', expected '%s'] "), Step, DomId, *Text, Expected);
		}
	}

	// The visible options of the select's open list, found in the layout under the select's DOM ('.') and non-DOM ('~') paths.
	TArray<FVector2f> GetOpenOptionCentres(const TCHAR* DomId)
	{
		TArray<FVector2f> Centres;
		View.ParseLayout(Layout);
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Select)
		{
			return Centres;
		}
		const FString DomPrefix = Select->Path + TEXT(".");
		const FString PartPrefix = Select->Path + TEXT("~");
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			const bool bUnderSelect = Element.Path.StartsWith(DomPrefix) || Element.Path.StartsWith(PartPrefix);
			if (bUnderSelect && Element.Tag == TEXT("option") && Element.bVisible && Element.Rect.Size.Y > 0.0f)
			{
				Centres.Add(Element.Rect.Position + Element.Rect.Size * 0.5f);
			}
		}
		return Centres;
	}

	void ExpectOpenOptions(const TCHAR* Step, const TCHAR* DomId, int32 Expected)
	{
		const int32 Open = GetOpenOptionCentres(DomId).Num();
		if (Open != Expected)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s shows %d open options, expected %d] "), Step, DomId, Open, Expected);
		}
	}

	// The host's pointer gate at Point, and the published hit regions against a full walk of the document.
	void ExpectGate(const TCHAR* Step, const FVector2f& Point, bool bExpected)
	{
		const bool bGate = View.IsInteractiveAt(Point);
		if (bGate != bExpected)
		{
			Mismatches += FString::Printf(TEXT("[%s: the gate at (%.0f,%.0f) is %s, expected %s] "), Step, Point.X, Point.Y, bGate ? TEXT("open") : TEXT("closed"),
				bExpected ? TEXT("open") : TEXT("closed"));
		}
		const FString RegionMismatch = ArcRmlUITestHelpers::DescribeHitRegionMismatch(View);
		if (!RegionMismatch.IsEmpty())
		{
			Mismatches += FString::Printf(TEXT("[%s: %s] "), Step, *RegionMismatch);
		}
	}

	// Opens the select with a pointer click and clicks the centre of its option at Index in the open list.
	bool PickOption(const TCHAR* DomId, int32 Index)
	{
		if (!Click(DomId))
		{
			return false;
		}
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Select)
		{
			Mismatches += FString::Printf(TEXT("[%s missing] "), DomId);
			return false;
		}
		// Non-DOM parts such as the selectbox extend the path with '~', DOM children with '.'.
		const FString DomPrefix = Select->Path + TEXT(".");
		const FString PartPrefix = Select->Path + TEXT("~");
		TArray<const FArcRmlTestLayoutElement*> Options;
		TArray<FString> Described;
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (!Element.Path.StartsWith(DomPrefix) && !Element.Path.StartsWith(PartPrefix))
			{
				continue;
			}
			Described.Add(FString::Printf(TEXT("%s %s%s %.0f,%.0f %.0fx%.0f"), *Element.Path, *Element.Tag, Element.bVisible ? TEXT("") : TEXT(" hidden"),
				Element.Rect.Position.X, Element.Rect.Position.Y, Element.Rect.Size.X, Element.Rect.Size.Y));
			if (Element.Tag == TEXT("option") && Element.bVisible && Element.Rect.Size.Y > 0.0f)
			{
				Options.Add(&Element);
			}
		}
		if (!Options.IsValidIndex(Index))
		{
			Mismatches += FString::Printf(TEXT("[%s open with %d visible options, option %d wanted; under it: %s] "), DomId, Options.Num(), Index,
				*FString::Join(Described, TEXT("; ")));
			return false;
		}
		return ClickAt(Options[Index]->Rect.Position + Options[Index]->Rect.Size * 0.5f);
	}

	TEST_METHOD(Open_FiresNothing)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetCalls().Num(), ArcRmlFormEventTestsPrivate::DescribeEvents(View, TEXT("cb"))));
	}

	TEST_METHOD(CheckboxClickSpaceAndEnter_FireInputThenChangeEach)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-cb"))));
		ExpectEvents(TEXT("click"), TEXT("cb"), TEXT("In 'yes' true, Ch 'yes' true"));
		ExpectFocus(TEXT("click"), TEXT("fe-cb"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::SpaceBar)));
		ExpectEvents(TEXT("Space"), TEXT("cb"), TEXT("In 'yes' false, Ch 'yes' false"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Enter"), TEXT("cb"), TEXT("In 'yes' true, Ch 'yes' true"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(CheckboxPadAccept_FiresInputThenChangeOncePerPress)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetInputType(ECommonInputType::Gamepad);
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(AreEqual(FString(TEXT("fe-cb")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(PadAccept()));
		ExpectEvents(TEXT("pad accept"), TEXT("cb"), TEXT("In 'yes' true, Ch 'yes' true"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(PadAccept()));
		ExpectEvents(TEXT("second pad accept"), TEXT("cb"), TEXT("In 'yes' false, Ch 'yes' false"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// fe-cb-label wraps "Wrapped " and fe-cb-wrapped, which has no value attribute ("on"). ClickElement presses the label without the
	// host's input gate; the pointer cases below go through it.
	TEST_METHOD(WrappingLabel_ClickOnItsTextAndOnTheCheckbox_ToggleOnce)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		View.ClickElement(TEXT("#fe-cb-label"));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("click on the label's text"), TEXT("cbw"), TEXT("In 'on' true, Ch 'on' true"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-cb-wrapped"))));
		ExpectEvents(TEXT("click on the checkbox inside the label"), TEXT("cbw"), TEXT("In 'on' false, Ch 'on' false"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(RadioPick_FiresOnThePickedRadioOnly_AndTheCheckedOneFiresNothing)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-r2"))));
		ExpectEvents(TEXT("click on the checked r2"), TEXT("r2"), TEXT(""));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-r1"))));
		ExpectEvents(TEXT("click on r1"), TEXT("r1"), TEXT("In 'r1' true, Ch 'r1' true"));
		ExpectEvents(TEXT("click on r1"), TEXT("r2"), TEXT(""));
		ExpectEvents(TEXT("click on r1"), TEXT("r3"), TEXT(""));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-r1"))));
		ExpectEvents(TEXT("second click on r1"), TEXT("r1"), TEXT(""));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-r2"))));
		ExpectEvents(TEXT("click on r2 after r1"), TEXT("r2"), TEXT("In 'r2' true, Ch 'r2' true"));
		ExpectEvents(TEXT("click on r2 after r1"), TEXT("r1"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(ForLabel_ClickPicksItsRadio)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		View.ClickElement(TEXT("#fe-r1-label"));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("click on the for label"), TEXT("r1"), TEXT("In 'r1' true, Ch 'r1' true"));
		ExpectEvents(TEXT("click on the for label"), TEXT("r2"), TEXT(""));
		View.ClearCalls();
		View.ClickElement(TEXT("#fe-r1-label"));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("second click on the for label"), TEXT("r1"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// A label whose control is enabled is interactive to the host's pointer gate (ArcRmlUIThread.cpp IsInteractive), so a pointer
	// click on its text reaches the document and the label clicks its control.
	TEST_METHOD(LabelPointerClickOnItsText_TogglesItsControlOnce)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-r1-label"))));
		ExpectEvents(TEXT("pointer click on the for label"), TEXT("r1"), TEXT("In 'r1' true, Ch 'r1' true"));
		ExpectEvents(TEXT("pointer click on the for label"), TEXT("r2"), TEXT(""));
		View.ParseLayout(Layout);
		const FArcRmlTestLayoutElement* Label = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("fe-cb-label"));
		ASSERT_THAT(IsNotNull(Label));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(Label->Rect.Position + FVector2f(2.0f, Label->Rect.Size.Y * 0.5f))));
		ExpectEvents(TEXT("pointer click on the wrapping label's text"), TEXT("cbw"), TEXT("In 'on' true, Ch 'on' true"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Decision 4: Space, Enter and pad accept pick a radio, as a click does.
	TEST_METHOD(RadioSpaceEnterAndPadAccept_PickTheFocusedRadio)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetInputType(ECommonInputType::Gamepad);
		ASSERT_THAT(IsTrue(Settle()));
		for (int32 Step = 0; Step < 6 && ArcRmlUITestHelpers::GetNavFocusDomId(View) != TEXT("fe-r1"); ++Step)
		{
			ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down)));
		}
		ASSERT_THAT(AreEqual(FString(TEXT("fe-r1")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(PadAccept()));
		ExpectEvents(TEXT("pad accept on r1"), TEXT("r1"), TEXT("In 'r1' true, Ch 'r1' true"));
		ExpectEvents(TEXT("pad accept on r1"), TEXT("r2"), TEXT(""));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(PadAccept()));
		ExpectEvents(TEXT("pad accept on the checked r1"), TEXT("r1"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Right)));
		ASSERT_THAT(AreEqual(FString(TEXT("fe-r2")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::SpaceBar)));
		ExpectEvents(TEXT("Space on r2"), TEXT("r2"), TEXT("In 'r2' true, Ch 'r2' true"));
		ExpectEvents(TEXT("Space on r2"), TEXT("r1"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Right)));
		ASSERT_THAT(AreEqual(FString(TEXT("fe-r3")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Enter on r3"), TEXT("r3"), TEXT("In 'r3' true, Ch 'r3' true"));
		ExpectEvents(TEXT("Enter on r3"), TEXT("r2"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(DisabledCheckbox_ClickFiresNothing)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-cb-off"))));
		ASSERT_THAT(IsTrue(Click(TEXT("fe-cb-off"))));
		ExpectEvents(TEXT("two clicks"), TEXT("cboff"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(SelectMousePick_FiresInputThenChangeOnlyWhenTheValueChanges)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSelectText(TEXT("open"), TEXT("fe-sel"), TEXT("Apple"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(PickOption(TEXT("fe-sel"), 1), Mismatches));
		ExpectEvents(TEXT("pick Banana"), TEXT("sel"), TEXT("In 'b', Ch 'b'"));
		ExpectSelectText(TEXT("pick Banana"), TEXT("fe-sel"), TEXT("Banana"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(PickOption(TEXT("fe-sel"), 1), Mismatches));
		ExpectEvents(TEXT("pick Banana again"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("pick Banana again"), TEXT("fe-sel"), TEXT("Banana"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(PickOption(TEXT("fe-sel"), 0), Mismatches));
		ExpectEvents(TEXT("pick Apple"), TEXT("sel"), TEXT("In 'a', Ch 'a'"));
		ExpectSelectText(TEXT("pick Apple"), TEXT("fe-sel"), TEXT("Apple"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Each edit kind fires one input and no change; blur commits once with the edited value.
	TEST_METHOD(TextEdits_FireInputEach_AndBlurFiresOneChange)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text"))));
		ASSERT_THAT(AreEqual(FString(TEXT("fe-text")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Type(TEXT("x"))));
		ASSERT_THAT(IsTrue(Press(EKeys::BackSpace)));
		ASSERT_THAT(IsTrue(Press(EKeys::Home) && Press(EKeys::Delete)));
		ASSERT_THAT(IsTrue(Press(EKeys::A, ArcRmlFormEventTestsPrivate::MakeCtrl()) && Press(EKeys::X, ArcRmlFormEventTestsPrivate::MakeCtrl())));
		FPlatformApplicationMisc::ClipboardCopy(TEXT("zz"));
		ASSERT_THAT(IsTrue(Press(EKeys::V, ArcRmlFormEventTestsPrivate::MakeCtrl())));
		ASSERT_THAT(IsTrue(ImeSetText(TEXT("q"), 0, 2)));
		ExpectEvents(TEXT("type x, Backspace, Home Delete, Ctrl+A Ctrl+X, Ctrl+V 'zz', IME SetText 'q' over 0..2"), TEXT("text"),
			TEXT("In 'abx', In 'ab', In 'b', In '', In 'zz', In 'q'"));
		ExpectFieldText(TEXT("after the edits"), TEXT("q"));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("blur"), TEXT("text"), TEXT("In 'abx', In 'ab', In 'b', In '', In 'zz', In 'q', Ch 'q', Up 'q'"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(TextBlurWithoutAChangedValue_FiresNoChange)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("focus and blur"), TEXT("text"), TEXT(""));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Type(TEXT("x")) && Press(EKeys::BackSpace)));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("type x, Backspace, blur"), TEXT("text"), TEXT("In 'abx', In 'ab'"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Leaving an edited field fires one change, seen by the field and bubbling to its row, then one blur, as Chrome's unfocusing steps
	// (Context::OnFocusChange commits before any blur).
	TEST_METHOD(TextBlurAfterAnEdit_FiresOneChangeAndOneBlur)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Type(TEXT("x"))));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("type x, blur"), TEXT("text"), TEXT("In 'abx', Ch 'abx', Up 'abx'"));
		ExpectFocus(TEXT("type x, blur"), TEXT("fe-after"));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Bl"), TEXT("text")), TEXT("one blur on fe-text")));
		ASSERT_THAT(AreEqual(FString(TEXT("In, Ch, Up, Bl")), ArcRmlFormEventTestsPrivate::DescribeCallOrder(View, TEXT("text"))));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(SingleLineEnter_CommitsOnlyAValueChangedSinceTheLastChange)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Enter without an edit"), TEXT("text"), TEXT(""));
		ASSERT_THAT(IsTrue(Type(TEXT("x"))));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("type x, Enter"), TEXT("text"), TEXT("In 'abx', Ch 'abx', Up 'abx'"));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("second Enter"), TEXT("text"), TEXT("In 'abx', Ch 'abx', Up 'abx'"));
		ExpectFieldText(TEXT("second Enter"), TEXT("abx"));
		ExpectFocus(TEXT("second Enter"), TEXT("fe-text"));
		ASSERT_THAT(IsTrue(Press(EKeys::BackSpace)));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("Backspace, blur"), TEXT("text"), TEXT("In 'abx', Ch 'abx', Up 'abx', In 'ab', Ch 'ab', Up 'ab'"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(TextAreaEnter_InsertsALineBreakWithInputOnly)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-ta"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Enter"), TEXT("ta"), TEXT("In 'ab\\n' true"));
		ExpectFieldText(TEXT("Enter"), TEXT("ab\n"));
		ASSERT_THAT(IsTrue(Type(TEXT("c"))));
		ExpectEvents(TEXT("type c"), TEXT("ta"), TEXT("In 'ab\\n' true, In 'ab\\nc' false"));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("blur"), TEXT("ta"), TEXT("In 'ab\\n' true, In 'ab\\nc' false, Ch 'ab\\nc'"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Positive controls first: the same keys on fe-text fire, so the readonly and disabled fields' silence is not a dead path.
	TEST_METHOD(ReadOnlyAndDisabledTextFields_FireNothing)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text-ro"))));
		ExpectFocus(TEXT("click on the readonly field"), TEXT("fe-text-ro"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Type(TEXT("x")) && Press(EKeys::BackSpace) && Press(EKeys::Enter)));
		ASSERT_THAT(IsTrue(Press(EKeys::A, ArcRmlFormEventTestsPrivate::MakeCtrl()) && Press(EKeys::X, ArcRmlFormEventTestsPrivate::MakeCtrl())));
		FPlatformApplicationMisc::ClipboardCopy(TEXT("zz"));
		ASSERT_THAT(IsTrue(Press(EKeys::V, ArcRmlFormEventTestsPrivate::MakeCtrl()) && ImeSetText(TEXT("q"), 0, 2)));
		ExpectFieldText(TEXT("edits on the readonly field"), TEXT("ab"));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("readonly: type, Backspace, Enter, cut, paste, IME, blur"), TEXT("textro"), TEXT(""));
		ASSERT_THAT(IsTrue(Click(TEXT("fe-text-off")) && Type(TEXT("x")) && Press(EKeys::BackSpace) && Blur()));
		ExpectEvents(TEXT("disabled: click, type, Backspace, blur"), TEXT("textoff"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Attribute and binding writes fire nothing; the user action after each shows the write took effect.
	TEST_METHOD(BindingWrites_FireNothingOnAnyControl)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(SetModelBool(TEXT("chk"), true)));
		ExpectEvents(TEXT("chk = true"), TEXT("cbb"), TEXT(""));
		ASSERT_THAT(IsTrue(Click(TEXT("fe-cb-bound"))));
		ExpectEvents(TEXT("click after chk = true"), TEXT("cbb"), TEXT("In 'on' false, Ch 'on' false"));

		View.ClearCalls();
		ASSERT_THAT(IsTrue(SetModelBool(TEXT("rad"), true)));
		ExpectEvents(TEXT("rad = true"), TEXT("r1"), TEXT(""));
		ExpectEvents(TEXT("rad = true"), TEXT("r2"), TEXT(""));
		ExpectEvents(TEXT("rad = true"), TEXT("r3"), TEXT(""));
		ASSERT_THAT(IsTrue(Click(TEXT("fe-r3"))));
		ExpectEvents(TEXT("click on r3 after rad = true"), TEXT("r3"), TEXT(""));
		ASSERT_THAT(IsTrue(Click(TEXT("fe-r2"))));
		ExpectEvents(TEXT("click on r2 after rad = true"), TEXT("r2"), TEXT("In 'r2' true, Ch 'r2' true"));

		View.ClearCalls();
		ASSERT_THAT(IsTrue(SetModelString(TEXT("sel"), TEXT("c"))));
		ExpectEvents(TEXT("sel = c"), TEXT("selb"), TEXT(""));
		ExpectSelectText(TEXT("sel = c"), TEXT("fe-sel-bound"), TEXT("Cherry"));

		// The tab is sanitised out of a single-line value; before step 7 that sanitising fired change.
		View.ClearCalls();
		ASSERT_THAT(IsTrue(SetModelString(TEXT("txt"), TEXT("z\tz"))));
		ExpectEvents(TEXT("txt = 'z\\tz'"), TEXT("textb"), TEXT(""));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text-bound"))));
		ExpectFieldText(TEXT("focus after txt = 'z\\tz'"), TEXT("zz"));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("focus and blur after the unfocused write"), TEXT("textb"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Review round 1: a model echoing each input back through data-attr-value while the field is focused does not move the value
	// change compares with, so blur still commits once (Chrome's TextFieldInputType::SetValue).
	TEST_METHOD(BoundTextEchoedWhileFocused_StillChangesOnceAtBlur)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text-bound"))));
		ExpectFieldText(TEXT("focus"), TEXT("ab"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Type(TEXT("x"))));
		ASSERT_THAT(IsTrue(SetModelString(TEXT("txt"), TEXT("abx"))));
		ExpectEvents(TEXT("type x, echo 'abx'"), TEXT("textb"), TEXT("In 'abx'"));
		ExpectFieldText(TEXT("echo 'abx'"), TEXT("abx"));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("blur after the echo"), TEXT("textb"), TEXT("In 'abx', Ch 'abx'"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// A binding write while the field is focused does not move the value change compares with either: blur commits the written
	// value against the one at focus, as in Chrome. An echo of the field's own value is a no-op write and does not reach that rule.
	TEST_METHOD(BoundTextWrittenWhileFocused_ChangesAtBlurAgainstTheValueAtFocus)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-text-bound"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(SetModelString(TEXT("txt"), TEXT("zz"))));
		ExpectEvents(TEXT("txt = 'zz' while focused"), TEXT("textb"), TEXT(""));
		ExpectFieldText(TEXT("txt = 'zz' while focused"), TEXT("zz"));
		ExpectFocus(TEXT("txt = 'zz' while focused"), TEXT("fe-text-bound"));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("blur after the write"), TEXT("textb"), TEXT("Ch 'zz'"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// fe-kind's type comes from data-attr-type; a type change rebuilds the input's widget while the field holds focus.
	TEST_METHOD(TypeChangeWhileFocused_FiresNoChange)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-kind"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(SetModelString(TEXT("kind"), TEXT("password"))));
		ExpectFocus(TEXT("type password"), TEXT("fe-kind"));
		ExpectEvents(TEXT("type password"), TEXT("kind"), TEXT(""));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("blur after the type change"), TEXT("kind"), TEXT(""));
		// Where x lands is not checked: in a password field a click past the text, End and Ctrl+A leave the caret at 0 with
		// nothing selected (known caret bug, reported 2026-11-06, not step 7's). One input and one change carry 'ab' plus x.
		ASSERT_THAT(IsTrue(Click(TEXT("fe-kind")) && Type(TEXT("x"))));
		ASSERT_THAT(IsTrue(Blur()));
		const FString Events = ArcRmlFormEventTestsPrivate::DescribeEvents(View, TEXT("kind"));
		const FArcRmlViewModelCall* Change = View.GetCalls().FindByPredicate([](const FArcRmlViewModelCall& Call) -> bool
		{
			return Call.FunctionName == TEXT("Ch") && Call.Arguments.Num() > 1 && Call.Arguments[0].String == TEXT("kind");
		});
		const FString Typed = Change ? Change->Arguments[1].String : FString();
		ExpectEvents(TEXT("type x as password, blur"), TEXT("kind"), *FString::Printf(TEXT("In '%s', Ch '%s'"), *Typed, *Typed));
		ASSERT_THAT(AreEqual(3, Typed.Len(), Events));
		ASSERT_THAT(AreEqual(FString(TEXT("ab")), Typed.Replace(TEXT("x"), TEXT("")), Events));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// The widget a type change builds while the field holds focus starts with has_focus taken from the context's focus, so a binding
	// write (fe-kind's value reads txt) does not move the committed value and blur commits against the value at focus.
	TEST_METHOD(TypeChangeWhileFocused_ThenABindingWrite_ChangesOnceAtBlur)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-kind"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(SetModelString(TEXT("kind"), TEXT("password"))));
		ExpectFocus(TEXT("type password"), TEXT("fe-kind"));
		ASSERT_THAT(IsTrue(SetModelString(TEXT("txt"), TEXT("zz"))));
		ExpectFocus(TEXT("txt = 'zz' as password"), TEXT("fe-kind"));
		ExpectEvents(TEXT("type password, txt = 'zz'"), TEXT("kind"), TEXT(""));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("blur after the write"), TEXT("kind"), TEXT("Ch 'zz'"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// The open list closes on blur and keeps the value, also after the pointer moved over another option; a press outside every
	// interactive region reaches the document while the list is open (bSelectListOpen) and closes the list (VENDORED 230).
	TEST_METHOD(SelectOpenThenBlurOrClickOutside_FiresNothingAndKeepsTheValue)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-sel"))));
		const TArray<FVector2f> Options = GetOpenOptionCentres(TEXT("fe-sel"));
		ASSERT_THAT(AreEqual(3, Options.Num(), TEXT("the click did not open fe-sel's list")));
		View.MouseMove(Options[2]);
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("open, hover Cherry, blur"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("open, hover Cherry, blur"), TEXT("fe-sel"), TEXT("Apple"));
		ExpectOpenOptions(TEXT("open, hover Cherry, blur"), TEXT("fe-sel"), 0);
		ExpectFocus(TEXT("open, hover Cherry, blur"), TEXT("fe-after"));

		ASSERT_THAT(IsTrue(Click(TEXT("fe-sel"))));
		ASSERT_THAT(AreEqual(3, GetOpenOptionCentres(TEXT("fe-sel")).Num(), TEXT("the second click did not open fe-sel's list")));
		const FVector2f Outside(static_cast<float>(ArcRmlUITestHelpers::ViewSize.X) - 10.0f, Centre(TEXT("fe-sel")).Y);
		ASSERT_THAT(IsFalse(View.IsInteractiveAt(Outside), TEXT("the outside point is a pointer region")));
		ASSERT_THAT(IsTrue(ClickAt(Outside)));
		ExpectEvents(TEXT("open, click outside"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("open, click outside"), TEXT("fe-sel"), TEXT("Apple"));
		ExpectOpenOptions(TEXT("open, click outside"), TEXT("fe-sel"), 0);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Positive controls: the same click opens the enabled fe-sel, and ClickElement picks the enabled fe-r1.
	TEST_METHOD(DisabledSelectAndRadio_FireNothing)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-sel-off")) && Click(TEXT("fe-sel-off"))));
		View.ClickElement(TEXT("#fe-sel-off"));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectOpenOptions(TEXT("clicks on the disabled select"), TEXT("fe-sel-off"), 0);
		ExpectSelectText(TEXT("clicks on the disabled select"), TEXT("fe-sel-off"), TEXT("Apple"));
		ExpectEvents(TEXT("clicks on the disabled select"), TEXT("seloff"), TEXT(""));
		ASSERT_THAT(IsTrue(Click(TEXT("fe-r-off")) && Click(TEXT("fe-r-off"))));
		View.ClickElement(TEXT("#fe-r-off"));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("clicks on the disabled radio"), TEXT("roff"), TEXT(""));

		ASSERT_THAT(IsTrue(Click(TEXT("fe-sel"))));
		ExpectOpenOptions(TEXT("click on the enabled select"), TEXT("fe-sel"), 3);
		ASSERT_THAT(IsTrue(Blur()));
		View.ClickElement(TEXT("#fe-r1"));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("ClickElement on the enabled r1"), TEXT("r1"), TEXT("In 'r1' true, Ch 'r1' true"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Positive control first: the same edit on fe-ta fires.
	TEST_METHOD(ReadOnlyTextArea_FiresNothing)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-ta"))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Type(TEXT("x")) && Blur()));
		ExpectEvents(TEXT("fe-ta: type x, blur"), TEXT("ta"), TEXT("In 'abx' false, Ch 'abx'"));

		ASSERT_THAT(IsTrue(FocusField(TEXT("fe-ta-ro"))));
		ExpectFocus(TEXT("click on the readonly textarea"), TEXT("fe-ta-ro"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Type(TEXT("x")) && Press(EKeys::BackSpace) && Press(EKeys::Enter)));
		ASSERT_THAT(IsTrue(Press(EKeys::A, ArcRmlFormEventTestsPrivate::MakeCtrl()) && Press(EKeys::X, ArcRmlFormEventTestsPrivate::MakeCtrl())));
		FPlatformApplicationMisc::ClipboardCopy(TEXT("zz"));
		ASSERT_THAT(IsTrue(Press(EKeys::V, ArcRmlFormEventTestsPrivate::MakeCtrl()) && ImeSetText(TEXT("q"), 0, 2)));
		ExpectFieldText(TEXT("edits on the readonly textarea"), TEXT("ab"));
		ASSERT_THAT(IsTrue(Blur()));
		ExpectEvents(TEXT("readonly textarea: type, Backspace, Enter, cut, paste, IME, blur"), TEXT("taro"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// fe-cb-for has no value attribute: both events carry 'on', checked or not.
	TEST_METHOD(CheckboxForLabelPointerClick_TogglesIt)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-cb-for-label"))));
		ExpectEvents(TEXT("pointer click on the for label"), TEXT("cbf"), TEXT("In 'on' true, Ch 'on' true"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("fe-cb-for-label"))));
		ExpectEvents(TEXT("second pointer click on the for label"), TEXT("cbf"), TEXT("In 'on' false, Ch 'on' false"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// fe-cb-dis's disabled is bound to off: its for label is a pointer target only while the control is enabled
	// (ElementFormControl::OnAttributeChange marks the label through ElementLabel::DirtyLabelsOf).
	TEST_METHOD(ForLabelOfABoundDisabledControl_TakesPointerClicksOnlyWhileEnabled)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Label = Centre(TEXT("fe-cb-dis-label"));
		ExpectGate(TEXT("enabled at load"), Label, true);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(Label)));
		ExpectEvents(TEXT("pointer click while enabled"), TEXT("cbd"), TEXT("In 'd' true, Ch 'd' true"));

		ASSERT_THAT(IsTrue(SetModelBool(TEXT("off"), true)));
		ExpectGate(TEXT("off = true"), Label, false);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(Label)));
		ExpectEvents(TEXT("pointer click while disabled"), TEXT("cbd"), TEXT(""));

		ASSERT_THAT(IsTrue(SetModelBool(TEXT("off"), false)));
		ExpectGate(TEXT("off = false"), Label, true);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(Label)));
		ExpectEvents(TEXT("pointer click enabled again"), TEXT("cbd"), TEXT("In 'd' false, Ch 'd' false"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// fe-rm carries data-if="keep" itself; once keep is false its for label labels nothing and is no pointer target
	// (ElementFormControl::OnChildRemove marks the label; GetElementById skips the hidden data-if template that keeps the id).
	TEST_METHOD(ForLabelOfAControlRemovedByDataIf_IsNoLongerAPointerTarget)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Label = Centre(TEXT("fe-rm-label"));
		ExpectGate(TEXT("with the control"), Label, true);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(Label)));
		ExpectEvents(TEXT("pointer click with the control"), TEXT("rm"), TEXT("In 'on' true, Ch 'on' true"));
		ASSERT_THAT(IsTrue(SetModelBool(TEXT("keep"), false)));
		ExpectGate(TEXT("keep = false"), Label, false);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(Label)));
		ExpectEvents(TEXT("pointer click without the control"), TEXT("rm"), TEXT(""));

		// A new copy is unchecked and takes the label's clicks again.
		ASSERT_THAT(IsTrue(SetModelBool(TEXT("keep"), true)));
		ExpectGate(TEXT("keep = true again"), Label, true);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(Label)));
		ExpectEvents(TEXT("pointer click on the new copy"), TEXT("rm"), TEXT("In 'on' true, Ch 'on' true"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};
