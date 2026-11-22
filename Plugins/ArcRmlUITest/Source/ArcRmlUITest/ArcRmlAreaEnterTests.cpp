// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "MVVM/ArcRmlModelBinding.h"

namespace ArcRmlAreaEnterTestsPrivate
{
	void SetPreviewHidden(bool bHidden)
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PreviewHidden")))
		{
			Variable->Set(bHidden ? 1 : 0, ECVF_SetByCode);
		}
	}
}

TEST_CLASS(ArcRmlAreaEnterTests, "ArcRmlUI.AreaEnter")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlAreaEnterTestsPrivate::SetPreviewHidden(false);
	}

	bool OpenTabWithPad(const FString& RelativePath, bool bResolved)
	{
		ArcRmlAreaEnterTestsPrivate::SetPreviewHidden(!bResolved);
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		if (!FArcRmlTestView::DropCompiledDocument(Path) || !ArcRmlUITestHelpers::OpenView(View, Path) || !View.Flush() || !View.GetProbe().bHasDocument)
		{
			return false;
		}
		View.SetInputType(ECommonInputType::Gamepad);
		return View.Flush() && View.Flush();
	}

	void Resolve(const TCHAR* ModelName)
	{
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("ArcResolved")), ArcRmlUITestHelpers::MakeBool(true));
	}

	void WaitSeconds(double Seconds, TFunction<void()>&& Then)
	{
		const double Until = FPlatformTime::Seconds() + Seconds;
		TestCommandBuilder
			.Until(TEXT("time passes"), [this, Until]() -> bool
			{
				View.Flush();
				return FPlatformTime::Seconds() > Until;
			}, FTimespan::FromSeconds(Seconds + 10.0))
			.Then(TEXT("check"), MoveTemp(Then));
	}

	TEST_METHOD(HiddenPrimaryArea_IsEnteredOnceItResolves)
	{
		ASSERT_THAT(IsTrue(OpenTabWithPad(TEXT("Areas/HiddenAreaTab.rml"), false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("list")), View.GetProbe().PendingFocusEnterArea, TEXT("the hidden primary area is not being entered")));

		WaitSeconds(2.5, [this]()
		{
			ASSERT_THAT(AreEqual(FString(TEXT("list")), View.GetProbe().PendingFocusEnterArea, TEXT("an unresolved hidden area was given up")));
			Resolve(TEXT("hiddenarea"));
			ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
			ASSERT_THAT(AreEqual(FString(TEXT("ha-row-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty()));
		});
	}

	TEST_METHOD(ShownPrimaryAreaWithUnresolvedData_WaitsForTheData)
	{
		ASSERT_THAT(IsTrue(OpenTabWithPad(TEXT("Areas/UnresolvedAreaTab.rml"), false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("list")), View.GetProbe().PendingFocusEnterArea));

		WaitSeconds(0.6, [this]()
		{
			ASSERT_THAT(AreEqual(FString(TEXT("list")), View.GetProbe().PendingFocusEnterArea, TEXT("a shown area with unresolved data was judged empty")));
			ASSERT_THAT(AreNotEqual(FString(TEXT("af-tab-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
			Resolve(TEXT("unresolvedarea"));
			ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
			ASSERT_THAT(AreEqual(FString(TEXT("ua-row-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		});
	}

	TEST_METHOD(AbsentOptionalModel_CountsAsResolved)
	{
		ASSERT_THAT(IsTrue(OpenTabWithPad(TEXT("Areas/OptionalAreaTab.rml"), true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), TEXT("an area bound to an absent optional model waits for data that never comes")));
		ASSERT_THAT(AreEqual(FString(TEXT("af-tab-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(ConfirmWhileEnteringArea_DropsTheEnterAndIsHandled)
	{
		ASSERT_THAT(IsTrue(OpenTabWithPad(TEXT("Areas/UnresolvedAreaTab.rml"), false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("list")), View.GetProbe().PendingFocusEnterArea));

		View.GamepadAcceptDown();
		View.GamepadAcceptUp();
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), TEXT("the confirm was swallowed by the pending area enter")));
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::GetNavFocusDomId(View).IsEmpty(), TEXT("the confirm did not recover navigation focus")));
	}

	TEST_METHOD(NavigationWhileEnteringArea_DropsTheEnterAndMovesFocus)
	{
		ASSERT_THAT(IsTrue(OpenTabWithPad(TEXT("Areas/UnresolvedAreaTab.rml"), false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("list")), View.GetProbe().PendingFocusEnterArea));

		View.PressKey(EKeys::Gamepad_DPad_Up);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), TEXT("navigation left the area enter pending")));
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::GetNavFocusDomId(View).IsEmpty(), TEXT("navigation found no focus")));
		Resolve(TEXT("unresolvedarea"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreNotEqual(FString(TEXT("ua-row-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("a dropped area enter still took focus once its data resolved")));
	}

	bool OpenFieldFrameAndClickTheField()
	{
		const FString TabA = ArcRmlUITestHelpers::MakePath(TEXT("Areas/FieldFrameTabA.rml"));
		if (!FArcRmlTestView::DropCompiledDocument(TabA) || !ArcRmlUITestHelpers::OpenView(View, TabA) || !View.Flush() || !View.GetProbe().bHasDocument)
		{
			return false;
		}
		View.ClickElement(TEXT("#ff-field"));
		return View.Flush() && View.Flush();
	}

	TEST_METHOD(SpaceAndEnterInATextField_KeepFocusInTheField)
	{
		const FString TabC = ArcRmlUITestHelpers::MakePath(TEXT("Areas/FieldFrameTabC.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(TabC)));
		ASSERT_THAT(IsTrue(OpenFieldFrameAndClickTheField(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("ff-field")), ArcRmlUITestHelpers::GetFocusDomId(View)));

		ArcRmlAreaEnterTestsPrivate::SetPreviewHidden(true);
		ASSERT_THAT(IsTrue(View.Embed(TabC), TEXT("the frame did not accept tab C")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("list")), View.GetProbe().PendingFocusEnterArea, TEXT("the tab swap did not start entering the unresolved primary area")));
		ASSERT_THAT(AreEqual(FString(TEXT("ff-field")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(IsTrue(View.GetSnapshot().TextField.bActive, TEXT("the field did not keep text input over the tab swap")));

		Resolve(TEXT("fieldtabc"));
		View.TypeText(TEXT(" "));
		View.PressKey(EKeys::SpaceBar);
		View.PressKey(EKeys::Enter);
		for (int32 Frame = 0; Frame < 3; ++Frame)
		{
			ASSERT_THAT(IsTrue(View.Flush()));
			ASSERT_THAT(AreEqual(FString(TEXT("ff-field")), ArcRmlUITestHelpers::GetFocusDomId(View),
				FString::Printf(TEXT("frame %d: the area enter took focus from the text field being typed into"), Frame)));
			ASSERT_THAT(IsTrue(View.GetSnapshot().TextField.bActive, FString::Printf(TEXT("frame %d: typing into the text field ended text input"), Frame)));
		}
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), TEXT("typing into the text field left the area enter pending")));
		ASSERT_THAT(AreEqual(0, View.GetCalls().Num(), TEXT("a key typed into the text field activated the area row")));
	}

	TEST_METHOD(TabSwapWhileTyping_KeepsFocusInTheFrameField)
	{
		const FString TabB = ArcRmlUITestHelpers::MakePath(TEXT("Areas/FieldFrameTabB.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(TabB)));
		ASSERT_THAT(IsTrue(OpenFieldFrameAndClickTheField(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("ff-field")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(IsTrue(View.GetSnapshot().TextField.bActive, TEXT("the frame field did not take text input")));

		ASSERT_THAT(IsTrue(View.Embed(TabB), TEXT("the frame did not accept tab B")));
		View.TypeText(TEXT("a"));
		for (int32 Frame = 0; Frame < 3; ++Frame)
		{
			ASSERT_THAT(IsTrue(View.Flush()));
			ASSERT_THAT(AreEqual(FString(TEXT("ff-field")), ArcRmlUITestHelpers::GetFocusDomId(View),
				FString::Printf(TEXT("frame %d: the tab swap moved focus out of the field being typed into"), Frame)));
		}
		ASSERT_THAT(AreEqual(TabB, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetSnapshot().TextField.bActive, TEXT("the tab swap ended text input")));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), TEXT("the tab swap left its area enter pending")));
	}

	TEST_METHOD(AwaitedViewModels_AreTheRequiredOnesWithAPlayer)
	{
		FArcRmlViewModelDeclaration Optional;
		Optional.bOptional = true;
		const FArcRmlViewModelDeclaration Required;
		ASSERT_THAT(IsFalse(FArcRmlModelBinding::AwaitsViewModel(Optional, true), TEXT("an absent optional view model holds an area enter")));
		ASSERT_THAT(IsTrue(FArcRmlModelBinding::AwaitsViewModel(Required, true), TEXT("a required view model with a player is not waited for")));
		ASSERT_THAT(IsFalse(FArcRmlModelBinding::AwaitsViewModel(Required, false), TEXT("a view model without a player is waited for")));
	}

	TEST_METHOD(ResolvedEmptyPrimaryArea_GoesToTheFrameAtOnce)
	{
		const double Start = FPlatformTime::Seconds();
		ASSERT_THAT(IsTrue(OpenTabWithPad(TEXT("Areas/EmptyAreaTab.rml"), true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), TEXT("a resolved empty area is still being entered")));
		ASSERT_THAT(AreEqual(FString(TEXT("af-tab-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View),
			FString::Printf(TEXT("after %.0f ms"), (FPlatformTime::Seconds() - Start) * 1000.0)));
	}
};
