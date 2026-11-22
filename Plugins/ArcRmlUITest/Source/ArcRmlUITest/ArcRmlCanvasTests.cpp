// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Algo/AnyOf.h"
#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace ArcRmlCanvasTestsPrivate
{
	const FIntPoint WideHost(3440, 1440);
	const FIntPoint ColumnHost(2560, 1440);
	constexpr float ColumnAspect = 16.0f / 9.0f;
	constexpr float UltraWideAspect = 21.0f / 9.0f;
	constexpr float Tolerance = 0.5f;

	const FColor CanvasColour(32, 48, 80, 255);
	const FColor ButtonColour(204, 51, 102, 255);
	const FColor VwColour(102, 204, 170, 255);
	const FColor OffBoxColour(192, 192, 193, 255);
	const FColor AnchorColour(255, 204, 0, 255);
	const FColor TransparentBoxColour(136, 68, 204, 255);
	const FColor NoRepeatCanvasColour(48, 32, 16, 255);
	const FColor HtmlCanvasColour(160, 48, 32, 255);
	const FColor HtmlBodyColour(32, 80, 160, 255);
	const FColor HalfBlack(0, 0, 0, 128);
	const FColor TabBodyColour(48, 160, 80, 255);

	const FBox2f HostBox(FVector2f(0.0f, 0.0f), FVector2f(3440.0f, 1440.0f));
	const FBox2f StripeBox(FVector2f(0.0f, 0.0f), FVector2f(3440.0f, 40.0f));
	const FBox2f OneRowBox(FVector2f(0.0f, 1439.0f), FVector2f(3440.0f, 1440.0f));
	const FBox2f TileBox(FVector2f(440.0f, 200.0f), FVector2f(540.0f, 300.0f));
	const FBox2f MarginTileBox(FVector2f(440.0f, 0.0f), FVector2f(540.0f, 100.0f));
	const FBox2f RightTileBox(FVector2f(1720.0f, 0.0f), FVector2f(3000.0f, 100.0f));

	bool IsFlat(const FArcRmlTestDraw& Draw)
	{
		return Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && !Draw.bTextured && !Draw.bShader;
	}

	bool SameBox(const FBox2f& A, const FBox2f& B)
	{
		return FMath::IsNearlyEqual(A.Min.X, B.Min.X, Tolerance) && FMath::IsNearlyEqual(A.Min.Y, B.Min.Y, Tolerance) && FMath::IsNearlyEqual(A.Max.X, B.Max.X, Tolerance)
			&& FMath::IsNearlyEqual(A.Max.Y, B.Max.Y, Tolerance);
	}

	TArray<const FArcRmlTestDraw*> FindFlat(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		TArray<const FArcRmlTestDraw*> Found;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (IsFlat(Draw) && Draw.FirstColourPremultiplied == Colour)
			{
				Found.Add(&Draw);
			}
		}
		return Found;
	}

	TArray<const FArcRmlTestDraw*> FindGradients(const TArray<FArcRmlTestDraw>& Draws, const FBox2f& Bounds)
	{
		TArray<const FArcRmlTestDraw*> Found;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bShader && Draw.ShaderName == TEXT("linear-gradient") && SameBox(Draw.Bounds, Bounds))
			{
				Found.Add(&Draw);
			}
		}
		return Found;
	}

	FString DescribeBox(const FBox2f& Box)
	{
		return FString::Printf(TEXT("(%.1f,%.1f)-(%.1f,%.1f)"), Box.Min.X, Box.Min.Y, Box.Max.X, Box.Max.Y);
	}

	FString DescribeDraws(const TArray<FArcRmlTestDraw>& Draws)
	{
		TArray<FString> Parts;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry)
			{
				const FString ScissorText = Draw.Scissor.IsSet() ? FString::Printf(TEXT("[%d,%d-%d,%d]"), Draw.Scissor->Min.X, Draw.Scissor->Min.Y, Draw.Scissor->Max.X, Draw.Scissor->Max.Y) : FString(TEXT("[]"));
				Parts.Add(FString::Printf(TEXT("%s%s%s(%d,%d,%d,%d)%s%s"), Draw.bShader ? TEXT("S") : TEXT(""), Draw.bTextured ? TEXT("T") : TEXT(""), Draw.bCallbackTexture ? TEXT("C") : TEXT(""),
					Draw.FirstColourPremultiplied.R, Draw.FirstColourPremultiplied.G, Draw.FirstColourPremultiplied.B, Draw.FirstColourPremultiplied.A, *DescribeBox(Draw.Bounds), *ScissorText));
			}
		}
		return FString::Join(Parts, TEXT(" "));
	}

	FBox2f ToBox(const FArcRmlInspectorRect& Rect, FVector2f Offset)
	{
		return FBox2f(FVector2f(Rect.Position.X, Rect.Position.Y) + Offset, FVector2f(Rect.Position.X + Rect.Size.X, Rect.Position.Y + Rect.Size.Y) + Offset);
	}

	FString DescribeRect(const FArcRmlInspectorRect& Rect)
	{
		return FString::Printf(TEXT("(%.1f,%.1f) size (%.1f,%.1f)"), Rect.Position.X, Rect.Position.Y, Rect.Size.X, Rect.Size.Y);
	}

	bool IsRect(const FArcRmlInspectorRect& Rect, float X, float Y, float Width, float Height)
	{
		return FMath::IsNearlyEqual(Rect.Position.X, X, Tolerance) && FMath::IsNearlyEqual(Rect.Position.Y, Y, Tolerance) && FMath::IsNearlyEqual(Rect.Size.X, Width, Tolerance)
			&& FMath::IsNearlyEqual(Rect.Size.Y, Height, Tolerance);
	}

	bool IsClippedTo(const FArcRmlTestDraw& Draw, const FIntRect& Column)
	{
		return Draw.Scissor.IsSet() && Draw.Scissor->Min.X >= Column.Min.X && Draw.Scissor->Max.X <= Column.Max.X && Draw.Scissor->Min.Y >= Column.Min.Y && Draw.Scissor->Max.Y <= Column.Max.Y;
	}

	bool ReachesSideBar(const FArcRmlTestDraw& Draw, const FIntRect& Column)
	{
		return Draw.Bounds.Min.X < static_cast<float>(Column.Min.X) - Tolerance || Draw.Bounds.Max.X > static_cast<float>(Column.Max.X) + Tolerance;
	}

	FArcRmlInputEvent MakePointer(EArcRmlInputType Type, FVector2f Point)
	{
		FArcRmlInputEvent Event;
		Event.Type = Type;
		Event.Position = ArcRmlPointer::ToRmlPoint(Point);
		Event.Button = 0;
		return Event;
	}

	FArcRmlViewCamera MakeCamera(FIntPoint Host, const FIntRect& Column)
	{
		FArcRmlViewCamera Camera;
		Camera.bValid = true;
		Camera.ViewOrigin = FVector::ZeroVector;
		Camera.ViewRect = FIntRect(0, 0, Host.X, Host.Y);
		Camera.ContentOrigin = FVector2D(static_cast<double>(Column.Min.X), static_cast<double>(Column.Min.Y));
		FMatrix Matrix = FMatrix::Identity;
		Matrix.M[0][0] = 2.0 / static_cast<double>(Host.X);
		Matrix.M[2][0] = -1.0;
		Matrix.M[1][1] = -2.0 / static_cast<double>(Host.Y);
		Matrix.M[2][1] = 1.0;
		Matrix.M[2][3] = 1.0;
		Matrix.M[3][3] = 0.0;
		Camera.TranslatedViewProjection = Matrix;
		return Camera;
	}

	const TSharedPtr<FJsonObject>* FindElementByTag(const TSharedPtr<FJsonObject>& Root, const FString& Tag)
	{
		const TArray<TSharedPtr<FJsonValue>>* Elements = nullptr;
		if (!Root->TryGetArrayField(TEXT("elements"), Elements))
		{
			return nullptr;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Elements)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FString ElementTag;
			if (Value->TryGetObject(Object) && (*Object)->TryGetStringField(TEXT("tag"), ElementTag) && ElementTag == Tag)
			{
				return Object;
			}
		}
		return nullptr;
	}

	bool ReadNumbers(const FJsonObject& Object, const TCHAR* Field, TArray<double>& OutNumbers)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object.TryGetArrayField(Field, Values))
		{
			return false;
		}
		OutNumbers.Reset();
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			OutNumbers.Add(Value->AsNumber());
		}
		return true;
	}
}

