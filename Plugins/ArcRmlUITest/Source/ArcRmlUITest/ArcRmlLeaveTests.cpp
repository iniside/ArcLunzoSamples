// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlKeptTabTestHelpers.h"
#include "ArcRmlUISettings.h"
#include "ArcRmlUITestHelpers.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "HAL/PlatformTime.h"
#include "Host/ArcRmlInputPolicy.h"
#include "InputCoreTypes.h"
#include "Misc/OutputDevice.h"
#include "Misc/ScopeLock.h"

namespace ArcRmlLeaveTestsPrivate
{
	const TCHAR* const DocumentPath = TEXT("Leave/LeaveToggleTest.rml");

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

	const TCHAR* RouteName(EArcRmlKeyRoute Route)
	{
		switch (Route)
		{
		case EArcRmlKeyRoute::Handled:
			return TEXT("Handled");
		case EArcRmlKeyRoute::BackAction:
			return TEXT("BackAction");
		case EArcRmlKeyRoute::UIAction:
			return TEXT("UIAction");
		case EArcRmlKeyRoute::Unhandled:
			break;
		}
		return TEXT("Unhandled");
	}

	class FLogLevelCapture final : public FOutputDevice
	{
	public:
		FLogLevelCapture()
		{
			GLog->AddOutputDevice(this);
		}

		virtual ~FLogLevelCapture() override
		{
			GLog->RemoveOutputDevice(this);
		}

		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category != TEXT("LogArcRmlUI") || (Verbosity & ELogVerbosity::VerbosityMask) != ELogVerbosity::Log)
			{
				return;
			}
			FScopeLock Lock(&LinesLock);
			Lines.Add(Text);
		}

		virtual bool CanBeUsedOnAnyThread() const override
		{
			return true;
		}

		TArray<FString> TakeLines()
		{
			GLog->Flush();
			FScopeLock Lock(&LinesLock);
			return MoveTemp(Lines);
		}

	private:
		FCriticalSection LinesLock;
		TArray<FString> Lines;
	};
}

