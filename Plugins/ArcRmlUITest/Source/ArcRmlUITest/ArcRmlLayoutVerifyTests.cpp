// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/ScopeLock.h"

namespace ArcRmlLayoutVerifyTestsPrivate
{
	constexpr int32 AnimatedFrames = 24;
	constexpr int32 SwitchFrames = 6;

	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
	}

	class FLayoutVerifyLogCapture final : public FOutputDevice
	{
	public:
		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category != FName(TEXT("LogArcRmlUI")) || !FCString::Strstr(Text, TEXT("layout verify")))
			{
				return;
			}
			FScopeLock Lock(&Mutex);
			if (FCString::Strstr(Text, TEXT("mismatches 0,")))
			{
				if (FCString::Strstr(Text, TEXT("layout verify '")))
				{
					CleanPasses++;
				}
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

	private:
		FCriticalSection Mutex;
		TArray<FString> Lines;
		int32 CleanPasses = 0;
	};

	struct FMarginBox
	{
		const TCHAR* Id;
		float Y;
		float Height;
	};

	const FMarginBox ChromeBigFirstChild[] = {
		{TEXT("mct-item-a"), 8.0f, 77.0f},
		{TEXT("mct-a-wrap"), 48.0f, 21.0f},
		{TEXT("mct-a-p"), 48.0f, 21.0f},
		{TEXT("mct-item-b"), 8.0f, 53.0f},
		{TEXT("mct-b-p"), 24.0f, 21.0f},
		{TEXT("mct-chain"), 97.0f, 58.0f},
		{TEXT("mct-chain-inner"), 97.0f, 58.0f},
		{TEXT("mct-chain-p"), 134.0f, 21.0f},
		{TEXT("mct-after"), 171.0f, 21.0f},
	};

	const FMarginBox ChromeFirstChildRemoved[] = {
		{TEXT("mct-chain"), 101.0f, 21.0f},
		{TEXT("mct-chain-inner"), 101.0f, 21.0f},
		{TEXT("mct-chain-p"), 101.0f, 21.0f},
		{TEXT("mct-after"), 138.0f, 21.0f},
	};

	template <int32 N>
	FString DescribeMarginBoxes(const TArray<FArcRmlTestLayoutElement>& Elements, const FMarginBox (&Cases)[N])
	{
		FString Mismatches;
		for (const FMarginBox& Case : Cases)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Case.Id);
			if (!Element)
			{
				Mismatches += FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
			}
			else if (FMath::Abs(Element->Rect.Position.Y - Case.Y) > 1.0f || FMath::Abs(Element->Rect.Size.Y - Case.Height) > 1.0f)
			{
				Mismatches += FString::Printf(TEXT("[%s is y %.2f h %.2f, Chrome y %.2f h %.2f] "), Case.Id, Element->Rect.Position.Y, Element->Rect.Size.Y, Case.Y,
					Case.Height);
			}
		}
		return Mismatches;
	}

	struct FSwitchedWidth
	{
		const TCHAR* Box;
		const TCHAR* Radio;
		float Width;
	};

	const FSwitchedWidth SwitchedWidths[] = {
		{TEXT("grid"), TEXT("r-a"), 600.0f},
		{TEXT("grid"), TEXT("r-b"), 820.0f},
		{TEXT("host"), TEXT("r-a"), 600.0f},
		{TEXT("host"), TEXT("r-b"), 900.0f},
		{TEXT("host"), TEXT("r-c"), 420.0f},
	};

	const TCHAR* const AnimatedWidthBoxes[] = {TEXT("grid"), TEXT("host"), TEXT("py-an")};

	bool HasClass(const FArcRmlTestLayoutElement& Element, const FString& Class)
	{
		TArray<FString> Classes;
		Element.Classes.ParseIntoArray(Classes, TEXT(" "));
		return Classes.Contains(Class);
	}

	FString DescribeRadioNotInEffect(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& Radio)
	{
		if (!ArcRmlUITestHelpers::FindByDomId(Elements, Radio))
		{
			return FString::Printf(TEXT("[%s is not in the layout]"), *Radio);
		}
		const FString Region = Radio.RightChop(2);
		FString Mismatches;
		int32 Checks = 0;
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			if (!HasClass(Element, TEXT("case")))
			{
				continue;
			}
			++Checks;
			const bool bExpected = Region == TEXT("all") || HasClass(Element, Region);
			if (Element.bVisible != bExpected || (bExpected && Element.Rect.Size.Y <= 0.0f))
			{
				Mismatches += FString::Printf(TEXT("[case '%s' is %s, h %.2f] "), *Element.Classes, Element.bVisible ? TEXT("shown") : TEXT("hidden"), Element.Rect.Size.Y);
			}
		}
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			for (const FSwitchedWidth& Case : SwitchedWidths)
			{
				if (Radio != Case.Radio || !HasClass(Element, Case.Box))
				{
					continue;
				}
				++Checks;
				if (FMath::Abs(Element.Rect.Size.X - Case.Width) > 1.0f || Element.Animated.Contains(TEXT("width")))
				{
					Mismatches += FString::Printf(TEXT("[.%s is w %.2f%s, expected %.2f] "), Case.Box, Element.Rect.Size.X,
						Element.Animated.Contains(TEXT("width")) ? TEXT(" animated") : TEXT(""), Case.Width);
				}
			}
		}
		if (Region == TEXT("anim"))
		{
			++Checks;
			const bool bAnimated = Elements.ContainsByPredicate([](const FArcRmlTestLayoutElement& Element)
			{
				for (const TCHAR* Box : AnimatedWidthBoxes)
				{
					if (HasClass(Element, Box) && Element.Animated.Contains(TEXT("width")))
					{
						return true;
					}
				}
				return false;
			});
			if (!bAnimated)
			{
				Mismatches += TEXT("[no switched box animates its width] ");
			}
		}
		if (Checks == 0)
		{
			Mismatches += FString::Printf(TEXT("[nothing shows the effect of %s] "), *Radio);
		}
		return Mismatches;
	}
}

