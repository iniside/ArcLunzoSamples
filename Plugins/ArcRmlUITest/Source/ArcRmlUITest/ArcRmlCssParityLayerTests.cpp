// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Algo/Sort.h"
#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityLayerTestsPrivate
{
	constexpr float Tolerance = 0.5f;
	constexpr float OriginTolerance = 0.05f;
	constexpr float TileTolerance = 0.01f;

	const TCHAR* const GeometryPath = TEXT("CssParity/Layers/Geometry.rml");
	const TCHAR* const DensePath = TEXT("CssParity/Layers/Dense.rml");
	const TCHAR* const AttachPath = TEXT("CssParity/Layers/Attach.rml");
	const TCHAR* const AnimPath = TEXT("CssParity/Layers/Anim.rml");
	const TCHAR* const WarnSizePath = TEXT("CssParity/Layers/WarnSize.rml");
	const TCHAR* const WarnOriginPath = TEXT("CssParity/Layers/WarnOrigin.rml");
	const TCHAR* const NoWarnPath = TEXT("CssParity/Layers/NoWarn.rml");

	FBox2f Ink(float X, float Y, float Width, float Height)
	{
		return FBox2f(FVector2f(X, Y), FVector2f(X + Width, Y + Height));
	}

	FString DescribeBox(const FBox2f& Box)
	{
		return FString::Printf(TEXT("(%.2f,%.2f)-(%.2f,%.2f)"), Box.Min.X, Box.Min.Y, Box.Max.X, Box.Max.Y);
	}

	bool SameBox(const FBox2f& A, const FBox2f& B, float Slack)
	{
		return FMath::IsNearlyEqual(A.Min.X, B.Min.X, Slack) && FMath::IsNearlyEqual(A.Min.Y, B.Min.Y, Slack) && FMath::IsNearlyEqual(A.Max.X, B.Max.X, Slack)
			&& FMath::IsNearlyEqual(A.Max.Y, B.Max.Y, Slack);
	}

	bool IsInside(const FBox2f& Inner, const FBox2f& Outer)
	{
		return Inner.Min.X >= Outer.Min.X - Tolerance && Inner.Min.Y >= Outer.Min.Y - Tolerance && Inner.Max.X <= Outer.Max.X + Tolerance && Inner.Max.Y <= Outer.Max.Y + Tolerance;
	}

	bool SameVector(const FVector2f& A, const FVector2f& B, float Slack)
	{
		return FMath::IsNearlyEqual(A.X, B.X, Slack) && FMath::IsNearlyEqual(A.Y, B.Y, Slack);
	}

	FIntRect ToIntRect(const FBox2f& Box)
	{
		return FIntRect(FMath::RoundToInt(Box.Min.X), FMath::RoundToInt(Box.Min.Y), FMath::RoundToInt(Box.Max.X), FMath::RoundToInt(Box.Max.Y));
	}

	TArray<const FArcRmlTestDraw*> FindShaderDraws(const TArray<FArcRmlTestDraw>& Draws, const FBox2f& Region, const TCHAR* Texture)
	{
		TArray<const FArcRmlTestDraw*> Found;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op != EArcRmlTestDrawOp::Geometry || !Draw.bShader || !IsInside(Draw.Bounds, Region))
			{
				continue;
			}
			if (Texture && !Draw.TexturePath.EndsWith(Texture))
			{
				continue;
			}
			Found.Add(&Draw);
		}
		return Found;
	}

	TArray<const FArcRmlTestDraw*> FindTileDraws(const TArray<FArcRmlTestDraw>& Draws, const FBox2f& Region, FVector2f TileSize)
	{
		TArray<const FArcRmlTestDraw*> Found;
		for (const FArcRmlTestDraw* Draw : FindShaderDraws(Draws, Region, nullptr))
		{
			if (SameVector(Draw->TileSize, TileSize, TileTolerance))
			{
				Found.Add(Draw);
			}
		}
		return Found;
	}

	int32 CountGeometryDraws(const TArray<FArcRmlTestDraw>& Draws, const FBox2f& Region)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && IsInside(Draw.Bounds, Region))
			{
				++Count;
			}
		}
		return Count;
	}

	bool ParseChromeLength(const FString& Text, float Free, float& OutValue)
	{
		FString Body = Text.TrimStartAndEnd();
		if (Body.StartsWith(TEXT("calc(")) && Body.EndsWith(TEXT(")")))
		{
			Body = Body.Mid(5, Body.Len() - 6);
		}
		TArray<FString> Tokens;
		Body.ParseIntoArray(Tokens, TEXT(" "), true);
		float Sign = 1.0f;
		float Total = 0.0f;
		bool bHasTerm = false;
		for (const FString& Token : Tokens)
		{
			if (Token == TEXT("+"))
			{
				Sign = 1.0f;
			}
			else if (Token == TEXT("-"))
			{
				Sign = -1.0f;
			}
			else if (Token.EndsWith(TEXT("%")))
			{
				Total += Sign * FCString::Atof(*Token.LeftChop(1)) / 100.0f * Free;
				Sign = 1.0f;
				bHasTerm = true;
			}
			else if (Token.EndsWith(TEXT("px")))
			{
				Total += Sign * FCString::Atof(*Token.LeftChop(2));
				Sign = 1.0f;
				bHasTerm = true;
			}
			else
			{
				return false;
			}
		}
		OutValue = Total;
		return bHasTerm;
	}

	TArray<FVector2f> GridOrigins(float StartX, float StepX, int32 CountX, float StartY, float StepY, int32 CountY)
	{
		TArray<FVector2f> Origins;
		for (int32 Y = 0; Y < CountY; ++Y)
		{
			for (int32 X = 0; X < CountX; ++X)
			{
				Origins.Add(FVector2f(StartX + StepX * static_cast<float>(X), StartY + StepY * static_cast<float>(Y)));
			}
		}
		return Origins;
	}

	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
	}

	void DropCompiledFixture(const TCHAR* RelativeRml)
	{
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(RelativeRml));
		const FString Dir = ArcRmlUITestHelpers::GetTestContentDir();
		const FString Sheet = FPaths::Combine(Dir, FPaths::GetPath(RelativeRml), FPaths::GetBaseFilename(RelativeRml) + TEXT(".rcssc"));
		IFileManager::Get().Delete(*Sheet, false, true, true);
	}

	class FIgnoredLayerWarningCapture final : public FOutputDevice
	{
	public:
		FIgnoredLayerWarningCapture()
		{
			GLog->AddOutputDevice(this);
		}

		virtual ~FIgnoredLayerWarningCapture() override
		{
			GLog->RemoveOutputDevice(this);
		}

		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category != TEXT("LogArcRmlUI") || (Verbosity & ELogVerbosity::VerbosityMask) != ELogVerbosity::Warning)
			{
				return;
			}
			if (FCString::Strstr(Text, TEXT("Decorator ignores the layer's")) != nullptr)
			{
				FScopeLock Lock(&CountLock);
				++Count;
			}
		}

		virtual bool CanBeUsedOnAnyThread() const override
		{
			return true;
		}

		int32 GetCount()
		{
			GLog->Flush();
			FScopeLock Lock(&CountLock);
			return Count;
		}

	private:
		FCriticalSection CountLock;
		int32 Count = 0;
	};
}

