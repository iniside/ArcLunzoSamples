// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/ScopeLock.h"

namespace ArcRmlRenderVerifyTestsPrivate
{
	const FColor Cls(0x33, 0x66, 0x99, 255);
	const FColor ClsHot(0xcc, 0x33, 0x33, 255);
	const FColor Atr(0x66, 0x99, 0x33, 255);
	const FColor AtrB(0x99, 0x33, 0x66, 255);
	const FColor Hov(0x33, 0x66, 0xcc, 255);
	const FColor HovOn(0xcc, 0x66, 0x33, 255);
	const FColor Kid(0xaa, 0x55, 0xaa, 255);
	const FColor Inner(0x88, 0xaa, 0x22, 255);
	const FColor Row0(0x2a, 0x6f, 0x6f, 255);
	const FColor TextColour(0x12, 0xab, 0x34, 255);
	const FColor RoundKid(0xff, 0xcc, 0x00, 255);
	const FColor ChunkMark(0x13, 0x57, 0x9b, 255);
	const FColor ChunkKid(0x5b, 0x2c, 0x8f, 255);
	const FColor SwapBefore(0xaa, 0x11, 0x11, 255);
	const FColor SwapAfter(0x11, 0xaa, 0x11, 255);
	constexpr float ChunkMarkScroll = 120.0f;
	constexpr int32 StaticElements = 6;
	constexpr int64 MaxRerecordedPerChange = 4;

	class FVerifyLogCapture final : public FOutputDevice
	{
	public:
		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category == FName(TEXT("LogArcRmlUI")) && FCString::Strstr(Text, TEXT("render verify")) && !FCString::Strstr(Text, TEXT("mismatches 0 (")))
			{
				FScopeLock Lock(&Mutex);
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

	private:
		FCriticalSection Mutex;
		TArray<FString> Lines;
	};

	int32 CountColour(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && !Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
			{
				++Count;
			}
		}
		return Count;
	}

	const FArcRmlTestDraw* FindColour(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && !Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
			{
				return &Draw;
			}
		}
		return nullptr;
	}

	bool IsSameDraw(const FArcRmlTestDraw& A, const FArcRmlTestDraw& B)
	{
		return A.Op == B.Op && A.Bounds.Min.Equals(B.Bounds.Min, 0.001f) && A.Bounds.Max.Equals(B.Bounds.Max, 0.001f)
			&& A.FirstColourPremultiplied == B.FirstColourPremultiplied && A.Layer == B.Layer && A.bClipMask == B.bClipMask && A.bTextured == B.bTextured
			&& A.bCallbackTexture == B.bCallbackTexture && A.Scissor == B.Scissor;
	}

	bool AreSameDraws(const TArray<FArcRmlTestDraw>& A, const TArray<FArcRmlTestDraw>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (!IsSameDraw(A[Index], B[Index]))
			{
				return false;
			}
		}
		return true;
	}

	int32 CountGlyphs(const TArray<FArcRmlTestDraw>& Draws)
	{
		return ArcRmlUITestHelpers::GetGlyphQuads(Draws, TextColour).Num();
	}
}

