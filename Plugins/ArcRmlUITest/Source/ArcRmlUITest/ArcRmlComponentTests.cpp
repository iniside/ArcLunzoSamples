// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

#include "RmlUi/Core/ComputedValues.h"
#include "RmlUi/Core/Element.h"
#include "RmlUi/Core/ElementDocument.h"
#include "RmlUi/Core/ElementText.h"

namespace ArcRmlComponentTestsPrivate
{
	FString GetTextAtPath(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& Path)
	{
		FString Text;
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			if (Element.Parent == Path && Element.Kind == TEXT("text"))
			{
				Text += Element.Text;
			}
		}
		return Text;
	}

	int32 CountWithClass(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& Class)
	{
		int32 Count = 0;
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			TArray<FString> Classes;
			Element.Classes.ParseIntoArrayWS(Classes);
			Count += Classes.Contains(Class) ? 1 : 0;
		}
		return Count;
	}

	void AppendLength(FString& Out, const TCHAR* Name, float Value)
	{
		if (Value != 0.0f)
		{
			Out += FString::Printf(TEXT(" %s%g"), Name, Value);
		}
	}

	FString DescribeBox(Rml::Element& Element)
	{
		const Rml::Style::ComputedValues& Values = Element.GetComputedValues();
		FString Out;
		AppendLength(Out, TEXT("ml"), Values.margin_left().value);
		AppendLength(Out, TEXT("mr"), Values.margin_right().value);
		AppendLength(Out, TEXT("pl"), Values.padding_left().value);
		AppendLength(Out, TEXT("pr"), Values.padding_right().value);
		AppendLength(Out, TEXT("mt"), Values.margin_top().value);
		AppendLength(Out, TEXT("mb"), Values.margin_bottom().value);
		AppendLength(Out, TEXT("pt"), Values.padding_top().value);
		AppendLength(Out, TEXT("pb"), Values.padding_bottom().value);
		return Out;
	}

	FString GetText(Rml::Element& Element)
	{
		return FString(UTF8_TO_TCHAR(Element.GetInnerRML().c_str())).TrimStartAndEnd();
	}

	FString DescribeContainer(Rml::ElementDocument& Document, const char* Id)
	{
		Rml::Element* Container = Document.GetElementById(Id);
		if (!Container)
		{
			return FString::Printf(TEXT("%hs missing"), Id);
		}
		FString Out = FString(UTF8_TO_TCHAR(Id)) + DescribeBox(*Container) + TEXT("[");
		bool bFirst = true;
		for (int32 Index = 0; Index < Container->GetNumChildren(); ++Index)
		{
			Rml::Element* Child = Container->GetChild(Index);
			if (Child->GetComputedValues().display() == Rml::Style::Display::None)
			{
				continue;
			}
			Out += (bFirst ? TEXT("") : TEXT("|")) + GetText(*Child) + DescribeBox(*Child);
			bFirst = false;
		}
		return Out + TEXT("]");
	}

	FString DescribeAnchorSelectors(Rml::ElementDocument& Document)
	{
		Rml::ElementList Rows;
		Document.QuerySelectorAll(Rows, "#list .ls-row");
		Rml::Element* List = Document.GetElementById("list");
		Rml::Element* Anchor = List && List->GetNumChildren() > 0 ? List->GetChild(List->GetNumChildren() - 1) : nullptr;
		FString AnchorState = TEXT("missing");
		if (Anchor)
		{
			AnchorState = Anchor->Matches(".ls-row") ? TEXT("matches")
				: (Anchor->GetComputedValues().display() == Rml::Style::Display::None ? TEXT("hidden") : TEXT("shown"));
		}
		Rml::Element* Last = Document.QuerySelector("#list .ls-row:last-child");
		FString Out = FString::Printf(TEXT("rows %d anchor %s last %s"), int32(Rows.size()), *AnchorState, Last ? *GetText(*Last) : TEXT("none"));
		for (const char* Id : {"list", "list2", "one", "cond"})
		{
			Out += TEXT(" / ") + DescribeContainer(Document, Id);
		}
		return Out;
	}

	FString CollectText(Rml::Element& Element)
	{
		if (Rml::ElementText* Text = rmlui_dynamic_cast<Rml::ElementText*>(&Element))
		{
			return FString(UTF8_TO_TCHAR(Text->GetText().c_str()));
		}
		FString Out;
		for (int32 Index = 0; Index < Element.GetNumChildren(); ++Index)
		{
			Out += CollectText(*Element.GetChild(Index));
		}
		return Out;
	}

	FString DescribeImport(Rml::ElementDocument& Document, const char* ContainerId)
	{
		Rml::Element* Container = Document.GetElementById(ContainerId);
		Rml::Element* Box = Container ? Container->QuerySelector(".tb") : nullptr;
		if (!Box)
		{
			return FString::Printf(TEXT("%hs: no component"), ContainerId);
		}
		TArray<FString> Parts;
		Parts.Add(Box->IsClassSet("tb-warn") ? TEXT("warn") : TEXT("info"));
		Rml::ElementList Lines;
		Box->QuerySelectorAll(Lines, ".tb-label, .tb-value, .tb-empty");
		for (Rml::Element* Line : Lines)
		{
			if (Line->GetComputedValues().display() != Rml::Style::Display::None)
			{
				Parts.Add(CollectText(*Line).TrimStartAndEnd());
			}
		}
		return FString::Join(Parts, TEXT(" | "));
	}

	FString DescribeVisibleChildren(Rml::Element& Parent)
	{
		TArray<FString> Parts;
		for (int32 Index = 0; Index < Parent.GetNumChildren(); ++Index)
		{
			Rml::Element* Child = Parent.GetChild(Index);
			if (!rmlui_dynamic_cast<Rml::ElementText*>(Child) && Child->GetComputedValues().display() != Rml::Style::Display::None)
			{
				Parts.Add(CollectText(*Child).TrimStartAndEnd());
			}
		}
		return FString::Join(Parts, TEXT(" "));
	}

	FString DescribeSlots(Rml::ElementDocument& Document, const char* ContainerId)
	{
		Rml::Element* Container = Document.GetElementById(ContainerId);
		Rml::Element* Title = Container ? Container->QuerySelector(".ts-title") : nullptr;
		if (!Title)
		{
			return FString::Printf(TEXT("%hs: no component"), ContainerId);
		}
		TArray<FString> Parts;
		Parts.Add(CollectText(*Title).TrimStartAndEnd());
		Rml::ElementList Slots;
		Container->QuerySelectorAll(Slots, ".ts-slot");
		for (Rml::Element* Slot : Slots)
		{
			Parts.Add(TEXT("[") + DescribeVisibleChildren(*Slot) + TEXT("]"));
		}
		Rml::Element* Rows = Container->QuerySelector(".ts-rows");
		Parts.Add(TEXT("rows [") + (Rows ? DescribeVisibleChildren(*Rows) : FString(TEXT("missing"))) + TEXT("]"));
		return FString::Join(Parts, TEXT(" ")) + TEXT(" / ") + DescribeImport(Document, ContainerId);
	}

	FString DescribeHostStyle(Rml::ElementDocument& Document)
	{
		Rml::Element* Container = Document.GetElementById("static");
		Rml::Element* Host = Container ? Container->QuerySelector(".sc-host") : nullptr;
		if (!Host)
		{
			return TEXT("no host");
		}
		const Rml::Colourb Colour = Host->GetComputedValues().color();
		const bool bInheritedColour = Colour == Container->GetComputedValues().color();
		return FString::Printf(TEXT("width %.0f left %.0f colour %s"), Host->GetBox().GetSize(Rml::BoxArea::Border).x,
			Host->GetAbsoluteLeft() - Container->GetAbsoluteLeft(), bInheritedColour ? TEXT("inherited") : TEXT("set"));
	}
}