TEST_CLASS(ArcRmlCssParityLayerTests, "ArcRmlUI.CssParity.Layers")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;
	double Elapsed = 0.0;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlCssParityLayerTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
	}

	bool SettleFrames()
	{
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			FPlatformProcess::Sleep(0.05f);
			if (!View.Flush())
			{
				return false;
			}
		}
		return true;
	}

	bool OpenFixture(const TCHAR* Relative)
	{
		ArcRmlCssParityLayerTestsPrivate::DropCompiledFixture(Relative);
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(Relative)) && View.FlushUntilSettled() && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ParseLayout(Elements);
	}

	bool OpenFrozen(const TCHAR* Relative)
	{
		ArcRmlCssParityLayerTestsPrivate::RunConsole(TEXT("ArcRml.Clock 1000"));
		Elapsed = 0.0;
		ArcRmlCssParityLayerTestsPrivate::DropCompiledFixture(Relative);
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(Relative)) && SettleFrames() && View.GetProbe().bHasDocument
			&& View.GetViewDiagnostics().Diagnostics.IsEmpty() && View.ParseLayout(Elements);
	}

	bool AdvanceTo(double Seconds)
	{
		ArcRmlCssParityLayerTestsPrivate::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.4f"), Seconds - Elapsed));
		Elapsed = Seconds;
		return SettleFrames() && View.ParseLayout(Elements);
	}

	bool GetRect(const TCHAR* Id, FBox2f& OutRegion)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Id);
		if (!Assert.IsNotNull(Element, FString::Printf(TEXT("element %s"), Id)))
		{
			return false;
		}
		OutRegion = FBox2f(FVector2f(Element->Rect.Position.X, Element->Rect.Position.Y),
			FVector2f(Element->Rect.Position.X + Element->Rect.Size.X, Element->Rect.Position.Y + Element->Rect.Size.Y));
		return true;
	}

	bool ExpectDraw(const FArcRmlTestDraw& Draw, const FString& Name, const FBox2f& ChromeInk, FVector2f Size, FVector2f Step, const TArray<FVector2f>& Origins, float InkSlack = 0.5f,
		float OriginSlack = 0.05f)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		if (!Assert.IsTrue(SameBox(Draw.Bounds, ChromeInk, InkSlack), FString::Printf(TEXT("%s: draw bounds %s, Chrome ink %s"), *Name, *DescribeBox(Draw.Bounds), *DescribeBox(ChromeInk))))
		{
			return false;
		}
		if (!Assert.IsTrue(!Draw.Scissor.IsSet() || (ChromeInk.Min.X >= Draw.Scissor->Min.X - Tolerance && ChromeInk.Min.Y >= Draw.Scissor->Min.Y - Tolerance
			&& ChromeInk.Max.X <= Draw.Scissor->Max.X + Tolerance && ChromeInk.Max.Y <= Draw.Scissor->Max.Y + Tolerance), FString::Printf(TEXT("%s: Chrome ink lies outside the scissor"), *Name)))
		{
			return false;
		}
		if (!Assert.IsTrue(SameVector(Draw.TileSize, Size, TileTolerance), FString::Printf(TEXT("%s: tile size (%.3f,%.3f), expected (%.3f,%.3f)"), *Name, Draw.TileSize.X, Draw.TileSize.Y, Size.X, Size.Y)))
		{
			return false;
		}
		if (!Assert.IsTrue(SameVector(Draw.TileStep, Step, TileTolerance), FString::Printf(TEXT("%s: tile step (%.3f,%.3f), expected (%.3f,%.3f)"), *Name, Draw.TileStep.X, Draw.TileStep.Y, Step.X, Step.Y)))
		{
			return false;
		}
		if (!Assert.AreEqual(Origins.Num(), Draw.TileOrigins.Num(), FString::Printf(TEXT("%s: tile origin count"), *Name)))
		{
			return false;
		}
		for (int32 Index = 0; Index < Origins.Num(); ++Index)
		{
			if (!Assert.IsTrue(SameVector(Draw.TileOrigins[Index], Origins[Index], OriginSlack),
				FString::Printf(TEXT("%s: tile origin %d is (%.3f,%.3f), expected (%.3f,%.3f)"), *Name, Index, Draw.TileOrigins[Index].X, Draw.TileOrigins[Index].Y, Origins[Index].X, Origins[Index].Y)))
			{
				return false;
			}
		}
		return true;
	}

	bool ExpectLayer(const TCHAR* Id, const FBox2f& ChromeInk, FVector2f Size, FVector2f Step, const TArray<FVector2f>& Origins, const TCHAR* Texture = nullptr, float InkSlack = 0.5f,
		float OriginSlack = 0.05f)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		FBox2f Region;
		if (!GetRect(Id, Region))
		{
			return false;
		}
		const TArray<const FArcRmlTestDraw*> Found = FindShaderDraws(View.GetDraws(), Region, Texture);
		if (!Assert.AreEqual(1, Found.Num(), FString::Printf(TEXT("%s: layer draws inside %s"), Id, *DescribeBox(Region))))
		{
			return false;
		}
		return ExpectDraw(*Found[0], Id, ChromeInk, Size, Step, Origins, InkSlack, OriginSlack);
	}

	bool ExpectCovers(const FArcRmlTestDraw& Draw, const FString& Name, int32 ExpectedColumns, int32 ExpectedRows)
	{
		TArray<float> Xs;
		TArray<float> Ys;
		for (const FVector2f& Origin : Draw.TileOrigins)
		{
			Xs.AddUnique(Origin.X);
			Ys.AddUnique(Origin.Y);
		}
		Algo::Sort(Xs);
		Algo::Sort(Ys);
		if (!Assert.AreEqual(ExpectedColumns, Xs.Num(), FString::Printf(TEXT("%s: distinct tile columns"), *Name)) || !Assert.AreEqual(ExpectedRows, Ys.Num(), FString::Printf(TEXT("%s: distinct tile rows"), *Name))
			|| !Assert.AreEqual(ExpectedColumns * ExpectedRows, Draw.TileOrigins.Num(), FString::Printf(TEXT("%s: tile origins"), *Name)))
		{
			return false;
		}
		const float Tolerance = ArcRmlCssParityLayerTestsPrivate::Tolerance;
		const float TileTolerance = ArcRmlCssParityLayerTestsPrivate::TileTolerance;
		if (!Assert.IsTrue(Xs[0] <= Draw.Bounds.Min.X + Tolerance && Xs.Last() + Draw.TileSize.X >= Draw.Bounds.Max.X - Tolerance, FString::Printf(TEXT("%s: columns do not cover the draw horizontally"), *Name))
			|| !Assert.IsTrue(Ys[0] <= Draw.Bounds.Min.Y + Tolerance && Ys.Last() + Draw.TileSize.Y >= Draw.Bounds.Max.Y - Tolerance, FString::Printf(TEXT("%s: rows do not cover the draw vertically"), *Name)))
		{
			return false;
		}
		for (int32 Index = 1; Index < Xs.Num(); ++Index)
		{
			if (!Assert.IsTrue(FMath::IsNearlyEqual(Xs[Index] - Xs[Index - 1], Draw.TileStep.X, TileTolerance), FString::Printf(TEXT("%s: column %d is not one step after the last"), *Name, Index)))
			{
				return false;
			}
		}
		for (int32 Index = 1; Index < Ys.Num(); ++Index)
		{
			if (!Assert.IsTrue(FMath::IsNearlyEqual(Ys[Index] - Ys[Index - 1], Draw.TileStep.Y, TileTolerance), FString::Printf(TEXT("%s: row %d is not one step after the last"), *Name, Index)))
			{
				return false;
			}
		}
		return true;
	}

	bool ChromeOrigin(const FBox2f& Region, const TCHAR* ChromeX, const TCHAR* ChromeY, FVector2f Tile, FVector2f& OutOrigin)
	{
		float OffsetX = 0.0f;
		float OffsetY = 0.0f;
		const FVector2f Free(Region.GetSize().X - Tile.X, Region.GetSize().Y - Tile.Y);
		if (!Assert.IsTrue(ArcRmlCssParityLayerTestsPrivate::ParseChromeLength(ChromeX, Free.X, OffsetX) && ArcRmlCssParityLayerTestsPrivate::ParseChromeLength(ChromeY, Free.Y, OffsetY),
			FString::Printf(TEXT("Chrome position '%s' '%s'"), ChromeX, ChromeY)))
		{
			return false;
		}
		OutOrigin = FVector2f(Region.Min.X + OffsetX, Region.Min.Y + OffsetY);
		return true;
	}

	TEST_METHOD(SizeAfterShorthand_AppliesTheLonghandSize)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("sizeafter"), Ink(10, 10, 50, 20), FVector2f(50, 20), FVector2f(0, 0), {FVector2f(10, 10)})));
	}

	TEST_METHOD(CoverOnPng_ScalesToTheLargerRatioAndCentres)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("cover"), Region)));
		const float Scale = FMath::Max(Region.GetSize().X / 40.0f, Region.GetSize().Y / 40.0f);
		const FVector2f Tile(40.0f * Scale, 40.0f * Scale);
		FVector2f Origin;
		ASSERT_THAT(IsTrue(ChromeOrigin(Region, TEXT("50%"), TEXT("50%"), Tile, Origin)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("cover"), Ink(150, 10, 120, 60), Tile, FVector2f(0, 0), {Origin}, TEXT("Cover.png"))));
	}

	TEST_METHOD(ContainOnPng_ScalesToTheSmallerRatioAndCentres)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("contain"), Region)));
		const float Scale = FMath::Min(Region.GetSize().X / 40.0f, Region.GetSize().Y / 40.0f);
		const FVector2f Tile(40.0f * Scale, 40.0f * Scale);
		FVector2f Origin;
		ASSERT_THAT(IsTrue(ChromeOrigin(Region, TEXT("50%"), TEXT("50%"), Tile, Origin)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("contain"), Ink(320, 10, 60, 60), Tile, FVector2f(0, 0), {Origin}, TEXT("Contain.png"))));
	}

	TEST_METHOD(RightBottomOffsets_PositionFromTheFarEdges)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("rightbottom"), Region)));
		FVector2f Origin;
		ASSERT_THAT(IsTrue(ChromeOrigin(Region, TEXT("calc(100% - 10px)"), TEXT("calc(100% - 5px)"), FVector2f(30, 20), Origin)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("rightbottom"), Ink(510, 45, 30, 20), FVector2f(30, 20), FVector2f(0, 0), {Origin})));
	}

	TEST_METHOD(ThreeValuePosition_BackgroundShorthand)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("pos3bg"), Region)));
		FVector2f Origin;
		ASSERT_THAT(IsTrue(ChromeOrigin(Region, TEXT("calc(100% - 10px)"), TEXT("100%"), FVector2f(30, 20), Origin)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("pos3bg"), Ink(650, 50, 30, 20), FVector2f(30, 20), FVector2f(0, 0), {Origin})));
	}

	TEST_METHOD(ThreeValuePosition_MaskShorthand)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("pos3mask"), Region)));
		FVector2f Origin;
		ASSERT_THAT(IsTrue(ChromeOrigin(Region, TEXT("calc(100% - 10px)"), TEXT("100%"), FVector2f(30, 20), Origin)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("pos3mask"), Ink(790, 50, 30, 20), FVector2f(30, 20), FVector2f(0, 0), {Origin})));
		const TArray<const FArcRmlTestDraw*> Found = FindShaderDraws(View.GetDraws(), Region, nullptr);
		ASSERT_THAT(AreEqual(1, Found.Num()));
		ASSERT_THAT(IsTrue(Found[0]->Scissor.IsSet() && Found[0]->Scissor.GetValue() == ToIntRect(Region), TEXT("the mask layer is scissored to the element box")));
	}

	TEST_METHOD(ThreeValuePosition_MaskPositionLonghandIsRejected)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("Declaration 'mask-position: right 10px bottom;' is dropped")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("pos3maskpos"), Ink(850, 10, 30, 20), FVector2f(30, 20), FVector2f(0, 0), {FVector2f(850, 10)})));
	}

	TEST_METHOD(Space_PlacesWholeTilesWithEqualGaps)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const float StepX = (1085.0f - 990.0f) / 3.0f;
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("space"), Ink(990, 10, 119, 60), FVector2f(25, 22), FVector2f(StepX, 38), {FVector2f(990, 10), FVector2f(1022, 10), FVector2f(1054, 10), FVector2f(1085, 10),
			FVector2f(990, 48), FVector2f(1022, 48), FVector2f(1054, 48), FVector2f(1085, 48)}, nullptr, 1.0f, 1.0f)));
	}

	TEST_METHOD(Space_OneTileFitsAndTheOtherAxisStacksWithoutGap)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("spaceone"), Ink(1140, 10, 70, 60), FVector2f(70, 30), FVector2f(0, 30), {FVector2f(1140, 10), FVector2f(1140, 40)})));
	}

	TEST_METHOD(Round_ScalesTilesToAWholeCount)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("round"), Ink(10, 90, 120, 60), FVector2f(40, 30), FVector2f(40, 30), GridOrigins(10, 40, 3, 90, 30, 2))));
	}

	TEST_METHOD(RepeatX_RepeatsOnlyHorizontally)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("repeatx"), Ink(150, 90, 120, 10), FVector2f(20, 10), FVector2f(20, 0), GridOrigins(150, 20, 6, 90, 0, 1))));
	}

	TEST_METHOD(TwoLayers_ListsCycleWhenLengthsDiffer)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("twolayers"), Region)));
		const TArray<const FArcRmlTestDraw*> Found = FindShaderDraws(View.GetDraws(), Region, nullptr);
		ASSERT_THAT(AreEqual(3, Found.Num(), TEXT("three layers, painted last to first")));
		ASSERT_THAT(IsTrue(ExpectDraw(*Found[0], TEXT("third layer: size 60 40, position cycled to 0 0"), Ink(290, 90, 60, 40), FVector2f(60, 40), FVector2f(0, 0), {FVector2f(290, 90)})));
		ASSERT_THAT(IsTrue(ExpectDraw(*Found[1], TEXT("second layer: size 40 20, position 50 30"), Ink(340, 120, 40, 20), FVector2f(40, 20), FVector2f(0, 0), {FVector2f(340, 120)})));
		ASSERT_THAT(IsTrue(ExpectDraw(*Found[2], TEXT("first layer: size 20 10, position 0 0"), Ink(290, 90, 20, 10), FVector2f(20, 10), FVector2f(0, 0), {FVector2f(290, 90)})));
	}

	TEST_METHOD(GlassPanelLayer_BottomHairline)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("glass"), Ink(430, 129, 120, 1), FVector2f(120, 1), FVector2f(0, 0), {FVector2f(430, 129)})));
	}

	TEST_METHOD(UrlMask_HalfSizeKeepsTheImageRatio)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("urlmask"), Ink(570, 90, 60, 30), FVector2f(60, 30), FVector2f(0, 0), {FVector2f(570, 90)}, TEXT("Wide.png"))));
	}

	TEST_METHOD(MaskShorthand_DrawsTheSameAsTheLonghands)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("maskshort"), Ink(720, 110, 30, 40), FVector2f(30, 40), FVector2f(0, 0), {FVector2f(720, 110)})));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("masklong"), Ink(860, 110, 30, 40), FVector2f(30, 40), FVector2f(0, 0), {FVector2f(860, 110)})));

		FBox2f Short;
		FBox2f Long;
		ASSERT_THAT(IsTrue(GetRect(TEXT("maskshort"), Short) && GetRect(TEXT("masklong"), Long)));
		const auto Collect = [this](const FBox2f& Region)
		{
			TArray<const FArcRmlTestDraw*> Found;
			for (const FArcRmlTestDraw& Draw : View.GetDraws())
			{
				const bool bInside = Draw.Op == EArcRmlTestDrawOp::Geometry && ArcRmlCssParityLayerTestsPrivate::IsInside(Draw.Bounds, Region);
				const bool bScissored = Draw.Op != EArcRmlTestDrawOp::Geometry && Draw.Scissor.IsSet() && Draw.Scissor.GetValue() == ArcRmlCssParityLayerTestsPrivate::ToIntRect(Region);
				if (bInside || bScissored)
				{
					Found.Add(&Draw);
				}
			}
			return Found;
		};
		const TArray<const FArcRmlTestDraw*> ShortDraws = Collect(Short);
		const TArray<const FArcRmlTestDraw*> LongDraws = Collect(Long);
		ASSERT_THAT(IsTrue(ShortDraws.Num() > 2, TEXT("the mask layer emits its draws")));
		ASSERT_THAT(AreEqual(ShortDraws.Num(), LongDraws.Num(), TEXT("draw count of the shorthand and the longhands")));
		for (int32 Index = 0; Index < ShortDraws.Num(); ++Index)
		{
			const FArcRmlTestDraw& A = *ShortDraws[Index];
			const FArcRmlTestDraw& B = *LongDraws[Index];
			ASSERT_THAT(IsTrue(A.Op == B.Op && A.Layer == B.Layer && A.BlendMode == B.BlendMode && A.SourceLayer == B.SourceLayer && A.DestinationLayer == B.DestinationLayer
				&& A.bShader == B.bShader && A.ShaderName == B.ShaderName && A.bClipMask == B.bClipMask, FString::Printf(TEXT("draw %d differs in kind"), Index)));
			ASSERT_THAT(IsTrue(A.Op != EArcRmlTestDrawOp::Geometry || (SameVector(A.Bounds.Min - Short.Min, B.Bounds.Min - Long.Min, TileTolerance) && SameVector(A.Bounds.Max - Short.Min, B.Bounds.Max - Long.Min, TileTolerance)),
				FString::Printf(TEXT("draw %d differs in bounds"), Index)));
			ASSERT_THAT(IsTrue(SameVector(A.TileSize, B.TileSize, TileTolerance) && SameVector(A.TileStep, B.TileStep, TileTolerance) && A.TileOrigins.Num() == B.TileOrigins.Num(),
				FString::Printf(TEXT("draw %d differs in tiles"), Index)));
			for (int32 Origin = 0; Origin < A.TileOrigins.Num(); ++Origin)
			{
				ASSERT_THAT(IsTrue(SameVector(A.TileOrigins[Origin] - Short.Min, B.TileOrigins[Origin] - Long.Min, TileTolerance), FString::Printf(TEXT("draw %d origin %d differs"), Index, Origin)));
			}
		}
	}

	bool ExpectComposite(const TCHAR* Id, const FBox2f& ChromeInk, bool bExpectsComposite, Rml::BlendMode Blend)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		FBox2f Region;
		if (!GetRect(Id, Region))
		{
			return false;
		}
		const TArray<const FArcRmlTestDraw*> Found = FindShaderDraws(View.GetDraws(), Region, nullptr);
		if (!Assert.AreEqual(2, Found.Num(), FString::Printf(TEXT("%s: mask layers"), Id)))
		{
			return false;
		}
		const FBox2f TopHalf = Ink(Region.Min.X, Region.Min.Y, 120, 30);
		const FBox2f LeftHalf = Ink(Region.Min.X, Region.Min.Y, 60, 60);
		if (!ExpectDraw(*Found[0], FString::Printf(TEXT("%s: second layer 100%% 50%%"), Id), TopHalf, FVector2f(120, 30), FVector2f(0, 0), {FVector2f(Region.Min.X, Region.Min.Y)})
			|| !ExpectDraw(*Found[1], FString::Printf(TEXT("%s: first layer 50%% 100%%"), Id), LeftHalf, FVector2f(60, 60), FVector2f(0, 0), {FVector2f(Region.Min.X, Region.Min.Y)}))
		{
			return false;
		}
		if (!Assert.IsTrue(Found[0]->Scissor.IsSet() && Found[0]->Scissor.GetValue() == ToIntRect(Region), FString::Printf(TEXT("%s: scissor is the element box"), Id)))
		{
			return false;
		}
		int32 Composites = 0;
		Rml::BlendMode Seen = Rml::BlendMode::Blend;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op == EArcRmlTestDrawOp::CompositeLayers && Draw.bClipMask && Draw.Scissor.IsSet() && Draw.Scissor.GetValue() == ToIntRect(Region))
			{
				++Composites;
				Seen = Draw.BlendMode;
			}
		}
		if (!Assert.AreEqual(bExpectsComposite ? 1 : 0, Composites, FString::Printf(TEXT("%s: clip-mask composites"), Id)))
		{
			return false;
		}
		return !bExpectsComposite || Assert.IsTrue(Seen == Blend, FString::Printf(TEXT("%s: composite blend %d, expected %d; Chrome ink %s"), Id, static_cast<int32>(Seen), static_cast<int32>(Blend), *DescribeBox(ChromeInk)));
	}

	TEST_METHOD(CompositeRules_KeepTheirLayersAndBlendModes)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectComposite(TEXT("addc"), Ink(10, 170, 120, 60), false, Rml::BlendMode::Blend)));
		ASSERT_THAT(IsTrue(ExpectComposite(TEXT("subc"), Ink(150, 200, 60, 30), true, Rml::BlendMode::SourceOut)));
		ASSERT_THAT(IsTrue(ExpectComposite(TEXT("interc"), Ink(290, 170, 60, 30), true, Rml::BlendMode::SourceIn)));
		ASSERT_THAT(IsTrue(ExpectComposite(TEXT("exclc"), Ink(430, 170, 120, 60), true, Rml::BlendMode::Xor)));
	}

	TEST_METHOD(MaskUrlWithAttachment_IsDropped)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("Declaration 'mask: url(MaskFixed.png) fixed;' is dropped")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("maskfixed"), Region)));
		ASSERT_THAT(AreEqual(0, FindShaderDraws(View.GetDraws(), Region, nullptr).Num(), TEXT("no mask layer: Chrome ink of the element colour is the whole 120x60 box")));
		int32 Flat = 0;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bShader && Draw.FirstColourPremultiplied == FColor(255, 216, 177, 255) && SameBox(Draw.Bounds, Region, Tolerance))
			{
				++Flat;
			}
		}
		ASSERT_THAT(AreEqual(1, Flat, TEXT("the element colour fills the box")));
	}

	TEST_METHOD(UrlWithExtraToken_IsDropped)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(GeometryPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("flip-horizontal")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("urlflip"), Region)));
		ASSERT_THAT(AreEqual(0, CountGeometryDraws(View.GetDraws(), Region), TEXT("Chrome drops the declaration: no ink of the image colour")));
	}

	TEST_METHOD(BorderBox1202_TilesCoverTheBorder)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DensePath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("border1202"), Region)));
		const TArray<const FArcRmlTestDraw*> Found = FindShaderDraws(View.GetDraws(), Region, nullptr);
		ASSERT_THAT(AreEqual(1, Found.Num()));
		ASSERT_THAT(IsTrue(ExpectDraw(*Found[0], TEXT("border1202"), Ink(10, 10, 1202, 40), FVector2f(1200, 38), FVector2f(1200, 38), GridOrigins(-1189, 1200, 3, -27, 38, 3))));
		ASSERT_THAT(IsTrue(ExpectCovers(*Found[0], TEXT("border1202"), 3, 3)));
	}

	TEST_METHOD(ImageTile4_PaintsEveryPixelWithoutACap)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DensePath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("tile4img"), Ink(10, 70, 1200, 20), FVector2f(4, 4), FVector2f(4, 4), GridOrigins(10, 4, 300, 70, 4, 5), TEXT("Tile4.png"))));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("tile4img"), Region)));
		ASSERT_THAT(IsTrue(ExpectCovers(*FindShaderDraws(View.GetDraws(), Region, nullptr)[0], TEXT("tile4img"), 300, 5)));
	}

	TEST_METHOD(GradientTile4_PaintsEveryPixelWithoutACap)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DensePath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("tile4grad"), Ink(10, 110, 1200, 20), FVector2f(4, 4), FVector2f(4, 4), GridOrigins(10, 4, 300, 110, 4, 5))));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("tile4grad"), Region)));
		ASSERT_THAT(IsTrue(ExpectCovers(*FindShaderDraws(View.GetDraws(), Region, nullptr)[0], TEXT("tile4grad"), 300, 5)));
	}

	TEST_METHOD(FixedAttachment_PositionsInTheViewport)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(AttachPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ExpectLayer(TEXT("fixedbox"), Ink(220, 160, 50, 30), FVector2f(50, 30), FVector2f(0, 0), {FVector2f(220, 160)})));
	}

	TEST_METHOD(LocalAttachment_MovesWithTheScroll)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(AttachPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(TEXT("localbox"), Region)));
		ASSERT_THAT(AreEqual(0, FindTileDraws(View.GetDraws(), Region, FVector2f(50, 20)).Num(), TEXT("scrolled to the top, the tile at content y 100 lies below the 60 px box: Chrome ink none")));

		const FVector2f Point(Region.Min.X + 50.0f, Region.Min.Y + 30.0f);
		View.MouseMove(Point);
		View.MouseWheel(Point, -1.0f);
		float ScrollY = 0.0f;
		float Previous = -1.0f;
		int32 Stable = 0;
		for (int32 Attempt = 0; Attempt < 200 && Stable < 5; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("localbox"));
			ASSERT_THAT(IsNotNull(Element));
			ScrollY = Element->Scroll.Y;
			Stable = (ScrollY > 0.0f && FMath::IsNearlyEqual(ScrollY, Previous)) ? Stable + 1 : 0;
			Previous = ScrollY;
		}
		ASSERT_THAT(IsTrue(ScrollY > 0.0f && Stable >= 5, FString::Printf(TEXT("the wheel scrolled the box, scroll top %.2f"), ScrollY)));
		const float TileTop = Region.Min.Y + 100.0f - ScrollY;
		ASSERT_THAT(IsTrue(TileTop >= Region.Min.Y && TileTop + 20.0f <= Region.Max.Y, FString::Printf(TEXT("the moved tile is inside the box, top %.2f"), TileTop)));
		const TArray<const FArcRmlTestDraw*> Tiles = FindTileDraws(View.GetDraws(), Region, FVector2f(50, 20));
		ASSERT_THAT(AreEqual(1, Tiles.Num(), TEXT("localbox: 50x20 tile draws")));
		ASSERT_THAT(IsTrue(ExpectDraw(*Tiles[0], TEXT("localbox"), Ink(410, TileTop, 50, 20), FVector2f(50, 20), FVector2f(0, 0), {FVector2f(410, TileTop)})));
	}

	void ExpectAnimatedTile(const TCHAR* Id, FVector2f Tile, const TCHAR* ChromeX, const TCHAR* ChromeY, const FBox2f& ChromeInk)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		FBox2f Region;
		ASSERT_THAT(IsTrue(GetRect(Id, Region)));
		FVector2f Origin;
		ASSERT_THAT(IsTrue(ChromeOrigin(Region, ChromeX, ChromeY, Tile, Origin)));
		ASSERT_THAT(IsTrue(ExpectLayer(Id, ChromeInk, Tile, FVector2f(0, 0), {Origin})));
	}

	void ExpectRunningInput(const TCHAR* Id, const TCHAR* Property, const TCHAR* Chrome)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Id);
		ASSERT_THAT(IsNotNull(Element));
		const FString* Running = Element->Animated.Find(Property);
		ASSERT_THAT(IsTrue(Running != nullptr, FString::Printf(TEXT("%s has no running %s, Chrome %s"), Id, Property, Chrome)));
		FString Value = *Running;
		Value.ReplaceInline(TEXT(" !important"), TEXT(""));
		ASSERT_THAT(AreEqual(FString(Chrome), Value, FString::Printf(TEXT("%s %s at %.2f s"), Id, Property, Elapsed)));
	}

	TEST_METHOD(AnimatedPosition_At05_LengthsMatchChrome)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(0.5)));
		ExpectRunningInput(TEXT("animpx"), TEXT("background-position-x"), TEXT("30px"));
		ExpectRunningInput(TEXT("animpx"), TEXT("background-position-y"), TEXT("15px"));
		ExpectAnimatedTile(TEXT("animpx"), FVector2f(30, 20), TEXT("30px"), TEXT("15px"), Ink(40, 25, 30, 20));
	}

	TEST_METHOD(AnimatedPosition_At05_PercentToCalcMatchesChrome)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(0.5)));
		ExpectAnimatedTile(TEXT("animcalc"), FVector2f(30, 20), TEXT("calc(50% - 10px)"), TEXT("50%"), Ink(185, 30, 30, 20));
	}

	TEST_METHOD(AnimatedSize_At05_MatchesChrome)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(0.5)));
		ExpectRunningInput(TEXT("animsize"), TEXT("background-size"), TEXT("40px 20px"));
		ExpectAnimatedTile(TEXT("animsize"), FVector2f(40, 20), TEXT("0%"), TEXT("0%"), Ink(290, 10, 40, 20));
	}

	TEST_METHOD(PositionTransition_At05_MatchesChrome)
	{
		using namespace ArcRmlCssParityLayerTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetRootClass(TEXT("go"), true);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(IsTrue(AdvanceTo(0.5)));
		ExpectRunningInput(TEXT("transpos"), TEXT("background-position-x"), TEXT("30px"));
		ExpectRunningInput(TEXT("transpos"), TEXT("background-position-y"), TEXT("15px"));
		ExpectAnimatedTile(TEXT("transpos"), FVector2f(30, 20), TEXT("30px"), TEXT("15px"), Ink(460, 25, 30, 20));
	}

	TEST_METHOD(Warning_ImageDecoratorWithASize)
	{
		ArcRmlCssParityLayerTestsPrivate::FIgnoredLayerWarningCapture Capture;
		ASSERT_THAT(IsTrue(OpenFixture(ArcRmlCssParityLayerTestsPrivate::WarnSizePath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(AreEqual(1, Capture.GetCount(), TEXT("one warning per decorator")));
	}

	TEST_METHOD(Warning_OriginBoxOutsideTheClipBox)
	{
		ArcRmlCssParityLayerTestsPrivate::FIgnoredLayerWarningCapture Capture;
		ASSERT_THAT(IsTrue(OpenFixture(ArcRmlCssParityLayerTestsPrivate::WarnOriginPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(AreEqual(1, Capture.GetCount(), TEXT("border-box origin with a padding-box clip")));
	}

	TEST_METHOD(NoWarning_OriginBoxInsideTheClipBox)
	{
		ArcRmlCssParityLayerTestsPrivate::FIgnoredLayerWarningCapture Capture;
		ASSERT_THAT(IsTrue(OpenFixture(ArcRmlCssParityLayerTestsPrivate::NoWarnPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(AreEqual(0, Capture.GetCount(), TEXT("content-box origin with a border-box clip")));
	}
};
