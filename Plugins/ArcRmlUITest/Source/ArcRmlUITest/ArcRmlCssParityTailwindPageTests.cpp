// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Dom/JsonObject.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace ArcRmlCssParityTailwindPageTestsPrivate
{
	const FIntPoint PageSize(1920, 1080);
	constexpr double Tolerance = 1.0;
	constexpr int32 ListedFailures = 30;

	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
	}

	struct FFieldValue
	{
		EJson Type = EJson::Null;
		double Number = 0.0;
		bool bValue = false;
		FString Text;

		FString ToString() const
		{
			switch (Type)
			{
			case EJson::Number:
				return FString::Printf(TEXT("%g"), Number);
			case EJson::Boolean:
				return bValue ? TEXT("true") : TEXT("false");
			case EJson::String:
				return TEXT("'") + Text + TEXT("'");
			default:
				return TEXT("none");
			}
		}
	};

	using FFields = TMap<FString, FFieldValue>;

	FFieldValue ToField(const TSharedPtr<FJsonValue>& Value)
	{
		FFieldValue Field;
		if (!Value.IsValid())
		{
			return Field;
		}
		Field.Type = Value->Type;
		if (Value->Type == EJson::Number)
		{
			Field.Number = Value->AsNumber();
		}
		else if (Value->Type == EJson::Boolean)
		{
			Field.bValue = Value->AsBool();
		}
		else if (Value->Type == EJson::String)
		{
			Field.Text = Value->AsString();
		}
		else
		{
			Field.Type = EJson::Null;
		}
		return Field;
	}

	FFieldValue MakeNumber(double Number)
	{
		FFieldValue Field;
		Field.Type = EJson::Number;
		Field.Number = Number;
		return Field;
	}

	FFieldValue MakeText(const FString& Text)
	{
		FFieldValue Field;
		Field.Type = EJson::String;
		Field.Text = Text;
		return Field;
	}

	const TArray<TSharedPtr<FJsonValue>>* GetArray(const FJsonObject& Object, const TCHAR* Name)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		return Object.TryGetArrayField(Name, Values) ? Values : nullptr;
	}

	double GetNumberAt(const FJsonObject& Object, const TCHAR* Name, int32 Index)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = GetArray(Object, Name);
		return Values && Values->IsValidIndex(Index) && (*Values)[Index].IsValid() && (*Values)[Index]->Type == EJson::Number ? (*Values)[Index]->AsNumber() : 0.0;
	}

	void AddArrayFields(const FJsonObject& Object, const TCHAR* Name, std::initializer_list<const TCHAR*> Keys, FFields& OutFields)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = GetArray(Object, Name);
		int32 Index = 0;
		for (const TCHAR* Key : Keys)
		{
			OutFields.Add(Key, Values && Values->IsValidIndex(Index) ? ToField((*Values)[Index]) : FFieldValue());
			++Index;
		}
	}

	// chrome_layout_compare.py box_fields.
	FFields GetBoxFields(const FJsonObject& Object)
	{
		FFields Fields;
		AddArrayFields(Object, TEXT("rect"), { TEXT("rect.x"), TEXT("rect.y"), TEXT("rect.w"), TEXT("rect.h") }, Fields);
		AddArrayFields(Object, TEXT("padding"), { TEXT("padding.top"), TEXT("padding.right"), TEXT("padding.bottom"), TEXT("padding.left") }, Fields);
		AddArrayFields(Object, TEXT("border"), { TEXT("border.top"), TEXT("border.right"), TEXT("border.bottom"), TEXT("border.left") }, Fields);
		AddArrayFields(Object, TEXT("margin"), { TEXT("margin.top"), TEXT("margin.right"), TEXT("margin.bottom"), TEXT("margin.left") }, Fields);
		AddArrayFields(Object, TEXT("scroll_size"), { TEXT("scroll_size.w"), TEXT("scroll_size.h") }, Fields);
		AddArrayFields(Object, TEXT("scroll"), { TEXT("scroll.left"), TEXT("scroll.top") }, Fields);
		AddArrayFields(Object, TEXT("scrollbars"), { TEXT("scrollbar.v"), TEXT("scrollbar.h") }, Fields);
		const TArray<TSharedPtr<FJsonValue>>* Fragments = GetArray(Object, TEXT("fragments"));
		if (Fragments && Fragments->Num() > 0)
		{
			Fields.Add(TEXT("fragments.count"), MakeNumber(Fragments->Num()));
		}
		return Fields;
	}

	// chrome_layout_compare.py line_text; ToLower stands in for casefold.
	FString NormaliseLineText(const FString& Text)
	{
		FString Result;
		bool bPendingSpace = false;
		for (const TCHAR Character : Text)
		{
			if (FChar::IsWhitespace(Character) || Character == TCHAR(0x00A0))
			{
				bPendingSpace = !Result.IsEmpty();
				continue;
			}
			if (bPendingSpace)
			{
				Result.AppendChar(TEXT(' '));
				bPendingSpace = false;
			}
			Result.AppendChar(Character);
		}
		return Result.ToLower();
	}

	struct FLine
	{
		FString Text;
		double X = 0.0;
		double Width = 0.0;
		double Top = 0.0;
		double Bottom = 0.0;
	};

	// chrome_layout_compare.py text_fields.
	FFields GetTextFields(const TArray<FLine>& Lines)
	{
		FFields Fields;
		Fields.Add(TEXT("lines.count"), MakeNumber(Lines.Num()));
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			const FLine& Line = Lines[Index];
			Fields.Add(FString::Printf(TEXT("line%d.text"), Index), MakeText(NormaliseLineText(Line.Text)));
			Fields.Add(FString::Printf(TEXT("line%d.x"), Index), MakeNumber(Line.X));
			Fields.Add(FString::Printf(TEXT("line%d.top"), Index), MakeNumber(Line.Top));
			Fields.Add(FString::Printf(TEXT("line%d.width"), Index), MakeNumber(Line.Width));
			Fields.Add(FString::Printf(TEXT("line%d.height"), Index), MakeNumber(Line.Bottom - Line.Top));
		}
		return Fields;
	}

	FString GetLineText(const FJsonObject& Line)
	{
		FString Text;
		Line.TryGetStringField(TEXT("text"), Text);
		return Text;
	}

	// chrome_layout_compare.py rml_lines.
	TArray<FLine> GetDumpLines(const FJsonObject& Object)
	{
		const double Ascent = GetNumberAt(Object, TEXT("ascent"), 0);
		const double Descent = GetNumberAt(Object, TEXT("descent"), 0);
		TArray<FLine> Lines;
		if (const TArray<TSharedPtr<FJsonValue>>* Values = GetArray(Object, TEXT("lines")))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				const FJsonObject& Line = *Value->AsObject();
				const double Baseline = Line.GetNumberField(TEXT("baseline"));
				Lines.Add({ GetLineText(Line), Line.GetNumberField(TEXT("x")), Line.GetNumberField(TEXT("width")), Baseline - Ascent, Baseline + Descent });
			}
		}
		return Lines;
	}

	// chrome_layout_compare.py chrome_lines.
	TArray<FLine> GetChromeLines(const FJsonObject& Object)
	{
		TArray<FLine> Lines;
		if (const TArray<TSharedPtr<FJsonValue>>* Values = GetArray(Object, TEXT("lines")))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				const FJsonObject& Line = *Value->AsObject();
				const double Top = Line.GetNumberField(TEXT("top"));
				Lines.Add({ GetLineText(Line), Line.GetNumberField(TEXT("x")), Line.GetNumberField(TEXT("width")), Top, Top + Line.GetNumberField(TEXT("height")) });
			}
		}
		return Lines;
	}

	// chrome_layout_compare.py differs.
	bool Differs(const FString& Field, const FFieldValue& A, const FFieldValue& B)
	{
		if (A.Type != EJson::Number || B.Type != EJson::Number)
		{
			return A.Type != B.Type || A.bValue != B.bValue || !A.Text.Equals(B.Text, ESearchCase::CaseSensitive);
		}
		const double Limit = Field.StartsWith(TEXT("scroll_size.")) || Field.StartsWith(TEXT("scroll.")) ? FMath::Max(Tolerance, 1.0) : Tolerance;
		return FMath::Abs(A.Number - B.Number) > Limit;
	}

	struct FDumpNode
	{
		TSharedPtr<FJsonObject> Object;
		FString Parent;
		FString Display;
	};

	bool IsHidden(const TMap<FString, FDumpNode>& Nodes, FString Id)
	{
		while (const FDumpNode* Node = Nodes.Find(Id))
		{
			if (Node->Display == TEXT("none"))
			{
				return true;
			}
			Id = Node->Parent;
		}
		return false;
	}

	struct FMismatch
	{
		FFieldValue RmlUi;
		FFieldValue Chrome;
		TOptional<double> Diff;
	};

	struct FComparison
	{
		int32 Compared = 0;
		TMap<FString, FMismatch> Mismatches;
		TArray<FString> Missing;
		TMap<FString, FString> Unmatched;
	};

	bool IsSameValue(const FFieldValue& A, const FFieldValue& B)
	{
		return A.Type == B.Type && A.Number == B.Number && A.bValue == B.bValue && A.Text.Equals(B.Text, ESearchCase::CaseSensitive);
	}

	TSharedPtr<FJsonValue> ToJson(const FFieldValue& Value)
	{
		switch (Value.Type)
		{
		case EJson::Number:
			return MakeShared<FJsonValueNumber>(Value.Number);
		case EJson::Boolean:
			return MakeShared<FJsonValueBoolean>(Value.bValue);
		case EJson::String:
			return MakeShared<FJsonValueString>(Value.Text);
		default:
			return MakeShared<FJsonValueNull>();
		}
	}

	bool ParseJson(const FString& Text, TSharedPtr<FJsonObject>& OutObject)
	{
		return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), OutObject) && OutObject.IsValid();
	}

	FString GetSkipReason(const TMap<FString, FDumpNode>& Nodes, const FString& Id)
	{
		const FDumpNode& Node = Nodes[Id];
		const FJsonObject& Object = *Node.Object;
		const FString Kind = Object.GetStringField(TEXT("kind"));
		if (!Object.GetBoolField(TEXT("dom")) || (Kind != TEXT("element") && Kind != TEXT("text")))
		{
			return TEXT("not-dom");
		}
		if (Node.Display == TEXT("contents"))
		{
			return TEXT("contents");
		}
		return IsHidden(Nodes, Id) ? TEXT("hidden") : FString();
	}

	// chrome_layout_compare.py compare_chrome with --tol 1: every DOM element and text node that is shown and not display: contents.
	// Reference nodes left out of that comparison go to Unmatched with the reason.
	FString Compare(const FJsonObject& Dump, const FJsonObject& Reference, FComparison& Out)
	{
		const TSharedPtr<FJsonObject>* Context = nullptr;
		if (!Dump.TryGetObjectField(TEXT("context"), Context))
		{
			return TEXT("the dump has no context");
		}
		if ((*Context)->GetNumberField(TEXT("layout_verify")) != 0.0)
		{
			return TEXT("the dump was taken with ArcRml.LayoutVerify on");
		}
		if (Dump.GetNumberField(TEXT("format")) < 5.0)
		{
			return TEXT("the dump format has no content_rect");
		}
		const FIntPoint DumpSize((*Context)->GetIntegerField(TEXT("width")), (*Context)->GetIntegerField(TEXT("height")));
		const FIntPoint ReferenceSize(static_cast<int32>(GetNumberAt(Reference, TEXT("viewport"), 0)), static_cast<int32>(GetNumberAt(Reference, TEXT("viewport"), 1)));
		if (DumpSize != ReferenceSize)
		{
			return FString::Printf(TEXT("the dump context is %s, the Chrome viewport %s"), *DumpSize.ToString(), *ReferenceSize.ToString());
		}
		const TSharedPtr<FJsonObject>* Measured = nullptr;
		if (!Reference.TryGetObjectField(TEXT("elements"), Measured))
		{
			return TEXT("the reference has no elements");
		}
		const TArray<TSharedPtr<FJsonValue>>* Elements = GetArray(Dump, TEXT("elements"));
		if (!Elements)
		{
			return TEXT("the dump has no elements");
		}

		TArray<FString> Order;
		TMap<FString, FDumpNode> Nodes;
		for (const TSharedPtr<FJsonValue>& Value : *Elements)
		{
			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			const FString Id = Object->GetStringField(TEXT("id"));
			FDumpNode& Node = Nodes.Add(Id);
			Node.Object = Object;
			Object->TryGetStringField(TEXT("parent"), Node.Parent);
			Node.Display = Object->GetStringField(TEXT("display"));
			if (Object->GetStringField(TEXT("inputs")).Contains(TEXT("!important")))
			{
				return FString::Printf(TEXT("%s has !important local properties (running transition values)"), *Id);
			}
			Order.Add(Id);
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Measured)->Values)
		{
			if (!Nodes.Contains(Pair.Key))
			{
				Out.Unmatched.Add(Pair.Key, TEXT("absent"));
				continue;
			}
			const FString Reason = GetSkipReason(Nodes, Pair.Key);
			if (!Reason.IsEmpty())
			{
				Out.Unmatched.Add(Pair.Key, Reason);
			}
		}

		for (const FString& Id : Order)
		{
			if (!GetSkipReason(Nodes, Id).IsEmpty())
			{
				continue;
			}
			const FJsonObject& Object = *Nodes[Id].Object;
			const TSharedPtr<FJsonObject>* Chrome = nullptr;
			if (!(*Measured)->TryGetObjectField(Id, Chrome))
			{
				Out.Missing.Add(Id);
				continue;
			}
			++Out.Compared;
			const bool bText = Object.GetStringField(TEXT("kind")) == TEXT("text");
			const FFields A = bText ? GetTextFields(GetDumpLines(Object)) : GetBoxFields(Object);
			const FFields B = bText ? GetTextFields(GetChromeLines(**Chrome)) : GetBoxFields(**Chrome);
			TSet<FString> Names;
			A.GetKeys(Names);
			for (const TPair<FString, FFieldValue>& Pair : B)
			{
				Names.Add(Pair.Key);
			}
			const FFieldValue None;
			for (const FString& Field : Names)
			{
				const FFieldValue* FieldA = A.Find(Field);
				const FFieldValue* FieldB = B.Find(Field);
				const FFieldValue& ValueA = FieldA ? *FieldA : None;
				const FFieldValue& ValueB = FieldB ? *FieldB : None;
				if (Differs(Field, ValueA, ValueB))
				{
					FMismatch& Mismatch = Out.Mismatches.Add(Id + TEXT("|") + Field);
					Mismatch.RmlUi = ValueA;
					Mismatch.Chrome = ValueB;
					if (ValueA.Type == EJson::Number && ValueB.Type == EJson::Number)
					{
						Mismatch.Diff = FMath::Abs(ValueA.Number - ValueB.Number);
					}
				}
			}
		}
		Out.Mismatches.KeySort(TLess<FString>());
		Out.Missing.Sort();
		Out.Unmatched.KeySort(TLess<FString>());
		return FString();
	}

	constexpr double SameValueLimit = 1e-3;

	// chrome_layout_compare.py's --json record, plus the reference nodes the comparison leaves out.
	FString MakeBaseline(const FComparison& Comparison, const FString& ReferenceHash)
	{
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetStringField(TEXT("reference_sha1"), ReferenceHash);
		Root->SetNumberField(TEXT("tol"), Tolerance);
		const TSharedRef<FJsonObject> Mismatches = MakeShared<FJsonObject>();
		for (const TPair<FString, FMismatch>& Pair : Comparison.Mismatches)
		{
			const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetField(TEXT("rmlui"), ToJson(Pair.Value.RmlUi));
			Entry->SetField(TEXT("chrome"), ToJson(Pair.Value.Chrome));
			if (Pair.Value.Diff.IsSet())
			{
				Entry->SetNumberField(TEXT("diff"), Pair.Value.Diff.GetValue());
			}
			else
			{
				Entry->SetField(TEXT("diff"), MakeShared<FJsonValueNull>());
			}
			Mismatches->SetObjectField(Pair.Key, Entry);
		}
		Root->SetObjectField(TEXT("mismatches"), Mismatches);
		TArray<TSharedPtr<FJsonValue>> Missing;
		for (const FString& Id : Comparison.Missing)
		{
			Missing.Add(MakeShared<FJsonValueString>(Id));
		}
		Root->SetArrayField(TEXT("missing"), Missing);
		const TSharedRef<FJsonObject> Unmatched = MakeShared<FJsonObject>();
		for (const TPair<FString, FString>& Pair : Comparison.Unmatched)
		{
			Unmatched->SetStringField(Pair.Key, Pair.Value);
		}
		Root->SetObjectField(TEXT("unmatched"), Unmatched);
		FString Text;
		FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Text));
		return Text;
	}

	struct FBaselineCheck
	{
		TArray<FString> Failures;
		TArray<FString> Improved;
		TArray<FString> Gone;
	};

	// chrome_layout_compare.py compare_baseline, stricter: a numeric fork value may move by the tolerance unless it moves closer to an
	// unchanged Chrome value on the same side by more than the tolerance, and a non-numeric value may not change.
	FString CheckBaseline(const FComparison& Comparison, const FJsonObject& Baseline, const FString& ReferenceHash, FBaselineCheck& Out)
	{
		if (Baseline.GetStringField(TEXT("reference_sha1")) != ReferenceHash || Baseline.GetNumberField(TEXT("tol")) != Tolerance)
		{
			return TEXT("the baseline was written for another reference or tolerance; write it again");
		}
		const TSharedPtr<FJsonObject>* Mismatches = nullptr;
		const TSharedPtr<FJsonObject>* Unmatched = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Missing = GetArray(Baseline, TEXT("missing"));
		if (!Baseline.TryGetObjectField(TEXT("mismatches"), Mismatches) || !Baseline.TryGetObjectField(TEXT("unmatched"), Unmatched) || !Missing)
		{
			return TEXT("the baseline has no mismatches, missing or unmatched");
		}

		for (const TPair<FString, FMismatch>& Pair : Comparison.Mismatches)
		{
			const FMismatch& Now = Pair.Value;
			const FString Values = FString::Printf(TEXT("rmlui %s, chrome %s"), *Now.RmlUi.ToString(), *Now.Chrome.ToString());
			const TSharedPtr<FJsonObject>* Entry = nullptr;
			if (!(*Mismatches)->TryGetObjectField(Pair.Key, Entry))
			{
				Out.Failures.Add(FString::Printf(TEXT("new %s: %s"), *Pair.Key, *Values));
				continue;
			}
			const FFieldValue BaseRmlUi = ToField((*Entry)->TryGetField(TEXT("rmlui")));
			const FFieldValue BaseChrome = ToField((*Entry)->TryGetField(TEXT("chrome")));
			if (Now.Diff.IsSet() && BaseRmlUi.Type == EJson::Number && BaseChrome.Type == EJson::Number)
			{
				const double NowOffset = Now.RmlUi.Number - Now.Chrome.Number;
				const double BaseOffset = BaseRmlUi.Number - BaseChrome.Number;
				const FString Was = FString::Printf(TEXT("was rmlui %s, chrome %s; now %s"), *BaseRmlUi.ToString(), *BaseChrome.ToString(), *Values);
				if (FMath::Abs(Now.Chrome.Number - BaseChrome.Number) > SameValueLimit)
				{
					Out.Failures.Add(FString::Printf(TEXT("chrome changed %s: %s"), *Pair.Key, *Was));
				}
				else if (NowOffset * BaseOffset > 0.0 && FMath::Abs(NowOffset) < FMath::Abs(BaseOffset) - Tolerance)
				{
					Out.Improved.Add(FString::Printf(TEXT("%s (%s)"), *Pair.Key, *Was));
				}
				else if (FMath::Abs(Now.RmlUi.Number - BaseRmlUi.Number) > Tolerance)
				{
					Out.Failures.Add(FString::Printf(TEXT("moved %s: %s"), *Pair.Key, *Was));
				}
			}
			else if (!IsSameValue(Now.RmlUi, BaseRmlUi) || !IsSameValue(Now.Chrome, BaseChrome))
			{
				Out.Failures.Add(FString::Printf(TEXT("changed %s: was rmlui %s, chrome %s; now %s"), *Pair.Key, *BaseRmlUi.ToString(), *BaseChrome.ToString(), *Values));
			}
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Mismatches)->Values)
		{
			if (!Comparison.Mismatches.Contains(Pair.Key))
			{
				Out.Gone.Add(Pair.Key);
			}
		}

		TSet<FString> BaseMissing;
		for (const TSharedPtr<FJsonValue>& Value : *Missing)
		{
			BaseMissing.Add(Value->AsString());
		}
		for (const FString& Id : Comparison.Missing)
		{
			if (!BaseMissing.Contains(Id))
			{
				Out.Failures.Add(FString::Printf(TEXT("newly missing in the reference %s"), *Id));
			}
		}
		for (const FString& Id : BaseMissing)
		{
			if (!Comparison.Missing.Contains(Id))
			{
				Out.Gone.Add(FString::Printf(TEXT("missing %s"), *Id));
			}
		}

		for (const TPair<FString, FString>& Pair : Comparison.Unmatched)
		{
			FString BaseReason;
			if (!(*Unmatched)->TryGetStringField(Pair.Key, BaseReason) || BaseReason != Pair.Value)
			{
				Out.Failures.Add(FString::Printf(TEXT("reference node %s not compared (%s)"), *Pair.Key, *Pair.Value));
			}
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Unmatched)->Values)
		{
			if (!Comparison.Unmatched.Contains(Pair.Key))
			{
				Out.Gone.Add(FString::Printf(TEXT("unmatched %s"), *Pair.Key));
			}
		}
		return FString();
	}

	bool IsWritingBaselines()
	{
		return FPlatformMisc::GetEnvironmentVariable(TEXT("ARC_RML_TAILWIND_BASELINE")) == TEXT("write");
	}
}

