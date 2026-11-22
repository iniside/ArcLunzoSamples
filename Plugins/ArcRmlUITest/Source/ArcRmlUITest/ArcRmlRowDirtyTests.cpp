// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlRowDirtyTestsPrivate
{
	FArcRmlValue MakeRow(const FString& Id, const FString& Name, int32 Value, const FString& Tag, bool bShow, const FString& Label)
	{
		FArcRmlValue Inner;
		Inner.Kind = EArcRmlValueKind::Struct;
		Inner.MemberNames = {TEXT("Label")};
		Inner.Children.Add(ArcRmlUITestHelpers::MakeString(Label));

		FArcRmlValue Row;
		Row.Kind = EArcRmlValueKind::Struct;
		Row.MemberNames = {TEXT("Id"), TEXT("Name"), TEXT("Value"), TEXT("Tag"), TEXT("Show"), TEXT("Inner")};
		Row.Children.Add(ArcRmlUITestHelpers::MakeString(Id));
		Row.Children.Add(ArcRmlUITestHelpers::MakeString(Name));
		Row.Children.Add(ArcRmlUITestHelpers::MakeInt(Value));
		Row.Children.Add(ArcRmlUITestHelpers::MakeString(Tag));
		Row.Children.Add(ArcRmlUITestHelpers::MakeBool(bShow));
		Row.Children.Add(MoveTemp(Inner));
		return Row;
	}

	FArcRmlListOp MakeUpdate(int32 Index, FArcRmlValue&& Value)
	{
		FArcRmlListOp Op;
		Op.Type = EArcRmlListOpType::Update;
		Op.Index = Index;
		Op.Value = MoveTemp(Value);
		return Op;
	}

	bool HasClass(const FArcRmlTestLayoutElement& Element, const FString& Class)
	{
		TArray<FString> Classes;
		Element.Classes.ParseIntoArrayWS(Classes);
		return Classes.Contains(Class);
	}
}

