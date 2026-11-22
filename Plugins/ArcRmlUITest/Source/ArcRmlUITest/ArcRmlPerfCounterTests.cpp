// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CountersTrace.h"
#include "Trace/Trace.h"

TEST_CLASS(ArcRmlPerfCounterTests, "ArcRmlUI.PerfCounters")
{
	FArcRmlTestView View;
	int32 SavedPerfLog = 0;
#if COUNTERSTRACE_ENABLED
	bool bSavedCountersChannel = false;
#endif

	BEFORE_EACH()
	{
		IConsoleVariable* PerfLog = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PerfLog"));
		ASSERT_THAT(IsNotNull(PerfLog));
		SavedPerfLog = PerfLog->GetInt();
		PerfLog->Set(0, ECVF_SetByCode);
#if COUNTERSTRACE_ENABLED
		bSavedCountersChannel = UE_TRACE_CHANNELEXPR_IS_ENABLED(CountersChannel);
#endif
	}

	AFTER_EACH()
	{
		View.Close();
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PerfLog"))->Set(SavedPerfLog, ECVF_SetByCode);
#if COUNTERSTRACE_ENABLED
		UE::Trace::ToggleChannel(TEXT("Counters"), bSavedCountersChannel);
#endif
	}

	bool SetRootClassAndRender(const TCHAR* Class)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(Class, true);
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > Before;
	}

	TEST_METHOD(ColourChangeMakesNoGeometry_WidthChangeDoes_AtPerfLogZero)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("PerfProbe/Counters.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(SetRootClassAndRender(TEXT("blue"))));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ClassChanges) > 0, TEXT("class change counted")));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRendered) > 0, TEXT("render counted")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::GeometriesMade), TEXT("geometry made by a colour change")));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(SetRootClassAndRender(TEXT("wide"))));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::GeometriesMade) > 0, TEXT("geometry made by a width change")));
	}

#if COUNTERSTRACE_ENABLED
	bool WaitForFrames(uint64 Frames)
	{
		const uint64 Target = View.GetProbe().FrameSerial + Frames;
		const double Deadline = FPlatformTime::Seconds() + 10.0;
		while (View.GetProbe().FrameSerial < Target)
		{
			if (FPlatformTime::Seconds() >= Deadline || !View.Flush())
			{
				return false;
			}
			FPlatformProcess::Sleep(0.005f);
		}
		return true;
	}

	bool OpenTraceFixture()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("PerfProbe/TraceCounters.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled();
	}

	TEST_METHOD(TracedFrameCountersMatchViewCounters_AtPerfLogZero)
	{
		ASSERT_THAT(IsTrue(UE::Trace::ToggleChannel(TEXT("Counters"), true), TEXT("counters channel on")));
		ASSERT_THAT(IsTrue(OpenTraceFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const FArcRmlTestProbe Mark = View.GetProbe();
		ASSERT_THAT(IsTrue(WaitForFrames(4), TEXT("animated frames ran")));

		const FArcRmlTestProbe& Probe = View.GetProbe();
		ASSERT_THAT(IsTrue(Probe.TracedFrames - Mark.TracedFrames >= 4, TEXT("every view frame traced its counters")));
		for (int32 Index = 0; Index < static_cast<int32>(Rml::ArcPerf::Counter::Count); ++Index)
		{
			const Rml::ArcPerf::Counter Counter = static_cast<Rml::ArcPerf::Counter>(Index);
			ASSERT_THAT(AreEqual(View.GetPerfCounter(Counter), Probe.TracedPerfCounters[Index] - Mark.TracedPerfCounters[Index],
				FString::Printf(TEXT("traced '%hs'"), Rml::ArcPerf::GetCounterName(Counter))));
		}
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRendered) > 0, TEXT("render counted")));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::AnimationsAdvanced) > 0, TEXT("animation counted")));
		const int32 ElementRender = static_cast<int32>(Rml::ArcPerf::Bucket::ElementRender);
		ASSERT_THAT(IsTrue(Probe.TracedBucketMs[ElementRender] > Mark.TracedBucketMs[ElementRender], TEXT("render.element time traced")));
	}

	TEST_METHOD(NoCountingWithCountersChannelOff_AtPerfLogZero)
	{
		ASSERT_THAT(IsFalse(UE::Trace::ToggleChannel(TEXT("Counters"), false), TEXT("counters channel off")));
		View.SetPerfCounting(false);
		ASSERT_THAT(IsTrue(OpenTraceFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(WaitForFrames(4), TEXT("animated frames ran")));

		const FArcRmlTestProbe& Off = View.GetProbe();
		for (int32 Index = 0; Index < static_cast<int32>(Rml::ArcPerf::Counter::Count); ++Index)
		{
			ASSERT_THAT(AreEqual(int64(0), Off.PerfCounters[Index],
				FString::Printf(TEXT("'%hs' counted"), Rml::ArcPerf::GetCounterName(static_cast<Rml::ArcPerf::Counter>(Index)))));
		}

		ASSERT_THAT(IsTrue(UE::Trace::ToggleChannel(TEXT("Counters"), true), TEXT("counters channel on")));
		ASSERT_THAT(IsTrue(WaitForFrames(2), TEXT("animated frames ran")));
		ASSERT_THAT(IsTrue(View.GetProbe().PerfCounters[static_cast<int32>(Rml::ArcPerf::Counter::ElementsRendered)] > 0,
			TEXT("the counters channel turns counting on")));
	}
#endif
};
