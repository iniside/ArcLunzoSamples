// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

#include "RmlUi/Core/ComputedValues.h"
#include "RmlUi/Core/Element.h"
#include "RmlUi/Core/ElementDocument.h"

namespace ArcRmlDataBindingTestsPrivate
{
	constexpr float Tolerance = 0.5f;
	const TCHAR* const ModelName = TEXT("bind");

	FArcRmlValue MakeFloat(double Number)
	{
		FArcRmlValue Value;
		Value.Kind = EArcRmlValueKind::Float;
		Value.Float = Number;
		return Value;
	}

	TArray<FArcRmlPathEntry> MakeAbilityPath(int32 Row, const FString& Field)
	{
		TArray<FArcRmlPathEntry> Path = ArcRmlUITestHelpers::MakeRowFieldPath(TEXT("Slots"), Row, TEXT("Ability"));
		Path.Add(FArcRmlPathEntry{Field, INDEX_NONE});
		return Path;
	}

	FString DescribeRect(const FArcRmlTestLayoutElement* Element)
	{
		return Element ? FString::Printf(TEXT("%s (%.1f, %.1f) %.1f x %.1f"), *Element->DomId, Element->Rect.Position.X, Element->Rect.Position.Y,
			Element->Rect.Size.X, Element->Rect.Size.Y) : FString(TEXT("missing"));
	}

	FString GetInner(Rml::Element& Element)
	{
		return FString(UTF8_TO_TCHAR(Element.GetInnerRML().c_str())).TrimStartAndEnd();
	}

	FString DescribeSlots(Rml::ElementDocument& Document)
	{
		Rml::Element* Title = Document.GetElementById("title");
		TArray<FString> Parts;
		Parts.Add(Title ? GetInner(*Title) : FString(TEXT("no title")));
		Rml::ElementList Rows;
		Document.QuerySelectorAll(Rows, ".slot");
		for (Rml::Element* Row : Rows)
		{
			if (Row->GetComputedValues().display() == Rml::Style::Display::None)
			{
				continue;
			}
			Parts.Add(GetInner(*Row) + (Row->IsClassSet("ready") ? TEXT(" ready") : TEXT("")));
		}
		return FString::Join(Parts, TEXT(" | "));
	}

	FString DescribeCalls(const FArcRmlTestView& View)
	{
		TArray<FString> Parts;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			TArray<FString> Arguments;
			for (const FArcRmlValue& Argument : Call.Arguments)
			{
				Arguments.Add(Argument.Kind == EArcRmlValueKind::Int ? FString::Printf(TEXT("%lld"), Argument.Int)
					: Argument.Kind == EArcRmlValueKind::Float ? FString::Printf(TEXT("%g"), Argument.Float) : Argument.String);
			}
			Parts.Add(FString::Printf(TEXT("%s(%s)"), *Call.FunctionName, *FString::Join(Arguments, TEXT(", "))));
		}
		return FString::Join(Parts, TEXT("; "));
	}
}

