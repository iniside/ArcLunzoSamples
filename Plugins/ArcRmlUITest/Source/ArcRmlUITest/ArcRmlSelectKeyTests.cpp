// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"

namespace ArcRmlSelectKeyTestsPrivate
{
	const TCHAR* const Fixture = TEXT("Form/SelectKeys.rml");
	const TCHAR* const Sheet = TEXT("SelectKeys.rcssc");
	const TCHAR* const ModelName = TEXT("sk");

	// SelectKeys.rml: sk-long holds One to Ten and 11 to 60; the widget caps its internal list at Chrome's 20 rows (the room below
	// sk-long in the 1280 x 720 view is larger, so the cap and not the view bounds it) and the list scrolls with the native
	// scrollbar (no author rule reaches the list).
	constexpr int32 LongOptions = 60;

	// SelectKeys.rml: every select calls In / Ch with its tag first, then ev.value. One call reads "In 'c'".
	FString DescribeEvents(const FArcRmlTestView& View, const TCHAR* Tag)
	{
		TArray<FString> Parts;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			const bool bFormEvent = Call.FunctionName == TEXT("In") || Call.FunctionName == TEXT("Ch");
			if (!bFormEvent || Call.Arguments.Num() < 2 || Call.Arguments[0].String != Tag)
			{
				continue;
			}
			Parts.Add(FString::Printf(TEXT("%s '%s'"), *Call.FunctionName, *Call.Arguments[1].String));
		}
		return FString::Join(Parts, TEXT(", "));
	}

	// SelectKeys.rml: sk-long's row calls Wrap and sk-long itself calls Own on click, each with 'long' first.
	int32 CountCalls(const FArcRmlTestView& View, const TCHAR* FunctionName, const TCHAR* Tag)
	{
		int32 Count = 0;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			if (Call.FunctionName == FunctionName && Call.Arguments.Num() > 0 && Call.Arguments[0].String == Tag)
			{
				++Count;
			}
		}
		return Count;
	}
}

