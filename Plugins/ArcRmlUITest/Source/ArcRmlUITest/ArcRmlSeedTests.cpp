// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlSeedTestsPrivate
{
	constexpr int32 SeedAndWriteBackPasses = 2;
	constexpr int32 FullscreenPassRhiCommands = 7;
}

TEST_CLASS(ArcRmlSeedTests, "ArcRmlUI.Pixels")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Seed/SeedClearTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		if (!Assert.IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)))
		{
			return false;
		}
		View.SetDestination(FIntRect(0, 0, 1280, 720));
		return true;
	}

	float ReadWidth(const TCHAR* DomId)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		if (!Assert.IsTrue(View.ParseLayout(Elements)))
		{
			return -1.0f;
		}
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? Element->Rect.Size.X : -1.0f;
	}

	TEST_METHOD(FrameThatSeedsTheBaseLayer_ClearsItWithoutADraw)
	{
		using namespace ArcRmlSeedTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture()));
		ASSERT_THAT(IsTrue(View.DrawFrame(), TEXT("the frame was not drawn")));
		ASSERT_THAT(IsTrue(View.GetReplayDrawCalls() > 0, TEXT("the frame drew no geometry")));
		const int32 ExtraDrawCalls = View.GetDrawGpuDrawCalls() - View.GetReplayDrawCalls() - SeedAndWriteBackPasses;
		const int32 ExtraRhiCommands = View.GetDrawGpuRhiCommands() - View.GetReplayRhiCommands() - SeedAndWriteBackPasses * FullscreenPassRhiCommands;
		ASSERT_THAT(IsTrue(ExtraDrawCalls == 0 && ExtraRhiCommands == 0,
			FString::Printf(TEXT("the frame did %+d draw calls and %+d RHI commands beyond its geometry (%d draws, %d RHI commands), the seed copy and the write-back: "
				"the base layer clear drew a quad"), ExtraDrawCalls, ExtraRhiCommands, View.GetReplayDrawCalls(), View.GetReplayRhiCommands())));
	}

	TEST_METHOD(TestDraw_PresentsNothingSoLaterChangesStillRender)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture()));
		ASSERT_THAT(IsTrue(View.DrawFrame(), TEXT("the frame was not drawn")));
		for (int32 Change = 0; Change < 3; ++Change)
		{
			View.SetRootClass(TEXT("wide"), Change % 2 == 0);
			ASSERT_THAT(IsTrue(View.Flush(), FString::Printf(TEXT("flush after change %d"), Change)));
		}
		ASSERT_THAT(IsTrue(View.FlushUntilSettled(), TEXT("the view did not settle after the test draw and three changes")));
		const float Width = ReadWidth(TEXT("a"));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Width, 200.0f, 0.5f),
			FString::Printf(TEXT("#a is %.2f px wide after the third change, 200 expected: the test draw presented a frame and the view is held"), Width)));
	}
};