TEST_CLASS(ArcRmlCssParityTailwindPageTests, "ArcRmlUI.CssParity.TailwindPages")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlCssParityTailwindPageTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
	}

	void ExpectBaseline(const TCHAR* Name)
	{
		using namespace ArcRmlCssParityTailwindPageTestsPrivate;
		const FString Dir = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("TailwindCompat/Comp10"));
		const FString ReferencePath = FPaths::Combine(Dir, FString(Name) + TEXT(".chrome.json"));
		const FString BaselinePath = FPaths::Combine(Dir, FString(Name) + TEXT(".baseline.json"));
		TArray<uint8> ReferenceBytes;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToArray(ReferenceBytes, *ReferencePath),
			FString::Printf(TEXT("no Chrome reference %s; write it with tailwind_page_refs.py"), *ReferencePath)));
		const FString ReferenceHash = FSHA1::HashBuffer(ReferenceBytes.GetData(), ReferenceBytes.Num()).ToString();
		FString ReferenceText;
		FFileHelper::BufferToString(ReferenceText, ReferenceBytes.GetData(), ReferenceBytes.Num());
		TSharedPtr<FJsonObject> Reference;
		ASSERT_THAT(IsTrue(ParseJson(ReferenceText, Reference), FString::Printf(TEXT("%s is not JSON"), *ReferencePath)));
		const TSharedPtr<FJsonObject>* Inputs = nullptr;
		ASSERT_THAT(IsTrue(Reference->TryGetObjectField(TEXT("inputs_sha1"), Inputs) && (*Inputs)->Values.Num() > 0,
			FString::Printf(TEXT("%s records no inputs_sha1; write it again with tailwind_page_refs.py"), *ReferencePath)));
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Input : (*Inputs)->Values)
		{
			TArray<uint8> InputBytes;
			const bool bRead = FFileHelper::LoadFileToArray(InputBytes, *FPaths::Combine(Dir, Input.Key));
			const FString InputHash = bRead ? FSHA1::HashBuffer(InputBytes.GetData(), InputBytes.Num()).ToString() : FString();
			ASSERT_THAT(IsTrue(bRead && InputHash.Equals(Input.Value->AsString(), ESearchCase::IgnoreCase),
				FString::Printf(TEXT("%s changed since %s was measured; write it again with tailwind_page_refs.py"), *Input.Key, *ReferencePath)));
		}

		RunConsole(TEXT("ArcRml.Clock 1000"));
		const FString Path = ArcRmlUITestHelpers::MakePath(FString(TEXT("TailwindCompat/Comp10/")) + Name + TEXT(".rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		ASSERT_THAT(IsTrue(View.Open(Path, PageSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0 && !View.GetProbe().bPendingAsyncWork,
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		const FString& LayoutJson = View.GetProbe().LayoutJson;
		FFileHelper::SaveStringToFile(LayoutJson, *FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("TailwindPages"), FString(Name) + TEXT(".layout.json")),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		TSharedPtr<FJsonObject> Dump;
		ASSERT_THAT(IsTrue(ParseJson(LayoutJson, Dump), TEXT("the layout dump is not JSON")));

		FComparison Comparison;
		const FString Error = Compare(*Dump, *Reference, Comparison);
		ASSERT_THAT(IsTrue(Error.IsEmpty(), Error));
		ASSERT_THAT(IsTrue(Comparison.Compared > 0, TEXT("no node was compared")));

		if (IsWritingBaselines())
		{
			ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(MakeBaseline(Comparison, ReferenceHash), *BaselinePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM),
				*BaselinePath));
			TestRunner->AddWarning(FString::Printf(TEXT("%s: baseline written to %s (%d mismatches, %d missing, %d unmatched), nothing checked; rerun without ARC_RML_TAILWIND_BASELINE"),
				Name, *BaselinePath, Comparison.Mismatches.Num(), Comparison.Missing.Num(), Comparison.Unmatched.Num()));
			return;
		}

		FString BaselineText;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(BaselineText, *BaselinePath),
			FString::Printf(TEXT("no baseline %s; write it with ARC_RML_TAILWIND_BASELINE=write"), *BaselinePath)));
		TSharedPtr<FJsonObject> Baseline;
		ASSERT_THAT(IsTrue(ParseJson(BaselineText, Baseline), FString::Printf(TEXT("%s is not JSON"), *BaselinePath)));
		FBaselineCheck Check;
		const FString BaselineError = CheckBaseline(Comparison, *Baseline, ReferenceHash, Check);
		ASSERT_THAT(IsTrue(BaselineError.IsEmpty(), BaselineError));
		if (!Check.Gone.IsEmpty())
		{
			TestRunner->AddInfo(FString::Printf(TEXT("%s: %d baseline entries now match Chrome (%s); write the baseline again"), Name, Check.Gone.Num(),
				*FString::Join(Check.Gone, TEXT(", "))));
		}
		if (!Check.Improved.IsEmpty())
		{
			TestRunner->AddInfo(FString::Printf(TEXT("%s: %d baseline entries moved closer to Chrome (%s); write the baseline again"), Name, Check.Improved.Num(),
				*FString::Join(Check.Improved, TEXT(" | "))));
		}
		const TArray<FString> Listed(Check.Failures.GetData(), FMath::Min(Check.Failures.Num(), ListedFailures));
		ASSERT_THAT(IsTrue(Check.Failures.IsEmpty(), FString::Printf(TEXT("%s: %d differences from the baseline: %s"), Name, Check.Failures.Num(),
			*FString::Join(Listed, TEXT(" | ")))));
	}

	TEST_METHOD(Badge_MatchesItsBaseline) { ExpectBaseline(TEXT("Badge")); }
	TEST_METHOD(ButtonGroup_MatchesItsBaseline) { ExpectBaseline(TEXT("ButtonGroup")); }
	TEST_METHOD(Card_MatchesItsBaseline) { ExpectBaseline(TEXT("Card")); }
	TEST_METHOD(Checkbox_MatchesItsBaseline) { ExpectBaseline(TEXT("Checkbox")); }
	TEST_METHOD(ContactForm_MatchesItsBaseline) { ExpectBaseline(TEXT("ContactForm")); }
	TEST_METHOD(DetailsList_MatchesItsBaseline) { ExpectBaseline(TEXT("DetailsList")); }
	TEST_METHOD(Dropdown_MatchesItsBaseline) { ExpectBaseline(TEXT("Dropdown")); }
	TEST_METHOD(FbAlert_MatchesItsBaseline) { ExpectBaseline(TEXT("FbAlert")); }
	TEST_METHOD(FbAvatar_MatchesItsBaseline) { ExpectBaseline(TEXT("FbAvatar")); }
	TEST_METHOD(FbBadge_MatchesItsBaseline) { ExpectBaseline(TEXT("FbBadge")); }
	TEST_METHOD(FbListGroup_MatchesItsBaseline) { ExpectBaseline(TEXT("FbListGroup")); }
	TEST_METHOD(Input_MatchesItsBaseline) { ExpectBaseline(TEXT("Input")); }
	TEST_METHOD(Modal_MatchesItsBaseline) { ExpectBaseline(TEXT("Modal")); }
	TEST_METHOD(Navbar_MatchesItsBaseline) { ExpectBaseline(TEXT("Navbar")); }
	TEST_METHOD(Pagination_MatchesItsBaseline) { ExpectBaseline(TEXT("Pagination")); }
	TEST_METHOD(Progress_MatchesItsBaseline) { ExpectBaseline(TEXT("Progress")); }
	TEST_METHOD(Select_MatchesItsBaseline) { ExpectBaseline(TEXT("Select")); }
	TEST_METHOD(Stats_MatchesItsBaseline) { ExpectBaseline(TEXT("Stats")); }
	TEST_METHOD(Table_MatchesItsBaseline) { ExpectBaseline(TEXT("Table")); }
	TEST_METHOD(Tabs_MatchesItsBaseline) { ExpectBaseline(TEXT("Tabs")); }

	TEST_METHOD(UtilProbe_MatchesItsBaseline)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("Property value does not support animations or transitions. Value: cubic-bezier"), ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains, 1);
		TestRunner->AddExpectedErrorPlain(TEXT("RmlUi: Invalid substitution, property 'mask-image' has invalid value: "), EAutomationExpectedErrorFlags::Contains, 6);
		TestRunner->AddExpectedErrorPlain(TEXT("RmlUi: Invalid substitution, variable '--tw-gradient-stops' not defined."), EAutomationExpectedErrorFlags::Contains, 5);
		ExpectBaseline(TEXT("UtilProbe"));
	}

	TEST_METHOD(VerticalMenu_MatchesItsBaseline) { ExpectBaseline(TEXT("VerticalMenu")); }
};
