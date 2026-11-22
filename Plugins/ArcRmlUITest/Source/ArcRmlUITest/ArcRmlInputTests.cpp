// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "Host/ArcRmlKeyLabels.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

namespace ArcRmlInputTestsPrivate
{
	const UInputMappingContext* LoadContext(const TCHAR* ObjectPath)
	{
		return LoadObject<UInputMappingContext>(nullptr, ObjectPath);
	}

	FString DescribeHead(const FArcRmlTestView& View)
	{
		TArray<FString> Lines;
		for (const FArcRmlDiagnostic& Diagnostic : View.GetHeadDiagnostics())
		{
			Lines.Add(FString::Printf(TEXT("%s(%d): %s"), *Diagnostic.File, Diagnostic.Line, *Diagnostic.Message));
		}
		return FString::Printf(TEXT("%d head diagnostics: [%s]"), Lines.Num(), *FString::Join(Lines, TEXT(" | ")));
	}

	int32 CountHeadErrors(const FArcRmlTestView& View, const FString& MessagePart, int32 Line)
	{
		int32 Count = 0;
		for (const FArcRmlDiagnostic& Diagnostic : View.GetHeadDiagnostics())
		{
			if (Diagnostic.Severity == EArcRmlDiagnosticSeverity::Error && Diagnostic.Message.Contains(MessagePart) && Diagnostic.Line == Line)
			{
				++Count;
			}
		}
		return Count;
	}

	float ReadScrollTop(const FArcRmlTestView& View, const FString& DomId)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		if (!View.ParseLayout(Elements))
		{
			return -1.0f;
		}
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? Element->Scroll.Y : -1.0f;
	}

	float WaitForScrollTop(FArcRmlTestView& View, const FString& DomId)
	{
		float ScrollTop = ReadScrollTop(View, DomId);
		for (int32 Attempt = 0; Attempt < 100 && ScrollTop <= 0.0f; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			if (!View.Flush())
			{
				return -1.0f;
			}
			ScrollTop = ReadScrollTop(View, DomId);
		}
		return ScrollTop;
	}

	bool IsShown(const TArray<FArcRmlTestLayoutElement>& Elements, const TCHAR* DomId)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element && Element->bVisible && Element->Rect.Size.Y > 0.0f;
	}

	FString DescribeTabSet(const TArray<FArcRmlTestLayoutElement>& Elements)
	{
		TArray<FString> Parts;
		for (const TCHAR* DomId : {TEXT("t1"), TEXT("t2"), TEXT("t3"), TEXT("p1"), TEXT("p2"), TEXT("p3")})
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
			Parts.Add(Element ? FString::Printf(TEXT("%s shown %d bg %s parent %s"), DomId, IsShown(Elements, DomId), *Element->BackgroundColor, *Element->Parent)
							  : FString::Printf(TEXT("%s missing"), DomId));
		}
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			if (Element.Path.StartsWith(TEXT("0.0.1")) && Element.Path.Len() <= 7)
			{
				Parts.Add(FString::Printf(TEXT("[%s kind %s tag %s id %s]"), *Element.Path, *Element.Kind, *Element.Tag, *Element.DomId));
			}
		}
		return FString::Join(Parts, TEXT("; "));
	}
}

