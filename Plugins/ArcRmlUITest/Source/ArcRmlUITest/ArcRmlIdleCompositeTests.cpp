// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Engine/Texture.h"
#include "GlobalRenderResources.h"
#include "Render/ArcRmlRecordingRenderInterface.h"
#include "Render/ArcRmlRenderTypes.h"
#include "RenderResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"

namespace ArcRmlIdleCompositeTestsPrivate
{
	constexpr int32 Tolerance = 1;

	struct FReplayResult
	{
		int32 Commands = 0;
		int32 IdleComposites = 0;
		int32 DrawCalls = 0;
		TArray<FColor> Pixels;
	};

	bool Replay(FArcRmlTestView& View, bool bIdlePath, FReplayResult& Out)
	{
		if (!View.RenderPixels(true, bIdlePath))
		{
			return false;
		}
		const FIntPoint Size = View.GetPixelSize();
		Out.Commands = View.GetReplayCommands();
		Out.IdleComposites = View.GetReplayIdleComposites();
		Out.DrawCalls = View.GetReplayDrawCalls();
		Out.Pixels.Reset(Size.X * Size.Y);
		for (int32 Y = 0; Y < Size.Y; ++Y)
		{
			for (int32 X = 0; X < Size.X; ++X)
			{
				Out.Pixels.Add(View.GetPixel(X, Y));
			}
		}
		return true;
	}

	bool IsNear(const FColor& A, const FColor& B)
	{
		return FMath::Abs(A.R - B.R) <= Tolerance && FMath::Abs(A.G - B.G) <= Tolerance && FMath::Abs(A.B - B.B) <= Tolerance
			&& FMath::Abs(A.A - B.A) <= Tolerance;
	}

	FString DescribeDifference(const FReplayResult& Idle, const FReplayResult& Direct, int32 Width)
	{
		if (Idle.Pixels.Num() != Direct.Pixels.Num())
		{
			return FString::Printf(TEXT("readback sizes differ: %d and %d"), Idle.Pixels.Num(), Direct.Pixels.Num());
		}
		int32 Differences = 0;
		int32 First = INDEX_NONE;
		for (int32 Index = 0; Index < Idle.Pixels.Num(); ++Index)
		{
			if (!IsNear(Idle.Pixels[Index], Direct.Pixels[Index]))
			{
				First = First == INDEX_NONE ? Index : First;
				++Differences;
			}
		}
		if (First == INDEX_NONE)
		{
			return TEXT("equal");
		}
		const FColor A = Idle.Pixels[First];
		const FColor B = Direct.Pixels[First];
		return FString::Printf(TEXT("%d pixels differ by more than %d, first at %d,%d: idle %d,%d,%d,%d direct %d,%d,%d,%d"), Differences, Tolerance, First % Width,
			First / Width, A.R, A.G, A.B, A.A, B.R, B.G, B.B, B.A);
	}

	bool AreNear(const FReplayResult& Idle, const FReplayResult& Direct)
	{
		if (Idle.Pixels.Num() != Direct.Pixels.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Idle.Pixels.Num(); ++Index)
		{
			if (!IsNear(Idle.Pixels[Index], Direct.Pixels[Index]))
			{
				return false;
			}
		}
		return true;
	}

	void SwapTextureReference(UTexture& Texture, FTextureReferenceRHIRef& InOut)
	{
		FTextureReference* Reference = &Texture.TextureReference;
		FTextureReferenceRHIRef* Swapped = &InOut;
		ENQUEUE_RENDER_COMMAND(ArcRmlIdleTestSwapTextureReference)(
			[Reference, Swapped](FRHICommandListImmediate&)
			{
				Swap(Reference->TextureReferenceRHI, *Swapped);
			});
		FlushRenderingCommands();
	}

	FRHITexture* PointTextureReferenceAt(UTexture& Texture, bool bWhite)
	{
		FTextureReference* Reference = &Texture.TextureReference;
		FRHITexture* Previous = nullptr;
		FRHITexture** PreviousOut = &Previous;
		ENQUEUE_RENDER_COMMAND(ArcRmlIdleTestPointTextureReference)(
			[Reference, bWhite, PreviousOut](FRHICommandListImmediate& RHICmdList)
			{
				*PreviousOut = Reference->TextureReferenceRHI->GetReferencedTexture();
				RHICmdList.UpdateTextureReference(Reference->TextureReferenceRHI, bWhite ? GWhiteTexture->TextureRHI.GetReference() : nullptr);
			});
		FlushRenderingCommands();
		return Previous;
	}

