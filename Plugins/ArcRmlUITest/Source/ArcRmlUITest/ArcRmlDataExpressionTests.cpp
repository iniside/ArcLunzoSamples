// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"

#include <atomic>

#include "RmlUi/Core/Context.h"
#include "RmlUi/Core/DataModelHandle.h"
#include "RmlUi/Core/Element.h"
#include "RmlUi/Core/ElementDocument.h"

namespace ArcRmlDataExpressionTestsPrivate
{
	const TCHAR* const ModelName = TEXT("expr");
	const TCHAR* const Fixture = TEXT("Expression/ExpressionTest.rml");
	const TCHAR* const CrossModelFixture = TEXT("Expression/CrossModelTest.rml");
	const TCHAR* const LateModelFixture = TEXT("Expression/LateModelTest.rml");
	const char* const LateModelName = "xlate";
	const TCHAR* const MissingVariableWarning = TEXT("Could not find variable name");

	class FLogLineCounter final : public FOutputDevice
	{
	public:
		explicit FLogLineCounter(const TCHAR* InNeedle)
			: Needle(InNeedle)
		{
			GLog->AddOutputDevice(this);
		}

		virtual ~FLogLineCounter() override
		{
			GLog->RemoveOutputDevice(this);
		}

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (FCString::Strstr(Message, *Needle))
			{
				++Count;
			}
		}

		virtual bool CanBeUsedOnAnyThread() const override
		{
			return true;
		}

		virtual bool CanBeUsedOnMultipleThreads() const override
		{
			return true;
		}

		int32 GetCount() const
		{
			return Count.load();
		}

	private:
		FString Needle;
		std::atomic<int32> Count{0};
	};

	FArcRmlValue MakeInt(int64 Number)
	{
		FArcRmlValue Value;
		Value.Kind = EArcRmlValueKind::Int;
		Value.Int = Number;
		return Value;
	}
}