TEST_CLASS(ArcRmlCanvasTests, "ArcRmlUI.Canvas")
{
	FArcRmlTestView View;
	FArcRmlTestView Reference;
	TArray<FArcRmlTestLayoutElement> Elements;
	TArray<FArcRmlTestLayoutElement> ReferenceElements;

	AFTER_EACH()
	{
		View.Close();
		Reference.Close();
	}

	bool WaitForFrame(FArcRmlTestView& Target, uint64 Before)
	{
		for (int32 Attempt = 0; Attempt < 50 && Target.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!Target.FlushUntilSettled())
			{
				return false;
			}
		}
		return Target.GetProbe().FrameSerial > Before && Target.FlushUntilSettled();
	}

	bool ReadLayout(FArcRmlTestView& Target, TArray<FArcRmlTestLayoutElement>& OutElements)
	{
		return Target.Flush() && Target.Flush() && Target.ParseLayout(OutElements);
	}

	bool OpenFixture(FArcRmlTestView& Target, TArray<FArcRmlTestLayoutElement>& OutElements, const TCHAR* Relative, FIntPoint Host)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Relative);
		Target.SetMaxContentAspect(ColumnAspect);
		if (!FArcRmlTestView::DropCompiledDocument(Path) || !ArcRmlUITestHelpers::OpenView(Target, Path) || !Target.FlushUntilSettled() || !Target.GetProbe().bHasDocument)
		{
			return false;
		}
		const uint64 Before = Target.GetProbe().FrameSerial;
		Target.Resize(Host, 1.0f);
		return WaitForFrame(Target, Before) && Target.GetProbe().Size == Host && Target.GetProbe().ContentRect == Target.GetContentRect() && ReadLayout(Target, OutElements);
	}

	bool OpenWide(const TCHAR* Relative)
	{
		return OpenFixture(View, Elements, Relative, ArcRmlCanvasTestsPrivate::WideHost);
	}

	bool ChangeAspect(float Aspect)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetMaxContentAspect(Aspect);
		return WaitForFrame(View, Before) && View.GetProbe().ContentRect == View.GetContentRect() && ReadLayout(View, Elements);
	}

	bool SetRootClassAndWait(const FString& Class)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(Class, true);
		return WaitForFrame(View, Before);
	}

	FString Observe(FVector2f Point)
	{
		const bool bGate = View.IsInteractiveAt(Point);
		View.ClearCalls();
		View.RawInput(ArcRmlCanvasTestsPrivate::MakePointer(EArcRmlInputType::MouseMove, Point));
		View.RawInput(ArcRmlCanvasTestsPrivate::MakePointer(EArcRmlInputType::MouseDown, Point));
		View.RawInput(ArcRmlCanvasTestsPrivate::MakePointer(EArcRmlInputType::MouseUp, Point));
		if (!View.Flush())
		{
			return TEXT("flush failed");
		}
		TArray<FString> Ids;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			if (Call.FunctionName == TEXT("Clicked") && !Call.Arguments.IsEmpty())
			{
				Ids.Add(Call.Arguments.Last().String);
			}
		}
		return FString::Printf(TEXT("gate=%d click='%s'"), bGate ? 1 : 0, *FString::Join(Ids, TEXT(",")));
	}

	bool ObserveIs(const FString& Expected, FVector2f Point, const TCHAR* Reason)
	{
		const FString Actual = Observe(Point);
		return Assert.AreEqual(Expected, Actual, FString::Printf(TEXT("at (%g,%g): %s"), Point.X, Point.Y, Reason));
	}

	bool OneGradient(const FBox2f& Bounds, const TCHAR* Name)
	{
		const TArray<const FArcRmlTestDraw*> Found = ArcRmlCanvasTestsPrivate::FindGradients(View.GetDraws(), Bounds);
		return Assert.AreEqual(1, Found.Num(), FString::Printf(TEXT("%s: linear-gradient draws with bounds %s among %s"), Name, *ArcRmlCanvasTestsPrivate::DescribeBox(Bounds),
			*ArcRmlCanvasTestsPrivate::DescribeDraws(View.GetDraws())));
	}

	TEST_METHOD(BodyColour_CoversTheWholeHost)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetProbe().ContentRect == FIntRect(440, 0, 3000, 1440), TEXT("content rect is the 2560 column at x = 440")));
		const TArray<const FArcRmlTestDraw*> Fills = FindFlat(View.GetDraws(), CanvasColour);
		ASSERT_THAT(AreEqual(1, Fills.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Fills[0]->Bounds, HostBox), FString::Printf(TEXT("body colour fill %s, host %s"), *DescribeBox(Fills[0]->Bounds), *DescribeBox(HostBox))));
		ASSERT_THAT(IsFalse(Fills[0]->Scissor.IsSet(), TEXT("the canvas fill is not scissored to the column")));
	}

	TEST_METHOD(RepeatingLayer_CoversTheSideBarsInItsRow)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(OneGradient(StripeBox, TEXT("repeat-x 200x40 layer: Chrome ink of its first colour is x 40..3340, of its second x 0..3440, both y 0..40"))));
		const TArray<const FArcRmlTestDraw*> Stripe = FindGradients(View.GetDraws(), StripeBox);
		ASSERT_THAT(AreEqual(1, Stripe.Num()));
		ASSERT_THAT(IsTrue(Stripe[0]->TileOrigins.Num() > 1, TEXT("the layer is more than one tile")));
		bool bHasColumnOrigin = false;
		for (const FVector2f& Origin : Stripe[0]->TileOrigins)
		{
			bHasColumnOrigin = bHasColumnOrigin || FMath::IsNearlyEqual(Origin.X, 440.0f, Tolerance);
			const float Phase = FMath::Fmod(FMath::Fmod(Origin.X, 200.0f) + 200.0f, 200.0f);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Phase, 40.0f, Tolerance), FString::Printf(TEXT("tile origin x %.1f is not 40 mod 200: Chrome ink of the first colour starts at x 40"), Origin.X)));
		}
		ASSERT_THAT(IsTrue(bHasColumnOrigin, TEXT("a tile starts at the column origin x = 440")));
		TArray<float> Xs;
		for (const FVector2f& Origin : Stripe[0]->TileOrigins)
		{
			Xs.Add(Origin.X);
		}
		Xs.Sort();
		ASSERT_THAT(AreEqual(18, Xs.Num(), TEXT("tiles of width 200 starting at -160 and ending at 3240 cover 0..3440")));
		ASSERT_THAT(IsNear(-160.0f, Xs[0], Tolerance));
		ASSERT_THAT(IsNear(3240.0f, Xs.Last(), Tolerance));
		for (int32 Index = 1; Index < Xs.Num(); ++Index)
		{
			ASSERT_THAT(IsNear(200.0f, Xs[Index] - Xs[Index - 1], Tolerance, FString::Printf(TEXT("stride between tile %d and %d"), Index - 1, Index)));
		}
	}

	TEST_METHOD(NoRepeatLayer_StaysOneTileAtTheColumnOrigin)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(OneGradient(TileBox, TEXT("no-repeat 100x100 at 0 200px: Chrome ink x 440, y 200, 100x100"))));
	}

	TEST_METHOD(ExplicitSizeLayer_IsHonoured)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(OneGradient(OneRowBox, TEXT("repeat-x 100% 1px at 0 100%: Chrome ink x 0, y 1439, 3440x1"))));
	}

	TEST_METHOD(NoRepeatLayersWithBodyMargin_PositionInTheColumnNotTheBodyBox)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundNoRepeatTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<const FArcRmlTestDraw*> Fills = FindFlat(View.GetDraws(), NoRepeatCanvasColour);
		ASSERT_THAT(AreEqual(1, Fills.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Fills[0]->Bounds, HostBox), DescribeBox(Fills[0]->Bounds)));
		ASSERT_THAT(IsTrue(OneGradient(MarginTileBox, TEXT("100x100 tile at 0 0, body margin 50: Chrome ink x 440, y 0 (the body box would give 490, 50)"))));
		ASSERT_THAT(IsTrue(OneGradient(RightTileBox, TEXT("50% 100px at 100% 0: Chrome ink x 1720, y 0, 1280x100, ending at the column's right edge 3000"))));
		const FArcRmlTestLayoutElement* Box = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("nr-box"));
		ASSERT_THAT(IsNotNull(Box, TEXT("nr-box")));
		ASSERT_THAT(IsTrue(IsRect(Box->Rect, 10.0f, 300.0f, 40.0f, 40.0f), FString::Printf(TEXT("left 10, top 300 against the initial containing block, the body margin 50 does not move it (Chrome 10, 300): %s"), *DescribeRect(Box->Rect))));
	}

	TEST_METHOD(ButtonBox_EqualsItsBoxInA2560HostPlusTheColumnOrigin)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(OpenFixture(Reference, ReferenceElements, TEXT("Canvas/BodyBackgroundTest.rml"), ColumnHost), ArcRmlUITestHelpers::DescribeDiagnostics(Reference)));
		ASSERT_THAT(IsTrue(Reference.GetProbe().ContentRect == FIntRect(0, 0, 2560, 1440), TEXT("2560 host has no side bars")));

		const FArcRmlTestLayoutElement* Button = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("bb-button"));
		const FArcRmlTestLayoutElement* ReferenceButton = ArcRmlUITestHelpers::FindByDomId(ReferenceElements, TEXT("bb-button"));
		ASSERT_THAT(IsNotNull(Button, TEXT("bb-button")));
		ASSERT_THAT(IsNotNull(ReferenceButton, TEXT("bb-button in the 2560 host")));
		ASSERT_THAT(IsTrue(IsRect(ReferenceButton->Rect, 100.0f, 80.0f, 200.0f, 60.0f), DescribeRect(ReferenceButton->Rect)));
		ASSERT_THAT(IsTrue(IsRect(Button->Rect, ReferenceButton->Rect.Position.X, ReferenceButton->Rect.Position.Y, ReferenceButton->Rect.Size.X, ReferenceButton->Rect.Size.Y),
			FString::Printf(TEXT("dump is column-relative: %s, the 2560 host has %s"), *DescribeRect(Button->Rect), *DescribeRect(ReferenceButton->Rect))));

		const TArray<const FArcRmlTestDraw*> Drawn = FindFlat(View.GetDraws(), ButtonColour);
		const TArray<const FArcRmlTestDraw*> ReferenceDrawn = FindFlat(Reference.GetDraws(), ButtonColour);
		ASSERT_THAT(AreEqual(1, Drawn.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, ReferenceDrawn.Num(), DescribeDraws(Reference.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(ReferenceDrawn[0]->Bounds, FBox2f(FVector2f(100.0f, 80.0f), FVector2f(300.0f, 140.0f))), DescribeBox(ReferenceDrawn[0]->Bounds)));
		ASSERT_THAT(IsTrue(SameBox(Drawn[0]->Bounds, FBox2f(FVector2f(540.0f, 80.0f), FVector2f(740.0f, 140.0f))),
			FString::Printf(TEXT("drawn %s, the 2560 host draws (100,80)-(300,140), plus (440,0)"), *DescribeBox(Drawn[0]->Bounds))));
	}

	TEST_METHOD(ViewportUnitsAndMedia_ResolveAgainstTheColumn)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(OpenFixture(Reference, ReferenceElements, TEXT("Canvas/BodyBackgroundTest.rml"), ColumnHost), ArcRmlUITestHelpers::DescribeDiagnostics(Reference)));

		const FArcRmlTestLayoutElement* Vw = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("bb-vw"));
		const FArcRmlTestLayoutElement* ReferenceVw = ArcRmlUITestHelpers::FindByDomId(ReferenceElements, TEXT("bb-vw"));
		ASSERT_THAT(IsNotNull(Vw, TEXT("bb-vw")));
		ASSERT_THAT(IsNotNull(ReferenceVw, TEXT("bb-vw in the 2560 host")));
		ASSERT_THAT(IsTrue(IsRect(ReferenceVw->Rect, 0.0f, 400.0f, 2560.0f, 10.0f), DescribeRect(ReferenceVw->Rect)));
		ASSERT_THAT(IsTrue(IsRect(Vw->Rect, 0.0f, 400.0f, 2560.0f, 10.0f), FString::Printf(TEXT("100vw is the column 2560, not the host 3440: %s"), *DescribeRect(Vw->Rect))));

		const FArcRmlTestLayoutElement* Media = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("bb-media"));
		const FArcRmlTestLayoutElement* ReferenceMedia = ArcRmlUITestHelpers::FindByDomId(ReferenceElements, TEXT("bb-media"));
		ASSERT_THAT(IsNotNull(Media, TEXT("bb-media")));
		ASSERT_THAT(IsNotNull(ReferenceMedia, TEXT("bb-media in the 2560 host")));
		ASSERT_THAT(AreEqual(FString(TEXT("#00aa00ff")), ReferenceMedia->BackgroundColor, TEXT("2560x1440 is 16/9, max-aspect-ratio: 16/9 matches")));
		ASSERT_THAT(AreEqual(ReferenceMedia->BackgroundColor, Media->BackgroundColor, TEXT("the 3440x1440 host is 43/18 but the viewport is the 2560x1440 column")));
	}

	TEST_METHOD(ChangingMaxAspect_RelaysOutAtAFixedHostSize)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(ChangeAspect(UltraWideAspect)));
		ASSERT_THAT(IsTrue(View.GetProbe().ContentRect == FIntRect(40, 0, 3400, 1440), TEXT("21/9 of 1440 is a 3360 column at x = 40")));
		const FArcRmlTestLayoutElement* Vw = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("bb-vw"));
		const FArcRmlTestLayoutElement* Media = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("bb-media"));
		ASSERT_THAT(IsNotNull(Vw, TEXT("bb-vw")));
		ASSERT_THAT(IsNotNull(Media, TEXT("bb-media")));
		ASSERT_THAT(IsTrue(IsRect(Vw->Rect, 0.0f, 400.0f, 3360.0f, 10.0f), DescribeRect(Vw->Rect)));
		ASSERT_THAT(AreEqual(FString(TEXT("#aa0000ff")), Media->BackgroundColor, TEXT("3360x1440 is wider than 16/9")));
		const TArray<const FArcRmlTestDraw*> Drawn = FindFlat(View.GetDraws(), ButtonColour);
		ASSERT_THAT(AreEqual(1, Drawn.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Drawn[0]->Bounds, FBox2f(FVector2f(140.0f, 80.0f), FVector2f(340.0f, 140.0f))), DescribeBox(Drawn[0]->Bounds)));

		ASSERT_THAT(IsTrue(ChangeAspect(0.0f)));
		ASSERT_THAT(IsTrue(View.GetProbe().ContentRect == FIntRect(0, 0, 3440, 1440), TEXT("no limit: the column is the host")));
		Vw = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("bb-vw"));
		ASSERT_THAT(IsNotNull(Vw, TEXT("bb-vw")));
		ASSERT_THAT(IsTrue(IsRect(Vw->Rect, 0.0f, 400.0f, 3440.0f, 10.0f), DescribeRect(Vw->Rect)));

		ASSERT_THAT(IsTrue(ChangeAspect(ColumnAspect)));
		Vw = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("bb-vw"));
		ASSERT_THAT(IsNotNull(Vw, TEXT("bb-vw")));
		ASSERT_THAT(IsTrue(IsRect(Vw->Rect, 0.0f, 400.0f, 2560.0f, 10.0f), DescribeRect(Vw->Rect)));
	}

	TEST_METHOD(ElementLeftOfTheColumn_DrawsNothingInTheSideBar)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FIntRect Column = View.GetProbe().ContentRect;
		const TArray<const FArcRmlTestDraw*> Own = FindFlat(View.GetDraws(), OffBoxColour);
		ASSERT_THAT(AreEqual(1, Own.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Own[0]->Bounds, FBox2f(FVector2f(240.0f, 600.0f), FVector2f(540.0f, 700.0f))), FString::Printf(TEXT("column x -200..100 is host 240..540: %s"), *DescribeBox(Own[0]->Bounds))));
		ASSERT_THAT(IsTrue(IsClippedTo(*Own[0], Column), TEXT("the box crossing the column edge is scissored to the column")));

		int32 Shadows = 0;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op != EArcRmlTestDrawOp::Geometry || (!Draw.bTextured && !Draw.bCallbackTexture) || Draw.Bounds.Max.Y < 560.0f || Draw.Bounds.Min.Y > 740.0f)
			{
				continue;
			}
			++Shadows;
			ASSERT_THAT(IsTrue(!ReachesSideBar(Draw, Column) || IsClippedTo(Draw, Column), FString::Printf(TEXT("shadow draw %s is not scissored to the column"), *DescribeBox(Draw.Bounds))));
		}
		ASSERT_THAT(IsTrue(Shadows > 0, TEXT("the box-shadow produced a textured draw")));
		ASSERT_THAT(IsTrue(Shadows > 0 && Algo::AnyOf(View.GetDraws(), [&Column](const FArcRmlTestDraw& Draw) -> bool
		{
			return Draw.TextureSource == EArcRmlTestTextureSource::SavedLayer && ReachesSideBar(Draw, Column);
		}), TEXT("the shadow composite extends past the column edge, so the scissor is what keeps the side bar clean")));
	}

	TEST_METHOD(InteractiveElementLeftOfTheColumn_IsNotHitInTheSideBar)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=0 click=''"), FVector2f(340.0f, 850.0f), TEXT("host 240..540 x 800..900 is the element; x 340 is column -100, in the side bar"))));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=1 click='bb-off-button'"), FVector2f(490.0f, 850.0f), TEXT("x 490 is column 50, the part of the element inside the column"))));
	}

	TEST_METHOD(SideBarPoint_DoesNotHitAndTheButtonDoes)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=0 click=''"), FVector2f(200.0f, 110.0f), TEXT("left side bar at the button's row"))));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=0 click=''"), FVector2f(3200.0f, 110.0f), TEXT("right side bar at the button's row"))));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=0 click=''"), FVector2f(640.0f, 600.0f), TEXT("inside the column, on the body only"))));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=1 click='bb-button'"), FVector2f(640.0f, 110.0f), TEXT("button at host x 540..740, y 80..140"))));
	}

	TEST_METHOD(BodyOpacity_DoesNotFadeTheSideBars)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetRootClassAndWait(TEXT("fade"))));
		const TArray<const FArcRmlTestDraw*> Fills = FindFlat(View.GetDraws(), CanvasColour);
		ASSERT_THAT(AreEqual(1, Fills.Num(), FString::Printf(TEXT("a full-alpha canvas fill: %s"), *DescribeDraws(View.GetDraws()))));
		ASSERT_THAT(IsTrue(SameBox(Fills[0]->Bounds, HostBox), DescribeBox(Fills[0]->Bounds)));
		ASSERT_THAT(IsTrue(OneGradient(OneRowBox, TEXT("the 1px layer is drawn"))));
		ASSERT_THAT(IsTrue(OneGradient(StripeBox, TEXT("the repeating layer is drawn"))));
		for (const FBox2f& Layer : {OneRowBox, StripeBox})
		{
			const TArray<const FArcRmlTestDraw*> Layers = FindGradients(View.GetDraws(), Layer);
			ASSERT_THAT(AreEqual(1, Layers.Num()));
			ASSERT_THAT(IsTrue(Layers[0]->FirstColourPremultiplied == FColor(255, 255, 255, 255), FString::Printf(TEXT("layer %s keeps full alpha under body opacity .5, got (%d,%d,%d,%d)"),
				*DescribeBox(Layer), Layers[0]->FirstColourPremultiplied.R, Layers[0]->FirstColourPremultiplied.G, Layers[0]->FirstColourPremultiplied.B, Layers[0]->FirstColourPremultiplied.A)));
		}
		const TArray<const FArcRmlTestDraw*> Faded = FindFlat(View.GetDraws(), ButtonColour);
		ASSERT_THAT(AreEqual(0, Faded.Num(), TEXT("the body's children are faded: the button is no longer drawn at its full colour")));
	}

	TEST_METHOD(BodyTransform_DoesNotMoveTheSideBars)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetRootClassAndWait(TEXT("slide"))));
		const TArray<const FArcRmlTestDraw*> Fills = FindFlat(View.GetDraws(), CanvasColour);
		ASSERT_THAT(AreEqual(1, Fills.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Fills[0]->Bounds, HostBox), FString::Printf(TEXT("fill %s stays on the host, not shifted by 100"), *DescribeBox(Fills[0]->Bounds))));
		ASSERT_THAT(IsTrue(OneGradient(StripeBox, TEXT("the repeating layer is not shifted either"))));
		const TArray<const FArcRmlTestDraw*> Moved = FindFlat(View.GetDraws(), ButtonColour);
		ASSERT_THAT(AreEqual(1, Moved.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Moved[0]->Bounds, FBox2f(FVector2f(640.0f, 80.0f), FVector2f(840.0f, 140.0f))), FString::Printf(TEXT("the button follows the body: %s"), *DescribeBox(Moved[0]->Bounds))));
	}

	TEST_METHOD(TransparentBody_DrawsNothingInTheSideBars)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyTransparentTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FIntRect Column = View.GetProbe().ContentRect;
		const TArray<const FArcRmlTestDraw*> Box = FindFlat(View.GetDraws(), TransparentBoxColour);
		ASSERT_THAT(AreEqual(1, Box.Num(), DescribeDraws(View.GetDraws())));
		int32 Geometry = 0;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry)
			{
				++Geometry;
				ASSERT_THAT(IsFalse(ReachesSideBar(Draw, Column), FString::Printf(TEXT("draw %s reaches a side bar"), *DescribeBox(Draw.Bounds))));
			}
		}
		ASSERT_THAT(AreEqual(1, Geometry, DescribeDraws(View.GetDraws())));
	}

	TEST_METHOD(DataWorldAnchor_LandsOnTheSameScreenPixelAtBothHostSizes)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(OpenFixture(Reference, ReferenceElements, TEXT("Canvas/BodyBackgroundTest.rml"), ColumnHost), ArcRmlUITestHelpers::DescribeDiagnostics(Reference)));
		ASSERT_THAT(AreEqual(0, FindFlat(View.GetDraws(), AnchorColour).Num(), TEXT("the anchor is hidden before a camera is published")));

		const uint64 Before = View.GetProbe().FrameSerial;
		const uint64 ReferenceBefore = Reference.GetProbe().FrameSerial;
		View.SetCamera(MakeCamera(WideHost, View.GetContentRect()));
		Reference.SetCamera(MakeCamera(ColumnHost, Reference.GetContentRect()));
		ASSERT_THAT(IsTrue(WaitForFrame(View, Before)));
		ASSERT_THAT(IsTrue(WaitForFrame(Reference, ReferenceBefore)));

		const TArray<const FArcRmlTestDraw*> Wide = FindFlat(View.GetDraws(), AnchorColour);
		const TArray<const FArcRmlTestDraw*> Column = FindFlat(Reference.GetDraws(), AnchorColour);
		ASSERT_THAT(AreEqual(1, Wide.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, Column.Num(), DescribeDraws(Reference.GetDraws())));
		const FBox2f Expected(FVector2f(1600.0f, 400.0f), FVector2f(1620.0f, 420.0f));
		ASSERT_THAT(IsTrue(SameBox(Column[0]->Bounds, Expected), FString::Printf(TEXT("2560 host: %s, world point projects to (1600,400)"), *DescribeBox(Column[0]->Bounds))));
		ASSERT_THAT(IsTrue(SameBox(Wide[0]->Bounds, Expected), FString::Printf(TEXT("3440 host: %s, the same pixel"), *DescribeBox(Wide[0]->Bounds))));
	}

	TEST_METHOD(DataWorldEmptyValue_HidesTheAnchorWithoutAnErrorAndABadValueStillErrors)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		TestRunner->AddExpectedErrorPlain(TEXT("data-world on element div#wa-bad"), EAutomationExpectedErrorFlags::Contains, 1);
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/WorldAnchorValueTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetCamera(MakeCamera(WideHost, View.GetContentRect()));
		ASSERT_THAT(IsTrue(WaitForFrame(View, Before)));

		ASSERT_THAT(AreEqual(1, FindFlat(View.GetDraws(), AnchorColour).Num(), FString::Printf(TEXT("the valid anchor is placed: %s"), *DescribeDraws(View.GetDraws()))));
		ASSERT_THAT(AreEqual(0, FindFlat(View.GetDraws(), FColor(0, 204, 255, 255)).Num(), TEXT("an empty value hides the anchor")));
		ASSERT_THAT(AreEqual(0, FindFlat(View.GetDraws(), FColor(51, 255, 102, 255)).Num(), TEXT("a whitespace-only value hides the anchor")));
		ASSERT_THAT(AreEqual(0, FindFlat(View.GetDraws(), FColor(255, 0, 204, 255)).Num(), TEXT("a value that is not a world point hides the anchor")));
	}

	TEST_METHOD(ElementWithClipNoneInClippingParent_LiftsTheParentClipButKeepsTheColumnClip)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FIntRect Column = View.GetProbe().ContentRect;
		const TArray<const FArcRmlTestDraw*> Own = FindFlat(View.GetDraws(), FColor(160, 192, 224, 255));
		ASSERT_THAT(AreEqual(1, Own.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Own[0]->Bounds, FBox2f(FVector2f(240.0f, 1000.0f), FVector2f(740.0f, 1100.0f))), DescribeBox(Own[0]->Bounds)));
		ASSERT_THAT(IsTrue(Own[0]->Scissor.IsSet() && Own[0]->Scissor.GetValue() == Column, TEXT("clip: none lifts the overflow: hidden parent's clip (host 460..660), leaving exactly the column")));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=0 click=''"), FVector2f(340.0f, 1050.0f), TEXT("column -100, in the side bar"))));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=1 click='bb-clip-none'"), FVector2f(445.0f, 1050.0f), TEXT("column 5, left of the parent box 20..220, inside the column"))));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=1 click='bb-clip-none'"), FVector2f(700.0f, 1050.0f), TEXT("column 260, right of the parent box, inside the column and the 500 wide element"))));
		ASSERT_THAT(IsTrue(ObserveIs(TEXT("gate=1 click='bb-clip-none'"), FVector2f(560.0f, 1050.0f), TEXT("column 120, inside the parent box"))));
	}

	TEST_METHOD(FocusedTextInput_ReportsImeBoundsInHostPixels)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/ImeFieldTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(IsTrue(View.GetSnapshot().TextField.bActive, TEXT("the autofocus field is the active text field")));
		const FIntRect Bounds = View.GetSnapshot().TextField.Bounds;
		ASSERT_THAT(IsTrue(Bounds.Min == FIntPoint(540, 200), FString::Printf(TEXT("left 100 + column 440, top 200: (%d,%d)-(%d,%d)"), Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y)));
		// The bounds are the field's border box: the 300x30 content plus the user-agent 1px 2px padding and 2px border (VENDORED 222).
		ASSERT_THAT(IsTrue(Bounds.Max == FIntPoint(848, 236), FString::Printf(TEXT("308x36 border box of the 300x30 field: (%d,%d)-(%d,%d)"), Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y)));
	}

	TEST_METHOD(DisplayNoneElement_HasAZeroRect)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Hidden = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("bb-none"));
		ASSERT_THAT(IsNotNull(Hidden, TEXT("bb-none")));
		ASSERT_THAT(IsTrue(IsRect(Hidden->Rect, 0.0f, 0.0f, 0.0f, 0.0f), FString::Printf(TEXT("left 70, top 90, width 30 in the style, but display: none: %s"), *DescribeRect(Hidden->Rect))));
		ASSERT_THAT(IsTrue(IsRect(Hidden->TransformedRect, 0.0f, 0.0f, 0.0f, 0.0f), DescribeRect(Hidden->TransformedRect)));
	}

	TEST_METHOD(Dump_IsColumnRelativeAndNamesCanvasAndContentRect)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyBackgroundTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TSharedPtr<FJsonObject> Root;
		ASSERT_THAT(IsTrue(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(View.GetProbe().LayoutJson), Root) && Root.IsValid(), TEXT("layout json")));
		double Format = 0.0;
		ASSERT_THAT(IsTrue(Root->TryGetNumberField(TEXT("format"), Format) && FMath::RoundToInt(Format) == 7, TEXT("dump format 7")));
		const TSharedPtr<FJsonObject>* Canvas = nullptr;
		ASSERT_THAT(IsTrue(Root->TryGetObjectField(TEXT("canvas"), Canvas), TEXT("canvas field")));
		double CanvasWidth = 0.0;
		double CanvasHeight = 0.0;
		ASSERT_THAT(IsTrue((*Canvas)->TryGetNumberField(TEXT("width"), CanvasWidth) && (*Canvas)->TryGetNumberField(TEXT("height"), CanvasHeight), TEXT("canvas size")));
		ASSERT_THAT(IsTrue(FMath::RoundToInt(CanvasWidth) == 3440 && FMath::RoundToInt(CanvasHeight) == 1440, FString::Printf(TEXT("canvas %g x %g"), CanvasWidth, CanvasHeight)));
		TArray<double> ContentRect;
		ASSERT_THAT(IsTrue(ReadNumbers(*Root, TEXT("content_rect"), ContentRect) && ContentRect.Num() == 4, TEXT("content_rect field")));
		ASSERT_THAT(IsTrue(FMath::RoundToInt(ContentRect[0]) == 440 && FMath::RoundToInt(ContentRect[1]) == 0 && FMath::RoundToInt(ContentRect[2]) == 2560 && FMath::RoundToInt(ContentRect[3]) == 1440,
			FString::Printf(TEXT("content_rect %g,%g %gx%g"), ContentRect[0], ContentRect[1], ContentRect[2], ContentRect[3])));

		const TSharedPtr<FJsonObject>* Body = FindElementByTag(Root, TEXT("body"));
		ASSERT_THAT(IsTrue(Body != nullptr, TEXT("body element in the dump")));
		TArray<double> RelativeOffset;
		ASSERT_THAT(IsTrue(ReadNumbers(**Body, TEXT("relative_offset"), RelativeOffset) && RelativeOffset.Num() == 2, TEXT("body relative_offset")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyZero(RelativeOffset[0], 0.01) && FMath::IsNearlyZero(RelativeOffset[1], 0.01), FString::Printf(TEXT("body relative_offset (%g,%g) at 3440x1440"), RelativeOffset[0], RelativeOffset[1])));
	}

	TEST_METHOD(WindowDocumentWithAutoLeft_DragMovesExactlyByThePointerDelta)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/WindowDragTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Handle = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("wd-handle"));
		ASSERT_THAT(IsNotNull(Handle, TEXT("wd-handle")));
		ASSERT_THAT(IsTrue(IsRect(Handle->Rect, 100.0f, 50.0f, 300.0f, 40.0f), FString::Printf(TEXT("left auto, margin-left 100, margin-top 50: %s"), *DescribeRect(Handle->Rect))));

		const FVector2f Grab(440.0f + 120.0f, 70.0f);
		View.MouseMove(Grab);
		View.MouseDown(Grab);
		View.MouseMove(Grab + FVector2f(30.0f, 0.0f));
		View.MouseUp(Grab + FVector2f(30.0f, 0.0f));
		ASSERT_THAT(IsTrue(ReadLayout(View, Elements)));
		Handle = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("wd-handle"));
		ASSERT_THAT(IsNotNull(Handle, TEXT("wd-handle")));
		ASSERT_THAT(IsTrue(IsRect(Handle->Rect, 130.0f, 50.0f, 300.0f, 40.0f), FString::Printf(TEXT("a +30 drag moves the window to 130, not by the column origin 440: %s"), *DescribeRect(Handle->Rect))));
	}

	TEST_METHOD(HtmlBackground_PaintsTheCanvasAndTheBodyKeepsItsOwn)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/HtmlCanvasTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<const FArcRmlTestDraw*> Canvas = FindFlat(View.GetDraws(), HtmlCanvasColour);
		ASSERT_THAT(AreEqual(1, Canvas.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Canvas[0]->Bounds, HostBox), FString::Printf(TEXT("the html colour fills the host, Chrome #a03020 under and beside the body: %s"), *DescribeBox(Canvas[0]->Bounds))));
		const TArray<const FArcRmlTestDraw*> Body = FindFlat(View.GetDraws(), HtmlBodyColour);
		ASSERT_THAT(AreEqual(1, Body.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Body[0]->Bounds, FBox2f(FVector2f(440.0f, 0.0f), FVector2f(3000.0f, 100.0f))),
			FString::Printf(TEXT("the body paints its own colour in its box, Chrome #2050a0 in the 100 px body: %s"), *DescribeBox(Body[0]->Bounds))));
	}

	TEST_METHOD(TranslucentBody_PaintsTheCanvasOnceAtHalfAlpha)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(OpenWide(TEXT("Canvas/BodyTranslucentTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<const FArcRmlTestDraw*> Black;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (IsFlat(Draw) && Draw.FirstColourPremultiplied.R == 0 && Draw.FirstColourPremultiplied.G == 0 && Draw.FirstColourPremultiplied.B == 0)
			{
				Black.Add(&Draw);
			}
		}
		ASSERT_THAT(AreEqual(1, Black.Num(), FString::Printf(TEXT("one black draw, Chrome paints the body background once (127 over white everywhere): %s"), *DescribeDraws(View.GetDraws()))));
		ASSERT_THAT(IsTrue(FMath::Abs(static_cast<int32>(Black[0]->FirstColourPremultiplied.A) - static_cast<int32>(HalfBlack.A)) <= 1,
			FString::Printf(TEXT("alpha %d, rgba(0,0,0,.5) is 128"), Black[0]->FirstColourPremultiplied.A)));
		ASSERT_THAT(IsTrue(SameBox(Black[0]->Bounds, HostBox), FString::Printf(TEXT("the body background fills the host: %s"), *DescribeBox(Black[0]->Bounds))));
	}

	TEST_METHOD(TabBodyBackground_FillsTheWholeEmbedBox)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("Canvas/TabCanvasFrame.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(TEXT("Canvas/TabCanvasTab.rml"))), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && ReadLayout(View, Elements), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<const FArcRmlTestDraw*> Fills = FindFlat(View.GetDraws(), TabBodyColour);
		ASSERT_THAT(AreEqual(1, Fills.Num(), DescribeDraws(View.GetDraws())));
		ASSERT_THAT(IsTrue(SameBox(Fills[0]->Bounds, FBox2f(FVector2f(30.0f, 40.0f), FVector2f(430.0f, 280.0f))),
			FString::Printf(TEXT("the 50 px tab body's colour fills the 400x240 arc-embed box at 30,40, as an iframe canvas: %s"), *DescribeBox(Fills[0]->Bounds))));
	}

	TEST_METHOD(TabCanvasUnderAFadedMovedClippedEmbed_FollowsTheEmbedOpacityTransformAndClip)
	{
		using namespace ArcRmlCanvasTestsPrivate;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("Canvas/TabCanvasFadedFrame.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(TEXT("Canvas/TabCanvasFadedTab.rml"))), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && ReadLayout(View, Elements), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<const FArcRmlTestDraw*> Fills;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			const FColor& Colour = Draw.FirstColourPremultiplied;
			if (IsFlat(Draw) && FMath::Abs(Colour.R - 24) <= 1 && FMath::Abs(Colour.G - 80) <= 1 && FMath::Abs(Colour.B - 40) <= 1)
			{
				Fills.Add(&Draw);
			}
		}
		ASSERT_THAT(AreEqual(1, Fills.Num(), FString::Printf(TEXT("one tab canvas fill at half the tab body colour: %s"), *DescribeDraws(View.GetDraws()))));
		ASSERT_THAT(IsTrue(FMath::Abs(static_cast<int32>(Fills[0]->FirstColourPremultiplied.A) - 128) <= 1,
			FString::Printf(TEXT("arc-embed opacity .5 gives alpha 128, got %d"), Fills[0]->FirstColourPremultiplied.A)));
		ASSERT_THAT(IsTrue(SameBox(Fills[0]->Bounds, FBox2f(FVector2f(40.0f, 40.0f), FVector2f(440.0f, 280.0f))),
			FString::Printf(TEXT("the 400x240 embed box at 30,40 moved by the embed's translateX(10px): %s"), *DescribeBox(Fills[0]->Bounds))));
		ASSERT_THAT(IsTrue(Fills[0]->Scissor.IsSet() && Fills[0]->Scissor->Max.X == 330 && Fills[0]->Scissor->Min.X == 40,
			FString::Printf(TEXT("clipped to the moved embed box (x 40..440) inside the 300 px overflow: hidden parent (x 30..330), so x 40..330: %s"), *DescribeDraws(View.GetDraws()))));
	}
};