TEST_CLASS(ArcRmlLayoutVerifyTests, "ArcRmlUI.LayoutVerify")
{
	FArcRmlTestView View;
	ArcRmlLayoutVerifyTestsPrivate::FLayoutVerifyLogCapture Capture;
	int32 SavedVerify = 0;
	int32 SavedMaxDepth = 0;
	bool bMaxDepthChanged = false;
	bool bClockFrozen = false;
	bool bCaptureAttached = false;
	TArray<FArcRmlTestLayoutElement> Elements;

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
		View.Close();
		if (bCaptureAttached)
		{
			GLog->RemoveOutputDevice(&Capture);
			bCaptureAttached = false;
		}
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify"))->Set(SavedVerify, ECVF_SetByCode);
		if (bMaxDepthChanged)
		{
			IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify.MaxContainerDepth"))->Set(SavedMaxDepth, ECVF_SetByCode);
			bMaxDepthChanged = false;
		}
		if (bClockFrozen)
		{
			ArcRmlLayoutVerifyTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
			bClockFrozen = false;
		}
	}

	bool OpenKeyedRows(const FString& File)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("LayoutReuse/%s"), *File));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ParseLayout(Elements);
	}

	bool ChangeRowName(int32 Row, const FString& Name)
	{
		View.SetModelValue(TEXT("rows"), ArcRmlUITestHelpers::MakeRowFieldPath(TEXT("Rows"), Row, TEXT("Name")), ArcRmlUITestHelpers::MakeString(Name));
		return View.FlushUntilSettled() && View.ParseLayout(Elements);
	}

	bool ChangeRowNameByListUpdate(int32 Row, const FString& Name)
	{
		FArcRmlValue Value;
		Value.Kind = EArcRmlValueKind::Struct;
		Value.MemberNames = {TEXT("Id"), TEXT("Name"), TEXT("Value")};
		Value.Children.Add(ArcRmlUITestHelpers::MakeString(FString::Printf(TEXT("r%d"), Row)));
		Value.Children.Add(ArcRmlUITestHelpers::MakeString(Name));
		Value.Children.Add(ArcRmlUITestHelpers::MakeInt(Row * 10));
		TArray<FArcRmlListOp> Ops;
		FArcRmlListOp& Update = Ops.AddDefaulted_GetRef();
		Update.Type = EArcRmlListOpType::Update;
		Update.Index = Row;
		Update.Value = MoveTemp(Value);
		View.ApplyListOps(TEXT("rows"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Ops));
		return View.FlushUntilSettled() && View.ParseLayout(Elements);
	}

	TEST_METHOD(ScrollContainerMeasuredForItsWidth_OpensWithoutMismatches)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("LayoutVerify/ScrollMeasure.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		GLog->Flush();
		ASSERT_THAT(AreEqual(0, Capture.Num(), FString::Printf(TEXT("layout verify reported %s"), *Capture.Describe())));
		ASSERT_THAT(IsTrue(Capture.NumCleanPasses() > 0, TEXT("no layout verify pass was logged")));

		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* List = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("list"));
		ASSERT_THAT(IsNotNull(List, TEXT("list")));
		const FArcRmlTestLayoutElement* Vertical = Elements.FindByPredicate([List](const FArcRmlTestLayoutElement& Element)
			{ return Element.Kind == TEXT("scrollbar") && Element.Parent == List->Path && Element.Tag == TEXT("scrollbarvertical"); });
		ASSERT_THAT(IsNull(Vertical, TEXT("list has a vertical scrollbar element, but its content never overflows, so none is created")));
	}

	void SetVerify(int32 Value)
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify"))->Set(Value, ECVF_SetByCode);
	}

	void ExpectCleanVerifyAfter(int32 CleanBefore)
	{
		GLog->Flush();
		ASSERT_THAT(AreEqual(0, Capture.Num(), FString::Printf(TEXT("layout verify reported %s"), *Capture.Describe())));
		ASSERT_THAT(IsTrue(Capture.NumCleanPasses() > CleanBefore, TEXT("no layout verify pass ran after the text change")));
	}

	void ExpectReuse(int32 RowCount)
	{
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::LayoutReused) > 0, TEXT("no clean row kept its layout")));
		const int64 FormatCalls = View.GetPerfCounter(Rml::ArcPerf::Counter::Formats) + View.GetPerfCounter(Rml::ArcPerf::Counter::FormatCallsWithoutContext);
		ASSERT_THAT(IsTrue(FormatCalls < RowCount / 2, FString::Printf(TEXT("%lld format calls for one changed row of %d"), FormatCalls, RowCount)));
	}

	void ExpectRowsStackedAfterGrowth(int32 RowCount, float HeightBefore, float LastYBefore)
	{
		const FArcRmlTestLayoutElement* Row5 = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("row-r5"));
		ASSERT_THAT(IsNotNull(Row5, TEXT("row-r5 after")));
		const float Growth = Row5->Rect.Size.Y - HeightBefore;
		ASSERT_THAT(IsTrue(Growth > 10.0f, FString::Printf(TEXT("row-r5 grew by %.2f"), Growth)));

		for (int32 Index = 6; Index < RowCount; ++Index)
		{
			const FArcRmlTestLayoutElement* Previous = ArcRmlUITestHelpers::FindByDomId(Elements, FString::Printf(TEXT("row-r%d"), Index - 1));
			const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, FString::Printf(TEXT("row-r%d"), Index));
			ASSERT_THAT(IsNotNull(Previous, TEXT("previous row")));
			ASSERT_THAT(IsNotNull(Row, TEXT("row")));
			ASSERT_THAT(IsNear(Previous->Rect.Position.Y + Previous->Rect.Size.Y, Row->Rect.Position.Y, 0.5f,
				FString::Printf(TEXT("row-r%d is not stacked under row-r%d"), Index, Index - 1)));
		}
		const FArcRmlTestLayoutElement* Last = ArcRmlUITestHelpers::FindByDomId(Elements, FString::Printf(TEXT("row-r%d"), RowCount - 1));
		ASSERT_THAT(IsNotNull(Last, TEXT("last row after")));
		ASSERT_THAT(IsNear(LastYBefore + Growth, Last->Rect.Position.Y, 0.5f, TEXT("the last row moved by the growth of row-r5")));
	}

	void OpenAndMeasureRows(const FString& File, int32 RowCount, float& OutHeightBefore, float& OutLastYBefore)
	{
		ASSERT_THAT(IsTrue(OpenKeyedRows(File), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Row5 = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("row-r5"));
		const FArcRmlTestLayoutElement* Last = ArcRmlUITestHelpers::FindByDomId(Elements, FString::Printf(TEXT("row-r%d"), RowCount - 1));
		ASSERT_THAT(IsNotNull(Row5, TEXT("row-r5")));
		ASSERT_THAT(IsNotNull(Last, TEXT("last row")));
		OutHeightBefore = Row5->Rect.Size.Y;
		OutLastYBefore = Last->Rect.Position.Y;
	}

	void ExpectTextChange(const FString& File, bool bVerify, bool bListUpdate)
	{
		constexpr int32 RowCount = 30;
		float HeightBefore = 0.0f;
		float LastYBefore = 0.0f;
		SetVerify(bVerify ? 1 : 0);
		OpenAndMeasureRows(File, RowCount, HeightBefore, LastYBefore);
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(bListUpdate ? ChangeRowNameByListUpdate(5, TEXT("changed")) : ChangeRowName(5, TEXT("changed"))));
		if (bVerify)
		{
			ExpectCleanVerifyAfter(CleanBefore);
		}
		ExpectReuse(RowCount);
		ASSERT_THAT(AreEqual(FString(TEXT("changed")), ArcRmlUITestHelpers::GetText(Elements, TEXT("name-r5")), TEXT("the changed row shows the new text")));
	}

	void ExpectGrowth(const FString& File, bool bVerify, bool bListUpdate)
	{
		constexpr int32 RowCount = 30;
		float HeightBefore = 0.0f;
		float LastYBefore = 0.0f;
		SetVerify(bVerify ? 1 : 0);
		OpenAndMeasureRows(File, RowCount, HeightBefore, LastYBefore);
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const FString LongName(TEXT("aaaa bbbb cccc dddd eeee ffff gggg hhhh"));
		ASSERT_THAT(IsTrue(bListUpdate ? ChangeRowNameByListUpdate(5, LongName) : ChangeRowName(5, LongName)));
		if (bVerify)
		{
			ExpectCleanVerifyAfter(CleanBefore);
		}
		ExpectReuse(RowCount);
		ExpectRowsStackedAfterGrowth(RowCount, HeightBefore, LastYBefore);
	}

	TEST_METHOD(OneKeyedRowTextChanged_VerifiesWithoutMismatches)
	{
		ExpectTextChange(TEXT("KeyedRows.rml"), true, false);
	}

	TEST_METHOD(OneKeyedRowTextChanged_ReusesCleanRows)
	{
		ExpectTextChange(TEXT("KeyedRows.rml"), false, false);
	}

	TEST_METHOD(OneKeyedRowGrowsTaller_VerifiesAndMovesTheRowsAfterIt)
	{
		ExpectGrowth(TEXT("KeyedRows.rml"), true, false);
	}

	TEST_METHOD(OneKeyedRowGrowsTaller_ReusesCleanRowsAndMovesTheRowsAfterIt)
	{
		ExpectGrowth(TEXT("KeyedRows.rml"), false, false);
	}

	TEST_METHOD(ListUpdateOpTextChanged_VerifiesAndReusesCleanRows)
	{
		ExpectTextChange(TEXT("KeyedRows.rml"), true, true);
	}

	TEST_METHOD(ListUpdateOpGrowsRowTaller_VerifiesAndMovesTheRowsAfterIt)
	{
		ExpectGrowth(TEXT("KeyedRows.rml"), true, true);
	}

	TEST_METHOD(ScrollListFixedRowTextChanged_VerifiesWithoutMismatches)
	{
		ExpectTextChange(TEXT("KeyedRowsScroll.rml"), true, false);
	}

	TEST_METHOD(ScrollListFixedRowTextChanged_ReusesCleanRows)
	{
		ExpectTextChange(TEXT("KeyedRowsScroll.rml"), false, false);
	}

	TEST_METHOD(ScrollListMinHeightRowTextChanged_VerifiesWithoutMismatches)
	{
		ExpectTextChange(TEXT("KeyedRowsScrollGrow.rml"), true, false);
	}

	TEST_METHOD(ScrollListMinHeightRowGrowsTaller_VerifiesAndMovesTheRowsAfterIt)
	{
		ExpectGrowth(TEXT("KeyedRowsScrollGrow.rml"), true, false);
	}

	TEST_METHOD(ScrollListMinHeightRowGrowsTaller_ReusesCleanRowsAndMovesTheRowsAfterIt)
	{
		ExpectGrowth(TEXT("KeyedRowsScrollGrow.rml"), false, false);
	}

	TEST_METHOD(DocumentDeeperThanTheBudget_IsSkippedAndNotCountedClean)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("LayoutVerify/ScrollMeasure.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		GLog->Flush();
		ASSERT_THAT(AreEqual(0, Capture.Num(), FString::Printf(TEXT("layout verify reported %s"), *Capture.Describe())));
		const int32 CleanBefore = Capture.NumCleanPasses();

		IConsoleVariable* MaxDepth = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify.MaxContainerDepth"));
		ASSERT_THAT(IsNotNull(MaxDepth));
		SavedMaxDepth = MaxDepth->GetInt();
		bMaxDepthChanged = true;
		MaxDepth->Set(2, ECVF_SetByCode);
		View.Resize(FIntPoint(1600, 900), 1.0f);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		GLog->Flush();
		ASSERT_THAT(IsTrue(Capture.Num() > 0 && Capture.Describe().Contains(TEXT("skipped: budget")), FString::Printf(TEXT("no skip was logged: %s"), *Capture.Describe())));
		ASSERT_THAT(AreEqual(CleanBefore, Capture.NumCleanPasses(), TEXT("a skipped verify counted as a clean pass")));
	}

	bool OpenMarginCollapseToggle()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("LayoutVerify/MarginCollapseToggle.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	bool SetMarginToggle(const TCHAR* Field, bool bValue)
	{
		View.SetModelValue(TEXT("mct"), ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeBool(bValue));
		return View.FlushUntilSettled() && View.ParseLayout(Elements);
	}

	TEST_METHOD(FirstChildMarginGrowsInACollapsingChain_ReusesTheOtherItemAndMatchesChrome)
	{
		using namespace ArcRmlLayoutVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenMarginCollapseToggle(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(SetMarginToggle(TEXT("big"), true)));
		ExpectCleanVerifyAfter(CleanBefore);
		ASSERT_THAT(AreEqual(FString(), DescribeMarginBoxes(Elements, ChromeBigFirstChild)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::LayoutReused) > 0, TEXT("no clean box kept its layout")));
	}

	TEST_METHOD(FirstChildRemovedAndAddedInACollapsingChain_VerifiesAndMatchesChrome)
	{
		using namespace ArcRmlLayoutVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenMarginCollapseToggle(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetMarginToggle(TEXT("big"), true)));
		GLog->Flush();
		int32 CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(SetMarginToggle(TEXT("showFirst"), false)));
		ExpectCleanVerifyAfter(CleanBefore);
		ASSERT_THAT(AreEqual(FString(), DescribeMarginBoxes(Elements, ChromeFirstChildRemoved), TEXT("removing the first child")));
		GLog->Flush();
		CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(SetMarginToggle(TEXT("showFirst"), true)));
		ExpectCleanVerifyAfter(CleanBefore);
		ASSERT_THAT(AreEqual(FString(), DescribeMarginBoxes(Elements, ChromeBigFirstChild), TEXT("adding the first child back")));
	}

	bool StepFrames(int32 Frames)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			ArcRmlLayoutVerifyTestsPrivate::RunConsole(TEXT("ArcRml.ClockAdvance 0.05"));
			FPlatformProcess::Sleep(0.02f);
			if (!View.Flush())
			{
				return false;
			}
		}
		return true;
	}

	bool OpenAnimated(const TCHAR* File)
	{
		ArcRmlLayoutVerifyTestsPrivate::RunConsole(TEXT("ArcRml.Clock 1000"));
		bClockFrozen = true;
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("LayoutVerify/%s"), File));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && StepFrames(4) && View.GetProbe().bHasDocument;
	}

	void ExpectCleanWhileSwitching(const TCHAR* File, const TArray<const TCHAR*>& Radios, int32 FramesPerRadio)
	{
		ASSERT_THAT(IsTrue(OpenAnimated(File), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		for (const TCHAR* Radio : Radios)
		{
			View.ClickElement(FString::Printf(TEXT("#%s"), Radio));
			ASSERT_THAT(IsTrue(StepFrames(FramesPerRadio) && View.ParseLayout(Elements), FString::Printf(TEXT("frames after %s"), Radio)));
			ASSERT_THAT(AreEqual(FString(), ArcRmlLayoutVerifyTestsPrivate::DescribeRadioNotInEffect(Elements, Radio), FString::Printf(TEXT("after clicking %s"), Radio)));
		}
		ExpectCleanVerifyAfter(CleanBefore);
	}

	void ExpectCleanWhileAnimating(const TCHAR* File, const TCHAR* Radio)
	{
		ExpectCleanWhileSwitching(File, {Radio}, ArcRmlLayoutVerifyTestsPrivate::AnimatedFrames);
	}

	void ExpectReuseAtContainingBlockHeight(const TCHAR* File, const TCHAR* Radio)
	{
		using namespace ArcRmlLayoutVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAnimated(File), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClickElement(FString::Printf(TEXT("#%s"), Radio));
		ASSERT_THAT(IsTrue(StepFrames(SwitchFrames) && View.ParseLayout(Elements) && View.ResetPerfCounters()));
		ASSERT_THAT(AreEqual(FString(), DescribeRadioNotInEffect(Elements, Radio), FString::Printf(TEXT("after clicking %s"), Radio)));
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(StepFrames(AnimatedFrames)));
		ExpectCleanVerifyAfter(CleanBefore);
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::LayoutReusedAtContainingBlockHeight) > 0,
			TEXT("no child kept its layout across a change of its container's height")));
	}

	TEST_METHOD(GridAutoRowsWithNestedFlex_ItemHeightAnimated_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileAnimating(TEXT("GridAutoRowsNestedFlex.rml"), TEXT("r-a"));
	}

	TEST_METHOD(GridAutoRowsWithNestedFlex_WidthSwitched_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileSwitching(TEXT("GridAutoRowsNestedFlex.rml"), {TEXT("r-b"), TEXT("r-a")}, ArcRmlLayoutVerifyTestsPrivate::SwitchFrames);
	}

	TEST_METHOD(GridAutoRowsWithNestedFlex_WidthAnimated_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileAnimating(TEXT("GridAutoRowsNestedFlex.rml"), TEXT("r-anim"));
	}

	TEST_METHOD(VerifySession_StillReusesTheCleanGridItems)
	{
		using namespace ArcRmlLayoutVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenAnimated(TEXT("GridAutoRowsNestedFlex.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(StepFrames(AnimatedFrames)));
		ExpectCleanVerifyAfter(CleanBefore);
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::LayoutReused) > 0, TEXT("no clean grid item kept its layout while verify was on")));
	}

	TEST_METHOD(DefiniteBlockSizeControlBlockItem_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c00"));
	}

	TEST_METHOD(DefiniteBlockSizeOwnMinHeightPercent_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c01"));
	}

	TEST_METHOD(DefiniteBlockSizeOwnRelativeTopPercent_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c02"));
	}

	TEST_METHOD(DefiniteBlockSizeDescendantHeightPercent_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c03"));
	}

	TEST_METHOD(DefiniteBlockSizeAbsolutesWithPercentInsetOrHeight_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c04"));
	}

	TEST_METHOD(DefiniteBlockSizeColumnFlex_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c05"));
	}

	TEST_METHOD(DefiniteBlockSizeWrappingFlex_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c06"));
	}

	TEST_METHOD(DefiniteBlockSizeAspectRatioFlexItem_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c07"));
	}

	TEST_METHOD(DefiniteBlockSizeRowStretchAtAnIndefiniteHeight_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c08"));
	}

	TEST_METHOD(DefiniteBlockSizeRowStretchAtADefiniteHeight_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c09"));
	}

	TEST_METHOD(DefiniteBlockSizeGridFrRows_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c10"));
	}

	TEST_METHOD(DefiniteBlockSizeGridPercentRows_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c11"));
	}

	TEST_METHOD(DefiniteBlockSizeTable_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c12"));
	}

	TEST_METHOD(DefiniteBlockSizeReplacedHeightPercent_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c13"));
	}

	TEST_METHOD(DefiniteBlockSizeOverflowAuto_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c14"));
	}

	TEST_METHOD(DefiniteBlockSizeGridItemInAFixedRow_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c15"));
	}

	TEST_METHOD(DefiniteBlockSizeRelativeDescendantTopPercent_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c16"));
	}

	TEST_METHOD(DefiniteBlockSizeButtonAtADefiniteHeight_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c17"));
	}

	TEST_METHOD(DefiniteBlockSizeBlockResultsUnderSizeOnly_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c18"));
	}

	TEST_METHOD(DefiniteBlockSizeGridAutoRowChildMarginTopPercent_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c19"));
	}

	TEST_METHOD(DefiniteBlockSizeGridAutoRowChildMarginTopCalc_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c20"));
	}

	TEST_METHOD(DefiniteBlockSizeGridAutoRowFixedChildAutoMargins_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c21"));
	}

	TEST_METHOD(DefiniteBlockSizeRowFlexStretchChildMarginTopPercent_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c22"));
	}

	TEST_METHOD(DefiniteBlockSizeRowFlexStretchChildMarginTopCalc_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c23"));
	}

	TEST_METHOD(DefiniteBlockSizeRowFlexStretchFixedChildAutoMargins_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeFlags.rml"), TEXT("r-c24"));
	}

	TEST_METHOD(ProbeWrapperAroundFlexAndGrid_WidthSwitched_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileSwitching(TEXT("ProbeWrapper.rml"), {TEXT("r-b"), TEXT("r-c"), TEXT("r-a")}, ArcRmlLayoutVerifyTestsPrivate::SwitchFrames);
	}

	TEST_METHOD(ProbeWrapperAroundFlexAndGrid_WidthAnimated_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileAnimating(TEXT("ProbeWrapper.rml"), TEXT("r-anim"));
	}

	TEST_METHOD(BreakWordUnderMinAndMaxContentProbes_WidthSwitched_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileSwitching(TEXT("ProbeBreakWord.rml"), {TEXT("r-b"), TEXT("r-c"), TEXT("r-a")}, ArcRmlLayoutVerifyTestsPrivate::SwitchFrames);
	}

	TEST_METHOD(BreakWordUnderMinAndMaxContentProbes_WidthAnimated_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileAnimating(TEXT("ProbeBreakWord.rml"), TEXT("r-anim"));
	}

	TEST_METHOD(AbsolutesEscapingReusedItems_SiblingAnimated_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileAnimating(TEXT("ProbeEscapedAbsolutes.rml"), TEXT("r-a"));
	}

	TEST_METHOD(AbsolutesEscapingReusedItems_WidthSwitched_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileSwitching(TEXT("ProbeEscapedAbsolutes.rml"), {TEXT("r-b"), TEXT("r-c"), TEXT("r-a")}, ArcRmlLayoutVerifyTestsPrivate::SwitchFrames);
	}

	TEST_METHOD(AbsolutesEscapingReusedItems_WidthAnimated_VerifiesWithoutMismatches)
	{
		ExpectCleanWhileAnimating(TEXT("ProbeEscapedAbsolutes.rml"), TEXT("r-anim"));
	}

	TEST_METHOD(ContainingBlockHeightColumnFlexJustifyContent_ReusesChildrenAndVerifies)
	{
		ExpectReuseAtContainingBlockHeight(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r01"));
	}

	TEST_METHOD(ContainingBlockHeightWrappingFlexAlignContent_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r02"));
	}

	TEST_METHOD(ContainingBlockHeightRowFlexAlignSelfAndColumnStretch_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r03"));
	}

	TEST_METHOD(ContainingBlockHeightGridAlignContentAndFixedRows_ReusesChildrenAndVerifies)
	{
		ExpectReuseAtContainingBlockHeight(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r04"));
	}

	TEST_METHOD(ContainingBlockHeightTableCellVerticalAlign_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r05"));
	}

	TEST_METHOD(ContainingBlockHeightFlexChildrenReadingTheHeight_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r06"));
	}

	TEST_METHOD(ContainingBlockHeightGridFrAndPercentRows_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r07"));
	}

	TEST_METHOD(ContainingBlockHeightTablePercentRowHeights_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r08"));
	}

	TEST_METHOD(ContainingBlockHeightNestedPercentHeightFlex_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r09"));
	}

	TEST_METHOD(ContainingBlockHeightScrollContainers_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r10"));
	}

	TEST_METHOD(ContainingBlockHeightEscapedAbsolutesInRepositionedItems_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r11"));
	}

	TEST_METHOD(ContainingBlockHeightBaselineAlignedFlexAndGrid_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r12"));
	}

	TEST_METHOD(ContainingBlockHeightFromTheParentStretch_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseFlexGrid.rml"), TEXT("r-r13"));
	}

	TEST_METHOD(ContainingBlockHeightBlockContainerChildren_ReusesChildrenAndVerifies)
	{
		ExpectReuseAtContainingBlockHeight(TEXT("BlockSizeReuseBlock.rml"), TEXT("r-r14"));
	}

	TEST_METHOD(ContainingBlockHeightBlockContainerAbsolutes_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseBlock.rml"), TEXT("r-r15"));
	}

	TEST_METHOD(ContainingBlockHeightBlockChildrenReadingTheHeight_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseBlock.rml"), TEXT("r-r16"));
	}

	TEST_METHOD(ContainingBlockHeightPercentHeightBlockWrapperAndListMarkers_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseBlock.rml"), TEXT("r-r17"));
	}

	TEST_METHOD(ContainingBlockHeightBlockScrollContainer_VerifiesWhileAnimating)
	{
		ExpectCleanWhileAnimating(TEXT("BlockSizeReuseBlock.rml"), TEXT("r-r18"));
	}
};