TEST_CLASS(ArcRmlRowDirtyTests, "ArcRmlUI.RowDirty")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("RowDirty/RowDirty.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	bool Settle()
	{
		return View.FlushUntilSettled() && View.ParseLayout(Elements);
	}

	FString Text(const FString& DomId) const
	{
		return ArcRmlUITestHelpers::GetText(Elements, DomId);
	}

	void ExpectText(const FString& DomId, const TCHAR* Expected)
	{
		ASSERT_THAT(AreEqual(FString(Expected), Text(DomId), FString::Printf(TEXT("text of #%s"), *DomId)));
	}

	TEST_METHOD(RowUpdateOps_UpdateEveryViewThatReadsTheRow)
	{
		using namespace ArcRmlRowDirtyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		ExpectText(TEXT("name2"), TEXT("Row C"));
		ExpectText(TEXT("dyn"), TEXT("Row C"));
		ExpectText(TEXT("fixed"), TEXT("Row D"));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if2")), TEXT("#if2 before the update")));
		ASSERT_THAT(IsNotNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tagt2")), TEXT("#tagt2 before the update")));

		TArray<FArcRmlListOp> Ops;
		Ops.Add(MakeUpdate(2, MakeRow(TEXT("c"), TEXT("New C"), 60, TEXT("u2"), true, TEXT("Deep C"))));
		Ops.Add(MakeUpdate(3, MakeRow(TEXT("d"), TEXT("New D"), 40, TEXT("t3"), false, TEXT("In D"))));
		View.ApplyListOps(TEXT("rows"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(Settle()));

		ExpectText(TEXT("name2"), TEXT("New C"));
		ExpectText(TEXT("alias2"), TEXT("New C"));
		ExpectText(TEXT("inner2"), TEXT("Deep C"));
		ExpectText(TEXT("comp2"), TEXT("New C"));
		ExpectText(TEXT("key2"), TEXT("New C"));
		ExpectText(TEXT("dyn"), TEXT("New C"));
		ExpectText(TEXT("fixed"), TEXT("New D"));
		ExpectText(TEXT("name3"), TEXT("New D"));
		ExpectText(TEXT("name1"), TEXT("Row B"));
		ExpectText(TEXT("count"), TEXT("5"));

		const FArcRmlTestLayoutElement* Class = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("cls2"));
		ASSERT_THAT(IsNotNull(Class, TEXT("#cls2")));
		ASSERT_THAT(IsTrue(HasClass(*Class, TEXT("hot")), FString::Printf(TEXT("#cls2 classes '%s'"), *Class->Classes)));
		const FArcRmlTestLayoutElement* Style = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sty2"));
		ASSERT_THAT(IsNotNull(Style, TEXT("#sty2")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Style->Rect.Size.X, 60.0f, 0.5f), FString::Printf(TEXT("#sty2 width %.1f"), Style->Rect.Size.X)));
		ASSERT_THAT(IsNotNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tagu2")), TEXT("data-attr-id did not follow the row")));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tagt2")), TEXT("the old #tagt2 stayed")));
		ASSERT_THAT(IsNotNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if2")), TEXT("data-if did not follow the row")));
	}

	TEST_METHOD(NestedListRowUpdate_UpdatesThatRowOnly)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		ExpectText(TEXT("group1"), TEXT("G1"));

		FArcRmlValue Row;
		Row.Kind = EArcRmlValueKind::Struct;
		Row.MemberNames = {TEXT("Name")};
		Row.Children.Add(ArcRmlUITestHelpers::MakeString(TEXT("G1 new")));
		TArray<FArcRmlListOp> Ops;
		Ops.Add(ArcRmlRowDirtyTestsPrivate::MakeUpdate(1, MoveTemp(Row)));
		TArray<FArcRmlPathEntry> Path = ArcRmlUITestHelpers::MakeFieldPath(TEXT("Group"));
		Path.Add(FArcRmlPathEntry{TEXT("Rows"), INDEX_NONE});
		View.ApplyListOps(TEXT("rows"), MoveTemp(Path), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(Settle()));

		ExpectText(TEXT("group1"), TEXT("G1 new"));
		ExpectText(TEXT("group0"), TEXT("G0"));
		ExpectText(TEXT("group2"), TEXT("G2"));
		ExpectText(TEXT("name1"), TEXT("Row B"));
	}

	TEST_METHOD(InsertRemoveMoveThenRowUpdates_FollowTheRows)
	{
		using namespace ArcRmlRowDirtyTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		TArray<FArcRmlListOp> Ops;
		FArcRmlListOp& Move = Ops.AddDefaulted_GetRef();
		Move.Type = EArcRmlListOpType::Move;
		Move.Index = 0;
		Move.ToIndex = 4;
		FArcRmlListOp& Insert = Ops.AddDefaulted_GetRef();
		Insert.Type = EArcRmlListOpType::Insert;
		Insert.Index = 0;
		Insert.Value = MakeRow(TEXT("f"), TEXT("Row F"), 5, TEXT("t5"), false, TEXT("In F"));
		FArcRmlListOp& Remove = Ops.AddDefaulted_GetRef();
		Remove.Type = EArcRmlListOpType::Remove;
		Remove.Index = 2;
		Ops.Add(MakeUpdate(1, MakeRow(TEXT("b"), TEXT("New B"), 20, TEXT("t1"), false, TEXT("In B"))));
		View.ApplyListOps(TEXT("rows"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(Settle()));

		const TCHAR* const Expected[] = {TEXT("Row F"), TEXT("New B"), TEXT("Row D"), TEXT("Row E"), TEXT("Row A")};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Expected); ++Index)
		{
			ExpectText(FString::Printf(TEXT("name%d"), Index), Expected[Index]);
			ExpectText(FString::Printf(TEXT("key%d"), Index), Expected[Index]);
			ExpectText(FString::Printf(TEXT("comp%d"), Index), Expected[Index]);
		}
		ExpectText(TEXT("count"), TEXT("5"));
		ExpectText(TEXT("dyn"), TEXT("Row D"));
		ExpectText(TEXT("fixed"), TEXT("Row E"));

		Ops.Reset();
		Ops.Add(MakeUpdate(2, MakeRow(TEXT("d"), TEXT("New D"), 40, TEXT("t3"), false, TEXT("In D"))));
		Ops.Add(MakeUpdate(4, MakeRow(TEXT("a"), TEXT("New A"), 10, TEXT("t0"), false, TEXT("In A"))));
		View.ApplyListOps(TEXT("rows"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(Settle()));

		ExpectText(TEXT("key2"), TEXT("New D"));
		ExpectText(TEXT("key4"), TEXT("New A"));
		ExpectText(TEXT("name2"), TEXT("New D"));
		ExpectText(TEXT("alias4"), TEXT("New A"));
		ExpectText(TEXT("comp4"), TEXT("New A"));
		ExpectText(TEXT("dyn"), TEXT("New D"));
		ExpectText(TEXT("key3"), TEXT("Row E"));
	}
};
