// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlModelNameCaseTestsPrivate
{
	const TCHAR* const DocumentPath = TEXT("Names/NameCaseTest.rml");
	const TCHAR* const ModelName = TEXT("names");

	FArcRmlValue MakeRow(int64 Id, const FString& Label)
	{
		FArcRmlValue Row;
		Row.Kind = EArcRmlValueKind::Struct;
		Row.MemberNames.Add(TEXT("ID"));
		Row.Children.Add(ArcRmlUITestHelpers::MakeInt(Id));
		Row.MemberNames.Add(TEXT("LABEL"));
		Row.Children.Add(ArcRmlUITestHelpers::MakeString(Label));
		return Row;
	}
}

TEST_CLASS(ArcRmlModelNameCaseTests, "ArcRmlUI.ModelNameCase")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	BEFORE_EACH()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(ArcRmlModelNameCaseTestsPrivate::DocumentPath);
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument,
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(FieldNameInOtherCase_Binds)
	{
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		ASSERT_THAT(AreEqual(FString(TEXT("plain")), ArcRmlUITestHelpers::GetText(Elements, TEXT("title")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(RowMemberNamesInOtherCase_KeyAndBindRows)
	{
		using namespace ArcRmlModelNameCaseTestsPrivate;
		FArcRmlValue Rows;
		Rows.Kind = EArcRmlValueKind::Array;
		Rows.Children.Add(MakeRow(7, TEXT("seven")));
		Rows.Children.Add(MakeRow(9, TEXT("nine")));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("rows")), MoveTemp(Rows));
		ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
		ASSERT_THAT(AreEqual(FString(TEXT("seven")), ArcRmlUITestHelpers::GetText(Elements, TEXT("row-7")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("nine")), ArcRmlUITestHelpers::GetText(Elements, TEXT("row-9")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}
};
