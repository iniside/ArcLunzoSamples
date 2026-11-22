// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlUpdateCostTestsPrivate
{
	constexpr int32 ListRows = 200;
	constexpr int32 ChangedRows = 10;
	constexpr int32 ElementsPerChangedRow = 15;
	constexpr int32 DataViewsPerChangedRow = 6;
	constexpr int32 WholeListDataViews = 2;
	constexpr int32 VisibleRowsPerList = 49;
	constexpr int32 PaintedElementsPerKeyedRow = 8;
	constexpr int32 PaintedElementsPerUnkeyedRow = 7;
	constexpr int32 PaintedElementsPerRowPair = PaintedElementsPerKeyedRow + PaintedElementsPerUnkeyedRow;
	constexpr int32 PaintedElementsOutsideRows = 5;
	constexpr int32 ChangedVisibleRowsPerList = 3;
	constexpr int32 HotChunkElements = 6;
	const FColor HotColour(0xcc, 0x33, 0x33, 255);

	FVector2f GetKeyedRowPoint(int32 Row)
	{
		return FVector2f(108.0f, 32.0f + 14.0f * Row + 7.0f);
	}

	FArcRmlValue MakeRow(int32 Row, const FString& Name, int32 Value)
	{
		FArcRmlValue RowValue;
		RowValue.Kind = EArcRmlValueKind::Struct;
		RowValue.MemberNames = {TEXT("Id"), TEXT("Name"), TEXT("Value")};
		RowValue.Children.Add(ArcRmlUITestHelpers::MakeString(FString::Printf(TEXT("r%d"), Row)));
		RowValue.Children.Add(ArcRmlUITestHelpers::MakeString(Name));
		RowValue.Children.Add(ArcRmlUITestHelpers::MakeInt(Value));
		return RowValue;
	}

	FVector2f GetCentre(const FArcRmlTestLayoutElement& Element)
	{
		return Element.Rect.Position + Element.Rect.Size * 0.5f;
	}

	int32 CountSolidDraws(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
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
}

TEST_CLASS(ArcRmlUpdateCostTests, "ArcRmlUI.UpdateCost")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* File)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("UpdateCost/%s"), File));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	int64 Count(Rml::ArcPerf::Counter Counter) const
	{
		return View.GetPerfCounter(Counter);
	}

	FString DescribeCounters() const
	{
		return FString::Printf(
			TEXT("elements updated %lld, definition lookups %lld, pseudo-class changes %lld, flex item formats %lld, boxes changed %lld, text geometry regens %lld, "
				 "elements rendered %lld, elements re-recorded %lld, scroll clamp visits %lld, layout dirty calls %lld, formats %lld"),
			Count(Rml::ArcPerf::Counter::ElementsUpdated), Count(Rml::ArcPerf::Counter::DefinitionLookups), Count(Rml::ArcPerf::Counter::PseudoClassChanges),
			Count(Rml::ArcPerf::Counter::FormatFinalFlex), Count(Rml::ArcPerf::Counter::BoxesChanged), Count(Rml::ArcPerf::Counter::TextGeometryRegens),
			Count(Rml::ArcPerf::Counter::ElementsRendered), Count(Rml::ArcPerf::Counter::ElementsRerecorded), Count(Rml::ArcPerf::Counter::ScrollClampVisits),
			Count(Rml::ArcPerf::Counter::LayoutDirtyCalls), Count(Rml::ArcPerf::Counter::Formats));
	}

	void ExpectAtMost(Rml::ArcPerf::Counter Counter, int64 Bound, const TCHAR* What)
	{
		const int64 Value = Count(Counter);
		ASSERT_THAT(IsTrue(Value <= Bound, FString::Printf(TEXT("%s %lld > %lld; %s"), What, Value, Bound, *DescribeCounters())));
	}

	void UpdateRows(bool bRestore)
	{
		TArray<FArcRmlListOp> Ops;
		for (int32 Index = 0; Index < ArcRmlUpdateCostTestsPrivate::ChangedRows; ++Index)
		{
			const int32 Row = 7 + Index * 17;
			FArcRmlListOp& Update = Ops.AddDefaulted_GetRef();
			Update.Type = EArcRmlListOpType::Update;
			Update.Index = Row;
			Update.Value = bRestore ? ArcRmlUpdateCostTestsPrivate::MakeRow(Row, FString::Printf(TEXT("Row %d"), Row), (Row * 37) % 1000)
									: ArcRmlUpdateCostTestsPrivate::MakeRow(Row, FString::Printf(TEXT("Updated %d"), Row), 1000 + Row);
		}
		View.ApplyListOps(TEXT("mock"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Ops));
	}

	int64 FramesSince(uint64 FrameBefore) const
	{
		return int64(View.GetProbe().FrameSerial - FrameBefore);
	}

	void ExpectPerFrameAtMost(Rml::ArcPerf::Counter Counter, int64 Frames, int64 Bound, const TCHAR* What)
	{
		const int64 Value = Count(Counter);
		ASSERT_THAT(IsTrue(Value <= Bound * Frames,
			FString::Printf(TEXT("%s %lld over %lld frames > %lld per frame; %s"), What, Value, Frames, Bound, *DescribeCounters())));
	}

	TEST_METHOD(IdleFrameWithChange_RendersNoCleanRow_AndPaintsOnlyTheVisibleRows)
	{
		using namespace ArcRmlUpdateCostTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("DataList.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("noop"), true);
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the root class change rendered no frame")));
		const int64 Frames = FramesSince(FrameBefore);
		TestRunner->AddInfo(FString::Printf(TEXT("%lld frames; elements retained %lld; %s"), Frames, Count(Rml::ArcPerf::Counter::ElementsRetained), *DescribeCounters()));

		ExpectPerFrameAtMost(Rml::ArcPerf::Counter::ElementsRendered, Frames, 4, TEXT("elements rendered"));
		ExpectPerFrameAtMost(Rml::ArcPerf::Counter::ElementsRetained, Frames, int64(VisibleRowsPerList) * PaintedElementsPerRowPair + PaintedElementsOutsideRows,
			TEXT("elements retained"));
		ASSERT_THAT(IsTrue(Count(Rml::ArcPerf::Counter::ElementsRetained) >= int64(VisibleRowsPerList) * PaintedElementsPerRowPair, TEXT("the visible rows were not painted")));
		ASSERT_THAT(AreEqual(int64(0), Count(Rml::ArcPerf::Counter::ElementsRerecorded), DescribeCounters()));
	}

	TEST_METHOD(ChangedRows_HitRegionWalkVisitsOnlyTheChangedRows)
	{
		using namespace ArcRmlUpdateCostTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("DataList.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		UpdateRows(false);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;
		UpdateRows(true);
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the row change rendered no frame")));
		const int64 Frames = FramesSince(FrameBefore);
		const int64 Walked = Count(Rml::ArcPerf::Counter::HitRegionElementsWalked);
		TestRunner->AddInfo(FString::Printf(TEXT("%lld frames; hit region elements walked %lld; %s"), Frames, Walked, *DescribeCounters()));
		ExpectPerFrameAtMost(Rml::ArcPerf::Counter::HitRegionElementsWalked, Frames, int64(2) * ChangedRows * ElementsPerChangedRow,
			TEXT("hit region elements walked by two lists of changed rows"));
	}

	TEST_METHOD(ChangedRows_RenderOnlyTheChangedVisibleRows)
	{
		using namespace ArcRmlUpdateCostTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("DataList.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		UpdateRows(false);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;
		UpdateRows(true);
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the row change rendered no frame")));
		const int64 Frames = FramesSince(FrameBefore);
		TestRunner->AddInfo(FString::Printf(TEXT("%lld frames; elements retained %lld; %s"), Frames, Count(Rml::ArcPerf::Counter::ElementsRetained), *DescribeCounters()));

		ExpectPerFrameAtMost(Rml::ArcPerf::Counter::ElementsRendered, Frames, int64(ChangedVisibleRowsPerList) * PaintedElementsPerRowPair + PaintedElementsOutsideRows,
			TEXT("elements rendered"));
		ExpectPerFrameAtMost(Rml::ArcPerf::Counter::ElementsRetained, Frames, int64(VisibleRowsPerList) * PaintedElementsPerRowPair + PaintedElementsOutsideRows,
			TEXT("elements retained"));
		ExpectPerFrameAtMost(Rml::ArcPerf::Counter::ElementsRerecorded, Frames, int64(ChangedVisibleRowsPerList) * PaintedElementsPerRowPair, TEXT("elements re-recorded"));
	}

	TEST_METHOD(ClassChangeInARow_RendersOnlyItsPaintChunk)
	{
		using namespace ArcRmlUpdateCostTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ChunkDirty.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, CountSolidDraws(View.GetDraws(), HotColour), TEXT("hot before the change")));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;
		View.SetModelValue(TEXT("cd"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Hot")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the class change rendered no frame")));
		const int64 Frames = FramesSince(FrameBefore);
		TestRunner->AddInfo(FString::Printf(TEXT("%lld frames; elements retained %lld; %s"), Frames, Count(Rml::ArcPerf::Counter::ElementsRetained), *DescribeCounters()));

		ASSERT_THAT(AreEqual(1, CountSolidDraws(View.GetDraws(), HotColour), TEXT("hot after the change")));
		ExpectPerFrameAtMost(Rml::ArcPerf::Counter::ElementsRendered, Frames, HotChunkElements + 2, TEXT("elements rendered"));
		ExpectPerFrameAtMost(Rml::ArcPerf::Counter::ElementsRerecorded, Frames, 1, TEXT("elements re-recorded"));
	}

	TEST_METHOD(MouseMovesOverRowsWithoutHoverRules_RunNoViewFrame)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("DataList.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.MouseMove(ArcRmlUpdateCostTestsPrivate::GetKeyedRowPoint(2));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;

		for (int32 Row = 3; Row < 12; ++Row)
		{
			View.MouseMove(ArcRmlUpdateCostTestsPrivate::GetKeyedRowPoint(Row));
			ASSERT_THAT(IsTrue(View.Flush()));
		}
		View.MouseMove(ArcRmlUpdateCostTestsPrivate::GetKeyedRowPoint(11) + FVector2f(3.0f, 1.0f));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));

		ASSERT_THAT(AreEqual(FrameBefore, View.GetProbe().FrameSerial, FString::Printf(TEXT("mouse moves over rows rendered frames; %s"), *DescribeCounters())));
		ASSERT_THAT(AreEqual(int64(0), Count(Rml::ArcPerf::Counter::ElementsUpdated), DescribeCounters()));
		ASSERT_THAT(AreEqual(int64(0), Count(Rml::ArcPerf::Counter::ElementsRendered), DescribeCounters()));
	}

	TEST_METHOD(ChangedKeyedRows_CostProportionalToTheChangedRows)
	{
		using namespace ArcRmlUpdateCostTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("DataList.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		UpdateRows(false);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		UpdateRows(true);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		TestRunner->AddInfo(DescribeCounters());
		ASSERT_THAT(AreEqual(int64(0), Count(Rml::ArcPerf::Counter::FontGlyphBatches), TEXT("the restored rows installed glyphs")));

		const int64 RowWork = int64(ChangedRows) * ElementsPerChangedRow;
		ASSERT_THAT(IsTrue(Count(Rml::ArcPerf::Counter::ElementsRendered) > 0, TEXT("the row change rendered no frame")));
		ExpectAtMost(Rml::ArcPerf::Counter::ElementsUpdated, RowWork + 20, TEXT("elements updated"));
		ExpectAtMost(Rml::ArcPerf::Counter::DefinitionLookups, int64(ChangedRows), TEXT("definition lookups"));
		ExpectAtMost(Rml::ArcPerf::Counter::FormatFinalFlex, int64(ChangedRows) * 7, TEXT("flex items formatted"));
		ExpectAtMost(Rml::ArcPerf::Counter::BoxesChanged, RowWork, TEXT("boxes changed"));
		ExpectAtMost(Rml::ArcPerf::Counter::TextGeometryRegens, int64(ChangedRows) * 4, TEXT("text geometry regens"));
		ExpectAtMost(Rml::ArcPerf::Counter::ElementsRerecorded, RowWork + 20, TEXT("elements re-recorded"));
		ExpectAtMost(Rml::ArcPerf::Counter::ScrollClampVisits, RowWork + 20, TEXT("scroll clamp visits"));
	}

	TEST_METHOD(ChangedRows_UpdateOnlyTheDataViewsThatReadThem)
	{
		using namespace ArcRmlUpdateCostTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("DataList.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		UpdateRows(false);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));

		const int64 Updated = Count(Rml::ArcPerf::Counter::DataViewsUpdated);
		const int64 RowViews = int64(ChangedRows) * DataViewsPerChangedRow;
		TestRunner->AddInfo(FString::Printf(TEXT("data views updated %lld for %d changed rows of %d"), Updated, ChangedRows, ListRows));
		ASSERT_THAT(IsTrue(Updated >= RowViews, FString::Printf(TEXT("data views updated %lld < %lld: a view of a changed row did not update"), Updated, RowViews)));
		ASSERT_THAT(IsTrue(Updated <= RowViews + WholeListDataViews,
			FString::Printf(TEXT("data views updated %lld > %lld: views of unchanged rows updated"), Updated, RowViews + WholeListDataViews)));
	}

	TEST_METHOD(HoverMovesSlotToSlot_RestylesOnlyTheTwoSlots)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("SlotGrid.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* From = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("s44"));
		const FArcRmlTestLayoutElement* To = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("s45"));
		ASSERT_THAT(IsNotNull(From, TEXT("s44")));
		ASSERT_THAT(IsNotNull(To, TEXT("s45")));
		View.MouseMove(From->Rect.Position + From->Rect.Size * 0.5f);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));

		View.MouseMove(To->Rect.Position + To->Rect.Size * 0.5f);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		TestRunner->AddInfo(DescribeCounters());

		ASSERT_THAT(IsTrue(Count(Rml::ArcPerf::Counter::PseudoClassChanges) > 0, TEXT("the move changed no :hover")));
		ASSERT_THAT(IsTrue(Count(Rml::ArcPerf::Counter::ElementsRendered) > 0, TEXT("the :hover change rendered no frame")));
		ExpectAtMost(Rml::ArcPerf::Counter::DefinitionLookups, 2, TEXT("definition lookups"));
		ExpectAtMost(Rml::ArcPerf::Counter::ElementsUpdated, 12, TEXT("elements updated"));
	}

	bool WaitForFrameAfter(uint64 FrameBefore)
	{
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= FrameBefore; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > FrameBefore;
	}

	int32 CountTextGlyphs() const
	{
		return ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), FColor(0xff, 0x00, 0x80, 0xff)).Num();
	}

	TEST_METHOD(EllipsisParentWidenedAndNarrowed_TextFollowsTheWidth)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("Ellipsis.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const int32 NarrowGlyphs = CountTextGlyphs();
		ASSERT_THAT(IsTrue(NarrowGlyphs > 0, TEXT("the narrow box draws no glyph")));

		uint64 FrameBefore = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("wide"), true);
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore)));
		const int32 WideGlyphs = CountTextGlyphs();
		ASSERT_THAT(IsTrue(WideGlyphs > NarrowGlyphs + 10, FString::Printf(TEXT("widened box draws %d glyphs, narrow %d: the ellipsis stayed"), WideGlyphs, NarrowGlyphs)));

		FrameBefore = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("wide"), false);
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore)));
		ASSERT_THAT(AreEqual(NarrowGlyphs, CountTextGlyphs(), TEXT("narrowed box does not draw the ellipsized text again")));
	}

	TEST_METHOD(KeyUpAndWheelWithoutEffect_RunNoViewFrame_TabMovesFocusAndRenders)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("InputNoop.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Plain = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("plain"));
		ASSERT_THAT(IsNotNull(Plain, TEXT("plain")));
		const FIntPoint PlainPoint(FMath::FloorToInt(Plain->Rect.Position.X + 10.0f), FMath::FloorToInt(Plain->Rect.Position.Y + 10.0f));
		View.MouseMove(FVector2f(PlainPoint));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;

		View.RawKeyUp(EKeys::A);
		FArcRmlInputEvent Wheel;
		Wheel.Type = EArcRmlInputType::MouseWheel;
		Wheel.Position = PlainPoint;
		Wheel.WheelDelta = -1.0f;
		View.RawInput(MoveTemp(Wheel));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(AreEqual(FrameBefore, View.GetProbe().FrameSerial, FString::Printf(TEXT("key up and wheel rendered frames; %s"), *DescribeCounters())));
		ASSERT_THAT(AreEqual(int64(0), Count(Rml::ArcPerf::Counter::ElementsRendered), DescribeCounters()));

		View.PressKey(EKeys::Tab);
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the focus move rendered no frame")));
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::GetFocusDomId(View).IsEmpty(), TEXT("Tab focused nothing")));
	}

	TEST_METHOD(HoverSelectorsThroughCombinatorsAndPseudoElement_Restyle)
	{
		using namespace ArcRmlUpdateCostTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("HoverRoutes.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TCHAR* const Hovered[] = {TEXT("h1"), TEXT("h2"), TEXT("h3"), TEXT("h4")};
		const TCHAR* const Targets[] = {TEXT("x1"), TEXT("x2"), TEXT("x3"), TEXT("x4")};
		for (int32 Index = 0; Index < 4; ++Index)
		{
			TArray<FArcRmlTestLayoutElement> Elements;
			ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
			const FArcRmlTestLayoutElement* Host = ArcRmlUITestHelpers::FindByDomId(Elements, Hovered[Index]);
			const FArcRmlTestLayoutElement* Target = ArcRmlUITestHelpers::FindByDomId(Elements, Targets[Index]);
			ASSERT_THAT(IsNotNull(Host, Hovered[Index]));
			ASSERT_THAT(IsNotNull(Target, Targets[Index]));
			ASSERT_THAT(AreEqual(FString(TEXT("#000000ff")), Target->BackgroundColor, FString::Printf(TEXT("%s before hover"), Targets[Index])));

			View.MouseMove(FVector2f(Host->Rect.Position.X + 150.0f, Host->Rect.Position.Y + 2.0f));
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
			ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
			Target = ArcRmlUITestHelpers::FindByDomId(Elements, Targets[Index]);
			ASSERT_THAT(IsNotNull(Target, Targets[Index]));
			ASSERT_THAT(AreEqual(FString(TEXT("#00ff00ff")), Target->BackgroundColor, FString::Printf(TEXT("%s while %s is hovered"), Targets[Index], Hovered[Index])));

			View.MouseMove(FVector2f(1200.0f, 700.0f));
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		}

		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Before = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("h5"));
		ASSERT_THAT(IsNotNull(Before, TEXT("h5")));
		const float HeightBefore = Before->Rect.Size.Y;
		View.MouseMove(FVector2f(Before->Rect.Position.X + 150.0f, Before->Rect.Position.Y + 2.0f));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* After = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("h5"));
		ASSERT_THAT(IsNotNull(After, TEXT("h5")));
		ASSERT_THAT(IsTrue(After->Rect.Size.Y > HeightBefore + 5.0f,
			FString::Printf(TEXT("h5 height %.1f while hovered, %.1f before: ::before did not appear"), After->Rect.Size.Y, HeightBefore)));
	}

	TEST_METHOD(CursorChangeWithoutHoverRule_RendersAFrame)
	{
		using namespace ArcRmlUpdateCostTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("HoverRoutes.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Pointer = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("pointer"));
		ASSERT_THAT(IsNotNull(Pointer, TEXT("pointer")));
		View.MouseMove(FVector2f(1200.0f, 700.0f));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;

		View.MouseMove(GetCentre(*Pointer));
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the cursor change rendered no frame")));
		ASSERT_THAT(AreEqual(FString(TEXT("pointer")), View.GetSnapshot().Cursor, TEXT("published cursor")));
	}
};
