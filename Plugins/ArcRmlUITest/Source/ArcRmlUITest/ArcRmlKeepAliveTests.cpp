// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlKeptTabTestHelpers.h"
#include "ArcRmlKeptTabTestTypes.h"
#include "CommonInputTypeEnum.h"
#include "Containers/Ticker.h"
#include "CQTest.h"
#include "HAL/PlatformProcess.h"
#include "InputCoreTypes.h"
#include "MVVM/ArcRmlModelBinding.h"
#include "UObject/StrongObjectPtr.h"

namespace ArcRmlKeepAliveTestsPrivate
{
	const TCHAR* const Kept = TEXT("KeepAlive/Kept.rml");
	const TCHAR* const Plain = TEXT("KeepAlive/Plain.rml");
	const TCHAR* const Frame = TEXT("KeepAlive/KFrame.rml");
	const TCHAR* const TabA = TEXT("KeepAlive/KTabA.rml");
	const TCHAR* const TabB = TEXT("KeepAlive/KTabB.rml");

	FArcRmlInputBindings MakeBindings()
	{
		FArcRmlInputBindings Bindings;
		Bindings.Confirm.Name = TEXT("confirm");
		Bindings.Confirm.ActionName = TEXT("IA_TestConfirm");
		Bindings.Confirm.Keys.Add(EKeys::Enter);
		Bindings.Back.Name = TEXT("back");
		Bindings.Back.ActionName = TEXT("IA_TestBack");
		Bindings.Back.Keys.Add(EKeys::Escape);
		Bindings.CloseKeys.Add(EKeys::Tab);
		return Bindings;
	}

	struct FDrawResult
	{
		int32 DrawCalls = 0;
		int32 RhiCommands = 0;
	};

	bool DrawSteady(FArcRmlTestView& View, FDrawResult& Out)
	{
		if (!View.DrawFrame() || !View.DrawFrame())
		{
			return false;
		}
		Out.DrawCalls = View.GetDrawGpuDrawCalls();
		Out.RhiCommands = View.GetDrawGpuRhiCommands();
		return true;
	}
}

