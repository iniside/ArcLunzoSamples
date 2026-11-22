// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Host/SArcRmlInspector.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ArcRmlInspectorTestsPrivate
{
	const TCHAR* const DocumentPath = TEXT("Inspector/InspectorTest.rml");
	const TCHAR* const ModelName = TEXT("insp");

	const FArcRmlInspectorSnapshot* FindLastSnapshot(const FArcRmlTestView& View)
	{
		const TArray<FArcRmlInspectorAnswer>& Answers = View.GetInspectorAnswers();
		for (int32 Index = Answers.Num() - 1; Index >= 0; --Index)
		{
			if (Answers[Index].Snapshot.IsValid())
			{
				return Answers[Index].Snapshot.Get();
			}
		}
		return nullptr;
	}

	float GetWidth(const FArcRmlTestView& View, const TCHAR* DomId)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		if (!View.ParseLayout(Elements))
		{
			return -1.0f;
		}
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? Element->Rect.Size.X : -1.0f;
	}

	TArray<uint8> ToBytes(const FString& Text)
	{
		const FTCHARToUTF8 Converted(*Text);
		return TArray<uint8>(reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
	}

	FArcRmlInspectorRule MakeRule(const FString& Source, const FString& Selector)
	{
		FArcRmlInspectorRule Rule;
		Rule.Selector = Selector;
		Rule.File = TEXT("/ArcRmlUITest/UI/Rml/Test/Inspector/Copy.rcss");
		Rule.Line = 1;
		Rule.SourceBegin = 0;
		Rule.SourceEnd = ToBytes(Source.TrimEnd()).Num();
		return Rule;
	}

	FArcRmlInspectorProperty MakeProperty(const TCHAR* Name, const TCHAR* Value)
	{
		FArcRmlInspectorProperty Property;
		Property.Name = Name;
		Property.Value = Value;
		return Property;
	}

	FString Copy(const FString& Source, const FArcRmlInspectorRule& Rule, const TArray<FArcRmlInspectorEdit>& Edits,
		const TMap<FString, TArray<FString>>& Shorthands = TMap<FString, TArray<FString>>())
	{
		FString Text;
		return SArcRmlInspector::MakeRuleCopy(ToBytes(Source), Rule, Edits, Shorthands, Text) == EArcRmlRuleCopyResult::Copied ? Text : FString(TEXT("<no rule>"));
	}

	FString GetLastEditPath(const FArcRmlTestView& View)
	{
		const TArray<FArcRmlInspectorAnswer>& Answers = View.GetInspectorAnswers();
		for (int32 Index = Answers.Num() - 1; Index >= 0; --Index)
		{
			if (Answers[Index].Request == EArcRmlInspectorRequest::SetProperty)
			{
				return Answers[Index].Path;
			}
		}
		return FString();
	}

	bool HasInlineProperty(const FArcRmlInspectorSnapshot& Snapshot, const TCHAR* Name)
	{
		return Snapshot.Rules.ContainsByPredicate([Name](const FArcRmlInspectorRule& Rule) -> bool
		{
			return Rule.bInline && Rule.InheritedFrom.IsEmpty() && Rule.Properties.ContainsByPredicate([Name](const FArcRmlInspectorProperty& Property) -> bool
			{
				return Property.Name == Name;
			});
		});
	}

	TArray<const FArcRmlInspectorRule*> FindRules(const FArcRmlInspectorSnapshot& Snapshot, const TCHAR* Selector)
	{
		TArray<const FArcRmlInspectorRule*> Rules;
		for (const FArcRmlInspectorRule& Rule : Snapshot.Rules)
		{
			if (Rule.Selector == Selector && Rule.InheritedFrom.IsEmpty())
			{
				Rules.Add(&Rule);
			}
		}
		return Rules;
	}

	bool HasProperty(const FArcRmlInspectorRule& Rule, const TCHAR* Name)
	{
		return Rule.Properties.ContainsByPredicate([Name](const FArcRmlInspectorProperty& Property) -> bool
		{
			return Property.Name == Name;
		});
	}
}