TEST_CLASS(ArcRmlInputTests, "ArcRmlUI.Input")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenAndFlush(const FString& RelativePath)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument;
	}

	FString DescribeCalls() const
	{
		TArray<FString> Parts;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			Parts.Add(FString::Printf(TEXT("%s(%s)"), *Call.FunctionName, Call.Arguments.IsEmpty() ? TEXT("") : *Call.Arguments.Last().String));
		}
		return FString::Printf(TEXT("%d calls: %s"), Parts.Num(), *FString::Join(Parts, TEXT(", ")));
	}

	TEST_METHOD(AnalogCursorAcceptThenAcceptKey_ActivatesOnce)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/EnterConfirmTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetInputType(ECommonInputType::Gamepad);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("ect-bar-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.ClearCalls();

		const FKey Accept = EKeys::Virtual_Gamepad_Accept.GetVirtualKey();
		View.GamepadAcceptDown();
		View.KeyDown(Accept);
		View.KeyUp(Accept);
		View.GamepadAcceptUp();
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Clicked"), TEXT("ect-bar-1")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, View.GetCalls().Num(), DescribeCalls()));
	}

	TEST_METHOD(GameModeAutofocusField_TakesKeyboardFocusAndText)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/GameFocusTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.GetSnapshot().TextField.bActive, TEXT("the autofocus field did not become active")));
		ASSERT_THAT(IsTrue(View.WantsKeyboardFocus(), TEXT("a game-mode view with an active text field must take keyboard focus")));

		View.TypeText(TEXT("abc"));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("abc")), View.GetSnapshot().TextField.Text));
	}

	TEST_METHOD(GameModeFieldBlur_ReturnsKeyboardFocus)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/GameFocusTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.WantsKeyboardFocus(), TEXT("the autofocus field did not take keyboard focus")));

		View.ClickElement(TEXT("#gf-label"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(IsFalse(View.GetSnapshot().TextField.bActive, TEXT("a click outside the field did not blur it")));
		ASSERT_THAT(IsFalse(View.WantsKeyboardFocus(), TEXT("a game-mode view without an active field must leave keyboard focus to the game")));
	}

	TEST_METHOD(ScreenContextMappingBackKey_IsLoadErrorAtMetaLine)
	{
		using namespace ArcRmlInputTestsPrivate;
		const UInputMappingContext* Common = LoadContext(TEXT("/ArcRmlUITest/Input/IMC_ArcRmlTestCommon.IMC_ArcRmlTestCommon"));
		ASSERT_THAT(IsNotNull(Common));
		TArray<FArcRmlAppliedContext> Applied;
		Applied.Add(FArcRmlAppliedContext{Common, 0});
		View.SetAppliedContexts(MoveTemp(Applied));

		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, ArcRmlUITestHelpers::MakePath(TEXT("Input/BackCollisionTest.rml")));
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/BackCollisionTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, CountHeadErrors(View, TEXT("maps Escape to 'IA_ExitCraftnig', which takes it from the CommonUI back action"), 6), DescribeHead(View)));
	}

	TEST_METHOD(ScreenContextAtGameplayPriority_IsLoadError)
	{
		using namespace ArcRmlInputTestsPrivate;
		const UInputMappingContext* AbilityBar = LoadContext(TEXT("/ArcRmlUITest/Input/IMC_AbilityBar.IMC_AbilityBar"));
		ASSERT_THAT(IsNotNull(AbilityBar));
		TArray<FArcRmlAppliedContext> Applied;
		Applied.Add(FArcRmlAppliedContext{AbilityBar, 0});
		View.SetAppliedContexts(MoveTemp(Applied));

		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, ArcRmlUITestHelpers::MakePath(TEXT("Input/PriorityCollisionTest.rml")));
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/PriorityCollisionTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, CountHeadErrors(View, TEXT("at priority 0 maps R to 'IA_UIAlt', but 'IMC_AbilityBar' maps it to 'IA_Weapon_Ability_01' at priority 0"), 6),
			DescribeHead(View)));
	}

	TEST_METHOD(ScreenContextOverContextFilteredByInputMode_IsNoError)
	{
		using namespace ArcRmlInputTestsPrivate;
		const UInputMappingContext* AbilityBar = LoadContext(TEXT("/ArcRmlUITest/Input/IMC_AbilityBar.IMC_AbilityBar"));
		ASSERT_THAT(IsNotNull(AbilityBar));
		ASSERT_THAT(IsTrue(AbilityBar->ShouldFilterMappingByInputMode(), TEXT("IMC_AbilityBar no longer filters by input mode")));
		TArray<FArcRmlAppliedContext> Applied;
		Applied.Add(FArcRmlAppliedContext{AbilityBar, 0});
		View.SetAppliedContexts(MoveTemp(Applied));
		View.SetEnhancedInputMode(FGameplayTagContainer());

		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/PriorityCollisionTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetHeadDiagnostics().Num(), DescribeHead(View)));
	}

	TEST_METHOD(ScreenContextWithSettingsPriority_OutranksGameplay)
	{
		using namespace ArcRmlInputTestsPrivate;
		const UInputMappingContext* AbilityBar = LoadContext(TEXT("/ArcRmlUITest/Input/IMC_AbilityBar.IMC_AbilityBar"));
		ASSERT_THAT(IsNotNull(AbilityBar));
		TArray<FArcRmlAppliedContext> Applied;
		Applied.Add(FArcRmlAppliedContext{AbilityBar, 0});
		View.SetAppliedContexts(MoveTemp(Applied));

		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/DefaultPriorityTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetHeadDiagnostics().Num(), DescribeHead(View)));
	}

	TEST_METHOD(ScreenContextMappingCloseKey_IsLoadError)
	{
		using namespace ArcRmlInputTestsPrivate;
		FArcRmlInputBindings Bindings;
		Bindings.Back.Name = TEXT("back");
		Bindings.Back.Keys.Add(EKeys::Escape);
		Bindings.CloseKeys.Add(EKeys::Q);
		View.SetInputBindings(Bindings);

		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, ArcRmlUITestHelpers::MakePath(TEXT("Input/DefaultPriorityTest.rml")));
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/DefaultPriorityTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, CountHeadErrors(View, TEXT("maps Q to 'IA_UITabPrev', but Q closes this document"), 6), DescribeHead(View)));
	}

	TEST_METHOD(ArcKeysWordOutsideExpressions_BindsNoDocumentModel)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/ArcKeysWordTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Literal = Elements.FindByPredicate([](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.DomId == TEXT("akw-literal");
		});
		ASSERT_THAT(IsNotNull(Literal));
		ASSERT_THAT(IsTrue(Literal->bVisible, FString::Printf(TEXT("the word ArcKeys in plain text and a foreign attribute put the body into the document model; %s"),
			*ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	TEST_METHOD(ArcKeysInDocumentWithoutViewModel_ShowLabels)
	{
		FArcRmlInputBindings Bindings;
		Bindings.Back.Name = TEXT("back");
		Bindings.Back.ActionName = TEXT("IA_TestBack");
		Bindings.Back.Keys.Add(EKeys::Escape);
		View.SetInputBindings(Bindings);

		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/ArcKeysNoModelTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FString Expected = ArcRmlKeyLabels::GetKeyLabel(EKeys::Escape);
		ASSERT_THAT(IsFalse(Expected.IsEmpty()));
		ASSERT_THAT(AreEqual(Expected, ArcRmlUITestHelpers::GetText(Elements, TEXT("ak-back")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("ArcKeys")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(WheelOverRowsWithoutHandlers_ScrollsContainer)
	{
		using namespace ArcRmlInputTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/ScrollTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ReadScrollTop(View, TEXT("sc-list")) == 0.0f, TEXT("the list did not start at the top")));

		const FVector2f RowPoint(200.0f, 130.0f);
		ASSERT_THAT(IsTrue(View.IsInteractiveAt(RowPoint), TEXT("a scrollable list is not a pointer region")));
		View.MouseMove(RowPoint);
		View.MouseWheel(RowPoint, -1.0f);
		ASSERT_THAT(IsTrue(View.Flush()));
		const float ScrollTop = WaitForScrollTop(View, TEXT("sc-list"));
		ASSERT_THAT(IsTrue(ScrollTop > 0.0f, FString::Printf(TEXT("wheel over a row did not scroll the list, scroll top %g"), ScrollTop)));
	}

	TEST_METHOD(HandlerRowClippedByList_IsNoPointerRegion)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/ScrollTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* List = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sc-list"));
		const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sc-row-action"));
		ASSERT_THAT(IsNotNull(List));
		ASSERT_THAT(IsNotNull(Row));
		ASSERT_THAT(IsTrue(Row->Rect.Position.Y >= List->Rect.Position.Y + List->Rect.Size.Y,
			FString::Printf(TEXT("the handler row at y %g is not below the list bottom %g"), Row->Rect.Position.Y, List->Rect.Position.Y + List->Rect.Size.Y)));

		const FVector2f RowCenter(Row->Rect.Position.X + Row->Rect.Size.X * 0.5f, Row->Rect.Position.Y + Row->Rect.Size.Y * 0.5f);
		ASSERT_THAT(IsFalse(View.IsInteractiveAt(RowCenter), FString::Printf(TEXT("the clipped handler row makes %g, %g a pointer region"), RowCenter.X, RowCenter.Y)));
	}

	TEST_METHOD(DragScrollbarThumb_ScrollsContainer)
	{
		using namespace ArcRmlInputTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/ScrollTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Thumb = Elements.FindByPredicate([](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Tag == TEXT("sliderbar") && Element.bVisible;
		});
		ASSERT_THAT(IsNotNull(Thumb, TEXT("the list shows no scrollbar thumb")));

		const FVector2f Start(Thumb->Rect.Position.X + Thumb->Rect.Size.X * 0.5f, Thumb->Rect.Position.Y + Thumb->Rect.Size.Y * 0.5f);
		const FVector2f End = Start + FVector2f(0.0f, 40.0f);
		View.MouseMove(Start);
		View.MouseDown(Start);
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(ReadScrollTop(View, TEXT("sc-list")) == 0.0f, TEXT("the press landed on the track, not the thumb")));
		View.MouseMove(End);
		ASSERT_THAT(IsTrue(View.Flush()));
		View.MouseUp(End);
		ASSERT_THAT(IsTrue(View.Flush()));
		const float ScrollTop = ReadScrollTop(View, TEXT("sc-list"));
		ASSERT_THAT(IsTrue(ScrollTop > 0.0f, FString::Printf(TEXT("dragging the scrollbar thumb did not scroll the list, scroll top %g"), ScrollTop)));
	}

	TEST_METHOD(TabSetWithIndentedMarkup_IndexesTabsAndPanelsByElement)
	{
		using namespace ArcRmlInputTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Input/TabSetTest.rml"))));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));

		const FArcRmlTestLayoutElement* Tabs = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ts-tabs"));
		const FArcRmlTestLayoutElement* Panels = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ts-panels"));
		ASSERT_THAT(IsNotNull(Tabs));
		ASSERT_THAT(IsNotNull(Panels));
		int32 TabContainers = 0;
		int32 TabCount = 0;
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			TabContainers += (Element.Kind == TEXT("element") && Element.Tag == TEXT("tabs")) ? 1 : 0;
			TabCount += (Element.Kind == TEXT("element") && Element.Tag == TEXT("tab") && Element.Parent == Tabs->Path) ? 1 : 0;
		}
		ASSERT_THAT(AreEqual(1, TabContainers, FString::Printf(TEXT("tabs containers; %s"), *DescribeTabSet(Elements))));
		ASSERT_THAT(AreEqual(3, TabCount, FString::Printf(TEXT("tabs in the authored container; %s"), *DescribeTabSet(Elements))));
		ASSERT_THAT(IsTrue(IsShown(Elements, TEXT("p1")) && !IsShown(Elements, TEXT("p2")) && !IsShown(Elements, TEXT("p3")), DescribeTabSet(Elements)));
		const FString SelectedColor = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("t1"))->BackgroundColor;
		ASSERT_THAT(IsTrue(SelectedColor != ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("t2"))->BackgroundColor, DescribeTabSet(Elements)));

		View.ClickElement(TEXT("#t2"));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		ASSERT_THAT(IsTrue(!IsShown(Elements, TEXT("p1")) && IsShown(Elements, TEXT("p2")) && !IsShown(Elements, TEXT("p3")), DescribeTabSet(Elements)));
		ASSERT_THAT(AreEqual(SelectedColor, ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("t2"))->BackgroundColor, DescribeTabSet(Elements)));
		ASSERT_THAT(IsTrue(SelectedColor != ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("t1"))->BackgroundColor, DescribeTabSet(Elements)));

		View.ClickElement(TEXT("#t3"));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		ASSERT_THAT(IsTrue(!IsShown(Elements, TEXT("p1")) && !IsShown(Elements, TEXT("p2")) && IsShown(Elements, TEXT("p3")), DescribeTabSet(Elements)));
	}
};