TEST_CLASS(ArcRmlDataExpressionTests, "ArcRmlUI.DataExpression")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool Open(bool bDropCompiled, const TCHAR* FixturePath = ArcRmlDataExpressionTestsPrivate::Fixture)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FixturePath);
		return (!bDropCompiled || FArcRmlTestView::DropCompiledDocument(Path)) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled()
			&& View.GetProbe().bHasDocument;
	}

	FString ReadText(const char* Id)
	{
		TSharedRef<FString> Text = MakeShared<FString>();
		const bool bAnswered = View.RunOnDocument([Text, Id](Rml::ElementDocument& Document)
		{
			Rml::Element* Element = Document.GetElementById(Id);
			*Text = Element ? FString(UTF8_TO_TCHAR(Element->GetInnerRML().c_str())).TrimStartAndEnd() : FString(TEXT("missing"));
		});
		return bAnswered ? *Text : FString(TEXT("the document did not answer"));
	}

	void ExpectText(const char* Id, const TCHAR* Expected)
	{
		const FString Actual = ReadText(Id);
		ASSERT_THAT(AreEqual(FString(Expected), Actual, FString::Printf(TEXT("#%s"), UTF8_TO_TCHAR(Id))));
	}

	FString ReadRows()
	{
		TSharedRef<FString> Text = MakeShared<FString>();
		const bool bAnswered = View.RunOnDocument([Text](Rml::ElementDocument& Document)
		{
			Rml::ElementList Rows;
			Document.QuerySelectorAll(Rows, ".xrow");
			TArray<FString> Names;
			for (Rml::Element* Row : Rows)
			{
				Names.Add(FString(UTF8_TO_TCHAR(Row->GetInnerRML().c_str())).TrimStartAndEnd());
			}
			*Text = FString::Join(Names, TEXT(","));
		});
		return bAnswered ? *Text : FString(TEXT("the document did not answer"));
	}

	void ExpectRows(const TCHAR* Expected)
	{
		ASSERT_THAT(AreEqual(FString(Expected), ReadRows(), TEXT(".xrow")));
	}

	void RemoveFirstItems(int32 Count)
	{
		TArray<FArcRmlListOp> Ops;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FArcRmlListOp& Remove = Ops.AddDefaulted_GetRef();
			Remove.Type = EArcRmlListOpType::Remove;
			Remove.Index = 0;
		}
		View.ApplyListOps(TEXT("xb"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Items")), MoveTemp(Ops));
	}

	TEST_METHOD(CrossModel_TextReadsAnotherModelAndOwnNamesShadowIt)
	{
		using namespace ArcRmlDataExpressionTestsPrivate;
		const FLogLineCounter Missing(MissingVariableWarning);
		ASSERT_THAT(IsTrue(Open(true, CrossModelFixture), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectText("own", TEXT("Own"));
		ExpectText("foreign", TEXT("B label"));
		ExpectText("foreign-expr", TEXT("20"));
		ExpectText("shadow", TEXT("A field"));
		ExpectText("row-count", TEXT("3"));
		ASSERT_THAT(AreEqual(0, Missing.GetCount(), MissingVariableWarning));
	}

	TEST_METHOD(CrossModel_ChangeOfTheOtherModelUpdatesTheReader)
	{
		using namespace ArcRmlDataExpressionTestsPrivate;
		const FLogLineCounter Missing(MissingVariableWarning);
		ASSERT_THAT(IsTrue(Open(true, CrossModelFixture), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetModelValue(TEXT("xb"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Label")), ArcRmlUITestHelpers::MakeString(TEXT("changed")));
		View.SetModelValue(TEXT("xb"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Count")), MakeInt(5));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ExpectText("foreign", TEXT("changed"));
		ExpectText("foreign-expr", TEXT("50"));
		ExpectText("own", TEXT("Own"));
		ASSERT_THAT(AreEqual(0, Missing.GetCount(), MissingVariableWarning));
	}

	TEST_METHOD(CrossModel_DataForOverTheOtherModelsListFollowsItsRows)
	{
		using namespace ArcRmlDataExpressionTestsPrivate;
		const FLogLineCounter Missing(MissingVariableWarning);
		ASSERT_THAT(IsTrue(Open(true, CrossModelFixture), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectRows(TEXT("one,two,three"));

		RemoveFirstItems(2);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ExpectRows(TEXT("three"));
		ExpectText("row-count", TEXT("1"));

		View.SetModelValue(TEXT("xb"), ArcRmlUITestHelpers::MakeRowFieldPath(TEXT("Items"), 0, TEXT("Name")), ArcRmlUITestHelpers::MakeString(TEXT("renamed")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ExpectRows(TEXT("renamed"));

		RemoveFirstItems(1);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ExpectRows(TEXT(""));
		ExpectText("row-count", TEXT("0"));
		ASSERT_THAT(AreEqual(0, Missing.GetCount(), MissingVariableWarning));
	}

	TEST_METHOD(CrossModel_ModelCreatedAfterTheLoadShowsInReadersParsedBeforeIt)
	{
		using namespace ArcRmlDataExpressionTestsPrivate;
		const FLogLineCounter Missing(MissingVariableWarning);
		ASSERT_THAT(IsTrue(Open(true, LateModelFixture), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectText("own", TEXT("Own"));
		ExpectText("late", TEXT(""));

		TSharedRef<Rml::String> Label = MakeShared<Rml::String>("late label");
		ASSERT_THAT(IsTrue(View.RunOnDocument([Label](Rml::ElementDocument& Document)
		{
			Rml::DataModelConstructor Constructor = Document.GetContext()->CreateDataModel(LateModelName);
			if (Constructor)
			{
				Constructor.BindFunc("Label", [Label](Rml::Variant& Out) { Out = *Label; });
			}
		})));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		const FString Created = ReadText("late");
		const FString CreatedLower = ReadText("late-lower");

		ASSERT_THAT(IsTrue(View.RunOnDocument([Label](Rml::ElementDocument& Document)
		{
			*Label = "changed";
			Document.GetContext()->GetDataModel(LateModelName).GetModelHandle().DirtyVariable("Label");
		})));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		const FString Changed = ReadText("late");
		const FString ChangedLower = ReadText("late-lower");
		View.RunOnDocument([](Rml::ElementDocument& Document) { Document.GetContext()->RemoveDataModel(LateModelName); });

		ASSERT_THAT(AreEqual(FString(TEXT("late label")), Created, TEXT("#late after the model is created")));
		ASSERT_THAT(AreEqual(FString(TEXT("late label")), CreatedLower, TEXT("#late-lower after the model is created")));
		ASSERT_THAT(AreEqual(FString(TEXT("changed")), Changed, TEXT("#late after the field changes")));
		ASSERT_THAT(AreEqual(FString(TEXT("changed")), ChangedLower, TEXT("#late-lower after the field changes")));
		ASSERT_THAT(AreEqual(0, Missing.GetCount(), MissingVariableWarning));
	}

	TEST_METHOD(Modulo_IsTheJsRemainderAtTermPrecedence)
	{
		ASSERT_THAT(IsTrue(Open(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectText("mod-literal", TEXT("1"));
		ExpectText("mod-int", TEXT("1"));
		ExpectText("mod-frac", TEXT("1.5"));
		ExpectText("mod-neg", TEXT("-1"));
		ExpectText("mod-prec", TEXT("3"));
	}

	TEST_METHOD(FloorAndCeil_RoundTowardsTheirSide)
	{
		ASSERT_THAT(IsTrue(Open(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectText("ceil", TEXT("3"));
		ExpectText("floor", TEXT("-3"));
	}

	TEST_METHOD(Abs_DropsTheSign)
	{
		ASSERT_THAT(IsTrue(Open(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectText("abs-neg", TEXT("2.2"));
		ExpectText("abs-pos", TEXT("2.2"));
		ExpectText("abs-int", TEXT("7"));
	}

	TEST_METHOD(Length_CountsCodePoints)
	{
		ASSERT_THAT(IsTrue(Open(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectText("length-literal", TEXT("3"));
		ExpectText("length-field", TEXT("3"));
		ExpectText("length-empty", TEXT("0"));
	}

	TEST_METHOD(CompiledModuloProgram_RunsAfterLoadingTheBinary)
	{
		using namespace ArcRmlDataExpressionTestsPrivate;
		ASSERT_THAT(IsTrue(Open(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Binary = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), FString(Fixture) + TEXT("c"));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*Binary), FString::Printf(TEXT("no compiled document at %s"), *Binary)));
		View.Close();

		ASSERT_THAT(IsTrue(Open(false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectText("mod-int", TEXT("1"));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("Seven")), MakeInt(8));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ExpectText("mod-int", TEXT("2"));
		ExpectText("mod-prec", TEXT("5"));
	}
};