TEST_CLASS(ArcRmlLeaveTests, "ArcRmlUI.Leave")
{
	FArcRmlTestView View;
	bool bFrozenClock = false;

	AFTER_EACH()
	{
		View.Close();
		if (bFrozenClock)
		{
			ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.Clock live"));
			bFrozenClock = false;
		}
	}

	bool OpenLeaveTest()
	{
		View.SetInputBindings(ArcRmlLeaveTestsPrivate::MakeBindings());
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(ArcRmlLeaveTestsPrivate::DocumentPath)) && View.Flush() && View.GetProbe().bHasDocument;
	}

	bool StartLeave()
	{
		View.PressKey(EKeys::Tab);
		return View.Flush() && View.GetProbe().bLeaving;
	}

	void WaitPastLeaveTimeout(const TCHAR* Description, TFunction<void()>&& Check)
	{
		const double Deadline = FPlatformTime::Seconds() + GetDefault<UArcRmlUISettings>()->LeaveTimeoutSeconds + 0.5;
		TestCommandBuilder
			.Until(Description, [Deadline]() -> bool
			{
				return FPlatformTime::Seconds() > Deadline;
			}, FTimespan::FromSeconds(10))
			.Then(Description, MoveTemp(Check));
	}

	TEST_METHOD(ToggleKeyDuringLeave_ReopensAndStaysOpen)
	{
		using namespace ArcRmlLeaveTestsPrivate;
		ASSERT_THAT(IsTrue(OpenLeaveTest(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(StartLeave(), TEXT("the close key did not start the leave")));
		ASSERT_THAT(AreEqual(1, View.GetCloseRequests()));

		const EArcRmlKeyRoute Route = View.KeyDown(EKeys::Tab);
		ASSERT_THAT(AreEqual(FString(RouteName(EArcRmlKeyRoute::Handled)), FString(RouteName(Route))));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the toggle during the leave failed")));
		ASSERT_THAT(IsFalse(View.GetProbe().bLeaving, TEXT("the toggle key during the leave did not cancel it")));
		ASSERT_THAT(IsFalse(View.GetProbe().bRemoved));
		ASSERT_THAT(AreEqual(1, View.GetCloseRequests(), TEXT("the toggle key during the leave was routed to the close path")));
		ASSERT_THAT(IsTrue(View.GetOpenRequests().IsEmpty()));
		View.KeyUp(EKeys::Tab);

		WaitPastLeaveTimeout(TEXT("the reopened view outlives the leave timeout"), [this]()
		{
			ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the leave timeout failed")));
			ASSERT_THAT(IsTrue(View.IsOpen(), TEXT("the reopened view was removed by the cancelled leave")));
			ASSERT_THAT(IsFalse(View.GetProbe().bLeaving));
			ASSERT_THAT(IsFalse(View.GetProbe().bRemoved));
		});
	}

	TEST_METHOD(OtherKeysDuringLeave_AreLeftToTheGame)
	{
		using namespace ArcRmlLeaveTestsPrivate;
		ASSERT_THAT(IsTrue(OpenLeaveTest(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(StartLeave(), TEXT("the close key did not start the leave")));

		for (const FKey& Key : {EKeys::Down, EKeys::W, EKeys::Escape, EKeys::Enter, EKeys::Gamepad_DPad_Down})
		{
			const EArcRmlKeyRoute Route = View.KeyDown(Key);
			ASSERT_THAT(AreEqual(FString(RouteName(EArcRmlKeyRoute::Unhandled)), FString(RouteName(Route)), FString::Printf(TEXT("key %s"), *Key.ToString())));
			View.KeyUp(Key);
		}
		ASSERT_THAT(IsFalse(View.Back(), TEXT("the CommonUI back action was consumed during the leave")));
		const EArcRmlKeyRoute RepeatRoute = View.KeyDown(EKeys::Tab, FModifierKeysState(), true);
		ASSERT_THAT(AreEqual(FString(RouteName(EArcRmlKeyRoute::Handled)), FString(RouteName(RepeatRoute)), TEXT("a held toggle key repeat")));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.GetProbe().bLeaving, TEXT("a key other than a toggle press ended the leave")));
		ASSERT_THAT(AreEqual(1, View.GetCloseRequests(), TEXT("a key during the leave was routed to the close path")));

		TestCommandBuilder
			.Until(TEXT("the leave removes the view"), [this]() -> bool
			{
				View.Flush();
				return View.GetProbe().bRemoved;
			}, FTimespan::FromSeconds(10))
			.Then(TEXT("the leave ends by its timeout"), [this]()
			{
				ASSERT_THAT(IsTrue(View.GetProbe().bLeaveTimedOut));
				ASSERT_THAT(IsFalse(View.IsOpen()));
			});
	}

	TEST_METHOD(DeviceClassChangeDuringLeave_DoesNotDelayIt)
	{
		View.SetInputBindings(ArcRmlLeaveTestsPrivate::MakeBindings());
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("Leave/DeviceClassLeaveTest.rml"))) && View.Flush()
			&& View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(StartLeave(), TEXT("the close key did not start the leave")));
		View.SetInputType(ECommonInputType::Gamepad);
		ASSERT_THAT(IsTrue(View.Flush()));

		TestCommandBuilder
			.Until(TEXT("the leave removes the view"), [this]() -> bool
			{
				View.Flush();
				return View.GetProbe().bRemoved;
			}, FTimespan::FromSeconds(10))
			.Then(TEXT("the leave ends by its root animation, not by the timeout a body transition would force"), [this]()
			{
				ASSERT_THAT(IsFalse(View.GetProbe().bLeaveTimedOut, TEXT("the device class change started a transition on body that delayed the leave")));
			});
	}

	TEST_METHOD(InputAndLeaveDiagnostics_AreVerbose)
	{
		ASSERT_THAT(IsTrue(OpenLeaveTest(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ArcRmlLeaveTestsPrivate::FLogLevelCapture Capture;

		View.PressKey(EKeys::Down);
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(StartLeave(), TEXT("the close key did not start the leave")));
		View.KeyDown(EKeys::Tab);
		ASSERT_THAT(IsTrue(View.Flush()));

		TArray<FString> Lines = Capture.TakeLines();
		Lines.RemoveAll([](const FString& Line) { return Line.Contains(TEXT("' requested, ")); });
		ASSERT_THAT(AreEqual(0, Lines.Num(), FString::Printf(TEXT("LogArcRmlUI lines at Log verbosity: [%s]"), *FString::Join(Lines, TEXT(" | ")))));
	}

	TEST_METHOD(BodyLeaveAnimation_RunsItsFullDurationBeforeTheClose)
	{
		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.Clock 1000"));
		bFrozenClock = true;
		View.SetInputBindings(ArcRmlLeaveTestsPrivate::MakeBindings());
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("Leave/BodyLeaveTest.rml"))) && View.FlushUntilSettled()
			&& View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(StartLeave(), TEXT("the close key did not start the leave")));

		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.ClockAdvance 0.5"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(IsFalse(View.GetProbe().bRemoved, TEXT("the view was removed halfway through the 1 s body.leave animation")));
		ASSERT_THAT(IsTrue(View.GetProbe().bLeaving));

		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.ClockAdvance 0.6"));
		for (int32 Attempt = 0; Attempt < 100 && !View.GetProbe().bRemoved; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			ASSERT_THAT(IsTrue(View.Flush()));
		}
		ASSERT_THAT(IsTrue(View.GetProbe().bRemoved, TEXT("the view was not removed after the body.leave animation ended")));
		ASSERT_THAT(IsFalse(View.GetProbe().bLeaveTimedOut, TEXT("the leave ended by its timeout, not by the body.leave animation end")));
	}
};