// Select keys and pad (rml-forms step 8, VENDORED 230; owner decisions 2 and 4): the open list's highlight, its commit and close,
// back on an open and a closed select, and the host gate while the list is open, driven through the view's input path.
TEST_CLASS(ArcRmlSelectKeyTests, "ArcRmlUI.Form.SelectKeys")
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
		using namespace ArcRmlSelectKeyTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
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
		if (!View.IsOpen())
		{
			Mismatches += TEXT("[the view closed] ");
			return false;
		}
		View.MouseMove(Point);
		View.MouseClick(Point);
		return Settle();
	}

	bool Click(const TCHAR* DomId)
	{
		return ClickAt(Centre(DomId));
	}

	bool Press(const FKey& Key)
	{
		if (!View.IsOpen())
		{
			Mismatches += TEXT("[the view closed] ");
			return false;
		}
		View.PressKey(Key);
		return Settle();
	}

	// The host's pad accept: the analog-cursor press around the accept key, as a pad's face button arrives.
	bool PadAccept()
	{
		if (!View.IsOpen())
		{
			Mismatches += TEXT("[the view closed] ");
			return false;
		}
		const FKey Accept = EKeys::Virtual_Gamepad_Accept.GetVirtualKey();
		View.GamepadAcceptDown();
		View.KeyDown(Accept);
		View.KeyUp(Accept);
		View.GamepadAcceptUp();
		return Settle();
	}

	// The pad's back reaches the view as the CommonUI back action (the view's back keys hold only Escape).
	bool PadBack()
	{
		if (!View.IsOpen())
		{
			Mismatches += TEXT("[the view closed] ");
			return false;
		}
		View.Back();
		return Settle();
	}

	FString DescribeBackKeys() const
	{
		TArray<FString> Names;
		for (const FKey& Key : View.GetInputBindings().Back.Keys)
		{
			Names.Add(Key.ToString());
		}
		return FString::Printf(TEXT("back keys: %s"), *FString::Join(Names, TEXT(", ")));
	}

	void ExpectEvents(const TCHAR* Step, const TCHAR* Tag, const TCHAR* Expected)
	{
		const FString Measured = ArcRmlSelectKeyTestsPrivate::DescribeEvents(View, Tag);
		if (Measured != Expected)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s fired '%s', expected '%s'] "), Step, Tag, *Measured, Expected);
		}
	}

	// One press on sk-long's open list: the select's own click listener and the row's click sound see it once, the row's
	// bubbling click listener never.
	void ExpectOneClickStoppedAtTheSelect(const TCHAR* Step)
	{
		using namespace ArcRmlSelectKeyTestsPrivate;
		const int32 Own = CountCalls(View, TEXT("Own"), TEXT("long"));
		const int32 Wrap = CountCalls(View, TEXT("Wrap"), TEXT("long"));
		const int32 Sounds = View.GetPlayedSounds().Num();
		if (Own != 1 || Wrap != 0 || Sounds != 1)
		{
			Mismatches += FString::Printf(TEXT("[%s: select click %d (expected 1), row click %d (expected 0), click sounds %d (expected 1): %s] "), Step, Own,
				Wrap, Sounds, *FString::Join(View.GetPlayedSounds(), TEXT(", ")));
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

	void ExpectNavFocus(const TCHAR* Step, const TCHAR* DomId)
	{
		const FString Focus = ArcRmlUITestHelpers::GetNavFocusDomId(View);
		if (Focus != DomId)
		{
			Mismatches += FString::Printf(TEXT("[%s: nav focus on '%s', expected '%s'] "), Step, *Focus, DomId);
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

	// The option elements of the select's list in layout order, found under the select's DOM ('.') and non-DOM ('~') paths.
	TArray<FArcRmlTestLayoutElement> GetOptions(const TCHAR* DomId, bool bOpenOnly)
	{
		TArray<FArcRmlTestLayoutElement> Options;
		View.ParseLayout(Layout);
		const FArcRmlTestLayoutElement* Select = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Select)
		{
			return Options;
		}
		const FString DomPrefix = Select->Path + TEXT(".");
		const FString PartPrefix = Select->Path + TEXT("~");
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			const bool bUnderSelect = Element.Path.StartsWith(DomPrefix) || Element.Path.StartsWith(PartPrefix);
			const bool bOpen = Element.bVisible && Element.Rect.Size.Y > 0.0f;
			if (bUnderSelect && Element.Tag == TEXT("option") && (bOpen || !bOpenOnly))
			{
				Options.Add(Element);
			}
		}
		return Options;
	}

	const FArcRmlTestLayoutElement* FindSelectBox(const TCHAR* DomId)
	{
		View.ParseLayout(Layout);
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

	// A scrollbar part (slidertrack, sliderbar) of the select's open list, found under its selectbox's path.
	const FArcRmlTestLayoutElement* FindListScrollPart(const TCHAR* DomId, const TCHAR* Tag)
	{
		const FArcRmlTestLayoutElement* Box = FindSelectBox(DomId);
		if (!Box)
		{
			return nullptr;
		}
		const FString BoxPath = Box->Path;
		const FString PartTag = Tag;
		return Layout.FindByPredicate([&BoxPath, &PartTag](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Path.StartsWith(BoxPath) && Element.Path != BoxPath && Element.Tag == PartTag && Element.bVisible;
		});
	}

	float GetListScroll(const TCHAR* DomId)
	{
		const FArcRmlTestLayoutElement* Box = FindSelectBox(DomId);
		return Box ? Box->Scroll.Y : -1.0f;
	}

	FVector2f GetOpenOptionCentre(const TCHAR* DomId, int32 Index)
	{
		const TArray<FArcRmlTestLayoutElement> Options = GetOptions(DomId, true);
		return Options.IsValidIndex(Index) ? Options[Index].Rect.Position + Options[Index].Rect.Size * 0.5f : FVector2f(-1.0f, -1.0f);
	}

	// The list is open (its options are laid out and visible) and the host snapshot says so for the focused select.
	void ExpectListOpen(const TCHAR* Step, const TCHAR* DomId, int32 ExpectedOptions)
	{
		const int32 Open = GetOptions(DomId, true).Num();
		if (Open != ExpectedOptions)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s shows %d open options, expected %d] "), Step, DomId, Open, ExpectedOptions);
		}
		if (!View.GetSnapshot().bSelectListOpen)
		{
			Mismatches += FString::Printf(TEXT("[%s: the snapshot's bSelectListOpen is false] "), Step);
		}
	}

	void ExpectListClosed(const TCHAR* Step, const TCHAR* DomId)
	{
		const int32 Open = GetOptions(DomId, true).Num();
		if (Open != 0)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s still shows %d open options] "), Step, DomId, Open);
		}
		if (View.GetSnapshot().bSelectListOpen)
		{
			Mismatches += FString::Printf(TEXT("[%s: the snapshot's bSelectListOpen is true] "), Step);
		}
	}

	void ExpectViewStays(const TCHAR* Step)
	{
		if (View.GetCloseRequests() != 0 || !View.IsOpen())
		{
			Mismatches += FString::Printf(TEXT("[%s: %d close requests, view %s] "), Step, View.GetCloseRequests(), View.IsOpen() ? TEXT("open") : TEXT("closed"));
		}
	}

	// Decision 2: a closed select is a spatial-navigation stop; Up / Down move focus and never the value.
	TEST_METHOD(ClosedUpDown_MoveFocusWithoutChangingTheValue)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-before"))));
		ASSERT_THAT(AreEqual(FString(TEXT("sk-before")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ExpectFocus(TEXT("Down from sk-before"), TEXT("sk-sel"));
		ExpectListClosed(TEXT("Down from sk-before"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ExpectFocus(TEXT("Down from the closed sk-sel"), TEXT("sk-sel-bound"));
		ASSERT_THAT(IsTrue(Press(EKeys::Up)));
		ExpectFocus(TEXT("Up from the closed sk-sel-bound"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(Press(EKeys::Up)));
		ExpectFocus(TEXT("Up from the closed sk-sel"), TEXT("sk-before"));
		ExpectEvents(TEXT("Down, Down, Up, Up"), TEXT("sel"), TEXT(""));
		ExpectEvents(TEXT("Down, Down, Up, Up"), TEXT("selb"), TEXT(""));
		ExpectSelectText(TEXT("Down, Down, Up, Up"), TEXT("sk-sel"), TEXT("Apple"));
		ExpectSelectText(TEXT("Down, Down, Up, Up"), TEXT("sk-sel-bound"), TEXT("Apple"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Enter and Space open the closed select with its selected option highlighted: Down from Cherry reaches Date, where a
	// highlight starting nowhere would reach Apple. Enter or Space on the unchanged highlight fires nothing.
	TEST_METHOD(ClosedEnterAndSpace_OpenWithTheSelectedOptionHighlighted)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ASSERT_THAT(IsTrue(ClickAt(GetOpenOptionCentre(TEXT("sk-sel"), 2))));
		ASSERT_THAT(AreEqual(FString(TEXT("Cherry")), GetSelectValueText(TEXT("sk-sel"))));
		ExpectListClosed(TEXT("pick Cherry"), TEXT("sk-sel"));
		ExpectFocus(TEXT("pick Cherry"), TEXT("sk-sel"));
		View.ClearCalls();

		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectListOpen(TEXT("Enter on the closed select"), TEXT("sk-sel"), 4);
		ExpectEvents(TEXT("Enter on the closed select"), TEXT("sel"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectListClosed(TEXT("Enter on the open list"), TEXT("sk-sel"));
		ExpectEvents(TEXT("Enter on the unchanged highlight"), TEXT("sel"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter) && Press(EKeys::Down)));
		ExpectEvents(TEXT("Enter, Down"), TEXT("sel"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Enter, Down, Enter"), TEXT("sel"), TEXT("In 'd', Ch 'd'"));
		ExpectSelectText(TEXT("Enter, Down, Enter"), TEXT("sk-sel"), TEXT("Date"));
		ExpectListClosed(TEXT("Enter, Down, Enter"), TEXT("sk-sel"));

		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::SpaceBar)));
		ExpectListOpen(TEXT("Space on the closed select"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(Press(EKeys::SpaceBar)));
		ExpectListClosed(TEXT("Space on the open list"), TEXT("sk-sel"));
		ExpectEvents(TEXT("Space on the unchanged highlight"), TEXT("sel"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::SpaceBar) && Press(EKeys::Up)));
		ExpectEvents(TEXT("Space, Up"), TEXT("sel"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::SpaceBar)));
		ExpectEvents(TEXT("Space, Up, Space"), TEXT("sel"), TEXT("In 'c', Ch 'c'"));
		ExpectSelectText(TEXT("Space, Up, Space"), TEXT("sk-sel"), TEXT("Cherry"));
		ExpectListClosed(TEXT("Space, Up, Space"), TEXT("sk-sel"));
		ExpectFocus(TEXT("Space, Up, Space"), TEXT("sk-sel"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Up / Down move the highlight past the disabled Banana and change nothing until Enter commits it once.
	TEST_METHOD(OpenUpDown_MoveTheHighlightOnly_AndEnterCommitsItOnce)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-sel"), 4);
		View.ClearCalls();
		const FKey Keys[] = {EKeys::Down, EKeys::Up, EKeys::Down, EKeys::Down};
		const TCHAR* const Steps[] = {TEXT("Down"), TEXT("Down, Up"), TEXT("Down, Up, Down"), TEXT("Down, Up, Down, Down")};
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Keys)); ++Index)
		{
			ASSERT_THAT(IsTrue(Press(Keys[Index])));
			ExpectListOpen(Steps[Index], TEXT("sk-sel"), 4);
			ExpectEvents(Steps[Index], TEXT("sel"), TEXT(""));
			ExpectSelectText(Steps[Index], TEXT("sk-sel"), TEXT("Apple"));
			ExpectFocus(Steps[Index], TEXT("sk-sel"));
		}
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Down, Up, Down, Down, Enter"), TEXT("sel"), TEXT("In 'd', Ch 'd'"));
		ExpectSelectText(TEXT("Down, Up, Down, Down, Enter"), TEXT("sk-sel"), TEXT("Date"));
		ExpectListClosed(TEXT("Down, Up, Down, Down, Enter"), TEXT("sk-sel"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Elder is display: none (data-style-display) and Banana disabled: End stops on Date, Home on Apple. sk-edge starts with a
	// disabled Zero and an inline display: none option and ends with a disabled Four and a data-style-display none Ex: from the
	// selected Two, End stops on Three and Home on One.
	TEST_METHOD(HomeAndEnd_SkipDisabledAndHiddenOptions)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(5, GetOptions(TEXT("sk-sel"), false).Num(), TEXT("sk-sel has five options")));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-sel"), 4);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::End)));
		ExpectEvents(TEXT("End"), TEXT("sel"), TEXT(""));
		ExpectListOpen(TEXT("End"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("End, Enter"), TEXT("sel"), TEXT("In 'd', Ch 'd'"));
		ExpectSelectText(TEXT("End, Enter"), TEXT("sk-sel"), TEXT("Date"));

		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Enter) && Press(EKeys::Home)));
		ExpectEvents(TEXT("Enter, Home"), TEXT("sel"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Enter, Home, Enter"), TEXT("sel"), TEXT("In 'a', Ch 'a'"));
		ExpectSelectText(TEXT("Enter, Home, Enter"), TEXT("sk-sel"), TEXT("Apple"));

		ASSERT_THAT(AreEqual(7, GetOptions(TEXT("sk-edge"), false).Num(), TEXT("sk-edge has seven options")));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-edge"))));
		ExpectListOpen(TEXT("click sk-edge"), TEXT("sk-edge"), 5);
		ExpectSelectText(TEXT("click sk-edge"), TEXT("sk-edge"), TEXT("Two"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::End) && Press(EKeys::Enter)));
		ExpectEvents(TEXT("sk-edge End, Enter"), TEXT("edge"), TEXT("In '3', Ch '3'"));
		ExpectSelectText(TEXT("sk-edge End, Enter"), TEXT("sk-edge"), TEXT("Three"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Enter) && Press(EKeys::Home) && Press(EKeys::Enter)));
		ExpectEvents(TEXT("sk-edge Enter, Home, Enter"), TEXT("edge"), TEXT("In '1', Ch '1'"));
		ExpectSelectText(TEXT("sk-edge Enter, Home, Enter"), TEXT("sk-edge"), TEXT("One"));
		ExpectListClosed(TEXT("sk-edge Enter, Home, Enter"), TEXT("sk-edge"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// The highlight follows the pointer over an enabled option, not over the disabled Banana; a click on an option picks it.
	TEST_METHOD(Mouseover_MovesTheHighlight_AndAClickCommits)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-sel"), 4);
		View.ClearCalls();
		View.MouseMove(GetOpenOptionCentre(TEXT("sk-sel"), 2));
		ASSERT_THAT(IsTrue(Settle()));
		View.MouseMove(GetOpenOptionCentre(TEXT("sk-sel"), 1));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("hover Cherry, hover Banana"), TEXT("sel"), TEXT(""));
		ExpectListOpen(TEXT("hover Cherry, hover Banana"), TEXT("sk-sel"), 4);
		ExpectSelectText(TEXT("hover Cherry, hover Banana"), TEXT("sk-sel"), TEXT("Apple"));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("hover Cherry, hover Banana, Enter"), TEXT("sel"), TEXT("In 'c', Ch 'c'"));
		ExpectSelectText(TEXT("hover Cherry, hover Banana, Enter"), TEXT("sk-sel"), TEXT("Cherry"));

		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ASSERT_THAT(IsTrue(ClickAt(GetOpenOptionCentre(TEXT("sk-sel"), 3))));
		ExpectEvents(TEXT("click on Date"), TEXT("sel"), TEXT("In 'd', Ch 'd'"));
		ExpectSelectText(TEXT("click on Date"), TEXT("sk-sel"), TEXT("Date"));
		ExpectListClosed(TEXT("click on Date"), TEXT("sk-sel"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// sk-long's list holds more than the 20 rows the widget shows: End scrolls the list, not the page, until the last option's (60)
	// bottom meets the list's padding edge (the scroll range's end, so a clamp would also hide an overshoot there; ArcRmlUI.Form.
	// SelectPopup checks unclamped scrolls); Home scrolls back.
	TEST_METHOD(LongList_ScrollsTheHighlightIntoViewInsideTheList)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f BeforeAtLoad = Centre(TEXT("sk-before"));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-long"))));
		ASSERT_THAT(IsTrue(View.GetSnapshot().bSelectListOpen, TEXT("the click did not open sk-long's list")));
		const FArcRmlTestLayoutElement* Box = FindSelectBox(TEXT("sk-long"));
		ASSERT_THAT(IsNotNull(Box));
		const float BoxBottom = Box->Rect.Position.Y + Box->Rect.Size.Y;
		TArray<FArcRmlTestLayoutElement> Options = GetOptions(TEXT("sk-long"), false);
		ASSERT_THAT(AreEqual(ArcRmlSelectKeyTestsPrivate::LongOptions, Options.Num()));
		ASSERT_THAT(IsTrue(Options.Last().Rect.Position.Y + Options.Last().Rect.Size.Y > BoxBottom + 1.0f, TEXT("60 is in view before End, so the list does not clip")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyZero(Box->Scroll.Y)));
		View.ClearCalls();

		ASSERT_THAT(IsTrue(Press(EKeys::End)));
		Box = FindSelectBox(TEXT("sk-long"));
		ASSERT_THAT(IsNotNull(Box));
		Options = GetOptions(TEXT("sk-long"), false);
		ASSERT_THAT(AreEqual(ArcRmlSelectKeyTestsPrivate::LongOptions, Options.Num()));
		const float LastBottom = Options.Last().Rect.Position.Y + Options.Last().Rect.Size.Y;
		// The list's 1 px border sits below its padding edge.
		const float ListPaddingBottom = Box->Rect.Position.Y + Box->Rect.Size.Y - 1.0f;
		if (Box->Scroll.Y <= 0.0f || !FMath::IsNearlyEqual(LastBottom, ListPaddingBottom, 0.5f))
		{
			Mismatches += FString::Printf(TEXT("[End: list scroll %.1f, 60's bottom %.1f, expected the list's padding bottom %.1f] "), Box->Scroll.Y, LastBottom, ListPaddingBottom);
		}
		const FVector2f BeforeAfterEnd = Centre(TEXT("sk-before"));
		if (!BeforeAfterEnd.Equals(BeforeAtLoad, 0.5f))
		{
			Mismatches += FString::Printf(TEXT("[End: sk-before moved from (%.1f,%.1f) to (%.1f,%.1f)] "), BeforeAtLoad.X, BeforeAtLoad.Y, BeforeAfterEnd.X, BeforeAfterEnd.Y);
		}
		ExpectEvents(TEXT("End"), TEXT("long"), TEXT(""));
		ExpectListOpen(TEXT("End"), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);

		ASSERT_THAT(IsTrue(Press(EKeys::Home)));
		Box = FindSelectBox(TEXT("sk-long"));
		ASSERT_THAT(IsNotNull(Box));
		if (!FMath::IsNearlyZero(Box->Scroll.Y, 0.5f))
		{
			Mismatches += FString::Printf(TEXT("[Home: list scroll %.1f] "), Box->Scroll.Y);
		}
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("End, Home, Enter"), TEXT("long"), TEXT(""));
		ExpectSelectText(TEXT("End, Home, Enter"), TEXT("sk-long"), TEXT("One"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Escape and the pad's back (the CommonUI back action) while the list is open close it, keep the value and stop the back: the view stays.
	TEST_METHOD(EscapeAndPadBackWhileOpen_CloseTheListAndKeepTheView)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetInputBindings().Back.Keys.Contains(EKeys::Escape), DescribeBackKeys()));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel")) && Press(EKeys::Down)));
		ExpectListOpen(TEXT("click, Down"), TEXT("sk-sel"), 4);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Escape)));
		ExpectListClosed(TEXT("Escape"), TEXT("sk-sel"));
		ExpectViewStays(TEXT("Escape"));
		ExpectEvents(TEXT("Escape"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("Escape"), TEXT("sk-sel"), TEXT("Apple"));
		ExpectFocus(TEXT("Escape"), TEXT("sk-sel"));

		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel")) && Press(EKeys::Down)));
		ExpectListOpen(TEXT("click, Down again"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(PadBack()));
		ExpectListClosed(TEXT("pad back"), TEXT("sk-sel"));
		ExpectViewStays(TEXT("pad back"));
		ExpectEvents(TEXT("pad back"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("pad back"), TEXT("sk-sel"), TEXT("Apple"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// The back on a closed select is not the widget's: it reaches the view, which asks to close.
	TEST_METHOD(BackOnAClosedSelect_ReachesTheView)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetInputBindings().Back.Keys.Contains(EKeys::Escape), DescribeBackKeys()));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(Press(EKeys::Escape)));
		ExpectListClosed(TEXT("Escape on the open list"), TEXT("sk-sel"));
		ExpectViewStays(TEXT("Escape on the open list"));
		ExpectFocus(TEXT("Escape on the open list"), TEXT("sk-sel"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
		ASSERT_THAT(IsTrue(Press(EKeys::Escape)));
		ASSERT_THAT(AreEqual(1, View.GetCloseRequests(), TEXT("Escape on the closed select did not reach the view")));
	}

	// Plan step 8 (d), through the host pad path: D-pad from a closed select moves focus; accept opens; Down twice moves the
	// highlight with no event; accept commits once; accept on the unchanged highlight fires nothing; back closes the list only.
	TEST_METHOD(PadPath_NavigatesOpensHighlightsCommitsAndBacks)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetInputType(ECommonInputType::Gamepad);
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(AreEqual(FString(TEXT("sk-before")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down)));
		ExpectNavFocus(TEXT("D-pad Down from sk-before"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down)));
		ExpectNavFocus(TEXT("D-pad Down from the closed sk-sel"), TEXT("sk-sel-bound"));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Up)));
		ExpectNavFocus(TEXT("D-pad Up from the closed sk-sel-bound"), TEXT("sk-sel"));
		ExpectListClosed(TEXT("D-pad Down, Down, Up"), TEXT("sk-sel"));
		ExpectEvents(TEXT("D-pad Down, Down, Up"), TEXT("sel"), TEXT(""));
		ExpectEvents(TEXT("D-pad Down, Down, Up"), TEXT("selb"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));

		ASSERT_THAT(IsTrue(PadAccept()));
		ExpectListOpen(TEXT("accept"), TEXT("sk-sel"), 4);
		ExpectEvents(TEXT("accept"), TEXT("sel"), TEXT(""));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down) && Press(EKeys::Gamepad_DPad_Down)));
		ExpectListOpen(TEXT("accept, Down, Down"), TEXT("sk-sel"), 4);
		ExpectEvents(TEXT("accept, Down, Down"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("accept, Down, Down"), TEXT("sk-sel"), TEXT("Apple"));
		ExpectNavFocus(TEXT("accept, Down, Down"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(PadAccept()));
		ExpectEvents(TEXT("accept, Down, Down, accept"), TEXT("sel"), TEXT("In 'd', Ch 'd'"));
		ExpectSelectText(TEXT("accept, Down, Down, accept"), TEXT("sk-sel"), TEXT("Date"));
		ExpectListClosed(TEXT("accept, Down, Down, accept"), TEXT("sk-sel"));

		View.ClearCalls();
		ASSERT_THAT(IsTrue(PadAccept() && PadAccept()));
		ExpectEvents(TEXT("accept, accept on the unchanged highlight"), TEXT("sel"), TEXT(""));
		ExpectListClosed(TEXT("accept, accept on the unchanged highlight"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(PadAccept() && Press(EKeys::Gamepad_DPad_Up)));
		ExpectListOpen(TEXT("accept, Up"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(PadBack()));
		ExpectListClosed(TEXT("accept, Up, back"), TEXT("sk-sel"));
		ExpectViewStays(TEXT("accept, Up, back"));
		ExpectEvents(TEXT("accept, Up, back"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("accept, Up, back"), TEXT("sk-sel"), TEXT("Date"));
		ExpectNavFocus(TEXT("accept, Up, back"), TEXT("sk-sel"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// A binding write while the list is open applies, fires nothing and leaves the list open.
	TEST_METHOD(BindingWriteWhileOpen_AppliesAndKeepsTheListOpen)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel-bound"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-sel-bound"), 3);
		View.ClearCalls();
		View.SetModelValue(ArcRmlSelectKeyTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("sel")), ArcRmlUITestHelpers::MakeString(TEXT("c")));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectListOpen(TEXT("sel = c while open"), TEXT("sk-sel-bound"), 3);
		ExpectSelectText(TEXT("sel = c while open"), TEXT("sk-sel-bound"), TEXT("Cherry"));
		ExpectEvents(TEXT("sel = c while open"), TEXT("selb"), TEXT(""));
		ExpectFocus(TEXT("sel = c while open"), TEXT("sk-sel-bound"));
		ASSERT_THAT(IsTrue(Press(EKeys::Escape)));
		ExpectListClosed(TEXT("sel = c, Escape"), TEXT("sk-sel-bound"));
		ExpectSelectText(TEXT("sel = c, Escape"), TEXT("sk-sel-bound"), TEXT("Cherry"));
		ExpectEvents(TEXT("sel = c, Escape"), TEXT("selb"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// sk-plain is under no pointer region, so the gate gives its presses to the game, except while the focused select's list is
	// open (bSelectListOpen): then the press reaches the document and closes the list. A press on another control closes it too.
	TEST_METHOD(OutsidePress_ClosesTheListWithNoEvent)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Plain = Centre(TEXT("sk-plain"));
		ASSERT_THAT(IsFalse(View.IsInteractiveAt(Plain), TEXT("sk-plain is a pointer region")));
		ASSERT_THAT(IsFalse(View.GetSnapshot().bSelectListOpen));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		View.MouseMove(GetOpenOptionCentre(TEXT("sk-sel"), 2));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectListOpen(TEXT("click, hover Cherry"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsFalse(View.IsInteractiveAt(Plain), TEXT("sk-plain is under the open list")));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(Plain)));
		ExpectListClosed(TEXT("press on sk-plain"), TEXT("sk-sel"));
		ExpectEvents(TEXT("press on sk-plain"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("press on sk-plain"), TEXT("sk-sel"), TEXT("Apple"));
		ExpectViewStays(TEXT("press on sk-plain"));

		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ExpectListOpen(TEXT("click again"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(Click(TEXT("sk-after"))));
		ExpectListClosed(TEXT("press on sk-after"), TEXT("sk-sel"));
		ExpectFocus(TEXT("press on sk-after"), TEXT("sk-after"));
		ExpectEvents(TEXT("press on sk-after"), TEXT("sel"), TEXT(""));
		ExpectSelectText(TEXT("press on sk-after"), TEXT("sk-sel"), TEXT("Apple"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// sk-long's disabled is bound to off: disabling it while its list is open closes the list with no event.
	TEST_METHOD(DisablingWhileOpen_ClosesTheList)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-long"))));
		ASSERT_THAT(IsTrue(View.GetSnapshot().bSelectListOpen, TEXT("the click did not open sk-long's list")));
		ASSERT_THAT(IsTrue(GetOptions(TEXT("sk-long"), true).Num() > 0));
		View.ClearCalls();
		View.SetModelValue(ArcRmlSelectKeyTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("off")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectListClosed(TEXT("off = true while open"), TEXT("sk-long"));
		ExpectEvents(TEXT("off = true while open"), TEXT("long"), TEXT(""));
		ExpectSelectText(TEXT("off = true while open"), TEXT("sk-long"), TEXT("One"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Removing the view, or reloading its document, while a select's list is open (its document listeners attached) must not
	// touch the freed document: the test sees only that nothing crashes and a new document's select still opens and commits.
	TEST_METHOD(CloseOrReloadWithTheListOpen_DoesNotCrash)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ExpectListOpen(TEXT("click before the reload"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(View.Reload(), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(Settle() && View.GetProbe().bHasDocument));
		ExpectListClosed(TEXT("after the reload"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ExpectListOpen(TEXT("click after the reload"), TEXT("sk-sel"), 4);
		View.Close();
		ASSERT_THAT(IsFalse(View.IsOpen()));

		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel")) && Press(EKeys::Down)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("open, Down, Enter after the close"), TEXT("sel"), TEXT("In 'c', Ch 'c'"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// A press on the list's scrollbar track below the thumb pages the list and picks nothing; the list stays open.
	TEST_METHOD(ListTrackPress_PagesTheListAndFiresNothing)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-long"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);
		const FArcRmlTestLayoutElement* Track = FindListScrollPart(TEXT("sk-long"), TEXT("slidertrack"));
		ASSERT_THAT(IsNotNull(Track, TEXT("sk-long's open list shows no scrollbar track")));
		const FVector2f TrackPress(Track->Rect.Position.X + Track->Rect.Size.X * 0.5f, Track->Rect.Position.Y + Track->Rect.Size.Y - 3.0f);
		const FArcRmlTestLayoutElement* Thumb = FindListScrollPart(TEXT("sk-long"), TEXT("sliderbar"));
		ASSERT_THAT(IsNotNull(Thumb));
		ASSERT_THAT(IsTrue(TrackPress.Y > Thumb->Rect.Position.Y + Thumb->Rect.Size.Y, TEXT("the press point is not on the track below the thumb")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyZero(GetListScroll(TEXT("sk-long")))));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(ClickAt(TrackPress)));
		const float Scroll = GetListScroll(TEXT("sk-long"));
		if (Scroll <= 0.0f)
		{
			Mismatches += FString::Printf(TEXT("[track press: list scroll %.1f] "), Scroll);
		}
		ExpectListOpen(TEXT("track press"), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);
		ExpectEvents(TEXT("track press"), TEXT("long"), TEXT(""));
		ExpectSelectText(TEXT("track press"), TEXT("sk-long"), TEXT("One"));
		ExpectFocus(TEXT("track press"), TEXT("sk-long"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// sk-long's row has a click sound and a click listener, and sk-long its own click listener. A press on the open list's left
	// border (the internal list has no padding since VENDORED 231; its 1 px border is the one part of it that is neither an option
	// nor the scrollbar), then on its track below the thumb, reaches the select's own listener and the click sound once each and
	// never the row (the select stops its click); the track press pages the list, the border press does not; neither picks.
	TEST_METHOD(ListBorderAndTrackPress_ClickTheSelectOnceAndNeverItsAncestor)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-long"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);
		ASSERT_THAT(AreEqual(1, ArcRmlSelectKeyTestsPrivate::CountCalls(View, TEXT("Wrap"), TEXT("long")), TEXT("the opening click did not reach sk-long's row")));
		ASSERT_THAT(AreEqual(1, View.GetPlayedSounds().Num(), TEXT("the opening click played no click sound")));
		const FArcRmlTestLayoutElement* Box = FindSelectBox(TEXT("sk-long"));
		ASSERT_THAT(IsNotNull(Box));
		const float BoxLeft = Box->Rect.Position.X;
		const TArray<FArcRmlTestLayoutElement> Open = GetOptions(TEXT("sk-long"), true);
		ASSERT_THAT(IsTrue(Open.Num() > 0));
		const float FirstLeft = Open[0].Rect.Position.X;
		// The view tests whole pixels (ArcRmlPointer::ToRmlPoint rounds), so the press pixel must lie left of One's box.
		const FVector2f BorderPress(BoxLeft + 0.2f, Open[0].Rect.Position.Y + Open[0].Rect.Size.Y * 0.5f);
		ASSERT_THAT(IsTrue(static_cast<float>(FMath::RoundToInt(BorderPress.X)) < FirstLeft && FirstLeft - BoxLeft >= 1.0f,
			*FString::Printf(TEXT("no border pixel beside One: list left %.1f, One left %.1f"), BoxLeft, FirstLeft)));

		View.ClearCalls();
		View.ClearPlayedSounds();
		ASSERT_THAT(IsTrue(ClickAt(BorderPress)));
		ExpectOneClickStoppedAtTheSelect(TEXT("border press"));
		ExpectListOpen(TEXT("border press"), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);
		ExpectEvents(TEXT("border press"), TEXT("long"), TEXT(""));
		ExpectSelectText(TEXT("border press"), TEXT("sk-long"), TEXT("One"));
		const float BorderScroll = GetListScroll(TEXT("sk-long"));
		if (!FMath::IsNearlyZero(BorderScroll, 0.5f))
		{
			Mismatches += FString::Printf(TEXT("[border press: list scroll %.1f] "), BorderScroll);
		}

		const FArcRmlTestLayoutElement* Track = FindListScrollPart(TEXT("sk-long"), TEXT("slidertrack"));
		ASSERT_THAT(IsNotNull(Track, TEXT("sk-long's open list shows no scrollbar track")));
		const FVector2f TrackPress(Track->Rect.Position.X + Track->Rect.Size.X * 0.5f, Track->Rect.Position.Y + Track->Rect.Size.Y - 3.0f);
		const FArcRmlTestLayoutElement* Thumb = FindListScrollPart(TEXT("sk-long"), TEXT("sliderbar"));
		ASSERT_THAT(IsNotNull(Thumb));
		ASSERT_THAT(IsTrue(TrackPress.Y > Thumb->Rect.Position.Y + Thumb->Rect.Size.Y, TEXT("the press point is not on the track below the thumb")));
		View.ClearCalls();
		View.ClearPlayedSounds();
		ASSERT_THAT(IsTrue(ClickAt(TrackPress)));
		ExpectOneClickStoppedAtTheSelect(TEXT("track press"));
		const float TrackScroll = GetListScroll(TEXT("sk-long"));
		if (TrackScroll <= 0.0f)
		{
			Mismatches += FString::Printf(TEXT("[track press: list scroll %.1f] "), TrackScroll);
		}
		ExpectListOpen(TEXT("track press"), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);
		ExpectEvents(TEXT("track press"), TEXT("long"), TEXT(""));
		ExpectSelectText(TEXT("track press"), TEXT("sk-long"), TEXT("One"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// sk-gone sits in a data-if="keep" row. Its open list is removed with it, once while focused and once after goneoff disabled
	// it; each time presses on the living document follow (the document mousedown a stale listener would take), the host snapshot
	// holds no open list, and a new sk-gone opens and commits. This checks the behaviour only: no build here observes a listener
	// left on the document, and no wheel is sent (a wheel on sk-long's open list keeps later clicks from opening sk-gone; VENDORED 230).
	TEST_METHOD(OpenSelectRemovedByDataIf_LeavesTheDocumentWorking)
	{
		using namespace ArcRmlSelectKeyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TCHAR* const Phases[] = {TEXT("focused"), TEXT("disabled")};
		for (int32 Phase = 0; Phase < static_cast<int32>(UE_ARRAY_COUNT(Phases)); ++Phase)
		{
			const FString Step = Phases[Phase];
			ASSERT_THAT(IsTrue(Click(TEXT("sk-gone"))));
			ExpectListOpen(*(Step + TEXT(": click")), TEXT("sk-gone"), 3);
			if (Phase == 1)
			{
				View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("goneoff")), ArcRmlUITestHelpers::MakeBool(true));
				ASSERT_THAT(IsTrue(Settle()));
			}
			View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("keep")), ArcRmlUITestHelpers::MakeBool(false));
			ASSERT_THAT(IsTrue(Settle()));
			ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("sk-gone")), *(Step + TEXT(": data-if left sk-gone in the tree"))));
			if (View.GetSnapshot().bSelectListOpen)
			{
				Mismatches += FString::Printf(TEXT("[%s: the snapshot's bSelectListOpen is true after the removal] "), *Step);
			}

			ASSERT_THAT(IsTrue(Click(TEXT("sk-before"))));
			ExpectFocus(*(Step + TEXT(": press on sk-before")), TEXT("sk-before"));
			ASSERT_THAT(IsTrue(Click(TEXT("sk-long"))));
			ExpectListOpen(*(Step + TEXT(": click sk-long")), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);
			ASSERT_THAT(IsTrue(Press(EKeys::Escape)));
			ExpectListClosed(*(Step + TEXT(": Escape")), TEXT("sk-long"));

			View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("goneoff")), ArcRmlUITestHelpers::MakeBool(false));
			View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("keep")), ArcRmlUITestHelpers::MakeBool(true));
			ASSERT_THAT(IsTrue(Settle()));
			ASSERT_THAT(IsNotNull(ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("sk-gone")), *(Step + TEXT(": data-if did not bring sk-gone back"))));
		}

		ASSERT_THAT(IsTrue(Click(TEXT("sk-gone"))));
		ExpectListOpen(TEXT("click the new sk-gone"), TEXT("sk-gone"), 3);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Down) && Press(EKeys::Enter)));
		ExpectEvents(TEXT("new sk-gone Down, Enter"), TEXT("gone"), TEXT("In '2', Ch '2'"));
		ExpectListClosed(TEXT("new sk-gone Down, Enter"), TEXT("sk-gone"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// A press on the scrollbar thumb released over an option picks nothing: only a press on an option picks.
	TEST_METHOD(ListThumbPressReleasedOverAnOption_PicksNothing)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-long"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);
		const FArcRmlTestLayoutElement* Thumb = FindListScrollPart(TEXT("sk-long"), TEXT("sliderbar"));
		ASSERT_THAT(IsNotNull(Thumb, TEXT("sk-long's open list shows no scrollbar thumb")));
		const FVector2f Grab = Thumb->Rect.Position + Thumb->Rect.Size * 0.5f;
		const FVector2f Two = GetOpenOptionCentre(TEXT("sk-long"), 1);
		View.ClearCalls();
		View.MouseMove(Grab);
		View.MouseDown(Grab);
		ASSERT_THAT(IsTrue(Settle()));
		View.MouseMove(Two);
		ASSERT_THAT(IsTrue(Settle()));
		View.MouseUp(Two);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("thumb press released over Two"), TEXT("long"), TEXT(""));
		ExpectSelectText(TEXT("thumb press released over Two"), TEXT("sk-long"), TEXT("One"));
		ExpectListOpen(TEXT("thumb press released over Two"), TEXT("sk-long"), ArcRmlSelectKeyTestsPrivate::LongOptions);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// The pointer rests where Date shows once the list opens; Enter opens it from the keyboard and the highlight stays on the
	// selected Apple (a hover refresh under a parked pointer is no move), so Down reaches Cherry and Enter picks it.
	TEST_METHOD(ListOpenedUnderAParkedPointer_KeepsTheKeyboardHighlight)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-sel"), 4);
		const FVector2f Date = GetOpenOptionCentre(TEXT("sk-sel"), 3);
		ASSERT_THAT(IsTrue(Press(EKeys::Escape)));
		ExpectListClosed(TEXT("Escape"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(View.IsOpen(), Mismatches));
		View.MouseMove(Date);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectFocus(TEXT("pointer parked"), TEXT("sk-sel"));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectListOpen(TEXT("Enter under the parked pointer"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Enter, Down, Enter under the parked pointer"), TEXT("sel"), TEXT("In 'c', Ch 'c'"));
		ExpectSelectText(TEXT("Enter, Down, Enter under the parked pointer"), TEXT("sk-sel"), TEXT("Cherry"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// A list opened from navigation focus survives a mouse move (which ends navigation focus); the move highlights the option
	// under the pointer, then Enter picks it; a second keyboard-opened list is finished by a click on an option.
	TEST_METHOD(KeyboardOpenedList_SurvivesAMouseMove_AndIsFinishedByKeysOrMouse)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-sel"))));
		const FVector2f Cherry = GetOpenOptionCentre(TEXT("sk-sel"), 2);
		const FVector2f Date = GetOpenOptionCentre(TEXT("sk-sel"), 3);
		ASSERT_THAT(IsTrue(Press(EKeys::Escape) && Click(TEXT("sk-before")) && Press(EKeys::Down)));
		ExpectFocus(TEXT("Down to sk-sel"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectListOpen(TEXT("Enter"), TEXT("sk-sel"), 4);
		View.ClearCalls();
		View.MouseMove(Cherry);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectListOpen(TEXT("Enter, mouse move to Cherry"), TEXT("sk-sel"), 4);
		ExpectFocus(TEXT("Enter, mouse move to Cherry"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectEvents(TEXT("Enter, mouse move to Cherry, Enter"), TEXT("sel"), TEXT("In 'c', Ch 'c'"));
		ExpectListClosed(TEXT("Enter, mouse move to Cherry, Enter"), TEXT("sk-sel"));

		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Up) && Press(EKeys::Down)));
		ExpectFocus(TEXT("Up, Down back to sk-sel"), TEXT("sk-sel"));
		ASSERT_THAT(IsTrue(Press(EKeys::Enter)));
		ExpectListOpen(TEXT("Enter again"), TEXT("sk-sel"), 4);
		View.MouseMove(Date);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectListOpen(TEXT("Enter, mouse move to Date"), TEXT("sk-sel"), 4);
		ASSERT_THAT(IsTrue(ClickAt(Date)));
		ExpectEvents(TEXT("Enter, mouse move to Date, click"), TEXT("sel"), TEXT("In 'd', Ch 'd'"));
		ExpectSelectText(TEXT("Enter, mouse move to Date, click"), TEXT("sk-sel"), TEXT("Date"));
		ExpectListClosed(TEXT("Enter, mouse move to Date, click"), TEXT("sk-sel"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// sk-hide's Two (inline style), Three (class rule) and Four (data-style-display) are display: none: the open list shows One and
	// Five, and Down from One reaches Five.
	TEST_METHOD(AuthorDisplayNone_HidesTheOptionFromTheListAndTheKeys)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(5, GetOptions(TEXT("sk-hide"), false).Num(), TEXT("sk-hide has five options")));
		ASSERT_THAT(IsTrue(Click(TEXT("sk-hide"))));
		ExpectListOpen(TEXT("click"), TEXT("sk-hide"), 2);
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Down) && Press(EKeys::Enter)));
		ExpectEvents(TEXT("Down, Enter"), TEXT("hide"), TEXT("In '5', Ch '5'"));
		ExpectSelectText(TEXT("Down, Enter"), TEXT("sk-hide"), TEXT("Five"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};