TEST_CLASS(ArcRmlInspectorTests, "ArcRmlUI.Inspector")
{
	FArcRmlTestView View;

	BEFORE_EACH()
	{
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(ArcRmlInspectorTestsPrivate::DocumentPath))));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(PausedView_AnswersInspectorRequests)
	{
		View.SetPaused(true);
		ASSERT_THAT(IsTrue(View.Flush()));
		View.ClearInspectorAnswers();
		View.SetInspectorActive(true);
		View.InspectorSnapshot();
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = ArcRmlInspectorTestsPrivate::FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot, TEXT("a paused view did not answer the inspector")));
		ASSERT_THAT(IsTrue(Snapshot->Nodes.Num() > 0));
	}

	TEST_METHOD(Styles_ListInheritedProperties)
	{
		View.InspectEdit(TEXT("#child"), FString(), FString());
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = ArcRmlInspectorTestsPrivate::FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot));
		ASSERT_THAT(IsTrue(Snapshot->Nodes.IsValidIndex(Snapshot->Selected)));
		ASSERT_THAT(AreEqual(FString(TEXT("child")), Snapshot->Nodes[Snapshot->Selected].Id));

		const FArcRmlInspectorRule* Inherited = Snapshot->Rules.FindByPredicate([](const FArcRmlInspectorRule& Rule) -> bool
		{
			return Rule.InheritedFrom.Contains(TEXT("#parent"));
		});
		ASSERT_THAT(IsNotNull(Inherited, TEXT("no rule is listed as inherited from #parent")));
		const auto HasProperty = [Inherited](const TCHAR* Name) -> bool
		{
			return Inherited->Properties.ContainsByPredicate([Name](const FArcRmlInspectorProperty& Property) -> bool
			{
				return Property.Name == Name;
			});
		};
		ASSERT_THAT(IsTrue(HasProperty(TEXT("color"))));
		ASSERT_THAT(IsTrue(HasProperty(TEXT("font-size"))));
		ASSERT_THAT(IsFalse(HasProperty(TEXT("width")), TEXT("width is not inherited")));
	}

	TEST_METHOD(HoverAnswer_CarriesThePathWithoutASnapshot)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Toggled = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("toggled"));
		ASSERT_THAT(IsNotNull(Toggled));
		View.SetInspectorActive(true);
		View.InspectorSnapshot();
		ASSERT_THAT(IsTrue(View.Flush()));
		View.ClearInspectorAnswers();
		View.InspectorAt(Toggled->Rect.Position + Toggled->Rect.Size * 0.5f, false);
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorAnswer* Hover = View.GetInspectorAnswers().FindByPredicate([](const FArcRmlInspectorAnswer& Answer) -> bool
		{
			return Answer.Request == EArcRmlInspectorRequest::Hover;
		});
		ASSERT_THAT(IsNotNull(Hover));
		ASSERT_THAT(IsFalse(Hover->Snapshot.IsValid(), TEXT("a hover answer built a snapshot the panel throws away")));
		ASSERT_THAT(AreEqual(FString(TEXT("0.0.7")), Hover->Path));
	}

	TEST_METHOD(LeavingATreeRow_KeepsThePointerHover)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Toggled = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("toggled"));
		ASSERT_THAT(IsNotNull(Toggled));
		View.SetInspectorActive(true);
		View.InspectorAt(Toggled->Rect.Position + Toggled->Rect.Size * 0.5f, false);
		View.InspectorHover(TEXT("0.0.1"));
		View.InspectorHover(FString());
		View.ClearInspectorAnswers();
		View.InspectorSnapshot();
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = ArcRmlInspectorTestsPrivate::FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot));
		ASSERT_THAT(IsTrue(Snapshot->Nodes.IsValidIndex(Snapshot->Hovered), TEXT("leaving a tree row cleared the pointer hover")));
		ASSERT_THAT(AreEqual(FString(TEXT("toggled")), Snapshot->Nodes[Snapshot->Hovered].Id));
	}

	TEST_METHOD(ClassChange_SendsATreeSnapshot)
	{
		View.SetInspectorActive(true);
		View.InspectorSnapshot();
		ASSERT_THAT(IsTrue(View.Flush()));
		View.ClearInspectorAnswers();
		View.SetRootClass(TEXT("insp-marked"), true);
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Flush()));
		const bool bTreeAnswer = View.GetInspectorAnswers().ContainsByPredicate([](const FArcRmlInspectorAnswer& Answer) -> bool
		{
			return Answer.Serial == 0 && Answer.Snapshot.IsValid();
		});
		ASSERT_THAT(IsTrue(bTreeAnswer, TEXT("a class change sent no tree snapshot, so the tree labels stay stale")));
	}

	TEST_METHOD(Edit_DoesNotMoveToAnotherElementAtItsPath)
	{
		using namespace ArcRmlInspectorTestsPrivate;
		View.InspectEdit(TEXT(".slot"), TEXT("width"), TEXT("33px"));
		ASSERT_THAT(IsTrue(View.Flush()));
		const FString EditPath = GetLastEditPath(View);
		ASSERT_THAT(IsFalse(EditPath.IsEmpty()));
		ASSERT_THAT(IsNear(60.0f, GetWidth(View, TEXT("slot-b")), 0.5f));

		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("showa")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(60.0f, GetWidth(View, TEXT("slot-b")), 0.5f, TEXT("the edit moved to the element that took its path")));
		View.ClearInspectorAnswers();
		View.InspectorSelect(EditPath);
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Placeholder = FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Placeholder));
		ASSERT_THAT(IsTrue(Placeholder->Nodes.IsValidIndex(Placeholder->Selected)));
		ASSERT_THAT(AreEqual(0, Placeholder->Edits.Num(), TEXT("the edit moved to the data-if placeholder at its path")));
		ASSERT_THAT(IsFalse(HasInlineProperty(*Placeholder, TEXT("width")), TEXT("the placeholder at the old path got the edited width")));

		View.SetModelValue(ArcRmlInspectorTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("showa")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Flush()));
		View.ClearInspectorAnswers();
		View.InspectEdit(TEXT(".slot"), FString(), FString());
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = ArcRmlInspectorTestsPrivate::FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot));
		ASSERT_THAT(AreEqual(1, Snapshot->Edits.Num(), TEXT("the edit was not applied again when its element came back")));
		ASSERT_THAT(IsNear(60.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("slot-b")), 0.5f));
	}

	TEST_METHOD(Edit_OfATextlessDataIfElement_SkipsItsPlaceholder)
	{
		using namespace ArcRmlInspectorTestsPrivate;
		View.InspectEdit(TEXT("#bare"), TEXT("width"), TEXT("33px"));
		View.InspectEdit(TEXT("#bare"), TEXT("display"), TEXT("block"));
		ASSERT_THAT(IsTrue(View.Flush()));
		const FString EditPath = GetLastEditPath(View);
		ASSERT_THAT(IsFalse(EditPath.IsEmpty()));
		ASSERT_THAT(IsNear(33.0f, GetWidth(View, TEXT("bare")), 0.5f));

		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("showbare")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Flush()));
		View.ClearInspectorAnswers();
		View.InspectorSelect(EditPath);
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Placeholder = FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Placeholder));
		ASSERT_THAT(IsTrue(Placeholder->Nodes.IsValidIndex(Placeholder->Selected)));
		ASSERT_THAT(IsFalse(Placeholder->Nodes[Placeholder->Selected].bVisible, TEXT("the display edit showed the data-if placeholder")));
		ASSERT_THAT(AreEqual(0, Placeholder->Edits.Num(), TEXT("the edit moved to the data-if placeholder at its path")));
		ASSERT_THAT(IsFalse(HasInlineProperty(*Placeholder, TEXT("width")), TEXT("the placeholder got the edited width")));

		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("showbare")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(33.0f, GetWidth(View, TEXT("bare")), 0.5f, TEXT("the edit was not applied to the shown element again")));
	}

	TEST_METHOD(TurningOff_ClearsSelectionAndEdits)
	{
		View.SetInspectorActive(true);
		View.InspectEdit(TEXT("#parent"), TEXT("width"), TEXT("50px"));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(50.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("parent")), 0.5f));

		View.SetInspectorActive(false);
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(300.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("parent")), 0.5f, TEXT("the edit outlived the inspector")));

		View.ClearInspectorAnswers();
		View.InspectorSnapshot();
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = ArcRmlInspectorTestsPrivate::FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot));
		ASSERT_THAT(AreEqual(static_cast<int32>(INDEX_NONE), Snapshot->Selected, TEXT("the selection outlived the inspector")));
	}

	TEST_METHOD(Edit_SurvivesReinstancing)
	{
		View.InspectEdit(TEXT("#toggled"), TEXT("width"), TEXT("77px"));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(77.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("toggled")), 0.5f));

		View.SetModelValue(ArcRmlInspectorTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("show")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.Flush()));
		View.SetModelValue(ArcRmlInspectorTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("show")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(77.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("toggled")), 0.5f, TEXT("the edit was lost when data-if made the element again")));
	}

	TEST_METHOD(Reset_RestoresTheAuthoredValue)
	{
		ASSERT_THAT(IsNear(10.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("bar")), 0.5f));
		View.InspectEdit(TEXT("#bar"), TEXT("width"), TEXT("50px"));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(50.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("bar")), 0.5f));

		View.SetModelValue(ArcRmlInspectorTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("w")), ArcRmlUITestHelpers::MakeInt(30));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(50.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("bar")), 0.5f, TEXT("data-style-width overwrote the edit")));

		View.InspectorResetEdits();
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsNear(30.0f, ArcRmlInspectorTestsPrivate::GetWidth(View, TEXT("bar")), 0.5f, TEXT("reset did not restore the value the data view set")));
	}

	TEST_METHOD(DomChange_SendsATreeSnapshot)
	{
		View.SetInspectorActive(true);
		View.InspectorSnapshot();
		ASSERT_THAT(IsTrue(View.Flush()));
		View.ClearInspectorAnswers();
		View.SetModelValue(ArcRmlInspectorTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("show")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Flush()));
		const bool bTreeAnswer = View.GetInspectorAnswers().ContainsByPredicate([](const FArcRmlInspectorAnswer& Answer) -> bool
		{
			return Answer.Serial == 0 && Answer.Snapshot.IsValid();
		});
		ASSERT_THAT(IsTrue(bTreeAnswer, TEXT("removing an element sent no tree snapshot")));
	}

	TEST_METHOD(TransformedElement_HasTransformedBoxes)
	{
		View.InspectEdit(TEXT("#rotated"), FString(), FString());
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = ArcRmlInspectorTestsPrivate::FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot));
		const FArcRmlInspectorBoxModel& Box = Snapshot->SelectedBox;
		ASSERT_THAT(IsTrue(Box.bTransformed, TEXT("the rotated element has no transformed boxes")));
		ASSERT_THAT(AreEqual(4, Box.BorderQuad.Num()));
		ASSERT_THAT(IsTrue(FMath::Abs(Box.BorderQuad[0].Y - Box.BorderQuad[1].Y) > 10.0f, TEXT("the top edge of a 45 degree box is level")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(FVector2f::Distance(Box.BorderQuad[0], Box.BorderQuad[1]), 100.0f, 1.0f), TEXT("the top edge is not 100 px long")));
	}

	TEST_METHOD(CopyRule_UsesTheParserRangesOfSameLineRules)
	{
		using namespace ArcRmlInspectorTestsPrivate;
		TArray<uint8> Source;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToArray(Source, *FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Inspector/InspectorTest.rcss")))));
		const TArray<FArcRmlInspectorEdit> NoEdits;
		const TMap<FString, TArray<FString>> NoShorthands;

		View.InspectEdit(TEXT("#twice"), FString(), FString());
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot));
		const TArray<const FArcRmlInspectorRule*> Twice = FindRules(*Snapshot, TEXT("#twice"));
		ASSERT_THAT(AreEqual(2, Twice.Num(), TEXT("the two #twice rules are not listed apart")));
		for (const FArcRmlInspectorRule* Rule : Twice)
		{
			FString Copied;
			ASSERT_THAT(IsTrue(SArcRmlInspector::MakeRuleCopy(Source, *Rule, NoEdits, NoShorthands, Copied) == EArcRmlRuleCopyResult::Copied));
			ASSERT_THAT(AreEqual(FString(HasProperty(*Rule, TEXT("width")) ? TEXT("#twice { width: 10px; }") : TEXT("#twice { height: 20px; }")), Copied));
		}

		View.InspectEdit(TEXT("#noted"), FString(), FString());
		ASSERT_THAT(IsTrue(View.Flush()));
		Snapshot = FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot));
		const TArray<const FArcRmlInspectorRule*> Noted = FindRules(*Snapshot, TEXT("#noted"));
		ASSERT_THAT(AreEqual(1, Noted.Num()));
		FString Copied;
		ASSERT_THAT(IsTrue(SArcRmlInspector::MakeRuleCopy(Source, *Noted[0], NoEdits, NoShorthands, Copied) == EArcRmlRuleCopyResult::Copied));
		ASSERT_THAT(AreEqual(FString(TEXT("#noted, /* x */ #other { width: 30px; }")), Copied));
	}

	TEST_METHOD(CopyRule_SkipsTheBomWrapsMediaAndNamesAnUnterminatedRule)
	{
		using namespace ArcRmlInspectorTestsPrivate;
		TArray<uint8> Source;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToArray(Source, *FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Inspector/BomTest.rcss")))));
		const TArray<FArcRmlInspectorEdit> NoEdits;
		const TMap<FString, TArray<FString>> NoShorthands;
		const auto CopyFirstRule = [this, &Source, &NoEdits, &NoShorthands](const TCHAR* Selector, FString& OutCopied) -> EArcRmlRuleCopyResult
		{
			View.ClearInspectorAnswers();
			View.InspectEdit(Selector, FString(), FString());
			if (!View.Flush())
			{
				return EArcRmlRuleCopyResult::NoRange;
			}
			const FArcRmlInspectorSnapshot* Snapshot = FindLastSnapshot(View);
			const TArray<const FArcRmlInspectorRule*> Rules = Snapshot ? FindRules(*Snapshot, Selector) : TArray<const FArcRmlInspectorRule*>();
			return Rules.IsEmpty() ? EArcRmlRuleCopyResult::NoRange : SArcRmlInspector::MakeRuleCopy(Source, *Rules[0], NoEdits, NoShorthands, OutCopied);
		};

		FString Copied;
		ASSERT_THAT(IsTrue(CopyFirstRule(TEXT("#bom-first"), Copied) == EArcRmlRuleCopyResult::Copied));
		ASSERT_THAT(AreEqual(FString(TEXT("#bom-first { width: 11px; }")), Copied, TEXT("the rule range starts at the BOM")));
		ASSERT_THAT(IsTrue(CopyFirstRule(TEXT("#media-rule"), Copied) == EArcRmlRuleCopyResult::Copied));
		ASSERT_THAT(AreEqual(FString(TEXT("@media (min-width: 1px) {\n\t#media-rule { width: 7px; }\n}")), Copied, TEXT("the rule was copied without its @media")));
		ASSERT_THAT(IsTrue(CopyFirstRule(TEXT("#tail"), Copied) == EArcRmlRuleCopyResult::Unterminated, TEXT("a rule cut off by the end of the file is not named as such")));
	}

	TEST_METHOD(CopyRule_KeepsImportantAndAddsNoStraySemicolon)
	{
		using namespace ArcRmlInspectorTestsPrivate;
		TArray<FArcRmlInspectorEdit> Edits;
		Edits.Add(FArcRmlInspectorEdit{TEXT("color"), TEXT("blue")});
		const FString ImportantSource = TEXT("a {\n\tcolor: red !important;\n}\n");
		ASSERT_THAT(AreEqual(FString(TEXT("a {\n\tcolor: blue !important;\n}")), Copy(ImportantSource, MakeRule(ImportantSource, TEXT("a")), Edits)));

		TArray<FArcRmlInspectorEdit> Appended;
		Appended.Add(FArcRmlInspectorEdit{TEXT("width"), TEXT("5px")});
		const FString NoteSource = TEXT("a {\n\tcolor: red;\n\t/* note */\n}\n");
		ASSERT_THAT(AreEqual(FString(TEXT("a {\n\tcolor: red;\n\t/* note */\n\twidth: 5px;\n}")), Copy(NoteSource, MakeRule(NoteSource, TEXT("a")), Appended)));
	}

	TEST_METHOD(CopyRule_EditedLonghandReplacesItsShorthandPart)
	{
		using namespace ArcRmlInspectorTestsPrivate;
		TArray<FArcRmlInspectorEdit> Edits;
		Edits.Add(FArcRmlInspectorEdit{TEXT("margin-top"), TEXT("5px")});
		TMap<FString, TArray<FString>> Shorthands;
		Shorthands.Add(TEXT("margin"), { TEXT("margin-top"), TEXT("margin-right"), TEXT("margin-bottom"), TEXT("margin-left") });
		const FString Source = TEXT("a {\n\tmargin: 1px 2px;\n}\n");
		FArcRmlInspectorRule Rule = MakeRule(Source, TEXT("a"));
		Rule.Properties.Add(MakeProperty(TEXT("margin-top"), TEXT("1px")));
		Rule.Properties.Add(MakeProperty(TEXT("margin-right"), TEXT("2px")));
		Rule.Properties.Add(MakeProperty(TEXT("margin-bottom"), TEXT("1px")));
		Rule.Properties.Add(MakeProperty(TEXT("margin-left"), TEXT("2px")));
		ASSERT_THAT(AreEqual(FString(TEXT("a {\n\tmargin-top: 5px;\n\tmargin-right: 2px;\n\tmargin-bottom: 1px;\n\tmargin-left: 2px;\n}")),
			Copy(Source, Rule, Edits, Shorthands)));

		const FString ImportantSource = TEXT("a {\n\tmargin: 1px 2px !important;\n}\n");
		FArcRmlInspectorRule ImportantRule = MakeRule(ImportantSource, TEXT("a"));
		ImportantRule.Properties = Rule.Properties;
		ASSERT_THAT(AreEqual(
			FString(TEXT("a {\n\tmargin-top: 5px !important;\n\tmargin-right: 2px !important;\n\tmargin-bottom: 1px !important;\n\tmargin-left: 2px !important;\n}")),
			Copy(ImportantSource, ImportantRule, Edits, Shorthands), TEXT("the edited longhand lost the !important of its shorthand")));
	}
};
