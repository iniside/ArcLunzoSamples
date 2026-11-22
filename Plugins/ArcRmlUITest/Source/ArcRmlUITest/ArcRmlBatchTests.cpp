// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Engine/Texture.h"
#include "Render/ArcRmlRenderTypes.h"
#include "RenderResource.h"
#include "RenderingThread.h"

namespace ArcRmlBatchTestsPrivate
{
	struct FReplayResult
	{
		int32 DrawCalls = 0;
		int32 RhiCommands = 0;
		TArray<int32> RunBreaks;
		TArray<FColor> Pixels;
	};

	const TCHAR* const SrgbIcon = TEXT("/Engine/EngineResources/AICON-Green.AICON-Green");
	const TCHAR* const LinearIcon = TEXT("/Engine/EngineMaterials/DefaultDiffuse_TC_Masks.DefaultDiffuse_TC_Masks");

	int32 CountRunBreaks(const FReplayResult& Result)
	{
		int32 Total = 0;
		for (const int32 Breaks : Result.RunBreaks)
		{
			Total += Breaks;
		}
		return Total;
	}

	int32 GetRunBreaks(const FReplayResult& Result, EArcRmlRunBreak Break)
	{
		const int32 Index = static_cast<int32>(Break);
		return Result.RunBreaks.IsValidIndex(Index) ? Result.RunBreaks[Index] : INDEX_NONE;
	}

	void SwapTextureReference(UTexture& Texture, FTextureReferenceRHIRef& InOut)
	{
		FTextureReference* Reference = &Texture.TextureReference;
		FTextureReferenceRHIRef* Swapped = &InOut;
		ENQUEUE_RENDER_COMMAND(ArcRmlBatchTestSwapTextureReference)(
			[Reference, Swapped](FRHICommandListImmediate&)
			{
				Swap(Reference->TextureReferenceRHI, *Swapped);
			});
		FlushRenderingCommands();
	}