	void RestoreTextureReference(UTexture& Texture, FRHITexture* Previous)
	{
		FTextureReference* Reference = &Texture.TextureReference;
		ENQUEUE_RENDER_COMMAND(ArcRmlIdleTestRestoreTextureReference)(
			[Reference, Previous](FRHICommandListImmediate& RHICmdList)
			{
				RHICmdList.UpdateTextureReference(Reference->TextureReferenceRHI, Previous);
			});
		FlushRenderingCommands();
	}

	bool RecordsBackdropRead(TFunctionRef<void(FArcRmlRecordingRenderInterface&)> Record)
	{
		FArcRmlRecordingRenderInterface Recorder;
		Recorder.BeginFrame(FIntPoint(256, 256));
		Record(Recorder);
		const TUniquePtr<FArcRmlRenderFrame> Frame = Recorder.EndFrame();
		return Frame->bReadsBackdrop;
	}

	void SetScissor(FArcRmlRecordingRenderInterface& Recorder)
	{
		Recorder.SetScissorRegion(Rml::Rectanglei::FromPositionSize(Rml::Vector2i(10, 10), Rml::Vector2i(64, 64)));
	}
}

TEST_CLASS(ArcRmlIdleCompositeTests, "ArcRmlUI.Pixels")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* RelativePath)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		FArcRmlTestView::DropCompiledDocument(Path);
		return Assert.IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(),
				   ArcRmlUITestHelpers::DescribeDiagnostics(View))
			&& Assert.AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View));
	}

	bool ReadsBackdropInProbe() const
	{
		return View.GetDraws().ContainsByPredicate([](const FArcRmlTestDraw& Draw) -> bool
		{
			return Draw.Op == EArcRmlTestDrawOp::CompositeLayers && Draw.SourceLayer == 0;
		});
	}

	void ExpectIdleMatchesDirect(const TCHAR* RelativePath)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(RelativePath)));
		ASSERT_THAT(IsFalse(ReadsBackdropInProbe(), FString::Printf(TEXT("%s reads the backdrop, so it does not cover the idle path"), RelativePath)));
		FReplayResult Direct;
		FReplayResult FirstIdle;
		FReplayResult SecondIdle;
		ASSERT_THAT(IsTrue(Replay(View, false, Direct), TEXT("direct replay")));
		ASSERT_THAT(IsTrue(Replay(View, true, FirstIdle), TEXT("first idle draw")));
		ASSERT_THAT(IsTrue(Replay(View, true, SecondIdle), TEXT("second idle draw")));
		ASSERT_THAT(IsTrue(Direct.Commands > 0, FString::Printf(TEXT("%s: the direct replay replayed no command"), RelativePath)));
		ASSERT_THAT(AreEqual(0, Direct.IdleComposites, TEXT("the direct replay composited an idle texture")));
		ASSERT_THAT(AreEqual(Direct.Commands, FirstIdle.Commands, TEXT("the first idle draw did not replay the frame once")));
		ASSERT_THAT(AreEqual(1, FirstIdle.IdleComposites, TEXT("the first idle draw did not composite its idle texture")));
		ASSERT_THAT(AreEqual(0, SecondIdle.Commands, TEXT("the second idle draw replayed commands")));
		ASSERT_THAT(AreEqual(1, SecondIdle.IdleComposites, TEXT("the second idle draw did not composite its idle texture")));
		ASSERT_THAT(IsTrue(AreNear(FirstIdle, Direct), FString::Printf(TEXT("%s first idle draw: %s"), RelativePath, *DescribeDifference(FirstIdle, Direct,
			View.GetPixelSize().X))));
		ASSERT_THAT(IsTrue(AreNear(SecondIdle, Direct), FString::Printf(TEXT("%s second idle draw: %s"), RelativePath, *DescribeDifference(SecondIdle, Direct,
			View.GetPixelSize().X))));
	}

	TEST_METHOD(SmokeFixture_IdlePixelsEqualDirectReplay)
	{
		ExpectIdleMatchesDirect(TEXT("CssParity/Pixels/PixelSmokeTest.rml"));
	}

	TEST_METHOD(GradientHardStopFixture_IdlePixelsEqualDirectReplay)
	{
		ExpectIdleMatchesDirect(TEXT("CssParity/Pixels/GradientHardStopTest.rml"));
	}

	TEST_METHOD(OutlineFixture_IdlePixelsEqualDirectReplay)
	{
		ExpectIdleMatchesDirect(TEXT("CssParity/Outline/OutlineTest.rml"));
	}

	TEST_METHOD(PaintOrderFixture_IdlePixelsEqualDirectReplay)
	{
		ExpectIdleMatchesDirect(TEXT("CssParity/Text/PaintOrderTest.rml"));
	}

	TEST_METHOD(FontFeatureFixture_IdlePixelsEqualDirectReplay)
	{
		ExpectIdleMatchesDirect(TEXT("CssParity/Text/FontFeaturePixelTest.rml"));
	}

	TEST_METHOD(SlotGridFixture_IdlePixelsEqualDirectReplay)
	{
		ExpectIdleMatchesDirect(TEXT("Batch/SlotGrid.rml"));
	}

	TEST_METHOD(ClipMaskFixture_IdlePixelsEqualDirectReplay)
	{
		ExpectIdleMatchesDirect(TEXT("Batch/ClipMaskBreak.rml"));
	}

	TEST_METHOD(LayersFiltersMasksAndShadows_IdlePixelsEqualDirectReplay)
	{
		ExpectIdleMatchesDirect(TEXT("Idle/Layers.rml"));
	}

	TEST_METHOD(FrameWithoutBackdropRead_FirstDrawIsDirectAndThirdDrawOnlyComposites)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Batch/SameRun.rml"))));
		FReplayResult First;
		ASSERT_THAT(IsTrue(Replay(View, true, First)));
		ASSERT_THAT(IsTrue(First.Commands > 0 && First.DrawCalls > 0, FString::Printf(TEXT("first draw: %d commands, %d draws"), First.Commands, First.DrawCalls)));
		ASSERT_THAT(AreEqual(0, First.IdleComposites, TEXT("the first draw of a frame composited an idle texture")));
		ASSERT_THAT(IsFalse(View.HasIdleComposite(), TEXT("the first draw of a frame kept an idle texture")));
		FReplayResult Second;
		FReplayResult Third;
		ASSERT_THAT(IsTrue(Replay(View, true, Second)));
		ASSERT_THAT(AreEqual(First.Commands, Second.Commands, TEXT("the second draw did not replay the frame once into the idle texture")));
		ASSERT_THAT(AreEqual(1, Second.IdleComposites));
		ASSERT_THAT(IsTrue(View.HasIdleComposite(), TEXT("the second draw of a frame kept no idle texture")));
		ASSERT_THAT(IsTrue(Replay(View, true, Third)));
		ASSERT_THAT(AreEqual(0, Third.Commands));
		ASSERT_THAT(AreEqual(0, Third.DrawCalls));
		ASSERT_THAT(AreEqual(1, Third.IdleComposites));
		ASSERT_THAT(IsTrue(AreNear(First, Third), DescribeDifference(Third, First, View.GetPixelSize().X)));
	}

	TEST_METHOD(NewFrameEveryDraw_KeepsNoIdleComposite)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Idle/Layers.rml"))));
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			View.SetRootClass(TEXT("alt"), Frame % 2 == 0);
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
			FReplayResult Drawn;
			ASSERT_THAT(IsTrue(Replay(View, true, Drawn)));
			ASSERT_THAT(IsTrue(Drawn.Commands > 0, FString::Printf(TEXT("frame %d: the first draw replayed nothing"), Frame)));
			ASSERT_THAT(AreEqual(0, Drawn.IdleComposites, FString::Printf(TEXT("frame %d: the first draw composited an idle texture"), Frame)));
			ASSERT_THAT(IsFalse(View.HasIdleComposite(), FString::Printf(TEXT("frame %d: a frame drawn once kept an idle texture"), Frame)));
			FReplayResult Direct;
			ASSERT_THAT(IsTrue(Replay(View, false, Direct)));
			ASSERT_THAT(IsTrue(AreNear(Drawn, Direct), FString::Printf(TEXT("frame %d: %s"), Frame, *DescribeDifference(Drawn, Direct, View.GetPixelSize().X))));
		}
	}

	TEST_METHOD(NewFrameAfterStaticFrame_ReplaysOnceIntoTheKeptTexture)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Idle/Layers.rml"))));
		FReplayResult Static;
		ASSERT_THAT(IsTrue(Replay(View, true, Static) && Replay(View, true, Static)));
		View.SetRootClass(TEXT("alt"), true);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		FReplayResult First;
		FReplayResult Second;
		FReplayResult Direct;
		ASSERT_THAT(IsTrue(Replay(View, true, First)));
		ASSERT_THAT(AreEqual(1, First.IdleComposites, TEXT("the first draw of a new frame after a static frame did not composite an idle texture")));
		ASSERT_THAT(IsTrue(View.HasIdleComposite(), TEXT("the first draw of a new frame after a static frame kept no idle texture")));
		ASSERT_THAT(IsTrue(Replay(View, true, Second) && Replay(View, false, Direct)));
		ASSERT_THAT(IsTrue(Direct.Commands > 0));
		ASSERT_THAT(AreEqual(Direct.Commands, First.Commands, TEXT("the first draw did not replay the new frame once")));
		ASSERT_THAT(AreEqual(0, Second.Commands, TEXT("the second draw of the new frame replayed it again")));
		ASSERT_THAT(AreEqual(1, Second.IdleComposites));
		ASSERT_THAT(IsTrue(First.Pixels != Static.Pixels, TEXT("the new frame shows the previous frame's pixels")));
		ASSERT_THAT(IsTrue(AreNear(First, Direct), DescribeDifference(First, Direct, View.GetPixelSize().X)));
		ASSERT_THAT(IsTrue(AreNear(Second, Direct), DescribeDifference(Second, Direct, View.GetPixelSize().X)));
	}

	TEST_METHOD(BackdropFilterFrame_LaterDrawsCompositeCachedOverlays)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Idle/Backdrop.rml"))));
		ASSERT_THAT(IsTrue(ReadsBackdropInProbe(), TEXT("the glass panel recorded no composite from layer 0")));
		FReplayResult Direct;
		FReplayResult First;
		FReplayResult Second;
		ASSERT_THAT(IsTrue(Replay(View, false, Direct) && Replay(View, true, First) && Replay(View, true, Second)));
		ASSERT_THAT(IsTrue(Direct.Commands > 0));
		ASSERT_THAT(IsTrue(Second.Commands < Direct.Commands, TEXT("a later draw of a frame that reads the backdrop replayed every command")));
		ASSERT_THAT(IsTrue(Second.IdleComposites > 0, TEXT("a later draw of a frame that reads the backdrop composited no cached overlay")));
		ASSERT_THAT(IsTrue(View.HasIdleComposite(), TEXT("a frame that reads the backdrop kept no cached overlay")));
		ASSERT_THAT(IsTrue(AreNear(First, Direct), DescribeDifference(First, Direct, View.GetPixelSize().X)));
		ASSERT_THAT(IsTrue(AreNear(Second, Direct), DescribeDifference(Second, Direct, View.GetPixelSize().X)));
	}

	bool RecordNewFrame(TFunctionRef<void()> Change)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		Change();
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > Before;
	}

	bool DrawSteady(int32& OutDrawCalls, int32& OutRhiCommands)
	{
		if (!View.DrawFrame() || !View.DrawFrame())
		{
			return false;
		}
		OutDrawCalls = View.GetDrawGpuDrawCalls();
		OutRhiCommands = View.GetDrawGpuRhiCommands();
		return true;
	}

	TEST_METHOD(BackdropFrameThatRegeneratesAShadow_SteadyDrawsDoNotRegenerateIt)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("RenderVerify/Volatile.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ReadsBackdropInProbe(), TEXT("the glass box recorded no composite from layer 0")));
		View.SetDestination(FIntRect(0, 0, 1280, 720));

		ASSERT_THAT(IsTrue(RecordNewFrame([this]()
		{
			View.SetModelValue(TEXT("vv"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Shadow")), ArcRmlUITestHelpers::MakeBool(true));
		}), TEXT("the shadow change recorded no frame")));
		int32 ChangedDrawCalls = 0;
		int32 ChangedRhiCommands = 0;
		ASSERT_THAT(IsTrue(DrawSteady(ChangedDrawCalls, ChangedRhiCommands)));
		ArcRmlIdleCompositeTestsPrivate::FReplayResult Changed;
		ASSERT_THAT(IsTrue(ArcRmlIdleCompositeTestsPrivate::Replay(View, false, Changed), TEXT("readback after the shadow change")));

		ASSERT_THAT(IsTrue(RecordNewFrame([this]()
		{
			View.SetRootClass(TEXT("noop"), true);
		}), TEXT("the root class recorded no frame")));
		int32 RerecordedDrawCalls = 0;
		int32 RerecordedRhiCommands = 0;
		ASSERT_THAT(IsFalse(View.GetDraws().ContainsByPredicate([](const FArcRmlTestDraw& Draw) -> bool
		{
			return Draw.Op == EArcRmlTestDrawOp::SaveLayerAsTexture;
		}), TEXT("the frame that changed nothing saved a layer texture, so it is no baseline")));
		ASSERT_THAT(IsTrue(DrawSteady(RerecordedDrawCalls, RerecordedRhiCommands)));
		ArcRmlIdleCompositeTestsPrivate::FReplayResult Rerecorded;
		ASSERT_THAT(IsTrue(ArcRmlIdleCompositeTestsPrivate::Replay(View, false, Rerecorded), TEXT("readback after the frame that changed nothing")));
		ASSERT_THAT(IsTrue(ArcRmlIdleCompositeTestsPrivate::AreNear(Changed, Rerecorded), FString::Printf(TEXT("the two frames show different scenes: %s"),
			*ArcRmlIdleCompositeTestsPrivate::DescribeDifference(Changed, Rerecorded, View.GetPixelSize().X))));

		ASSERT_THAT(IsTrue(ChangedDrawCalls == RerecordedDrawCalls && ChangedRhiCommands == RerecordedRhiCommands,
			FString::Printf(TEXT("a steady draw after the shadow change did %d draw calls and %d RHI commands, after a frame that changed nothing %d and %d: "
				"every draw regenerates the shadow texture of the last recorded frame"), ChangedDrawCalls, ChangedRhiCommands, RerecordedDrawCalls,
				RerecordedRhiCommands)));
	}

	TEST_METHOD(Recorder_CompositeFromBaseLayer_ReadsBackdrop)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		ASSERT_THAT(IsTrue(RecordsBackdropRead([](FArcRmlRecordingRenderInterface& Recorder)
		{
			const Rml::LayerHandle Temp = Recorder.PushLayer();
			Recorder.CompositeLayers(0, Temp, Rml::BlendMode::Blend, {});
			Recorder.CompositeLayers(Temp, 0, Rml::BlendMode::Blend, {});
			Recorder.PopLayer();
		})));
	}

	TEST_METHOD(Recorder_NonBlendCompositeIntoBaseLayer_ReadsBackdrop)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		for (const Rml::BlendMode Mode : {Rml::BlendMode::Replace, Rml::BlendMode::SourceIn, Rml::BlendMode::SourceOut, Rml::BlendMode::Xor})
		{
			ASSERT_THAT(IsTrue(RecordsBackdropRead([Mode](FArcRmlRecordingRenderInterface& Recorder)
			{
				const Rml::LayerHandle Layer = Recorder.PushLayer();
				Recorder.CompositeLayers(Layer, 0, Mode, {});
				Recorder.PopLayer();
			}), FString::Printf(TEXT("blend mode %d"), static_cast<int32>(Mode))));
		}
	}

	TEST_METHOD(Recorder_SaveLayerWhileTheTopIsTheBaseLayer_ReadsBackdrop)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		ASSERT_THAT(IsTrue(RecordsBackdropRead([](FArcRmlRecordingRenderInterface& Recorder)
		{
			SetScissor(Recorder);
			Recorder.SaveLayerAsTexture();
		}), TEXT("SaveLayerAsTexture")));
		ASSERT_THAT(IsTrue(RecordsBackdropRead([](FArcRmlRecordingRenderInterface& Recorder)
		{
			Recorder.SaveLayerAsMaskImage();
		}), TEXT("SaveLayerAsMaskImage")));
	}

	TEST_METHOD(Recorder_LayerOpsThatKeepOffTheBaseLayer_DoNotReadBackdrop)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		FArcRmlRecordingRenderInterface Recorder;
		Recorder.BeginFrame(FIntPoint(256, 256));
		Recorder.PushLayer();
		Recorder.CompositeLayers(0, 1, Rml::BlendMode::Blend, {});
		Recorder.PopLayer();
		ASSERT_THAT(IsTrue(Recorder.EndFrame()->bReadsBackdrop, TEXT("the reading frame")));

		Recorder.BeginFrame(FIntPoint(256, 256));
		const Rml::LayerHandle Outer = Recorder.PushLayer();
		const Rml::LayerHandle Inner = Recorder.PushLayer();
		Recorder.CompositeLayers(Outer, Inner, Rml::BlendMode::Blend, {});
		Recorder.CompositeLayers(Inner, Outer, Rml::BlendMode::SourceIn, {});
		SetScissor(Recorder);
		Recorder.SaveLayerAsTexture();
		Recorder.SaveLayerAsMaskImage();
		Recorder.PopLayer();
		Recorder.CompositeLayers(Outer, 0, Rml::BlendMode::Blend, {});
		Recorder.PopLayer();
		ASSERT_THAT(IsFalse(Recorder.EndFrame()->bReadsBackdrop, TEXT("a frame after a reading frame, with layer ops above the base layer only")));
	}

	TEST_METHOD(IdleComposite_IsDroppedOnTheNextSubmit)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Idle/Layers.rml"))));
		FReplayResult Before;
		ASSERT_THAT(IsTrue(Replay(View, true, Before) && Replay(View, true, Before)));
		ASSERT_THAT(IsTrue(View.HasIdleComposite(), TEXT("the second draw kept no idle texture")));
		View.SetRootClass(TEXT("alt"), true);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsFalse(View.HasIdleComposite(), TEXT("the submitted frame kept the previous frame's idle texture")));
		FReplayResult After;
		FReplayResult Direct;
		ASSERT_THAT(IsTrue(Replay(View, true, After) && Replay(View, false, Direct)));
		ASSERT_THAT(IsTrue(After.Commands > 0, TEXT("the first draw of the new frame replayed nothing")));
		ASSERT_THAT(AreEqual(1, After.IdleComposites, TEXT("the first draw of the new frame after a static frame did not composite an idle texture")));
		ASSERT_THAT(IsTrue(After.Pixels != Before.Pixels, TEXT("the new frame shows the previous frame's pixels")));
		ASSERT_THAT(IsTrue(AreNear(After, Direct), DescribeDifference(After, Direct, View.GetPixelSize().X)));
	}

	TEST_METHOD(IdleComposite_IsDroppedOnResize)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Idle/Layers.rml"))));
		View.SetDestination(FIntRect(0, 0, 1280, 720));
		FReplayResult Result;
		ASSERT_THAT(IsTrue(Replay(View, true, Result) && Replay(View, true, Result)));
		View.SetDestination(FIntRect(40, 30, 1320, 750));
		ASSERT_THAT(IsTrue(View.HasIdleComposite(), TEXT("moving the destination without resizing it dropped the idle texture")));
		View.SetDestination(FIntRect(40, 30, 1000, 600));
		ASSERT_THAT(IsFalse(View.HasIdleComposite(), TEXT("resizing the destination kept the idle texture")));

		ASSERT_THAT(IsTrue(Replay(View, true, Result)));
		ASSERT_THAT(IsTrue(View.HasIdleComposite()));
		View.Resize(FIntPoint(1024, 600), 1.0f);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsFalse(View.HasIdleComposite(), TEXT("resizing the view kept the idle texture")));
		ASSERT_THAT(IsTrue(Replay(View, true, Result)));
		ASSERT_THAT(IsTrue(View.GetPixelSize() == FIntPoint(1024, 600), View.GetPixelSize().ToString()));
	}

	TEST_METHOD(IdleComposite_IsDroppedWhenTheViewCloses)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Idle/Layers.rml"))));
		FReplayResult Result;
		ASSERT_THAT(IsTrue(Replay(View, true, Result) && Replay(View, true, Result)));
		ASSERT_THAT(IsTrue(View.HasIdleComposite()));
		View.Remove();
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsFalse(View.HasIdleComposite(), TEXT("the closed view kept its idle texture")));
	}

	TEST_METHOD(UnreadyAssetTexture_KeepsNoIdleCompositeUntilItResolves)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		UTexture* Icon = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/AICON-Green.AICON-Green"));
		ASSERT_THAT(IsNotNull(Icon));
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Batch/AssetIcons.rml"))));
		FReplayResult First;
		ASSERT_THAT(IsTrue(Replay(View, true, First)));

		FTextureReferenceRHIRef Saved;
		SwapTextureReference(*Icon, Saved);
		FReplayResult Unready;
		const bool bReplayed = Replay(View, true, Unready);
		const bool bKept = View.HasIdleComposite();
		SwapTextureReference(*Icon, Saved);
		ASSERT_THAT(IsTrue(bReplayed));
		ASSERT_THAT(AreEqual(1, Unready.IdleComposites));
		ASSERT_THAT(IsFalse(bKept, TEXT("a replay that missed a texture kept its idle texture")));

		FReplayResult Ready;
		FReplayResult Reused;
		FReplayResult Direct;
		ASSERT_THAT(IsTrue(Replay(View, true, Ready) && Replay(View, true, Reused) && Replay(View, false, Direct)));
		ASSERT_THAT(IsTrue(Ready.Commands > 0, TEXT("the draw after the texture resolved did not replay")));
		ASSERT_THAT(IsTrue(Ready.Pixels != Unready.Pixels, TEXT("the resolved icons are not drawn")));
		ASSERT_THAT(AreEqual(0, Reused.Commands));
		ASSERT_THAT(IsTrue(AreNear(Reused, Direct), DescribeDifference(Reused, Direct, View.GetPixelSize().X)));
	}

	TEST_METHOD(AssetTextureAtTheDefaultTexture_KeepsNoIdleComposite)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		UTexture* Icon = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/AICON-Green.AICON-Green"));
		ASSERT_THAT(IsNotNull(Icon));
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Batch/AssetIcons.rml"))));
		FReplayResult First;
		ASSERT_THAT(IsTrue(Replay(View, true, First)));

		FRHITexture* const Previous = PointTextureReferenceAt(*Icon, false);
		FReplayResult Unready;
		const bool bReplayed = Replay(View, true, Unready);
		const bool bKept = View.HasIdleComposite();
		RestoreTextureReference(*Icon, Previous);
		ASSERT_THAT(IsTrue(bReplayed));
		ASSERT_THAT(AreEqual(1, Unready.IdleComposites));
		ASSERT_THAT(IsFalse(bKept, TEXT("a replay that drew the default texture for an asset kept its idle texture")));
	}

	TEST_METHOD(KeptIdleComposite_IsReplacedWhenAnAssetTextureChanges)
	{
		using namespace ArcRmlIdleCompositeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		UTexture* Icon = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/AICON-Green.AICON-Green"));
		ASSERT_THAT(IsNotNull(Icon));
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Batch/AssetIcons.rml"))));

		FReplayResult First;
		FReplayResult Kept;
		FReplayResult Reused;
		ASSERT_THAT(IsTrue(Replay(View, true, First) && Replay(View, true, Kept) && Replay(View, true, Reused)));
		ASSERT_THAT(AreEqual(0, Reused.Commands));
		ASSERT_THAT(IsTrue(View.HasIdleComposite()));

		FRHITexture* const Previous = PointTextureReferenceAt(*Icon, true);
		FReplayResult Changed;
		FReplayResult Direct;
		const bool bReplayed = Replay(View, true, Changed) && Replay(View, false, Direct);
		RestoreTextureReference(*Icon, Previous);
		ASSERT_THAT(IsTrue(bReplayed));
		ASSERT_THAT(IsTrue(Changed.Commands > 0, TEXT("the draw after the asset texture changed reused the kept idle texture")));
		ASSERT_THAT(IsTrue(Changed.Pixels != Reused.Pixels, TEXT("the changed asset texture is not drawn")));
		ASSERT_THAT(IsTrue(AreNear(Changed, Direct), DescribeDifference(Changed, Direct, View.GetPixelSize().X)));
	}
};
