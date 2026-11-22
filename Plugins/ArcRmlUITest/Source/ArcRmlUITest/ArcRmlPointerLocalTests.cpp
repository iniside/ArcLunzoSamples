// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "InputCoreTypes.h"

namespace ArcRmlPointerLocalTestsPrivate
{
	constexpr double PixelTolerance = 0.51;
	constexpr double FractionTolerance = 0.01;

	const int32 TagIndex = 0;
	const int32 CurrentXIndex = 1;
	const int32 CurrentWidthIndex = 2;
	const int32 OffsetXIndex = 3;
	const int32 CurrentYIndex = 4;
	const int32 CurrentHeightIndex = 5;
	const int32 OffsetYIndex = 6;
	const int32 ArgumentCount = 7;

	const FArcRmlViewModelCall* FindLastCall(const TArray<FArcRmlViewModelCall>& Calls, const FString& Tag)
	{
		for (int32 Index = Calls.Num() - 1; Index >= 0; --Index)
		{
			const FArcRmlViewModelCall& Call = Calls[Index];
			if (Call.FunctionName == TEXT("P") && Call.Arguments.Num() > TagIndex && Call.Arguments[TagIndex].String == Tag)
			{
				return &Call;
			}
		}
		return nullptr;
	}

	int32 CountTagged(const TArray<FArcRmlViewModelCall>& Calls, const FString& Tag)
	{
		int32 Count = 0;
		for (const FArcRmlViewModelCall& Call : Calls)
		{
			if (Call.FunctionName == TEXT("P") && Call.Arguments.Num() > TagIndex && Call.Arguments[TagIndex].String == Tag)
			{
				++Count;
			}
		}
		return Count;
	}

	FString DescribeCalls(const TArray<FArcRmlViewModelCall>& Calls)
	{
		TArray<FString> Parts;
		for (const FArcRmlViewModelCall& Call : Calls)
		{
			TArray<FString> Arguments;
			for (const FArcRmlValue& Argument : Call.Arguments)
			{
				Arguments.Add(Argument.Kind == EArcRmlValueKind::Float ? FString::Printf(TEXT("%.3f"), Argument.Float) : Argument.String);
			}
			Parts.Add(FString::Printf(TEXT("%s(%s)"), *Call.FunctionName, *FString::Join(Arguments, TEXT(", "))));
		}
		return FString::Printf(TEXT("%d calls: %s"), Parts.Num(), *FString::Join(Parts, TEXT("; ")));
	}

	FString CheckArgument(const FArcRmlViewModelCall& Call, int32 Index, const TCHAR* Name, double Expected, double Tolerance)
	{
		if (Call.Arguments.Num() != ArgumentCount)
		{
			return FString::Printf(TEXT("%s: the call has %d arguments, expected %d. "), Name, Call.Arguments.Num(), ArgumentCount);
		}
		const FArcRmlValue& Value = Call.Arguments[Index];
		if (Value.Kind != EArcRmlValueKind::Float)
		{
			return FString::Printf(TEXT("%s: the value is not a number (kind %d). "), Name, static_cast<int32>(Value.Kind));
		}
		if (!FMath::IsNearlyEqual(Value.Float, Expected, Tolerance))
		{
			return FString::Printf(TEXT("%s: got %.3f, expected %.3f. "), Name, Value.Float, Expected);
		}
		return FString();
	}
}

