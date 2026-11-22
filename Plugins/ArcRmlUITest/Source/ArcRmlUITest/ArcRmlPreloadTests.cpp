// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlKeptTabTestHelpers.h"
#include "ArcRmlKeptTabTestTypes.h"
#include "Containers/Ticker.h"
#include "CQTest.h"
#include "HAL/PlatformProcess.h"
#include "InputCoreTypes.h"
#include "MVVM/ArcRmlModelBinding.h"
#include "UObject/StrongObjectPtr.h"

namespace ArcRmlPreloadTestsPrivate
{
	const TCHAR* const Model = TEXT("Preload/PModel.rml");
	const TCHAR* const Frame = TEXT("Preload/PFrame.rml");
	const TCHAR* const TabA = TEXT("Preload/PTabA.rml");
	const TCHAR* const TabB = TEXT("Preload/PTabB.rml");
	const TCHAR* const Missing = TEXT("Preload/Missing.rml");
	const TCHAR* const OnCloseCall = TEXT("call('', 'OnLeftA') on 'pm'");

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

	FString LoadedLine(const TCHAR* Relative)
	{
		return FString::Printf(TEXT(" loaded '%s'"), *ArcRmlUITestHelpers::MakePath(Relative));
	}

	FString ReopenedLine(const TCHAR* Relative)
	{
		return FString::Printf(TEXT("reopened '%s' without loading it"), *ArcRmlUITestHelpers::MakePath(Relative));
	}

	FString FirstFrameLine(uint32 ViewId, const TCHAR* Relative)
	{
		return FString::Printf(TEXT("view %u '%s' first frame recorded"), ViewId, *ArcRmlUITestHelpers::MakePath(Relative));
	}

	FString ParkedLine(uint32 ViewId, const TCHAR* Relative)
	{
		return FString::Printf(TEXT("view %u parked '%s'"), ViewId, *ArcRmlUITestHelpers::MakePath(Relative));
	}
}