TEST_CLASS(ArcRmlKeepAliveTests, "ArcRmlUI.KeepAlive")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;
	bool bFrozenClock = false;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.Clock live"));
		bFrozenClock = false;
	}

	void FreezeClock()
	{
		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.Clock 1000"));
		bFrozenClock = true;
	}

	bool Read()
	{
		return View.Flush() && View.ParseLayout(Elements);
	}

	bool Settle()
	{
		return ArcRmlKeptTabTestHelpers::SettleFrames(View) && Read();
	}

	bool OpenDocument(const TCHAR* Relative)
	{
		View.SetInputBindings(ArcRmlKeepAliveTestsPrivate::MakeBindings());
		return Assert.IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(Relative)), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View))
			&& Assert.IsTrue(View.FlushUntilSettled() && View.Flush(), ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Read();
	}

	bool Reopen(const TCHAR* Relative)
	{
		return Assert.IsTrue(View.Open(ArcRmlUITestHelpers::MakePath(Relative), ArcRmlUITestHelpers::ViewSize, 1.0f), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View))
			&& Settle();
	}

	bool CloseAndWait(bool bBack)
	{
		if (bBack)
		{
			View.Back();
		}
		else
		{
			View.PressKey(EKeys::Tab);
		}
		if (!Assert.IsTrue(View.Flush() && View.GetProbe().bLeaving, TEXT("the close did not start the leave")))
		{
			return false;
		}
		for (int32 Attempt = 0; Attempt < 300; ++Attempt)
		{
			if (bFrozenClock)
			{
				ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.ClockAdvance 0.05"));
			}
			FPlatformProcess::Sleep(0.02f);
			if (!View.Flush())
			{
				return false;
			}
			if (View.IsParked() || View.GetProbe().bRemoved)
			{
				return true;
			}
		}
		return Assert.IsTrue(false, TEXT("the leave never ended"));
	}

	bool EndAnimations()
	{
		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.ClockAdvance 5"));
		uint64 Serial = 0;
		for (int32 Attempt = 0; Attempt < 50; ++Attempt)
		{
			FPlatformProcess::Sleep(0.1f);
			if (!View.Flush())
			{
				return false;
			}
			if (View.GetProbe().FrameSerial == Serial)
			{
				return true;
			}
			Serial = View.GetProbe().FrameSerial;
		}
		return Assert.IsTrue(false, TEXT("the view still records a frame on every pump after its animations ended"));
	}

	bool Park(const TCHAR* Relative, bool bBack = false)
	{
		return OpenDocument(Relative) && CloseAndWait(bBack) && Assert.IsTrue(View.IsParked(), TEXT("the close did not park the view"));
	}

	void BindKeptModel(UArcRmlKeptTabViewModel& Model)
	{
		View.SwapWith(ArcRmlUITestHelpers::MakePath(ArcRmlKeepAliveTestsPrivate::Kept), [&Model](const TSharedRef<FArcRmlUIThread>&, uint32, FArcRmlModelBinding& FrameBinding,
			FArcRmlTabModels&) -> bool
		{
			FrameBinding.SetViewModelObject(TEXT("kept"), &Model);
			return false;
		});
		FTSTicker::GetCoreTicker().Tick(0.0f);
	}

	bool ExpectColdOpen(const TCHAR* Relative, uint32 OldViewId)
	{
		if (!Assert.IsTrue(View.Open(ArcRmlUITestHelpers::MakePath(Relative), ArcRmlUITestHelpers::ViewSize, 1.0f), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View))
			|| !Assert.IsTrue(View.FlushUntilSettled() && View.Flush()))
		{
			return false;
		}
		return Assert.IsTrue(View.IsOpen() && !View.IsParked(), TEXT("the open did not leave an open view"))
			&& Assert.IsTrue(View.GetViewId() != OldViewId, TEXT("the open reused the old view"))
			&& Assert.IsTrue(View.GetProbe().bHasDocument, TEXT("the open loaded no document"));
	}

	FString BodyClasses() const
	{
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			if (Element.Tag == TEXT("body"))
			{
				return Element.Classes;
			}
		}
		return FString();
	}

	bool BodyHasClass(const TCHAR* Class) const
	{
		TArray<FString> Names;
		BodyClasses().ParseIntoArrayWS(Names);
		return Names.Contains(Class);
	}

	FVector2f CentreOf(const TCHAR* DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? Element->Rect.Position + Element->Rect.Size * 0.5f : FVector2f::ZeroVector;
	}

	float ScrollOf(const TCHAR* DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? Element->Scroll.Y : -1.0f;
	}

	float WidthOf(const TCHAR* DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? Element->Rect.Size.X : -1.0f;
	}

	bool IsVisible(const TCHAR* DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element && Element->bVisible;
	}

	TEST_METHOD(DocumentWithoutTheMeta_ClosesAsToday_NotParkedAndTheNextOpenInstancesAgain)
	{
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Plain)));
		const uint32 OldViewId = View.GetViewId();
		ASSERT_THAT(IsTrue(CloseAndWait(false)));
		ASSERT_THAT(IsFalse(View.IsParked(), TEXT("a document without the keep-alive meta was parked")));
		ASSERT_THAT(IsTrue(View.GetProbe().bRemoved, TEXT("a document without the keep-alive meta was not removed")));
		ASSERT_THAT(IsFalse(View.IsOpen()));
		ASSERT_THAT(IsTrue(ExpectColdOpen(ArcRmlKeepAliveTestsPrivate::Plain, OldViewId)));
	}

	TEST_METHOD(CloseOfAKeepAliveDocument_ParksItNotOpenAndNotRemoved)
	{
		ASSERT_THAT(IsTrue(Park(ArcRmlKeepAliveTestsPrivate::Kept)));
		ASSERT_THAT(IsFalse(View.IsOpen(), TEXT("a parked view reports open")));
		ASSERT_THAT(IsFalse(View.GetProbe().bRemoved, TEXT("a parked view was removed")));
		ASSERT_THAT(IsFalse(View.GetProbe().bLeaving, TEXT("a parked view is still leaving")));
		ASSERT_THAT(IsTrue(View.GetViewId() != 0));
	}

	TEST_METHOD(Reopen_LoadsNothingAndKeepsTheView)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture ColdLog;
		const FString LoadedLine = FString::Printf(TEXT("loaded '%s'"), *ArcRmlUITestHelpers::MakePath(ArcRmlKeepAliveTestsPrivate::Kept));
		const FString ReopenedLine = FString::Printf(TEXT("reopened '%s' without loading it"), *ArcRmlUITestHelpers::MakePath(ArcRmlKeepAliveTestsPrivate::Kept));
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		ASSERT_THAT(AreEqual(1, ColdLog.CountContaining(*LoadedLine), ColdLog.Describe()));
		const uint32 ViewId = View.GetViewId();
		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ArcRmlKeptTabTestHelpers::FLogCapture ReopenLog;
		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::Kept)));
		ASSERT_THAT(AreEqual(0, ReopenLog.CountContaining(*LoadedLine), ReopenLog.Describe()));
		ASSERT_THAT(AreEqual(1, ReopenLog.CountContaining(*ReopenedLine), ReopenLog.Describe()));
		ASSERT_THAT(IsTrue(View.IsOpen() && !View.IsParked()));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId(), TEXT("the reopen made a new view")));
		ASSERT_THAT(IsTrue(IsVisible(TEXT("kk-count")), TEXT("the reopened document is not visible")));
	}

	TEST_METHOD(Reopen_KeepsDataScrollAndFieldText)
	{
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		BindKeptModel(*Model);
		Model->SetCount(7);
		FTSTicker::GetCoreTicker().Tick(0.0f);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("7")), ArcRmlUITestHelpers::GetText(Elements, TEXT("kk-count")), TEXT("the model value does not show, so the probe is not valid")));

		const FVector2f ScrollPoint = CentreOf(TEXT("kk-scroll"));
		View.MouseMove(ScrollPoint);
		ASSERT_THAT(IsTrue(View.Flush()));
		View.MouseWheel(ScrollPoint, -1.0f);
		float ScrollBefore = 0.0f;
		float Previous = -1.0f;
		int32 Stable = 0;
		for (int32 Attempt = 0; Attempt < 200 && Stable < 5; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			ASSERT_THAT(IsTrue(Read()));
			ScrollBefore = ScrollOf(TEXT("kk-scroll"));
			Stable = FMath::IsNearlyEqual(ScrollBefore, Previous, 0.01f) ? Stable + 1 : 0;
			Previous = ScrollBefore;
		}
		ASSERT_THAT(IsTrue(ScrollBefore > 10.0f, FString::Printf(TEXT("the wheel did not scroll, scroll %g"), ScrollBefore)));

		View.ClickElement(TEXT("#kk-field"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		View.TypeText(TEXT("abc"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("abc")), View.GetSnapshot().TextField.Text, TEXT("the field did not take the text")));
		View.ClickElement(TEXT("#kk-count"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(IsFalse(View.GetSnapshot().TextField.bActive, TEXT("the field stayed active, so the close key would be typed")));

		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));
		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::Kept)));
		ASSERT_THAT(AreEqual(FString(TEXT("7")), ArcRmlUITestHelpers::GetText(Elements, TEXT("kk-count")), TEXT("the data did not survive the reopen")));
		ASSERT_THAT(IsNear(ScrollBefore, ScrollOf(TEXT("kk-scroll")), 0.5f, TEXT("the scroll offset did not survive the reopen")));
		View.ClickElement(TEXT("#kk-field"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("abc")), View.GetSnapshot().TextField.Text, TEXT("the field text did not survive the reopen")));
	}

	TEST_METHOD(Reopen_RestartsFocusAsAColdOpenDoes)
	{
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		View.SetInputType(ECommonInputType::Gamepad);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("kk-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("the cold open did not enter the primary area")));
		View.PressKey(EKeys::Gamepad_DPad_Down);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("kk-btn2")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("the pad did not move the focus, so the probe is not valid")));

		ASSERT_THAT(IsTrue(CloseAndWait(true) && View.IsParked()));
		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::Kept)));
		ASSERT_THAT(AreEqual(FString(TEXT("kk-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("the reopen kept the focus instead of entering the primary area again")));
	}

	TEST_METHOD(Reopen_SwapsLeaveForEnterAndRestartsTheEnterAnimation)
	{
		FreezeClock();
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.ClockAdvance 2.0"));
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(IsTrue(BodyHasClass(TEXT("enter")) && !BodyHasClass(TEXT("leave"))));
		ASSERT_THAT(IsNear(200.0f, WidthOf(TEXT("kk-anim")), 1.0f, TEXT("the enter animation does not run, so the probe is not valid")));

		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));
		ASSERT_THAT(IsTrue(View.Flush() && Read()));
		ASSERT_THAT(IsTrue(BodyHasClass(TEXT("leave")) && !BodyHasClass(TEXT("enter")), FString::Printf(TEXT("the parked root has classes '%s'"), *BodyClasses())));

		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::Kept)));
		ASSERT_THAT(IsTrue(BodyHasClass(TEXT("enter")), FString::Printf(TEXT("the reopened root has classes '%s'"), *BodyClasses())));
		ASSERT_THAT(IsFalse(BodyHasClass(TEXT("leave")), FString::Printf(TEXT("the reopened root keeps leave, classes '%s'"), *BodyClasses())));
		ASSERT_THAT(IsNear(100.0f, WidthOf(TEXT("kk-anim")), 1.0f, TEXT("the enter animation did not restart")));
		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.ClockAdvance 1.0"));
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(IsNear(150.0f, WidthOf(TEXT("kk-anim")), 1.0f, TEXT("the restarted enter animation does not run from the reopen")));
	}

	TEST_METHOD(ModelChangeWhileParked_AppearsAfterReopenAndNothingChangesBefore)
	{
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		BindKeptModel(*Model);
		Model->SetCount(2);
		FTSTicker::GetCoreTicker().Tick(0.0f);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("2")), ArcRmlUITestHelpers::GetText(Elements, TEXT("kk-count")), TEXT("a change of the open view does not show, so the probe is not valid")));

		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		Model->SetCount(9);
		FTSTicker::GetCoreTicker().Tick(0.0f);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::DataViewsUpdated), TEXT("a data view was updated while the view is parked")));

		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::Kept)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::DataViewsUpdated) > 0, TEXT("the reopen updated no data view, so the counter is not measuring")));
		ASSERT_THAT(AreEqual(FString(TEXT("9")), ArcRmlUITestHelpers::GetText(Elements, TEXT("kk-count")), TEXT("the change made while parked did not appear after the reopen")));
	}

	TEST_METHOD(OnClose_RunsOnceAtParkAndNotAgainWhenTheParkedViewIsDestroyed)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		BindKeptModel(*Model);
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(0, Log.CountContaining(TEXT("call('', 'OnLeftA') on 'kept'")), TEXT("on-close ran while the view is open")));
		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("call('', 'OnLeftA') on 'kept'")), Log.Describe()));

		View.Discard();
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("call('', 'OnLeftA') on 'kept'")), TEXT("destroying the parked view by a discard ran on-close again")));

		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		BindKeptModel(*Model);
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));
		ASSERT_THAT(AreEqual(2, Log.CountContaining(TEXT("call('', 'OnLeftA') on 'kept'")), Log.Describe()));
		View.Close();
		ASSERT_THAT(AreEqual(2, Log.CountContaining(TEXT("call('', 'OnLeftA') on 'kept'")), TEXT("destroying the parked view by a close ran on-close again")));
	}

	TEST_METHOD(OpenOfAKeptTabOfAParkedFrame_ReopensTheFrameWithOneViewAndTheTabShown)
	{
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Frame)));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(ArcRmlKeptTabTestHelpers::Visit(View, ArcRmlKeepAliveTestsPrivate::TabA) && Read()));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsInstanced) > 0, TEXT("the first visit instanced nothing, so the counter is not measuring")));
		ASSERT_THAT(IsTrue(ArcRmlKeptTabTestHelpers::Visit(View, ArcRmlKeepAliveTestsPrivate::TabB) && Read()));
		const uint32 ViewId = View.GetViewId();
		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::TabA)));
		ASSERT_THAT(IsTrue(View.IsOpen()));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId(), TEXT("opening the tab made a second view")));
		ASSERT_THAT(AreEqual(ArcRmlUITestHelpers::MakePath(ArcRmlKeepAliveTestsPrivate::TabA), View.GetProbe().EmbeddedPath));
		ASSERT_THAT(IsTrue(IsVisible(TEXT("kta-text")) && !IsVisible(TEXT("ktb-text")), TEXT("the requested kept tab is not the shown one")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsInstanced), TEXT("showing the kept tab instanced elements")));
	}

	TEST_METHOD(OpenOfANewTabOfAParkedFrame_ReopensTheFrameWithOneViewAndEmbedsTheTab)
	{
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Frame)));
		ASSERT_THAT(IsTrue(ArcRmlKeptTabTestHelpers::Visit(View, ArcRmlKeepAliveTestsPrivate::TabA) && Read()));
		const uint32 ViewId = View.GetViewId();
		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));

		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::TabB)));
		ASSERT_THAT(IsTrue(View.IsOpen()));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId(), TEXT("opening the tab made a second view")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreEqual(ArcRmlUITestHelpers::MakePath(ArcRmlKeepAliveTestsPrivate::TabB), View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(IsVisible(TEXT("ktb-text")) && !IsVisible(TEXT("kta-text")), TEXT("the requested tab is not the shown one")));
	}

	TEST_METHOD(OpenOfTheFrameItself_ReopensTheParkedFrameWithItsShownTab)
	{
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Frame)));
		ASSERT_THAT(IsTrue(ArcRmlKeptTabTestHelpers::Visit(View, ArcRmlKeepAliveTestsPrivate::TabA) && Read()));
		const uint32 ViewId = View.GetViewId();
		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));
		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::Frame)));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId()));
		ASSERT_THAT(IsTrue(View.IsOpen() && IsVisible(TEXT("kta-text"))));
	}

	TEST_METHOD(ReloadOfAParkedView_DestroysItAndTheNextOpenInstancesAgain)
	{
		ASSERT_THAT(IsTrue(Park(ArcRmlKeepAliveTestsPrivate::Kept)));
		const uint32 OldViewId = View.GetViewId();
		ASSERT_THAT(IsFalse(View.Reload(), TEXT("the reload of a parked view reopened it")));
		ASSERT_THAT(IsFalse(View.IsParked(), TEXT("the reload left the view parked")));
		ASSERT_THAT(IsFalse(View.IsOpen()));
		ASSERT_THAT(IsTrue(ExpectColdOpen(ArcRmlKeepAliveTestsPrivate::Kept, OldViewId)));
	}

	TEST_METHOD(DiscardOfAParkedView_DestroysItAndTheNextOpenInstancesAgain)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		ASSERT_THAT(IsTrue(Park(ArcRmlKeepAliveTestsPrivate::Kept)));
		const uint32 OldViewId = View.GetViewId();
		View.Discard();
		ASSERT_THAT(IsFalse(View.IsParked(), TEXT("the discard left the view parked")));
		ASSERT_THAT(IsFalse(View.IsOpen()));
		ASSERT_THAT(IsTrue(View.Flush() && View.GetProbe().bRemoved, TEXT("the discard did not remove the view")));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("discarded the parked '")), Log.Describe()));
		ASSERT_THAT(IsTrue(ExpectColdOpen(ArcRmlKeepAliveTestsPrivate::Kept, OldViewId)));
	}

	TEST_METHOD(DiscardOfAPathThatIsNotParked_OnlyLogs)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		const uint32 ViewId = View.GetViewId();
		View.Discard();
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.IsOpen() && !View.GetProbe().bRemoved, TEXT("a discard destroyed an open view")));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId()));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("ignored, it is not parked")), Log.Describe()));
		ASSERT_THAT(AreEqual(0, Log.CountContaining(TEXT("discarded the parked")), Log.Describe()));
	}

	TEST_METHOD(DiscardOfAKeepAliveViewStillLeaving_DestroysItInsteadOfParking)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		FreezeClock();
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		BindKeptModel(*Model);
		ASSERT_THAT(IsTrue(View.Flush()));
		const uint32 OldViewId = View.GetViewId();
		View.PressKey(EKeys::Tab);
		ASSERT_THAT(IsTrue(View.Flush() && View.GetProbe().bLeaving, TEXT("the close did not start the leave")));
		ASSERT_THAT(IsFalse(View.IsParked(), TEXT("the view parked before its leave ended")));

		View.Discard();
		for (int32 Attempt = 0; Attempt < 40; ++Attempt)
		{
			ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.ClockAdvance 0.05"));
			FTSTicker::GetCoreTicker().Tick(1.0f);
			ASSERT_THAT(IsTrue(View.Flush()));
		}
		ASSERT_THAT(IsFalse(View.IsParked(), TEXT("a discard during the leave still let the view park")));
		ASSERT_THAT(IsFalse(View.IsOpen()));
		ASSERT_THAT(IsTrue(View.GetProbe().bRemoved, TEXT("a discard during the leave did not destroy the view")));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("call('', 'OnLeftA') on 'kept'")), Log.Describe()));
		ASSERT_THAT(IsTrue(ExpectColdOpen(ArcRmlKeepAliveTestsPrivate::Kept, OldViewId)));
	}

	TEST_METHOD(ReopenedFrame_DrawsAndRhiCommandsEqualAColdOpen)
	{
		FreezeClock();
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ArcRmlKeepAliveTestsPrivate::FDrawResult Cold;
		ArcRmlKeepAliveTestsPrivate::FDrawResult Reopened;
		ASSERT_THAT(IsTrue(OpenDocument(ArcRmlKeepAliveTestsPrivate::Kept)));
		View.SetDestination(FIntRect(0, 0, 1280, 720));
		ASSERT_THAT(IsTrue(Settle() && View.FlushUntilSettled() && EndAnimations()));
		ASSERT_THAT(IsTrue(ArcRmlKeepAliveTestsPrivate::DrawSteady(View, Cold)));
		ASSERT_THAT(IsTrue(Cold.DrawCalls > 0 && Cold.RhiCommands > 0, TEXT("the cold open drew nothing, so the probe is not valid")));
		const int32 ColdRecorded = View.GetDraws().Num();

		ASSERT_THAT(IsTrue(CloseAndWait(false) && View.IsParked()));
		ASSERT_THAT(IsTrue(Reopen(ArcRmlKeepAliveTestsPrivate::Kept) && View.FlushUntilSettled() && EndAnimations()));
		ASSERT_THAT(IsTrue(ArcRmlKeepAliveTestsPrivate::DrawSteady(View, Reopened)));
		const int32 ReopenedRecorded = View.GetDraws().Num();
		const FString Numbers = FString::Printf(TEXT("cold %d/%d recorded %d, reopened %d/%d recorded %d"), Cold.DrawCalls, Cold.RhiCommands, ColdRecorded,
			Reopened.DrawCalls, Reopened.RhiCommands, ReopenedRecorded);
		ASSERT_THAT(AreEqual(ColdRecorded, ReopenedRecorded, Numbers));
		ASSERT_THAT(AreEqual(Cold.DrawCalls, Reopened.DrawCalls, Numbers));
		ASSERT_THAT(AreEqual(Cold.RhiCommands, Reopened.RhiCommands, Numbers));
	}
};