TEST_CLASS(ArcRmlRenderVerifyTests, "ArcRmlUI.RenderVerify")
{
	FArcRmlTestView View;
	ArcRmlRenderVerifyTestsPrivate::FVerifyLogCapture Capture;
	int32 SavedVerify = 0;
	int32 SavedPerfLog = 0;
	bool bCaptureAttached = false;

	BEFORE_EACH()
	{
		IConsoleVariable* Verify = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.RenderVerify"));
		IConsoleVariable* PerfLog = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PerfLog"));
		ASSERT_THAT(IsNotNull(Verify));
		ASSERT_THAT(IsNotNull(PerfLog));
		SavedVerify = Verify->GetInt();
		SavedPerfLog = PerfLog->GetInt();
		Verify->Set(1, ECVF_SetByCode);
		PerfLog->Set(0, ECVF_SetByCode);
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
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.RenderVerify"))->Set(SavedVerify, ECVF_SetByCode);
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PerfLog"))->Set(SavedPerfLog, ECVF_SetByCode);
	}

	bool OpenFixture(const TCHAR* RelativePath)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ResetPerfCounters();
	}

	bool RunUntilFrame(TFunctionRef<void()> Action)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		Action();
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > Before && View.FlushUntilSettled();
	}

	bool SetBool(const TCHAR* ModelName, const TCHAR* Field, bool bValue)
	{
		return RunUntilFrame([this, ModelName, Field, bValue]()
		{
			View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeBool(bValue));
		});
	}

	bool SetString(const TCHAR* ModelName, const TCHAR* Field, const TCHAR* Value)
	{
		return RunUntilFrame([this, ModelName, Field, Value]()
		{
			View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeString(Value));
		});
	}

	bool ExpectCleanVerify(const TCHAR* Moment)
	{
		GLog->Flush();
		return Assert.AreEqual(0, Capture.Num(), FString::Printf(TEXT("%s: render verify reported %s"), Moment, *Capture.Describe()));
	}

	bool ExpectOnlyChangedRerecorded(const TCHAR* Moment, int64 MaxRerecorded = ArcRmlRenderVerifyTestsPrivate::MaxRerecordedPerChange)
	{
		const int64 Retained = View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRetained);
		const int64 Rerecorded = View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRerecorded);
		return Assert.IsTrue(Retained >= ArcRmlRenderVerifyTestsPrivate::StaticElements, FString::Printf(TEXT("%s: retained %lld"), Moment, Retained))
			&& Assert.IsTrue(Rerecorded >= 1 && Rerecorded <= MaxRerecorded,
				FString::Printf(TEXT("%s: re-recorded %lld, retained %lld"), Moment, Rerecorded, Retained))
			&& ExpectCleanVerify(Moment);
	}

	bool FindRect(const TCHAR* DomId, FVector2f& OutCentre)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		if (!View.ParseLayout(Elements))
		{
			return false;
		}
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		if (!Element)
		{
			return false;
		}
		OutCentre = Element->Rect.Position + Element->Rect.Size * 0.5f;
		return true;
	}

	TEST_METHOD(UnchangedFrame_RetainsEveryElement_AndDrawsTheSameCommands)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDraw> Before = View.GetDraws();
		ASSERT_THAT(IsTrue(Before.Num() > 0));
		ASSERT_THAT(IsTrue(RunUntilFrame([this]() { View.SetRootClass(TEXT("noop"), true); })));
		const int64 Retained = View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRetained);
		const int64 Rerecorded = View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRerecorded);
		ASSERT_THAT(IsTrue(Retained > 0, FString::Printf(TEXT("retained %lld"), Retained)));
		ASSERT_THAT(AreEqual(int64(0), Rerecorded, TEXT("elements re-recorded by an unchanged frame")));
		ASSERT_THAT(IsTrue(AreSameDraws(Before, View.GetDraws()), TEXT("the retained frame draws the same commands")));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("unchanged"))));
	}

	TEST_METHOD(ClassChange_RerecordsOnlyTheChangedElement)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), Cls)));
		ASSERT_THAT(IsTrue(SetBool(TEXT("rv"), TEXT("Hot"), true)));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), Cls), TEXT("old colour")));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), ClsHot), TEXT("new colour")));
		ASSERT_THAT(IsTrue(ExpectOnlyChangedRerecorded(TEXT("class change"))));
	}

	TEST_METHOD(TextChangeByDataBinding_RerecordsTheTextAndRelayouts)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(2, CountGlyphs(View.GetDraws()), TEXT("glyphs of 'ab'")));
		ASSERT_THAT(IsTrue(SetString(TEXT("rv"), TEXT("Label"), TEXT("abcde"))));
		ASSERT_THAT(AreEqual(5, CountGlyphs(View.GetDraws()), TEXT("glyphs of 'abcde'")));
		ASSERT_THAT(IsTrue(ExpectOnlyChangedRerecorded(TEXT("text change"), 1000)));
	}

	TEST_METHOD(AttributeChange_RerecordsOnlyTheChangedElement)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), Atr)));
		ASSERT_THAT(IsTrue(SetString(TEXT("rv"), TEXT("Mode"), TEXT("b"))));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), Atr), TEXT("old colour")));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), AtrB), TEXT("new colour")));
		ASSERT_THAT(IsTrue(ExpectOnlyChangedRerecorded(TEXT("attribute change"))));
	}

	TEST_METHOD(Hover_RerecordsOnlyTheHoveredElement)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FVector2f Centre;
		ASSERT_THAT(IsTrue(FindRect(TEXT("hov"), Centre)));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), Hov)));
		ASSERT_THAT(IsTrue(RunUntilFrame([this, Centre]() { View.MouseMove(Centre); })));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), HovOn), TEXT("hover colour")));
		ASSERT_THAT(IsTrue(ExpectOnlyChangedRerecorded(TEXT("hover"))));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(RunUntilFrame([this]() { View.MouseMove(FVector2f(900.0f, 600.0f)); })));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), Hov), TEXT("hover left")));
		ASSERT_THAT(IsTrue(ExpectOnlyChangedRerecorded(TEXT("hover end"))));
	}

	TEST_METHOD(ImageLoad_IsNotReplayedBeforeTheTextureArrives)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		bool bImageDrawn = false;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			bImageDrawn = bImageDrawn || (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bTextured && !Draw.bCallbackTexture && Draw.Bounds.GetSize().X >= 19.0f
				&& Draw.Bounds.GetSize().X <= 21.0f && Draw.Bounds.Min.X >= 559.0f && Draw.Bounds.Min.X <= 561.0f);
		}
		ASSERT_THAT(IsTrue(bImageDrawn, TEXT("the loaded image is drawn")));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("image load"))));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(RunUntilFrame([this]() { View.SetRootClass(TEXT("noop"), true); })));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRerecorded), TEXT("the loaded image is retained")));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("after image load"))));
	}

	TEST_METHOD(Scroll_KeepsTheStaticElementsRetainedAndMatchesTheReference)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FVector2f Centre;
		ASSERT_THAT(IsTrue(FindRect(TEXT("scr"), Centre)));
		const TArray<FArcRmlTestDraw> BeforeScroll = View.GetDraws();
		View.MouseMove(Centre);
		ASSERT_THAT(IsTrue(View.Flush() && View.ResetPerfCounters()));
		View.MouseWheel(Centre, -1.0f);
		float ScrollY = 0.0f;
		float Previous = -1.0f;
		int32 Stable = 0;
		for (int32 Attempt = 0; Attempt < 200 && Stable < 5; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			TArray<FArcRmlTestLayoutElement> Elements;
			ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("scr"));
			ASSERT_THAT(IsNotNull(Element));
			ScrollY = Element->Scroll.Y;
			Stable = (ScrollY > 0.0f && FMath::IsNearlyEqual(ScrollY, Previous)) ? Stable + 1 : 0;
			Previous = ScrollY;
		}
		ASSERT_THAT(IsTrue(ScrollY > 0.0f && Stable >= 5, FString::Printf(TEXT("the wheel scrolled the list, scroll top %.2f"), ScrollY)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRetained) >= StaticElements, TEXT("static elements retained while scrolling")));
		ASSERT_THAT(IsFalse(AreSameDraws(BeforeScroll, View.GetDraws()), TEXT("the scrolled rows moved")));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("scroll"))));
	}

	bool ScrollUntil(const TCHAR* DomId, float MinScrollY, float& OutScrollY)
	{
		FVector2f Centre;
		if (!FindRect(DomId, Centre))
		{
			return false;
		}
		View.MouseMove(Centre);
		if (!View.Flush())
		{
			return false;
		}
		OutScrollY = 0.0f;
		for (int32 Wheel = 0; Wheel < 40 && OutScrollY < MinScrollY; ++Wheel)
		{
			View.MouseWheel(Centre, -1.0f);
			float Previous = -1.0f;
			int32 Stable = 0;
			for (int32 Attempt = 0; Attempt < 200 && Stable < 5; ++Attempt)
			{
				FPlatformProcess::Sleep(0.02f);
				TArray<FArcRmlTestLayoutElement> Elements;
				if (!View.Flush() || !View.ParseLayout(Elements))
				{
					return false;
				}
				const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
				if (!Element)
				{
					return false;
				}
				OutScrollY = Element->Scroll.Y;
				Stable = FMath::IsNearlyEqual(OutScrollY, Previous) ? Stable + 1 : 0;
				Previous = OutScrollY;
			}
		}
		return OutScrollY >= MinScrollY && View.FlushUntilSettled();
	}

	TEST_METHOD(Scroll_CulledRowsScrollIntoView_AndMatchTheReference)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/ChunkScroll.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), ChunkMark), TEXT("the row below the scrollport is drawn before it scrolls into view")));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("chunk scroll open"))));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		float ScrollY = 0.0f;
		ASSERT_THAT(IsTrue(ScrollUntil(TEXT("list"), ChunkMarkScroll, ScrollY), FString::Printf(TEXT("the wheel scrolled the list to %.2f"), ScrollY)));
		const int64 Replayed = View.GetPerfCounter(Rml::ArcPerf::Counter::PaintChunksReplayed);
		TestRunner->AddInfo(FString::Printf(TEXT("while scrolling: paint chunks replayed %lld, culled %lld, elements retained %lld, re-recorded %lld"), Replayed,
			View.GetPerfCounter(Rml::ArcPerf::Counter::PaintChunksCulled), View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRetained),
			View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRerecorded)));
		ASSERT_THAT(IsTrue(Replayed > 0, TEXT("no moved chunk was replayed while scrolling")));
		const FArcRmlTestDraw* Mark = FindColour(View.GetDraws(), ChunkMark);
		ASSERT_THAT(IsNotNull(Mark, TEXT("the row scrolled into view is drawn")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Mark->Bounds.Min.Y, 20.0f + 15.0f * 20.0f - ScrollY, 0.5f),
			FString::Printf(TEXT("the scrolled row is drawn at %s for scroll top %.2f"), *Mark->Bounds.ToString(), ScrollY)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRetained) > View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRerecorded),
			FString::Printf(TEXT("retained %lld, re-recorded %lld while scrolling"), View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRetained),
				View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRerecorded))));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("chunk scroll"))));
	}

	TEST_METHOD(TransformedPanelMovesIntoView_DrawsItsChild_AndMatchesTheReference)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/ChunkTransform.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(RunUntilFrame([this]() { View.SetRootClass(TEXT("noop"), true); })));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("panel off screen"))));
		ASSERT_THAT(IsTrue(SetBool(TEXT("ct"), TEXT("Shown"), true)));
		const bool bClean = ExpectCleanVerify(TEXT("panel moved into view"));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), ChunkKid), TEXT("the child of the panel moved into view is drawn")));
		ASSERT_THAT(IsTrue(bClean));
	}

	TEST_METHOD(DataIfSwapInAnInlineBlock_DrawsTheNewContent_AndMatchesTheReference)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/ChunkSwap.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), SwapBefore).Num() > 0, TEXT("the first branch is drawn")));
		ASSERT_THAT(IsTrue(SetBool(TEXT("cs"), TEXT("A"), false)));
		const bool bClean = ExpectCleanVerify(TEXT("data-if swap"));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), SwapBefore).Num(), TEXT("the removed branch is still drawn")));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), SwapAfter).Num() > 0, TEXT("the new branch is drawn")));
		ASSERT_THAT(IsTrue(bClean));
	}

	TEST_METHOD(MaskedPanelInAnEmbeddedTab_MatchesTheReference)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/EmbedFrame.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString TabPath = ArcRmlUITestHelpers::MakePath(TEXT("RenderVerify/EmbedGlassTab.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(TabPath) && View.Embed(TabPath) && View.FlushUntilSettled(),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(RunUntilFrame([this]() { View.SetRootClass(TEXT("noop"), true); })));
		int32 Layers = 0;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			Layers += Draw.Op == EArcRmlTestDrawOp::PushLayer ? 1 : 0;
		}
		ASSERT_THAT(IsTrue(Layers > 0, TEXT("the masked panel edges push a layer")));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("masked panel in an embedded tab"))));
	}

	TEST_METHOD(AncestorTransformChange_RerecordsTheAncestorAndItsChild)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestDraw* Draw = FindColour(View.GetDraws(), Kid);
		ASSERT_THAT(IsNotNull(Draw));
		const FBox2f Before = Draw->Bounds;
		ASSERT_THAT(IsTrue(SetBool(TEXT("rv"), TEXT("Spin"), true)));
		Draw = FindColour(View.GetDraws(), Kid);
		ASSERT_THAT(IsNotNull(Draw));
		ASSERT_THAT(IsTrue(Draw->Bounds.Min.X > Before.Min.X + 5.0f, FString::Printf(TEXT("the child moved with its ancestor: %s to %s"), *Before.ToString(), *Draw->Bounds.ToString())));
		ASSERT_THAT(IsTrue(ExpectOnlyChangedRerecorded(TEXT("ancestor transform"))));
	}

	TEST_METHOD(ClipChange_ResizedOverflowAncestorClipsTheChildAgain)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Retained.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestDraw* Draw = FindColour(View.GetDraws(), Inner);
		ASSERT_THAT(IsNotNull(Draw));
		ASSERT_THAT(IsTrue(SetBool(TEXT("rv"), TEXT("Narrow"), true)));
		Draw = FindColour(View.GetDraws(), Inner);
		ASSERT_THAT(IsNotNull(Draw));
		ASSERT_THAT(IsTrue(ExpectOnlyChangedRerecorded(TEXT("clip change"))));
	}

	TEST_METHOD(BoxShadowBackdropAndRoundedClip_MatchTheReferenceAcrossResizes)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Volatile.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("volatile open"))));
		ASSERT_THAT(IsTrue(RunUntilFrame([this]() { View.SetRootClass(TEXT("noop"), true); })));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRetained) >= 2, TEXT("the static boxes are retained next to the volatile ones")));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("volatile unchanged"))));

		const TCHAR* Fields[] = {TEXT("Shadow"), TEXT("Backdrop"), TEXT("Round")};
		for (const TCHAR* Field : Fields)
		{
			ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
			ASSERT_THAT(IsTrue(SetBool(TEXT("vv"), Field, true)));
			ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRerecorded) >= 1, FString::Printf(TEXT("%s: the resized element is re-recorded"), Field)));
			ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRetained) >= 2, FString::Printf(TEXT("%s: the static boxes stay retained"), Field)));
			ASSERT_THAT(IsTrue(ExpectCleanVerify(Field)));
		}
		for (const TCHAR* Field : Fields)
		{
			ASSERT_THAT(IsTrue(SetBool(TEXT("vv"), Field, false)));
			ASSERT_THAT(IsTrue(ExpectCleanVerify(Field)));
		}
	}

	TEST_METHOD(RoundedClipResize_ChildIsClippedToTheNewBoxInTheSameFrame)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/Volatile.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetBool(TEXT("vv"), TEXT("Round"), true)));
		const FArcRmlTestDraw* Draw = FindColour(View.GetDraws(), RoundKid);
		ASSERT_THAT(IsNotNull(Draw));
		ASSERT_THAT(IsTrue(Draw->bClipMask, TEXT("the child of the rounded overflow box is drawn under the clip mask")));
		ASSERT_THAT(IsTrue(Draw->Bounds.GetSize().X > 159.0f && Draw->Bounds.GetSize().X < 161.0f, FString::Printf(TEXT("child width %s"), *Draw->Bounds.ToString())));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("rounded clip resize"))));
	}

	TEST_METHOD(CanvasSourceChange_PaintsTheTranslucentBodyOnceInBothDirections)
	{
		using namespace ArcRmlRenderVerifyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("RenderVerify/CanvasSource.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		auto FindHalfBlack = [this]() -> TArray<FBox2f>
		{
			TArray<FBox2f> Found;
			for (const FArcRmlTestDraw& Draw : View.GetDraws())
			{
				const FColor& Colour = Draw.FirstColourPremultiplied;
				if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && !Draw.bTextured && !Draw.bShader && Colour.R == 0 && Colour.G == 0 && Colour.B == 0
					&& Colour.A >= 127 && Colour.A <= 128)
				{
					Found.Add(Draw.Bounds);
				}
			}
			return Found;
		};
		auto Describe = [](const TArray<FBox2f>& Boxes) -> FString
		{
			TArray<FString> Parts;
			for (const FBox2f& Box : Boxes)
			{
				Parts.Add(FString::Printf(TEXT("(%.0f,%.0f)-(%.0f,%.0f)"), Box.Min.X, Box.Min.Y, Box.Max.X, Box.Max.Y));
			}
			return FString::Join(Parts, TEXT(" "));
		};
		TArray<FBox2f> Black = FindHalfBlack();
		ASSERT_THAT(IsTrue(Black.Num() == 1 && Black[0].Max == FVector2f(1280.0f, 720.0f), FString::Printf(TEXT("open: the body background paints the canvas once: %s"), *Describe(Black))));

		ASSERT_THAT(IsTrue(RunUntilFrame([this]() { View.MouseMove(FVector2f(20.0f, 20.0f)); })));
		Black = FindHalfBlack();
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), FColor(160, 48, 32, 255)), TEXT("html:hover gains a background: it paints the canvas")));
		ASSERT_THAT(IsTrue(Black.Num() == 1 && Black[0].Max == FVector2f(1280.0f, 300.0f),
			FString::Printf(TEXT("html gains a background: the body paints its own 300 px box once, Chrome as well: %s"), *Describe(Black))));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("html gains a background"))));

		ASSERT_THAT(IsTrue(RunUntilFrame([this]() { View.MouseMove(FVector2f(600.0f, 500.0f)); })));
		Black = FindHalfBlack();
		ASSERT_THAT(IsTrue(Black.Num() == 1 && Black[0].Max == FVector2f(1280.0f, 720.0f),
			FString::Printf(TEXT("html loses its background: the body background paints the canvas once and not its box again: %s"), *Describe(Black))));
		ASSERT_THAT(IsTrue(ExpectCleanVerify(TEXT("html loses its background"))));
	}
};