	bool Replay(FArcRmlTestView& View, bool bMergeDraws, FReplayResult& Out)
	{
		if (!View.RenderPixels(bMergeDraws))
		{
			return false;
		}
		const FIntPoint Size = View.GetPixelSize();
		Out.DrawCalls = View.GetReplayDrawCalls();
		Out.RhiCommands = View.GetReplayRhiCommands();
		Out.RunBreaks = View.GetReplayRunBreaks();
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

	FString DescribeFirstPixelDifference(const FReplayResult& Merged, const FReplayResult& Unmerged, int32 Width)
	{
		int32 Differences = 0;
		int32 First = INDEX_NONE;
		for (int32 Index = 0; Index < Merged.Pixels.Num(); ++Index)
		{
			if (Merged.Pixels[Index] != Unmerged.Pixels[Index])
			{
				First = First == INDEX_NONE ? Index : First;
				++Differences;
			}
		}
		if (First == INDEX_NONE)
		{
			return TEXT("identical");
		}
		const FColor A = Merged.Pixels[First];
		const FColor B = Unmerged.Pixels[First];
		return FString::Printf(TEXT("%d pixels differ, first at %d,%d: merged %d,%d,%d,%d unmerged %d,%d,%d,%d"), Differences, First % Width, First / Width, A.R, A.G,
			A.B, A.A, B.R, B.G, B.B, B.A);
	}
}

TEST_CLASS(ArcRmlBatchTests, "ArcRmlUI.Pixels")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool ReplayBoth(const TCHAR* RelativePath, ArcRmlBatchTestsPrivate::FReplayResult& OutMerged, ArcRmlBatchTestsPrivate::FReplayResult& OutUnmerged)
	{
		using namespace ArcRmlBatchTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		FArcRmlTestView::DropCompiledDocument(Path);
		return Assert.IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View))
			&& Assert.AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View))
			&& Assert.IsTrue(Replay(View, false, OutUnmerged), TEXT("unmerged replay")) && Assert.IsTrue(Replay(View, true, OutMerged), TEXT("merged replay"))
			&& Assert.IsTrue(OutMerged.Pixels.Num() == OutUnmerged.Pixels.Num(), TEXT("readback sizes differ"));
	}

	void ExpectMergedDrawsSaved(const TCHAR* RelativePath, int32 ExpectedSaved)
	{
		using namespace ArcRmlBatchTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FReplayResult Merged;
		FReplayResult Unmerged;
		ASSERT_THAT(IsTrue(ReplayBoth(RelativePath, Merged, Unmerged)));
		ASSERT_THAT(AreEqual(ExpectedSaved, Unmerged.DrawCalls - Merged.DrawCalls,
			FString::Printf(TEXT("%s: unmerged %d draws, merged %d draws"), RelativePath, Unmerged.DrawCalls, Merged.DrawCalls)));
		ASSERT_THAT(IsTrue(Merged.Pixels == Unmerged.Pixels, DescribeFirstPixelDifference(Merged, Unmerged, View.GetPixelSize().X)));
	}

	TEST_METHOD(EightConsecutiveColourRects_ReplayAsOneDraw_WithUnmergedPixels)
	{
		using namespace ArcRmlBatchTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FReplayResult Merged;
		FReplayResult Unmerged;
		ASSERT_THAT(IsTrue(ReplayBoth(TEXT("Batch/SameRun.rml"), Merged, Unmerged)));
		ASSERT_THAT(AreEqual(8, Unmerged.DrawCalls));
		ASSERT_THAT(AreEqual(1, Merged.DrawCalls));
		ASSERT_THAT(IsTrue(Merged.Pixels == Unmerged.Pixels, DescribeFirstPixelDifference(Merged, Unmerged, View.GetPixelSize().X)));
	}

	TEST_METHOD(TenRowsAlternatingBackgroundAndText_ReplayAsOneDraw_WithUnmergedPixels)
	{
		using namespace ArcRmlBatchTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FReplayResult Merged;
		FReplayResult Unmerged;
		ASSERT_THAT(IsTrue(ReplayBoth(TEXT("Batch/AlternatingRows.rml"), Merged, Unmerged)));
		ASSERT_THAT(AreEqual(20, Unmerged.DrawCalls));
		ASSERT_THAT(AreEqual(1, Merged.DrawCalls));
		ASSERT_THAT(IsTrue(Merged.Pixels == Unmerged.Pixels, DescribeFirstPixelDifference(Merged, Unmerged, View.GetPixelSize().X)));
	}

	TEST_METHOD(SixClippedGroups_ReplayAsOneDrawWithOneStateSet)
	{
		using namespace ArcRmlBatchTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		constexpr int32 PassCommands = 2;
		constexpr int32 StateCommands = 5;
		FReplayResult Merged;
		FReplayResult Unmerged;
		ASSERT_THAT(IsTrue(ReplayBoth(TEXT("Batch/ScissorRuns.rml"), Merged, Unmerged)));
		ASSERT_THAT(AreEqual(12, Unmerged.DrawCalls));
		ASSERT_THAT(AreEqual(1, Merged.DrawCalls));
		ASSERT_THAT(IsTrue(Merged.RhiCommands <= PassCommands + StateCommands + Merged.DrawCalls,
			FString::Printf(TEXT("%d RHI commands for %d draws"), Merged.RhiCommands, Merged.DrawCalls)));
		ASSERT_THAT(IsTrue(Merged.Pixels == Unmerged.Pixels, DescribeFirstPixelDifference(Merged, Unmerged, View.GetPixelSize().X)));
	}

	TEST_METHOD(TwoFontAtlasesBetweenColourRects_ReplayAsOneDraw)
	{
		ExpectMergedDrawsSaved(TEXT("Batch/TextureBreak.rml"), 4);
	}

	TEST_METHOD(SeventeenIconTextures_SplitAfterTheSixteenthTextureSlot)
	{
		using namespace ArcRmlBatchTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FReplayResult Merged;
		FReplayResult Unmerged;
		ASSERT_THAT(IsTrue(ReplayBoth(TEXT("Batch/TextureSlots.rml"), Merged, Unmerged)));
		ASSERT_THAT(AreEqual(16, Unmerged.DrawCalls - Merged.DrawCalls, FString::Printf(TEXT("unmerged %d draws, merged %d draws"), Unmerged.DrawCalls, Merged.DrawCalls)));
		ASSERT_THAT(AreEqual(1, GetRunBreaks(Merged, EArcRmlRunBreak::Texture)));
		ASSERT_THAT(AreEqual(1, CountRunBreaks(Merged)));
		ASSERT_THAT(IsTrue(Merged.Pixels == Unmerged.Pixels, DescribeFirstPixelDifference(Merged, Unmerged, View.GetPixelSize().X)));
	}

	TEST_METHOD(TwoClippedGroups_MergeAcrossScissors_WithClippedPixels)
	{
		ExpectMergedDrawsSaved(TEXT("Batch/ScissorBreak.rml"), 7);
	}

	TEST_METHOD(SlotGridWithIconsCountsAndClippedNames_ReplaysAsOneDraw_WithUnmergedPixels)
	{
		using namespace ArcRmlBatchTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FReplayResult Merged;
		FReplayResult Unmerged;
		ASSERT_THAT(IsTrue(ReplayBoth(TEXT("Batch/SlotGrid.rml"), Merged, Unmerged)));
		ASSERT_THAT(IsTrue(Unmerged.DrawCalls >= 12 * 4, FString::Printf(TEXT("unmerged %d draws"), Unmerged.DrawCalls)));
		ASSERT_THAT(AreEqual(1, Merged.DrawCalls));
		ASSERT_THAT(AreEqual(0, CountRunBreaks(Merged)));
		ASSERT_THAT(IsTrue(Merged.Pixels == Unmerged.Pixels, DescribeFirstPixelDifference(Merged, Unmerged, View.GetPixelSize().X)));
	}

	TEST_METHOD(AssetIconRowsWithSvgIconsBackgroundsAndText_ReplayAsOneDraw_InEveryDecodeMode)
	{
		using namespace ArcRmlBatchTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		UTexture* Srgb = LoadObject<UTexture>(nullptr, SrgbIcon);
		UTexture* Linear = LoadObject<UTexture>(nullptr, LinearIcon);
		ASSERT_THAT(IsTrue(Srgb != nullptr && Srgb->SRGB, TEXT("the sRGB icon is missing or not sRGB, so StraightLinear is not covered")));
		ASSERT_THAT(IsTrue(Linear != nullptr && !Linear->SRGB, TEXT("the linear icon is missing or sRGB, so Straight is not covered")));
		FReplayResult Merged;
		FReplayResult Unmerged;
		ASSERT_THAT(IsTrue(ReplayBoth(TEXT("Batch/AssetIcons.rml"), Merged, Unmerged)));
		FReplayResult UnmergedAgain;
		ASSERT_THAT(IsTrue(Replay(View, false, UnmergedAgain), TEXT("second unmerged replay")));
		ASSERT_THAT(IsTrue(UnmergedAgain.Pixels == Unmerged.Pixels, FString::Printf(TEXT("the frame changed between replays: %s"),
			*DescribeFirstPixelDifference(UnmergedAgain, Unmerged, View.GetPixelSize().X))));
		ASSERT_THAT(IsTrue(Unmerged.DrawCalls >= 6 * 5, FString::Printf(TEXT("unmerged %d draws"), Unmerged.DrawCalls)));
		ASSERT_THAT(AreEqual(1, Merged.DrawCalls));
		ASSERT_THAT(AreEqual(0, CountRunBreaks(Merged)));
		ASSERT_THAT(IsTrue(Merged.Pixels == Unmerged.Pixels, DescribeFirstPixelDifference(Merged, Unmerged, View.GetPixelSize().X)));

		for (UTexture* Icon : {Srgb, Linear})
		{
			FTextureReferenceRHIRef Saved;
			SwapTextureReference(*Icon, Saved);
			FReplayResult Without;
			const bool bReplayed = Replay(View, false, Without);
			SwapTextureReference(*Icon, Saved);
			ASSERT_THAT(IsTrue(bReplayed, TEXT("replay without the icon")));
			ASSERT_THAT(IsTrue(Without.Pixels != Unmerged.Pixels, FString::Printf(TEXT("'%s' draws no visible pixel, so its decode mode is not covered"), *Icon->GetPathName())));
		}
	}

	TEST_METHOD(UnreadyAssetTexture_DropsOnlyItsIcons_WithUnmergedPixels)
	{
		using namespace ArcRmlBatchTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		UTexture* Srgb = LoadObject<UTexture>(nullptr, SrgbIcon);
		ASSERT_THAT(IsNotNull(Srgb));
		FReplayResult Ready;
		FReplayResult ReadyUnmerged;
		ASSERT_THAT(IsTrue(ReplayBoth(TEXT("Batch/AssetIcons.rml"), Ready, ReadyUnmerged)));

		FTextureReferenceRHIRef Saved;
		SwapTextureReference(*Srgb, Saved);
		FReplayResult Merged;
		FReplayResult Unmerged;
		const bool bReplayed = Replay(View, false, Unmerged) && Replay(View, true, Merged);
		SwapTextureReference(*Srgb, Saved);

		ASSERT_THAT(IsTrue(bReplayed, TEXT("replay with the unready texture")));
		ASSERT_THAT(AreEqual(6, ReadyUnmerged.DrawCalls - Unmerged.DrawCalls, TEXT("the unmerged replay drew a different number of unready icons")));
		ASSERT_THAT(AreEqual(1, Merged.DrawCalls));
		ASSERT_THAT(IsTrue(Merged.Pixels != Ready.Pixels, TEXT("the unready icons were drawn")));
		ASSERT_THAT(IsTrue(Merged.Pixels == Unmerged.Pixels, DescribeFirstPixelDifference(Merged, Unmerged, View.GetPixelSize().X)));
	}

	TEST_METHOD(TranslatedGroupBetweenTwoGroups_SplitsAtEachTransformChange)
	{
		ExpectMergedDrawsSaved(TEXT("Batch/TransformBreak.rml"), 6);
	}

	TEST_METHOD(RoundedClipGroupBetweenTwoGroups_SplitsAtEachClipMaskChange)
	{
		ExpectMergedDrawsSaved(TEXT("Batch/ClipMaskBreak.rml"), 6);
	}

	TEST_METHOD(GradientBetweenColourRects_ShaderDrawNeverMerges)
	{
		ExpectMergedDrawsSaved(TEXT("Batch/ShaderBreak.rml"), 2);
	}

	TEST_METHOD(RotatedGroupBetweenTwoGroups_AntialiasedDrawsNeverMerge)
	{
		ExpectMergedDrawsSaved(TEXT("Batch/AntialiasBreak.rml"), 4);
	}
};