TEST_CLASS(ArcRmlDataBindingTests, "ArcRmlUI.DataBinding")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool Open()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Binding/BindingTest.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ParseLayout(Elements);
	}

	bool Read()
	{
		return View.Flush() && View.Flush() && View.ParseLayout(Elements);
	}

	const FArcRmlTestLayoutElement* Find(const TCHAR* DomId) const
	{
		return ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
	}

	int32 CountVisible(const TCHAR* DomId) const
	{
		int32 Count = 0;
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			Count += Element.DomId == DomId && Element.bVisible ? 1 : 0;
		}
		return Count;
	}

	FString ReadSlots()
	{
		TSharedRef<FString> State = MakeShared<FString>();
		if (!View.RunOnDocument([State](Rml::ElementDocument& Document) { *State = ArcRmlDataBindingTestsPrivate::DescribeSlots(Document); }))
		{
			return TEXT("the document did not answer");
		}
		return *State;
	}

	FString DescribeItemMismatch(const TArray<const TCHAR*>& Ids) const
	{
		using namespace ArcRmlDataBindingTestsPrivate;
		const FArcRmlTestLayoutElement* Row = Find(TEXT("crow"));
		if (!Row)
		{
			return TEXT("the row is missing");
		}
		TArray<FString> Mismatches;
		for (int32 Index = 0; Index < Ids.Num(); ++Index)
		{
			const FArcRmlTestLayoutElement* Item = Find(Ids[Index]);
			const bool bPlaced = Item && FMath::IsNearlyEqual(Item->Rect.Position.X, Row->Rect.Position.X + 48.0f * Index, Tolerance)
				&& FMath::IsNearlyEqual(Item->Rect.Position.Y, Row->Rect.Position.Y, Tolerance)
				&& FMath::IsNearlyEqual(Item->Rect.Size.X, 40.0f, Tolerance) && FMath::IsNearlyEqual(Item->Rect.Size.Y, 24.0f, Tolerance);
			if (!bPlaced)
			{
				Mismatches.Add(FString::Printf(TEXT("%s expected at +%d: %s"), Ids[Index], 48 * Index, *DescribeRect(Item)));
			}
		}
		if (!FMath::IsNearlyEqual(Row->Rect.Size.Y, 24.0f, Tolerance))
		{
			Mismatches.Add(DescribeRect(Row));
		}
		return Mismatches.IsEmpty() ? FString() : FString::Printf(TEXT("row %s: %s"), *DescribeRect(Row), *FString::Join(Mismatches, TEXT("; ")));
	}

	TEST_METHOD(DataStyleWidth_FollowsTheModelValue)
	{
		using namespace ArcRmlDataBindingTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Fill = Find(TEXT("fill"));
		ASSERT_THAT(IsTrue(Fill && FMath::IsNearlyEqual(Fill->Rect.Size.X, 160.0f, Tolerance), FString::Printf(TEXT("40%% of the 400px bar: %s"), *DescribeRect(Fill))));

		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("Normalized")), MakeFloat(0.75));
		ASSERT_THAT(IsTrue(Read()));
		Fill = Find(TEXT("fill"));
		ASSERT_THAT(IsTrue(Fill && FMath::IsNearlyEqual(Fill->Rect.Size.X, 300.0f, Tolerance), FString::Printf(TEXT("75%% of the 400px bar: %s"), *DescribeRect(Fill))));
	}

	TEST_METHOD(DataIfOnDisplayContents_ChildrenAreFlexItemsOfTheGrandparent)
	{
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeItemMismatch({TEXT("c0"), TEXT("c1"), TEXT("c2"), TEXT("c3")})));

		View.SetModelValue(ArcRmlDataBindingTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("Value")), ArcRmlUITestHelpers::MakeInt(25));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, CountVisible(TEXT("c1")) + CountVisible(TEXT("c2")), TEXT("the children of the false data-if are still shown")));
		ASSERT_THAT(AreEqual(FString(), DescribeItemMismatch({TEXT("c0"), TEXT("c3")})));

		View.SetModelValue(ArcRmlDataBindingTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("Value")), ArcRmlUITestHelpers::MakeInt(80));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(FString(), DescribeItemMismatch({TEXT("c0"), TEXT("c1"), TEXT("c2"), TEXT("c3")})));
	}

	TEST_METHOD(CallOnMouseDownAndMouseUp_CallsEachWithItsArgument)
	{
		using namespace ArcRmlDataBindingTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Press = Find(TEXT("press"));
		ASSERT_THAT(IsNotNull(Press));
		const FVector2f Centre(Press->Rect.Position.X + Press->Rect.Size.X * 0.5f, Press->Rect.Position.Y + Press->Rect.Size.Y * 0.5f);
		View.MouseMove(Centre);
		ASSERT_THAT(IsTrue(Read()));
		View.ClearCalls();

		View.MouseDown(Centre);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("SetValue(25)")), DescribeCalls(View)));

		View.MouseUp(Centre);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("SetValue(25); SetValue(75)")), DescribeCalls(View)));
	}

	TEST_METHOD(DataForOverNestedStructFields_ShowsMembersAndSize)
	{
		using namespace ArcRmlDataBindingTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("QB1 (2 slots) | 0 S0 0.25 ready | 1 S1 0.50")), ReadSlots()));

		View.SetModelValue(ModelName, MakeAbilityPath(1, TEXT("Cooldown")), MakeFloat(0.75));
		View.SetModelValue(ModelName, MakeAbilityPath(1, TEXT("bHasAbility")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("QB1 (2 slots) | 0 S0 0.25 ready | 1 S1 0.75 ready")), ReadSlots()));

		TArray<FArcRmlListOp> Ops;
		FArcRmlListOp& Remove = Ops.AddDefaulted_GetRef();
		Remove.Type = EArcRmlListOpType::Remove;
		Remove.Index = 0;
		View.ApplyListOps(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("Slots")), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(View.Flush() && View.FlushUntilSettled()));
		ASSERT_THAT(AreEqual(FString(TEXT("QB1 (1 slots) | 0 S1 0.75 ready")), ReadSlots()));
	}
};