TEST_CLASS(ArcRmlPointerLocalTests, "ArcRmlUI.PointerLocal")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(float DpRatio)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Input/PointerLocalTest.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path, DpRatio) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ParseLayout(Elements);
	}

	bool Read()
	{
		return View.Flush() && View.Flush() && View.ParseLayout(Elements);
	}

	FVector2f OriginOf(const FString& DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		check(Element);
		return FVector2f(Element->Rect.Position.X, Element->Rect.Position.Y);
	}

	FVector2f SizeOf(const FString& DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		check(Element);
		return FVector2f(Element->Rect.Size.X, Element->Rect.Size.Y);
	}

	void Press(FVector2f Point)
	{
		View.ClearCalls();
		View.MouseMove(Point);
		View.MouseDown(Point);
		View.MouseUp(Point);
	}

	TEST_METHOD(Press_AtQuarterOfTrack_CurrentGivesTrackBox)
	{
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlPointerLocalTestsPrivate;
		const FVector2f Origin = OriginOf(TEXT("trk"));
		const FVector2f Size = SizeOf(TEXT("trk"));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Size.X, 200.0f, 0.01f), FString::Printf(TEXT("the track is %.2f wide, not 200"), Size.X)));

		Press(Origin + FVector2f(50.0f, 10.0f));
		ASSERT_THAT(IsTrue(Read()));
		const FArcRmlViewModelCall* Call = FindLastCall(View.GetCalls(), TEXT("trk-down"));
		ASSERT_THAT(IsNotNull(Call, DescribeCalls(View.GetCalls())));
		const FString Error = CheckArgument(*Call, CurrentXIndex, TEXT("current_x"), 50.0, PixelTolerance) + CheckArgument(*Call, CurrentWidthIndex, TEXT("current_width"), 200.0, PixelTolerance)
			+ CheckArgument(*Call, OffsetXIndex, TEXT("offset_x"), 50.0, PixelTolerance) + CheckArgument(*Call, CurrentYIndex, TEXT("current_y"), 10.0, PixelTolerance)
			+ CheckArgument(*Call, CurrentHeightIndex, TEXT("current_height"), 20.0, PixelTolerance) + CheckArgument(*Call, OffsetYIndex, TEXT("offset_y"), 10.0, PixelTolerance);
		ASSERT_THAT(IsTrue(Error.IsEmpty(), Error + DescribeCalls(View.GetCalls())));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Call->Arguments[CurrentXIndex].Float / Call->Arguments[CurrentWidthIndex].Float, 0.25, FractionTolerance)));
	}

	TEST_METHOD(Press_OnTransformedThumb_CurrentGivesTrackAndOffsetGivesThumb)
	{
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlPointerLocalTestsPrivate;
		const FVector2f Origin = OriginOf(TEXT("trk"));

		Press(Origin + FVector2f(150.0f, 10.0f));
		ASSERT_THAT(IsTrue(Read()));
		const FArcRmlViewModelCall* Call = FindLastCall(View.GetCalls(), TEXT("trk-down"));
		ASSERT_THAT(IsNotNull(Call, DescribeCalls(View.GetCalls())));
		const FString Error = CheckArgument(*Call, CurrentXIndex, TEXT("current_x"), 150.0, PixelTolerance) + CheckArgument(*Call, CurrentWidthIndex, TEXT("current_width"), 200.0, PixelTolerance)
			+ CheckArgument(*Call, OffsetXIndex, TEXT("offset_x"), 10.0, PixelTolerance) + CheckArgument(*Call, OffsetYIndex, TEXT("offset_y"), 10.0, PixelTolerance);
		ASSERT_THAT(IsTrue(Error.IsEmpty(), Error + DescribeCalls(View.GetCalls())));
	}

	TEST_METHOD(Drag_PastTrackEnd_FractionExceedsOne)
	{
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlPointerLocalTestsPrivate;
		const FVector2f Origin = OriginOf(TEXT("trk"));
		const FVector2f Press = Origin + FVector2f(50.0f, 10.0f);

		View.ClearCalls();
		View.MouseMove(Press);
		View.MouseDown(Press);
		View.MouseMove(Press + FVector2f(6.0f, 0.0f));
		View.MouseMove(Origin + FVector2f(280.0f, 10.0f));
		View.MouseUp(Origin + FVector2f(280.0f, 10.0f));
		ASSERT_THAT(IsTrue(Read()));
		const FArcRmlViewModelCall* Call = FindLastCall(View.GetCalls(), TEXT("trk-drag"));
		ASSERT_THAT(IsNotNull(Call, DescribeCalls(View.GetCalls())));
		const FString Error = CheckArgument(*Call, CurrentXIndex, TEXT("current_x"), 280.0, PixelTolerance) + CheckArgument(*Call, CurrentWidthIndex, TEXT("current_width"), 200.0, PixelTolerance)
			+ CheckArgument(*Call, OffsetXIndex, TEXT("offset_x"), 280.0, PixelTolerance);
		ASSERT_THAT(IsTrue(Error.IsEmpty(), Error + DescribeCalls(View.GetCalls())));
		ASSERT_THAT(IsTrue(Call->Arguments[CurrentXIndex].Float / Call->Arguments[CurrentWidthIndex].Float > 1.0));
	}

	TEST_METHOD(Press_AtDensityTwo_ReturnsCssPixels)
	{
		ASSERT_THAT(IsTrue(OpenFixture(2.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlPointerLocalTestsPrivate;
		const FVector2f Origin = OriginOf(TEXT("trk"));
		const FVector2f Size = SizeOf(TEXT("trk"));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Size.X, 400.0f, 0.01f), FString::Printf(TEXT("the track is %.2f device pixels wide, not 400"), Size.X)));

		Press(Origin + FVector2f(Size.X * 0.25f, Size.Y * 0.5f));
		ASSERT_THAT(IsTrue(Read()));
		const FArcRmlViewModelCall* Call = FindLastCall(View.GetCalls(), TEXT("trk-down"));
		ASSERT_THAT(IsNotNull(Call, DescribeCalls(View.GetCalls())));
		const FString Error = CheckArgument(*Call, CurrentXIndex, TEXT("current_x"), 50.0, PixelTolerance) + CheckArgument(*Call, CurrentWidthIndex, TEXT("current_width"), 200.0, PixelTolerance)
			+ CheckArgument(*Call, OffsetXIndex, TEXT("offset_x"), 50.0, PixelTolerance) + CheckArgument(*Call, CurrentHeightIndex, TEXT("current_height"), 20.0, PixelTolerance);
		ASSERT_THAT(IsTrue(Error.IsEmpty(), Error + DescribeCalls(View.GetCalls())));
	}

	TEST_METHOD(Press_UnderScaledAncestor_ReturnsUnscaledBox)
	{
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlPointerLocalTestsPrivate;
		const FVector2f Origin = OriginOf(TEXT("sanc"));

		Press(Origin + FVector2f(50.0f, 10.0f));
		ASSERT_THAT(IsTrue(Read()));
		const FArcRmlViewModelCall* Call = FindLastCall(View.GetCalls(), TEXT("sc-down"));
		ASSERT_THAT(IsNotNull(Call, DescribeCalls(View.GetCalls())));
		const FString Error = CheckArgument(*Call, CurrentXIndex, TEXT("current_x"), 25.0, PixelTolerance) + CheckArgument(*Call, CurrentWidthIndex, TEXT("current_width"), 100.0, PixelTolerance)
			+ CheckArgument(*Call, OffsetXIndex, TEXT("offset_x"), 25.0, PixelTolerance) + CheckArgument(*Call, CurrentYIndex, TEXT("current_y"), 5.0, PixelTolerance);
		ASSERT_THAT(IsTrue(Error.IsEmpty(), Error + DescribeCalls(View.GetCalls())));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Call->Arguments[CurrentXIndex].Float / Call->Arguments[CurrentWidthIndex].Float, 0.25, FractionTolerance)));
	}

	TEST_METHOD(Press_InPaddingOfBorderedTarget_OffsetStartsAtPaddingEdge)
	{
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlPointerLocalTestsPrivate;
		const FVector2f Origin = OriginOf(TEXT("bar"));
		const FVector2f Size = SizeOf(TEXT("bar"));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Size.X, 240.0f, 0.01f) && FMath::IsNearlyEqual(Size.Y, 26.0f, 0.01f), FString::Printf(TEXT("the bar is %.2f x %.2f, not 240 x 26"), Size.X, Size.Y)));

		Press(Origin + FVector2f(3.0f, 13.0f));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, CountTagged(View.GetCalls(), TEXT("bt-down")), TEXT("the press reached the inner track, not the bar padding")));
		const FArcRmlViewModelCall* Call = FindLastCall(View.GetCalls(), TEXT("bar-down"));
		ASSERT_THAT(IsNotNull(Call, DescribeCalls(View.GetCalls())));
		const FString Error = CheckArgument(*Call, CurrentXIndex, TEXT("current_x"), 3.0, PixelTolerance) + CheckArgument(*Call, CurrentWidthIndex, TEXT("current_width"), 240.0, PixelTolerance)
			+ CheckArgument(*Call, OffsetXIndex, TEXT("offset_x"), 2.0, PixelTolerance) + CheckArgument(*Call, CurrentYIndex, TEXT("current_y"), 13.0, PixelTolerance)
			+ CheckArgument(*Call, CurrentHeightIndex, TEXT("current_height"), 26.0, PixelTolerance) + CheckArgument(*Call, OffsetYIndex, TEXT("offset_y"), 12.0, PixelTolerance);
		ASSERT_THAT(IsTrue(Error.IsEmpty(), Error + DescribeCalls(View.GetCalls())));
	}

	TEST_METHOD(Press_AtInnerBarTrackEnds_FractionSpansZeroToOne)
	{
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlPointerLocalTestsPrivate;
		const FVector2f Origin = OriginOf(TEXT("bartrack"));
		const FVector2f Size = SizeOf(TEXT("bartrack"));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Size.X, 232.0f, 0.01f), FString::Printf(TEXT("the inner track is %.2f wide, not 232"), Size.X)));

		Press(Origin + FVector2f(Size.X - 1.0f, 9.0f));
		ASSERT_THAT(IsTrue(Read()));
		const FArcRmlViewModelCall* RightEnd = FindLastCall(View.GetCalls(), TEXT("bt-down"));
		ASSERT_THAT(IsNotNull(RightEnd, DescribeCalls(View.GetCalls())));
		const FString RightError = CheckArgument(*RightEnd, CurrentWidthIndex, TEXT("current_width"), 232.0, PixelTolerance) + CheckArgument(*RightEnd, CurrentXIndex, TEXT("current_x"), 231.0, PixelTolerance);
		ASSERT_THAT(IsTrue(RightError.IsEmpty(), RightError + DescribeCalls(View.GetCalls())));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(RightEnd->Arguments[CurrentXIndex].Float / RightEnd->Arguments[CurrentWidthIndex].Float, 1.0, FractionTolerance),
			TEXT("the right end is not about 1.0")));

		Press(Origin + FVector2f(0.0f, 9.0f));
		ASSERT_THAT(IsTrue(Read()));
		const FArcRmlViewModelCall* LeftEnd = FindLastCall(View.GetCalls(), TEXT("bt-down"));
		ASSERT_THAT(IsNotNull(LeftEnd, DescribeCalls(View.GetCalls())));
		const FString LeftError = CheckArgument(*LeftEnd, CurrentXIndex, TEXT("current_x"), 0.0, PixelTolerance);
		ASSERT_THAT(IsTrue(LeftError.IsEmpty(), LeftError + DescribeCalls(View.GetCalls())));
	}

	TEST_METHOD(KeyDown_ReadingPointerParameters_PassesEmptyValues)
	{
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlPointerLocalTestsPrivate;
		const FVector2f Origin = OriginOf(TEXT("keyel"));

		View.MouseClick(Origin + FVector2f(10.0f, 10.0f));
		ASSERT_THAT(IsTrue(Read()));
		View.ClearCalls();
		View.PressKey(EKeys::A);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(1, CountTagged(View.GetCalls(), TEXT("key")), DescribeCalls(View.GetCalls())));
		const FArcRmlViewModelCall* Call = FindLastCall(View.GetCalls(), TEXT("key"));
		ASSERT_THAT(IsNotNull(Call));
		ASSERT_THAT(AreEqual(ArgumentCount, Call->Arguments.Num(), DescribeCalls(View.GetCalls())));
		for (int32 Index = CurrentXIndex; Index < ArgumentCount; ++Index)
		{
			ASSERT_THAT(IsTrue(Call->Arguments[Index].Kind == EArcRmlValueKind::Empty, FString::Printf(TEXT("argument %d is not empty: %s"), Index, *DescribeCalls(View.GetCalls()))));
		}
	}
};
