// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlCssParityProbeTestsPrivate
{
	constexpr float Tolerance = 0.5f;
	constexpr float TextTolerance = 3.0f;
	constexpr int32 BorderWidth = 4;

	const FColor SolidColour(200, 30, 40, 255);
	const FColor MovedColour(40, 200, 30, 255);
	const FColor ClipChildColour(30, 200, 60, 255);
	const FColor RoundChildColour(30, 60, 200, 255);
	const FColor TextColour(120, 20, 200, 255);
	const FColor ShadowBoxColour(220, 180, 20, 255);

	TArray<const FArcRmlTestDraw*> FindBoxDraws(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		TArray<const FArcRmlTestDraw*> Found;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && Draw.FirstColourPremultiplied == Colour)
			{
				Found.Add(&Draw);
			}
		}
		return Found;
	}

	const FColor ShadowColour(100, 100, 100, 255);
	const FColor RecolouredShadowColour(60, 140, 200, 255);

	TArray<const FArcRmlTestDraw*> FindCallbackDraws(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour, uint64 AfterSerial)
	{
		TArray<const FArcRmlTestDraw*> Found;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bCallbackTexture && Draw.FirstColourPremultiplied == Colour && Draw.FrameSerial > AfterSerial)
			{
				Found.Add(&Draw);
			}
		}
		return Found;
	}

	FString DescribeBounds(const FBox2f& Bounds)
	{
		return FString::Printf(TEXT("(%.2f, %.2f) to (%.2f, %.2f)"), Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y);
	}

	FString DescribeRect(const FArcRmlTestLayoutElement& Element)
	{
		return FString::Printf(TEXT("(%.2f, %.2f) size (%.2f, %.2f)"), Element.Rect.Position.X, Element.Rect.Position.Y, Element.Rect.Size.X, Element.Rect.Size.Y);
	}

	bool BoundsMatchRect(const FBox2f& Bounds, const FArcRmlTestLayoutElement& Element)
	{
		return FMath::IsNearlyEqual(Bounds.Min.X, Element.Rect.Position.X, Tolerance) && FMath::IsNearlyEqual(Bounds.Min.Y, Element.Rect.Position.Y, Tolerance)
			&& FMath::IsNearlyEqual(Bounds.Max.X, Element.Rect.Position.X + Element.Rect.Size.X, Tolerance)
			&& FMath::IsNearlyEqual(Bounds.Max.Y, Element.Rect.Position.Y + Element.Rect.Size.Y, Tolerance);
	}

	FIntRect PaddingBox(const FArcRmlTestLayoutElement& Element)
	{
		const int32 Left = FMath::RoundToInt(Element.Rect.Position.X) + BorderWidth;
		const int32 Top = FMath::RoundToInt(Element.Rect.Position.Y) + BorderWidth;
		const int32 Right = FMath::RoundToInt(Element.Rect.Position.X + Element.Rect.Size.X) - BorderWidth;
		const int32 Bottom = FMath::RoundToInt(Element.Rect.Position.Y + Element.Rect.Size.Y) - BorderWidth;
		return FIntRect(Left, Top, Right, Bottom);
	}
}

