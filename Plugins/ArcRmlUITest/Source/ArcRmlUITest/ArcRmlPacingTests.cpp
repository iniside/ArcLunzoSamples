// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

TEST_CLASS(ArcRmlPacingTests, "ArcRmlUI.Pixels")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(OffscreenReadback_AfterTwoFlushes_ShowsSecondFlushContent)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Render/PacingHoverTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		const FColor Idle = View.GetPixel(70, 50);
		ASSERT_THAT(IsTrue(Idle == FColor(51, 102, 204, 255), FString::Printf(TEXT("idle %d,%d,%d,%d"), Idle.R, Idle.G, Idle.B, Idle.A)));

		View.MouseMove(FVector2f(70.0f, 50.0f));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		const FColor Hovered = View.GetPixel(70, 50);
		ASSERT_THAT(IsTrue(Hovered == FColor(204, 51, 51, 255), FString::Printf(TEXT("hovered %d,%d,%d,%d"), Hovered.R, Hovered.G, Hovered.B, Hovered.A)));
	}
};