TEST_CLASS(ArcRmlComponentTests, "ArcRmlUI.Components")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFresh(const FString& Path)
	{
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument;
	}

	TEST_METHOD(ClickEventOnComponentTag_CallsViewModelFunction)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/CallTest.rml"));
		ASSERT_THAT(IsTrue(OpenFresh(Path), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.ClickElement(TEXT(".cb-button"));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the click failed")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Pressed"), TEXT("one")), FString::Printf(TEXT("%d calls"), View.GetCalls().Num())));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("HandleAction"), FString()), TEXT("an empty action prop routed a HandleAction call")));
	}

	TEST_METHOD(KeyedDataForOnComponentTag_KeepsShiftedRow)
	{
		using namespace ArcRmlComponentTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/KeyTest.rml"));
		ASSERT_THAT(IsTrue(OpenFresh(Path), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		ASSERT_THAT(AreEqual(2, CountWithClass(Elements, TEXT("kr-row")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.ClickElement(TEXT(".kr-row"), 1);
		ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
		ASSERT_THAT(AreEqual(FString(TEXT("Row B")), GetTextAtPath(Elements, View.GetProbe().FocusPath), FString::Printf(TEXT("focus '%s'"), *View.GetProbe().FocusPath)));

		TArray<FArcRmlListOp> Ops;
		FArcRmlListOp& Remove = Ops.AddDefaulted_GetRef();
		Remove.Type = EArcRmlListOpType::Remove;
		Remove.Index = 0;
		View.ApplyListOps(TEXT("keytest"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
		ASSERT_THAT(AreEqual(1, CountWithClass(Elements, TEXT("kr-row")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("Row B")), GetTextAtPath(Elements, View.GetProbe().FocusPath),
			FString::Printf(TEXT("the focused row did not survive the shift (focus '%s')"), *View.GetProbe().FocusPath)));
	}

	TEST_METHOD(KeyedDataForRepeatedKeys_EveryEntryKeepsARow)
	{
		using namespace ArcRmlComponentTestsPrivate;
		TestRunner->AddExpectedMessagePlain(TEXT("Keyed data-for over 'Rows'"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/KeyRepeatTest.rml"));
		ASSERT_THAT(IsTrue(OpenFresh(Path), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		ASSERT_THAT(AreEqual(4, CountWithClass(Elements, TEXT("kr-row")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.ClickElement(TEXT(".kr-row"), 2);
		ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
		ASSERT_THAT(AreEqual(FString(TEXT("Row B")), GetTextAtPath(Elements, View.GetProbe().FocusPath), FString::Printf(TEXT("focus '%s'"), *View.GetProbe().FocusPath)));

		TArray<FArcRmlListOp> Ops;
		FArcRmlListOp& Remove = Ops.AddDefaulted_GetRef();
		Remove.Type = EArcRmlListOpType::Remove;
		Remove.Index = 0;
		View.ApplyListOps(TEXT("keyrepeat"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
		ASSERT_THAT(AreEqual(3, CountWithClass(Elements, TEXT("kr-row")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("Row B")), GetTextAtPath(Elements, View.GetProbe().FocusPath),
			FString::Printf(TEXT("the focused row did not survive the removal (focus '%s')"), *View.GetProbe().FocusPath)));
	}

	TEST_METHOD(DataForAnchor_SelectorsSkipItLikeChromeSkipsTheVuePlaceholder)
	{
		using namespace ArcRmlComponentTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/AnchorSelectorTest.rml"));
		ASSERT_THAT(IsTrue(OpenFresh(Path), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		TSharedRef<FString> State = MakeShared<FString>();
		ASSERT_THAT(IsTrue(View.RunOnDocument([State](Rml::ElementDocument& Document) { *State = DescribeAnchorSelectors(Document); })));
		ASSERT_THAT(AreEqual(FString(TEXT("rows 3 anchor hidden last C / list[A ml1 pl3|B mt5 mb6|C mr2 pl3 mt5 mb6] / list2[A ml1 pl3|B mt5 mb6|C pl3 mt5 mb6|T pt7] / one[X pr4] / cond[A|B pl9]")),
			*State));

		TArray<FArcRmlListOp> RowOps;
		FArcRmlListOp& RemoveRow = RowOps.AddDefaulted_GetRef();
		RemoveRow.Type = EArcRmlListOpType::Remove;
		RemoveRow.Index = 2;
		View.ApplyListOps(TEXT("anchorsel"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Rows")), MoveTemp(RowOps));
		TArray<FArcRmlListOp> OneOps;
		FArcRmlListOp& RemoveOne = OneOps.AddDefaulted_GetRef();
		RemoveOne.Type = EArcRmlListOpType::Remove;
		RemoveOne.Index = 0;
		View.ApplyListOps(TEXT("anchorsel"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("One")), MoveTemp(OneOps));
		ASSERT_THAT(IsTrue(View.Flush() && View.FlushUntilSettled()));

		ASSERT_THAT(IsTrue(View.RunOnDocument([State](Rml::ElementDocument& Document) { *State = DescribeAnchorSelectors(Document); })));
		ASSERT_THAT(AreEqual(FString(TEXT("rows 2 anchor hidden last B / list[A ml1 pl3|B mr2 mt5 mb6] / list2[A ml1 pl3|B mt5 mb6|T pt7] / one pb8[] / cond[A pl9]")), *State));
	}

	TEST_METHOD(ImportProps_StaticBoundDefaultAndModelHost)
	{
		using namespace ArcRmlComponentTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/PropsTest.rml"));
		ASSERT_THAT(IsTrue(OpenFresh(Path), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		TSharedRef<FString> State = MakeShared<FString>();
		const TFunction<void(Rml::ElementDocument&)> Describe = [State](Rml::ElementDocument& Document)
		{
			*State = DescribeImport(Document, "static") + TEXT(" / ") + DescribeHostStyle(Document) + TEXT(" / ") + DescribeImport(Document, "bound")
				+ TEXT(" / ") + DescribeImport(Document, "defaults") + TEXT(" / ") + DescribeImport(Document, "hosted");
		};
		ASSERT_THAT(IsTrue(View.RunOnDocument(Describe)));
		ASSERT_THAT(AreEqual(FString(TEXT("info | Static label | A & B \\ back-slash | note text / width 360 left 12 colour inherited / warn | QB1 | 2")
			TEXT(" / info | Box (default label, with spaces) | no value / info | Ranger | 120")), *State));

		View.SetModelValue(TEXT("props"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("BarId")), ArcRmlUITestHelpers::MakeString(TEXT("QB2")));
		View.SetModelValue(TEXT("other"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Gold")), ArcRmlUITestHelpers::MakeInt(7));
		ASSERT_THAT(IsTrue(View.Flush() && View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.RunOnDocument(Describe)));
		ASSERT_THAT(AreEqual(FString(TEXT("info | Static label | A & B \\ back-slash | note text / width 360 left 12 colour inherited / warn | QB2 | 2")
			TEXT(" / info | Box (default label, with spaces) | no value / info | Ranger | 7")), *State));
	}

	TEST_METHOD(ImportSlotsAndArrayProps_EverySlotGetsTheChildren)
	{
		using namespace ArcRmlComponentTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/PropsTest.rml"));
		ASSERT_THAT(IsTrue(OpenFresh(Path), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		TSharedRef<FString> State = MakeShared<FString>();
		const TFunction<void(Rml::ElementDocument&)> Describe = [State](Rml::ElementDocument& Document)
		{
			*State = DescribeSlots(Document, "slots") + TEXT(" // ") + DescribeSlots(Document, "slotsmissing");
		};
		ASSERT_THAT(IsTrue(View.RunOnDocument(Describe)));
		ASSERT_THAT(AreEqual(FString(TEXT("Two slots (2 rows) [A c0 c1] [B c0 c1] rows [r0 r1] / warn | Two slots | note set")
			TEXT(" // Missing rows (0 rows) [A] [B] rows [] / warn | Missing rows | no value")), *State));

		TArray<FArcRmlListOp> Ops;
		FArcRmlListOp& Remove = Ops.AddDefaulted_GetRef();
		Remove.Type = EArcRmlListOpType::Remove;
		Remove.Index = 0;
		View.ApplyListOps(TEXT("props"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Slots")), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(View.Flush() && View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(View.RunOnDocument(Describe)));
		ASSERT_THAT(AreEqual(FString(TEXT("Two slots (1 rows) [A c0] [B c0] rows [r0] / warn | Two slots | note set")
			TEXT(" // Missing rows (0 rows) [A] [B] rows [] / warn | Missing rows | no value")), *State));
	}

	TEST_METHOD(ImportPropErrors_FailTheCompileOncePerCase)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/PropsErrorTest.rml"));
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, TEXT("/ArcRmlUITest/UI/Rml/"));
		ASSERT_THAT(IsFalse(OpenFresh(Path), TEXT("a document with three prop errors compiled and opened")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnostics(View, TEXT("whose escaped quote a {{ }} text expression cannot hold")),
			ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnostics(View, TEXT("$item needs a member access, which requires a whole-attribute {{ path }} value")),
			ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, TEXT("has no prop 'dataUnknown'"), Path, 11),
			ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
	}
};