TEST_CLASS(ArcRmlRenderVerifyPixelTests, "ArcRmlUI.Pixels.RetainedClip")
{
	FArcRmlTestView View;
	int32 SavedVerify = 0;

	BEFORE_EACH()
	{
		IConsoleVariable* Verify = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.RenderVerify"));
		ASSERT_THAT(IsNotNull(Verify));
		SavedVerify = Verify->GetInt();
		Verify->Set(1, ECVF_SetByCode);
	}

	AFTER_EACH()
	{
		View.Close();
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.RenderVerify"))->Set(SavedVerify, ECVF_SetByCode);
	}

	TEST_METHOD(RoundedOverflowBox_ResizedBox_PaintsTheNewShapeInTheSameFrame)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("RenderVerify/Volatile.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		const FColor Narrow = View.GetPixel(150, 220);
		ASSERT_THAT(IsTrue(Narrow == FColor(255, 255, 255, 255), FString::Printf(TEXT("outside the narrow box %d,%d,%d,%d"), Narrow.R, Narrow.G, Narrow.B, Narrow.A)));

		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetModelValue(TEXT("vv"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Round")), ArcRmlUITestHelpers::MakeBool(true));
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		}
		ASSERT_THAT(IsTrue(View.GetProbe().FrameSerial > Before));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		const FColor Wide = View.GetPixel(150, 220);
		ASSERT_THAT(IsTrue(FMath::Abs(Wide.R - 255) <= 2 && FMath::Abs(Wide.G - 204) <= 2 && Wide.B <= 2, FString::Printf(TEXT("inside the widened box %d,%d,%d,%d"), Wide.R, Wide.G, Wide.B, Wide.A)));
		const FColor Corner = View.GetPixel(168, 202);
		ASSERT_THAT(IsTrue(Corner.B > 200, FString::Printf(TEXT("the rounded corner of the widened box stays cut %d,%d,%d,%d"), Corner.R, Corner.G, Corner.B, Corner.A)));
	}
};