TEST_CLASS(ArcRmlPreloadTests, "ArcRmlUI.Preload")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool Read()
	{
		return View.Flush() && View.ParseLayout(Elements);
	}

	bool Settle()
	{
		return ArcRmlKeptTabTestHelpers::SettleFrames(View) && Read();
	}

	bool PreloadAndWait(const TArray<FString>& Paths)
	{
		View.SetInputBindings(ArcRmlPreloadTestsPrivate::MakeBindings());
		if (!Assert.IsTrue(View.Preload(Paths, ArcRmlUITestHelpers::ViewSize, 1.0f), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)))
		{
			return false;
		}
		for (int32 Attempt = 0; Attempt < 500; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			if (!View.Flush())
			{
				return false;
			}
			if (!View.IsPreloading())
			{
				return Assert.IsTrue(View.IsParked(), TEXT("the preload ended without a parked view"));
			}
		}
		return Assert.IsTrue(false, TEXT("the preload never ended"));
	}

	bool PreloadOne(const TCHAR* Relative)
	{
		TArray<FString> Paths;
		Paths.Add(ArcRmlUITestHelpers::MakePath(Relative));
		return PreloadAndWait(Paths);
	}

	bool OpenByPath(const TCHAR* Relative)
	{
		return Assert.IsTrue(View.Open(ArcRmlUITestHelpers::MakePath(Relative), ArcRmlUITestHelpers::ViewSize, 1.0f), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View))
			&& Settle();
	}

	bool ColdOpen(const TCHAR* Relative)
	{
		View.SetInputBindings(ArcRmlPreloadTestsPrivate::MakeBindings());
		return Assert.IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(Relative)), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View))
			&& Assert.IsTrue(View.FlushUntilSettled() && View.Flush(), ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Read();
	}

	bool CloseAndWait()
	{
		View.PressKey(EKeys::Tab);
		if (!Assert.IsTrue(View.Flush() && View.GetProbe().bLeaving, TEXT("the close did not start the leave")))
		{
			return false;
		}
		for (int32 Attempt = 0; Attempt < 300; ++Attempt)
		{
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

	void BindModel(UArcRmlKeptTabViewModel& ViewModel)
	{
		View.SwapWith(ArcRmlUITestHelpers::MakePath(ArcRmlPreloadTestsPrivate::Model), [&ViewModel](const TSharedRef<FArcRmlUIThread>&, uint32, FArcRmlModelBinding& FrameBinding,
			FArcRmlTabModels&) -> bool
		{
			FrameBinding.SetViewModelObject(TEXT("pm"), &ViewModel);
			return false;
		});
		FTSTicker::GetCoreTicker().Tick(0.0f);
	}

	bool IsVisible(const TCHAR* DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element && Element->bVisible;
	}

	TEST_METHOD(Preload_RendersOneFrameBeforeParking_AndTheFirstOpenLoadsNothing)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture PreloadLog;
		ASSERT_THAT(IsTrue(PreloadOne(ArcRmlPreloadTestsPrivate::Model)));
		const uint32 ViewId = View.GetViewId();
		ASSERT_THAT(IsFalse(View.IsOpen(), TEXT("a preloaded view reports open")));
		ASSERT_THAT(IsFalse(View.GetProbe().bRemoved, TEXT("the preloaded view was removed")));
		ASSERT_THAT(AreEqual(1, PreloadLog.CountContaining(*ArcRmlPreloadTestsPrivate::LoadedLine(ArcRmlPreloadTestsPrivate::Model)), PreloadLog.Describe()));
		ASSERT_THAT(AreEqual(1, PreloadLog.CountContaining(*ArcRmlPreloadTestsPrivate::FirstFrameLine(ViewId, ArcRmlPreloadTestsPrivate::Model)), PreloadLog.Describe()));
		ASSERT_THAT(AreEqual(1, PreloadLog.CountContaining(*ArcRmlPreloadTestsPrivate::ParkedLine(ViewId, ArcRmlPreloadTestsPrivate::Model)), PreloadLog.Describe()));
		const FString LogText = PreloadLog.Describe();
		const int32 FrameAt = LogText.Find(ArcRmlPreloadTestsPrivate::FirstFrameLine(ViewId, ArcRmlPreloadTestsPrivate::Model));
		const int32 ParkedAt = LogText.Find(ArcRmlPreloadTestsPrivate::ParkedLine(ViewId, ArcRmlPreloadTestsPrivate::Model));
		ASSERT_THAT(IsTrue(FrameAt != INDEX_NONE && FrameAt < ParkedAt, TEXT("the view parked before its first frame was recorded")));

		ArcRmlKeptTabTestHelpers::FLogCapture OpenLog;
		ASSERT_THAT(IsTrue(OpenByPath(ArcRmlPreloadTestsPrivate::Model)));
		ASSERT_THAT(AreEqual(0, OpenLog.CountContaining(*ArcRmlPreloadTestsPrivate::LoadedLine(ArcRmlPreloadTestsPrivate::Model)), OpenLog.Describe()));
		ASSERT_THAT(AreEqual(1, OpenLog.CountContaining(*ArcRmlPreloadTestsPrivate::ReopenedLine(ArcRmlPreloadTestsPrivate::Model)), OpenLog.Describe()));
		ASSERT_THAT(IsTrue(View.IsOpen() && !View.IsParked()));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId(), TEXT("the first open made a new view")));
		ASSERT_THAT(IsTrue(IsVisible(TEXT("pm-count")), TEXT("the opened preloaded document is not visible")));
	}

	TEST_METHOD(CloseOfAPreloadedDocumentWithoutTheMeta_RunsOnCloseOnceAndParks)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> ViewModel(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsTrue(PreloadOne(ArcRmlPreloadTestsPrivate::Model)));
		const uint32 ViewId = View.GetViewId();
		ASSERT_THAT(IsTrue(OpenByPath(ArcRmlPreloadTestsPrivate::Model)));
		BindModel(*ViewModel);
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(0, Log.CountContaining(ArcRmlPreloadTestsPrivate::OnCloseCall), TEXT("on-close ran before the close")));

		ASSERT_THAT(IsTrue(CloseAndWait()));
		ASSERT_THAT(IsTrue(View.IsParked(), TEXT("the close of a preloaded document did not park it")));
		ASSERT_THAT(IsFalse(View.GetProbe().bRemoved, TEXT("the close of a preloaded document removed it")));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId()));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(ArcRmlPreloadTestsPrivate::OnCloseCall), Log.Describe()));
	}

	TEST_METHOD(PreloadedFrame_OpensWithTheFirstListedTab_AndTheOtherTabOpensWithoutLoading)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture PreloadLog;
		TArray<FString> Paths;
		Paths.Add(ArcRmlUITestHelpers::MakePath(ArcRmlPreloadTestsPrivate::TabB));
		Paths.Add(ArcRmlUITestHelpers::MakePath(ArcRmlPreloadTestsPrivate::TabA));
		ASSERT_THAT(IsTrue(PreloadAndWait(Paths)));
		const uint32 ViewId = View.GetViewId();
		ASSERT_THAT(AreEqual(1, PreloadLog.CountContaining(*ArcRmlPreloadTestsPrivate::LoadedLine(ArcRmlPreloadTestsPrivate::Frame)), PreloadLog.Describe()));

		ArcRmlKeptTabTestHelpers::FLogCapture FrameLog;
		ASSERT_THAT(IsTrue(OpenByPath(ArcRmlPreloadTestsPrivate::Frame)));
		ASSERT_THAT(AreEqual(0, FrameLog.CountContaining(*ArcRmlPreloadTestsPrivate::LoadedLine(ArcRmlPreloadTestsPrivate::Frame)), FrameLog.Describe()));
		ASSERT_THAT(AreEqual(1, FrameLog.CountContaining(*ArcRmlPreloadTestsPrivate::ReopenedLine(ArcRmlPreloadTestsPrivate::Frame)), FrameLog.Describe()));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId(), TEXT("opening the frame made a new view")));
		ASSERT_THAT(AreEqual(ArcRmlUITestHelpers::MakePath(ArcRmlPreloadTestsPrivate::TabB), View.GetProbe().EmbeddedPath));
		ASSERT_THAT(IsTrue(IsVisible(TEXT("ptb-text")) && !IsVisible(TEXT("pta-text")), TEXT("the opened frame does not show the first listed tab")));

		ASSERT_THAT(IsTrue(CloseAndWait()));
		ASSERT_THAT(IsTrue(View.IsParked(), TEXT("the close of a preloaded frame did not park it")));

		ArcRmlKeptTabTestHelpers::FLogCapture TabLog;
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(OpenByPath(ArcRmlPreloadTestsPrivate::TabA)));
		ASSERT_THAT(AreEqual(ViewId, View.GetViewId(), TEXT("opening the second tab made a new view")));
		ASSERT_THAT(AreEqual(0, TabLog.CountContaining(*ArcRmlPreloadTestsPrivate::LoadedLine(ArcRmlPreloadTestsPrivate::TabA)), TabLog.Describe()));
		ASSERT_THAT(AreEqual(0, TabLog.CountContaining(*ArcRmlPreloadTestsPrivate::LoadedLine(ArcRmlPreloadTestsPrivate::Frame)), TabLog.Describe()));
		ASSERT_THAT(AreEqual(ArcRmlUITestHelpers::MakePath(ArcRmlPreloadTestsPrivate::TabA), View.GetProbe().EmbeddedPath));
		ASSERT_THAT(IsTrue(IsVisible(TEXT("pta-text")) && !IsVisible(TEXT("ptb-text")), TEXT("the second tab is not the shown one")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsInstanced), TEXT("showing the preloaded second tab instanced elements")));
	}

	TEST_METHOD(PreloadOfAMissingFile_ReportsFailed_AndTheNextPreloadStillRuns)
	{
		TestRunner->AddExpectedErrorPlain(TEXT("Preload/Missing.rml: cannot compile"), EAutomationExpectedErrorFlags::Contains, 0);
		TestRunner->AddExpectedErrorPlain(TEXT("Preload/Missing.rml: no usable compiled file"), EAutomationExpectedErrorFlags::Contains, 0);
		TArray<FString> Paths;
		Paths.Add(ArcRmlUITestHelpers::MakePath(ArcRmlPreloadTestsPrivate::Missing));
		ASSERT_THAT(IsFalse(View.Preload(Paths, ArcRmlUITestHelpers::ViewSize, 1.0f), TEXT("the preload of a missing file did not report failed")));
		ASSERT_THAT(IsFalse(View.IsPreloading(), TEXT("the failed preload left the host preloading")));
		ASSERT_THAT(IsFalse(View.IsParked() || View.IsOpen(), TEXT("the failed preload left a view")));

		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		ASSERT_THAT(IsTrue(PreloadOne(ArcRmlPreloadTestsPrivate::Model)));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(*ArcRmlPreloadTestsPrivate::LoadedLine(ArcRmlPreloadTestsPrivate::Model)), Log.Describe()));
	}

	TEST_METHOD(DiscardOfAPreloadedPath_DestroysItAndTheNextCloseOfAnOpenOneDoesNotPark)
	{
		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		ASSERT_THAT(IsTrue(PreloadOne(ArcRmlPreloadTestsPrivate::Model)));
		const uint32 PreloadedViewId = View.GetViewId();
		View.Discard();
		ASSERT_THAT(IsFalse(View.IsParked(), TEXT("the discard left the preloaded view parked")));
		ASSERT_THAT(IsTrue(View.Flush() && View.GetProbe().bRemoved, TEXT("the discard did not remove the preloaded view")));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("discarded the parked '")), Log.Describe()));

		ASSERT_THAT(IsTrue(ColdOpen(ArcRmlPreloadTestsPrivate::Model)));
		ASSERT_THAT(IsTrue(View.GetViewId() != PreloadedViewId, TEXT("the open reused the discarded view")));
		ASSERT_THAT(IsTrue(CloseAndWait()));
		ASSERT_THAT(IsFalse(View.IsParked(), TEXT("the close parked a document whose preload was discarded")));
		ASSERT_THAT(IsTrue(View.GetProbe().bRemoved, TEXT("the close did not remove the document whose preload was discarded")));
	}
};