TEST_CLASS(ArcRmlCssParityProbeTests, "ArcRmlUI.CssParity.Probe")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenProbe()
	{
		return View.Open(ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Probe.rml")), ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled()
			&& View.GetProbe().FrameSerial > 0 && !View.GetProbe().bPendingAsyncWork && View.ParseLayout(Elements);
	}

	TEST_METHOD(SolidBoxBoundsEqualBorderBox)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		const FArcRmlTestLayoutElement* Solid = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("solid"));
		ASSERT_THAT(IsNotNull(Solid, TEXT("solid element")));

		const TArray<const FArcRmlTestDraw*> Draws = ArcRmlCssParityProbeTestsPrivate::FindBoxDraws(View.GetDraws(), ArcRmlCssParityProbeTestsPrivate::SolidColour);
		ASSERT_THAT(AreEqual(1, Draws.Num(), FString::Printf(TEXT("Geometry draws of the solid colour among %d draws"), View.GetDraws().Num())));
		ASSERT_THAT(IsTrue(ArcRmlCssParityProbeTestsPrivate::BoundsMatchRect(Draws[0]->Bounds, *Solid),
			FString::Printf(TEXT("draw %s, border box %s"), *ArcRmlCssParityProbeTestsPrivate::DescribeBounds(Draws[0]->Bounds),
				*ArcRmlCssParityProbeTestsPrivate::DescribeRect(*Solid))));
		ASSERT_THAT(IsTrue(Solid->Rect.Size.X > 99.0f && Solid->Rect.Size.Y > 49.0f, TEXT("the border box has the CSS size")));
	}

	TEST_METHOD(OverflowHiddenParentScissorsChild)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		const FArcRmlTestLayoutElement* Parent = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("clipbox"));
		ASSERT_THAT(IsNotNull(Parent, TEXT("clipbox element")));

		ASSERT_THAT(IsNear(100.0f + 2.0f * ArcRmlCssParityProbeTestsPrivate::BorderWidth, Parent->Rect.Size.X, ArcRmlCssParityProbeTestsPrivate::Tolerance,
			TEXT("the parent border box includes its border")));

		const TArray<const FArcRmlTestDraw*> Draws = ArcRmlCssParityProbeTestsPrivate::FindBoxDraws(View.GetDraws(), ArcRmlCssParityProbeTestsPrivate::ClipChildColour);
		ASSERT_THAT(AreEqual(1, Draws.Num(), TEXT("Geometry draws of the clipped child colour")));
		ASSERT_THAT(IsTrue(Draws[0]->Scissor.IsSet(), TEXT("the clipped child carries a scissor")));
		const FIntRect Expected = ArcRmlCssParityProbeTestsPrivate::PaddingBox(*Parent);
		const FIntRect Actual = Draws[0]->Scissor.GetValue();
		ASSERT_THAT(IsTrue(Actual == Expected,
			FString::Printf(TEXT("scissor (%d, %d)-(%d, %d), padding box (%d, %d)-(%d, %d)"), Actual.Min.X, Actual.Min.Y, Actual.Max.X, Actual.Max.Y,
				Expected.Min.X, Expected.Min.Y, Expected.Max.X, Expected.Max.Y)));
		ASSERT_THAT(IsFalse(Draws[0]->bClipMask, TEXT("a square parent needs no clip mask")));
	}

	TEST_METHOD(RoundedOverflowHiddenParentUsesClipMask)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		const TArray<const FArcRmlTestDraw*> Draws = ArcRmlCssParityProbeTestsPrivate::FindBoxDraws(View.GetDraws(), ArcRmlCssParityProbeTestsPrivate::RoundChildColour);
		ASSERT_THAT(AreEqual(1, Draws.Num(), TEXT("Geometry draws of the rounded-clipped child colour")));
		ASSERT_THAT(IsTrue(Draws[0]->bClipMask, TEXT("the child of a rounded overflow-hidden parent is drawn under the clip mask")));
	}

	TEST_METHOD(TextEmitsGlyphQuads)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("label"));
		ASSERT_THAT(IsTrue(Lines.Num() > 0, TEXT("the label has a text line")));

		const FArcRmlTestDraw* Text = nullptr;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bTextured && Draw.TextureSource == EArcRmlTestTextureSource::Generated
				&& Draw.FirstColourPremultiplied == ArcRmlCssParityProbeTestsPrivate::TextColour)
			{
				Text = &Draw;
				break;
			}
		}
		ASSERT_THAT(IsNotNull(Text, TEXT("a generated-texture draw in the text colour")));
		ASSERT_THAT(IsTrue(Text->Quads.Num() > 0, TEXT("the text draw has quads")));

		float LeftmostX = TNumericLimits<float>::Max();
		for (const FArcRmlTestDrawQuad& Quad : Text->Quads)
		{
			ASSERT_THAT(IsTrue(Quad.AtlasRect.bIsValid, TEXT("quad atlas rect is valid")));
			ASSERT_THAT(IsTrue(Quad.AtlasRect.GetSize().X > 0.0f && Quad.AtlasRect.GetSize().Y > 0.0f, TEXT("quad atlas rect has an area")));
			ASSERT_THAT(IsTrue(Quad.Width > 0.0f && Quad.Height > 0.0f, TEXT("quad has positive size")));
			LeftmostX = FMath::Min(LeftmostX, Quad.X);
		}
		ASSERT_THAT(IsNear(Lines[0].X, LeftmostX, ArcRmlCssParityProbeTestsPrivate::TextTolerance,
			FString::Printf(TEXT("leftmost quad X %.2f, line X %.2f"), LeftmostX, Lines[0].X)));
	}

	TEST_METHOD(LinearGradientHasOneShaderDraw)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		const FArcRmlTestLayoutElement* Gradient = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("gradient"));
		ASSERT_THAT(IsNotNull(Gradient, TEXT("gradient element")));

		TArray<const FArcRmlTestDraw*> Shaders;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bShader && Draw.ShaderName == TEXT("linear-gradient"))
			{
				Shaders.Add(&Draw);
			}
		}
		ASSERT_THAT(AreEqual(1, Shaders.Num(), TEXT("linear-gradient shader draws")));
		ASSERT_THAT(IsTrue(ArcRmlCssParityProbeTestsPrivate::BoundsMatchRect(Shaders[0]->Bounds, *Gradient),
			FString::Printf(TEXT("draw %s, border box %s"), *ArcRmlCssParityProbeTestsPrivate::DescribeBounds(Shaders[0]->Bounds),
				*ArcRmlCssParityProbeTestsPrivate::DescribeRect(*Gradient))));
	}

	TEST_METHOD(BoxShadowDrawsAreCallbackTextures)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		const FArcRmlTestLayoutElement* Shadow = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("shadow"));
		ASSERT_THAT(IsNotNull(Shadow, TEXT("shadow element")));

		const TArray<const FArcRmlTestDraw*> Callback = ArcRmlCssParityProbeTestsPrivate::FindCallbackDraws(View.GetCallbackDraws(), ArcRmlCssParityProbeTestsPrivate::ShadowColour, 0);
		ASSERT_THAT(IsTrue(Callback.Num() > 0, FString::Printf(TEXT("callback shadow-colour draws among %d callback draws"), View.GetCallbackDraws().Num())));
		ASSERT_THAT(AreEqual(0, ArcRmlCssParityProbeTestsPrivate::FindCallbackDraws(View.GetCallbackDraws(), ArcRmlCssParityProbeTestsPrivate::ShadowBoxColour, 0).Num(),
			TEXT("callback draws of the box's own colour")));

		const TArray<const FArcRmlTestDraw*> Own = ArcRmlCssParityProbeTestsPrivate::FindBoxDraws(View.GetDraws(), ArcRmlCssParityProbeTestsPrivate::ShadowBoxColour);
		ASSERT_THAT(AreEqual(1, Own.Num(), TEXT("non-callback Geometry draws of the shadow box colour")));
		ASSERT_THAT(IsTrue(ArcRmlCssParityProbeTestsPrivate::BoundsMatchRect(Own[0]->Bounds, *Shadow),
			FString::Printf(TEXT("draw %s, border box %s"), *ArcRmlCssParityProbeTestsPrivate::DescribeBounds(Own[0]->Bounds),
				*ArcRmlCssParityProbeTestsPrivate::DescribeRect(*Shadow))));

		const FArcRmlTestDraw* Composite = nullptr;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.TextureSource == EArcRmlTestTextureSource::SavedLayer)
			{
				Composite = &Draw;
			}
		}
		ASSERT_THAT(IsNotNull(Composite, TEXT("the shadow is drawn from a saved-layer texture")));
		ASSERT_THAT(IsFalse(Composite->bCallbackTexture, TEXT("the shadow composite is a main-frame draw")));
		ASSERT_THAT(IsTrue(Composite->Bounds.Min.X < Shadow->Rect.Position.X && Composite->Bounds.Max.X > Shadow->Rect.Position.X + Shadow->Rect.Size.X,
			TEXT("the shadow extends beyond the box")));
	}

	TEST_METHOD(ClearCallbackDraws_EmptiesTheRetainedDraws)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		ASSERT_THAT(IsTrue(View.GetCallbackDraws().Num() > 0, TEXT("callback draws are retained after the shadow was generated")));
		View.ClearCallbackDraws();
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(0, View.GetCallbackDraws().Num(), TEXT("callback draws after the clear and a frame with no regeneration")));
	}

	TEST_METHOD(ChangedBoxShadowColourReplacesTheCallbackDraws)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		View.ClearCallbackDraws();
		ASSERT_THAT(IsTrue(View.Flush()));
		const uint64 Before = View.GetProbe().FrameSerial;

		View.SetRootClass(TEXT("recolor"), true);
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		}
		ASSERT_THAT(IsTrue(View.GetProbe().FrameSerial > Before, TEXT("a frame after the change")));

		ASSERT_THAT(IsTrue(ArcRmlCssParityProbeTestsPrivate::FindCallbackDraws(View.GetCallbackDraws(), ArcRmlCssParityProbeTestsPrivate::RecolouredShadowColour, Before).Num() > 0,
			TEXT("the new shadow colour is among the callback draws of the frames after the change")));
		ASSERT_THAT(AreEqual(0, ArcRmlCssParityProbeTestsPrivate::FindCallbackDraws(View.GetCallbackDraws(), ArcRmlCssParityProbeTestsPrivate::ShadowColour, 0).Num(),
			TEXT("the old shadow colour among the callback draws kept since the clear")));
	}

	TEST_METHOD(FileImageReportsFileSource)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		const FArcRmlTestLayoutElement* Image = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("image"));
		ASSERT_THAT(IsNotNull(Image, TEXT("image element")));

		TArray<const FArcRmlTestDraw*> Files;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.TextureSource == EArcRmlTestTextureSource::File)
			{
				Files.Add(&Draw);
			}
		}
		ASSERT_THAT(AreEqual(1, Files.Num(), TEXT("File-texture Geometry draws")));
		ASSERT_THAT(IsTrue(Files[0]->bTextured, TEXT("the file image draw is textured")));
		ASSERT_THAT(IsTrue(Files[0]->TexturePath.EndsWith(TEXT("Probe.png")), FString::Printf(TEXT("texture path '%s'"), *Files[0]->TexturePath)));
		ASSERT_THAT(IsTrue(ArcRmlCssParityProbeTestsPrivate::BoundsMatchRect(Files[0]->Bounds, *Image),
			FString::Printf(TEXT("draw %s, border box %s"), *ArcRmlCssParityProbeTestsPrivate::DescribeBounds(Files[0]->Bounds),
				*ArcRmlCssParityProbeTestsPrivate::DescribeRect(*Image))));
	}

	TEST_METHOD(FrameSerialGrowsAfterStyleChange)
	{
		ASSERT_THAT(IsTrue(OpenProbe()));
		const uint64 Before = View.GetProbe().FrameSerial;
		ASSERT_THAT(AreEqual(1, ArcRmlCssParityProbeTestsPrivate::FindBoxDraws(View.GetDraws(), ArcRmlCssParityProbeTestsPrivate::SolidColour).Num(), TEXT("solid draw before the change")));
		ASSERT_THAT(AreEqual(0, ArcRmlCssParityProbeTestsPrivate::FindBoxDraws(View.GetDraws(), ArcRmlCssParityProbeTestsPrivate::MovedColour).Num(), TEXT("moved draw before the change")));

		View.SetRootClass(TEXT("moved"), true);
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		}
		ASSERT_THAT(IsTrue(View.GetProbe().FrameSerial > Before, FString::Printf(TEXT("frame serial %llu, before %llu"), View.GetProbe().FrameSerial, Before)));

		const TArray<const FArcRmlTestDraw*> Moved = ArcRmlCssParityProbeTestsPrivate::FindBoxDraws(View.GetDraws(), ArcRmlCssParityProbeTestsPrivate::MovedColour);
		ASSERT_THAT(AreEqual(1, Moved.Num(), TEXT("moved draws after the change")));
		ASSERT_THAT(IsNear(60.0f, Moved[0]->Bounds.Min.X, ArcRmlCssParityProbeTestsPrivate::Tolerance, TEXT("moved box left")));
		ASSERT_THAT(IsNear(90.0f, Moved[0]->Bounds.Min.Y, ArcRmlCssParityProbeTestsPrivate::Tolerance, TEXT("moved box top")));
		ASSERT_THAT(AreEqual(0, ArcRmlCssParityProbeTestsPrivate::FindBoxDraws(View.GetDraws(), ArcRmlCssParityProbeTestsPrivate::SolidColour).Num(), TEXT("solid draw after the change")));
	}
};
