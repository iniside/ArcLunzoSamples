// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlInvalidationTestsPrivate
{
	const FString White(TEXT("#ffffffff"));
	const FString Blue(TEXT("#0000ffff"));
	const FString Red(TEXT("#ff0000ff"));
	const FString Green(TEXT("#00ff00ff"));
	const FString Magenta(TEXT("#ff00ffff"));
	const FString Yellow(TEXT("#ffff00ff"));
	const FString Cyan(TEXT("#00ffffff"));
	const FColor CloneColour(20, 180, 220, 255);

	bool HasClass(const FArcRmlTestLayoutElement& Element, const FString& Class)
	{
		TArray<FString> Classes;
		Element.Classes.ParseIntoArrayWS(Classes);
		return Classes.Contains(Class);
	}

	int32 CountBoxDraws(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
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

TEST_CLASS(ArcRmlInvalidationTests, "ArcRmlUI.Invalidation")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool Open(const TCHAR* RelativePath)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ParseLayout(Elements);
	}

	bool Read()
	{
		return View.Flush() && View.Flush() && View.ParseLayout(Elements);
	}

	bool SetFlag(const TCHAR* ModelName, const TCHAR* Field, bool bValue)
	{
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeBool(bValue));
		return Read();
	}

	bool SetText(const TCHAR* ModelName, const TCHAR* Field, const TCHAR* Value)
	{
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeString(Value));
		return Read();
	}

	void ExpectColour(const TCHAR* Moment, const TCHAR* DomId, const FString& Expected)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element, FString::Printf(TEXT("%s: no element %s"), Moment, DomId)));
		ASSERT_THAT(AreEqual(Expected, Element->Color, FString::Printf(TEXT("%s: color of %s"), Moment, DomId)));
	}

	void ExpectSlot(const TCHAR* Moment, const TCHAR* Rarity, bool bSelected)
	{
		const FArcRmlTestLayoutElement* Slot = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("slot"));
		ASSERT_THAT(IsNotNull(Slot, Moment));
		const FString Detail = FString::Printf(TEXT("%s: classes '%s'"), Moment, *Slot->Classes);
		ASSERT_THAT(IsTrue(ArcRmlInvalidationTestsPrivate::HasClass(*Slot, TEXT("slot")), Detail));
		ASSERT_THAT(IsTrue(ArcRmlInvalidationTestsPrivate::HasClass(*Slot, Rarity), Detail));
		ASSERT_THAT(AreEqual(bSelected, ArcRmlInvalidationTestsPrivate::HasClass(*Slot, TEXT("selected")), Detail));
	}

	void StartDrag()
	{
		const FArcRmlTestLayoutElement* Source = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("src"));
		check(Source);
		const FVector2f Grab(Source->Rect.Position.X + Source->Rect.Size.X * 0.5f - 20.0f, Source->Rect.Position.Y + Source->Rect.Size.Y * 0.5f - 10.0f);
		View.MouseMove(Grab);
		View.MouseDown(Grab);
		View.MouseMove(Grab + FVector2f(6.0f, 4.0f));
	}

	TEST_METHOD(AttributeOnBody_RestylesDescendantRuleSubjectsOnly)
	{
		using namespace ArcRmlInvalidationTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Invalidation/Invalidation.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("baseline"), TEXT("t1"), Blue);
		ExpectColour(TEXT("baseline"), TEXT("t2"), Blue);
		ExpectColour(TEXT("baseline"), TEXT("u1"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasX"), true)));
		ExpectColour(TEXT("data-x set"), TEXT("t1"), Red);
		ExpectColour(TEXT("data-x set"), TEXT("t2"), Red);
		ExpectColour(TEXT("data-x set"), TEXT("u1"), White);
		ExpectColour(TEXT("data-x set"), TEXT("c2"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasX"), false)));
		ExpectColour(TEXT("data-x removed"), TEXT("t1"), Blue);
		ExpectColour(TEXT("data-x removed"), TEXT("t2"), Blue);
	}

	TEST_METHOD(ClassOnAncestor_RestylesChildCombinatorSubjectUnderIt)
	{
		using namespace ArcRmlInvalidationTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Invalidation/Invalidation.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("baseline"), TEXT("c1"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasA"), true)));
		ExpectColour(TEXT("a set"), TEXT("c1"), Green);
		ExpectColour(TEXT("a set"), TEXT("c2"), White);
		ExpectColour(TEXT("a set"), TEXT("t1"), Blue);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasA"), false)));
		ExpectColour(TEXT("a removed"), TEXT("c1"), White);
		ExpectColour(TEXT("a removed"), TEXT("c2"), White);
	}

	TEST_METHOD(ClassOfCompoundSubject_RestylesOnlyThatElement)
	{
		using namespace ArcRmlInvalidationTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Invalidation/Invalidation.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("baseline"), TEXT("pq"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasP"), true)));
		ExpectColour(TEXT("p set"), TEXT("pq"), Magenta);
		ExpectColour(TEXT("p set"), TEXT("q2"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasP"), false)));
		ExpectColour(TEXT("p removed"), TEXT("pq"), White);
	}

	TEST_METHOD(ClassOnFirstSibling_RestylesTheAdjacentSiblingOnly)
	{
		using namespace ArcRmlInvalidationTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Invalidation/Invalidation.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("baseline"), TEXT("sb"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasS"), true)));
		ExpectColour(TEXT("on set"), TEXT("sb"), Yellow);
		ExpectColour(TEXT("on set"), TEXT("sc"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasS"), false)));
		ExpectColour(TEXT("on removed"), TEXT("sb"), White);
	}

	TEST_METHOD(DataClassToggle_RestylesClassAttributePrefixRule)
	{
		using namespace ArcRmlInvalidationTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Invalidation/Invalidation.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("baseline"), TEXT("ic"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasIcon"), true)));
		ExpectColour(TEXT("icon set"), TEXT("ic"), Cyan);
		ExpectColour(TEXT("icon set"), TEXT("u1"), White);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("inv"), TEXT("HasIcon"), false)));
		ExpectColour(TEXT("icon removed"), TEXT("ic"), White);
	}

	TEST_METHOD(ClassBinding_KeepsDataClassWhenTheAttributeDataResends)
	{
		ASSERT_THAT(IsTrue(Open(TEXT("Invalidation/Slot.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSlot(TEXT("opened"), TEXT("rare"), false);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("slot"), TEXT("bSelected"), true)));
		ExpectSlot(TEXT("selected"), TEXT("rare"), true);

		ASSERT_THAT(IsTrue(SetText(TEXT("slot"), TEXT("Rarity"), TEXT("rare"))));
		ExpectSlot(TEXT("rarity resent"), TEXT("rare"), true);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("slot"), TEXT("bSelected"), false)));
		ExpectSlot(TEXT("deselected"), TEXT("rare"), false);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("slot"), TEXT("bSelected"), true)));
		ExpectSlot(TEXT("selected again"), TEXT("rare"), true);

		ASSERT_THAT(IsTrue(SetText(TEXT("slot"), TEXT("Rarity"), TEXT("epic"))));
		const FArcRmlTestLayoutElement* Slot = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("slot"));
		ASSERT_THAT(IsNotNull(Slot));
		ASSERT_THAT(IsTrue(ArcRmlInvalidationTestsPrivate::HasClass(*Slot, TEXT("epic")), Slot->Classes));
		ASSERT_THAT(IsFalse(ArcRmlInvalidationTestsPrivate::HasClass(*Slot, TEXT("rare")), Slot->Classes));
	}

	TEST_METHOD(CloneOfSourceWithDataClass_MatchesClassAttributeRule)
	{
		using namespace ArcRmlInvalidationTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Invalidation/CloneClass.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, CountBoxDraws(View.GetDraws(), CloneColour), TEXT("the source is drawn with the drag colour before the drag")));

		StartDrag();
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(1, CountBoxDraws(View.GetDraws(), CloneColour), TEXT("the clone does not match the class attribute rule")));
	}

	TEST_METHOD(CloneOfSourceAfterDataClassRemoved_DoesNotMatchClassAttributeRule)
	{
		using namespace ArcRmlInvalidationTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Invalidation/CloneClass.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetFlag(TEXT("cc"), TEXT("HasIcon"), false)));

		StartDrag();
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, CountBoxDraws(View.GetDraws(), CloneColour), TEXT("the clone kept the removed class in its class attribute")));
	}
};
