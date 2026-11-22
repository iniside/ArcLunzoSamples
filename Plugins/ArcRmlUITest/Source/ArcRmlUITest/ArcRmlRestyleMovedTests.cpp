// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

#include "RmlUi/Core/ComputedValues.h"
#include "RmlUi/Core/Element.h"
#include "RmlUi/Core/ElementDocument.h"

namespace ArcRmlRestyleMovedTestsPrivate
{
	struct FMovedStyle
	{
		bool bFound = false;
		float MarginLeft = -1.0f;
		float KidPaddingLeft = -1.0f;
		Rml::Colourb Background;
	};

	FMovedStyle ReadStyle(Rml::ElementDocument& Document, const char* Id, const char* KidId)
	{
		FMovedStyle Style;
		Rml::Element* Element = Document.GetElementById(Id);
		if (!Element)
		{
			return Style;
		}
		Style.bFound = true;
		const Rml::Style::ComputedValues& Values = Element->GetComputedValues();
		Style.MarginLeft = Values.margin_left().value;
		Style.Background = Values.background_color();
		if (KidId)
		{
			if (Rml::Element* Kid = Document.GetElementById(KidId))
			{
				Style.KidPaddingLeft = Kid->GetComputedValues().padding_left().value;
			}
		}
		return Style;
	}

	Rml::Element* AppendNewDiv(Rml::ElementDocument& Document, Rml::Element& Parent, const char* Classes)
	{
		Rml::ElementPtr Div = Document.CreateElement("div");
		if (Classes)
		{
			Div->SetClassNames(Classes);
		}
		return Parent.AppendChild(std::move(Div));
	}

	Rml::ElementPtr Detach(Rml::ElementDocument& Document, const char* Id)
	{
		Rml::Element* Element = Document.GetElementById(Id);
		return Element && Element->GetParentNode() ? Element->GetParentNode()->RemoveChild(Element) : Rml::ElementPtr();
	}
}

TEST_CLASS(ArcRmlRestyleMovedTests, "ArcRmlUI.Restyle.Moved")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool Open()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Restyle/Moved.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	bool Settle()
	{
		return View.Flush() && View.Flush() && View.FlushUntilSettled();
	}

	ArcRmlRestyleMovedTestsPrivate::FMovedStyle Read(const char* Id, const char* KidId)
	{
		const TSharedRef<ArcRmlRestyleMovedTestsPrivate::FMovedStyle, ESPMode::ThreadSafe> Style =
			MakeShared<ArcRmlRestyleMovedTestsPrivate::FMovedStyle, ESPMode::ThreadSafe>();
		if (!View.RunOnDocument([Style, Id, KidId](Rml::ElementDocument& Document) { *Style = ArcRmlRestyleMovedTestsPrivate::ReadStyle(Document, Id, KidId); }))
		{
			return ArcRmlRestyleMovedTestsPrivate::FMovedStyle();
		}
		return *Style;
	}

	void ExpectStyle(const TCHAR* Moment, const ArcRmlRestyleMovedTestsPrivate::FMovedStyle& Style, float Margin, const Rml::Colourb& Background)
	{
		ASSERT_THAT(IsTrue(Style.bFound, FString::Printf(TEXT("%s: element not found"), Moment)));
		ASSERT_THAT(IsNear(Margin, Style.MarginLeft, 0.001f, FString::Printf(TEXT("%s: margin-left"), Moment)));
		ASSERT_THAT(IsNear(Margin, Style.KidPaddingLeft, 0.001f, FString::Printf(TEXT("%s: child padding-left"), Moment)));
		ASSERT_THAT(IsTrue(Style.Background == Background,
			FString::Printf(TEXT("%s: background-color (%d,%d,%d,%d)"), Moment, Style.Background.red, Style.Background.green, Style.Background.blue,
				Style.Background.alpha)));
	}

	TEST_METHOD(ComputedSubtreeMovedUnderNewThemedParent_TakesTheThemeValues)
	{
		using namespace ArcRmlRestyleMovedTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectStyle(TEXT("baseline"), Read("c1", "c1k"), 1.0f, Rml::Colourb(0, 0, 255, 255));

		ASSERT_THAT(IsTrue(View.RunOnDocument([](Rml::ElementDocument& Document) {
			Rml::ElementPtr Moved = Detach(Document, "c1");
			Rml::ElementPtr Theme = Document.CreateElement("div");
			Theme->SetClassNames("theme");
			Theme->AppendChild(std::move(Moved));
			Document.GetElementById("b")->AppendChild(std::move(Theme));
		})));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectStyle(TEXT("moved under a new themed parent"), Read("c1", "c1k"), 20.0f, Rml::Colourb(255, 0, 0, 255));
	}

	TEST_METHOD(ComputedSubtreeMovedTwoNewLevelsBelowThemedParent_TakesTheThemeValues)
	{
		using namespace ArcRmlRestyleMovedTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectStyle(TEXT("baseline"), Read("c2", "c2k"), 1.0f, Rml::Colourb(0, 0, 255, 255));

		ASSERT_THAT(IsTrue(View.RunOnDocument([](Rml::ElementDocument& Document) {
			Rml::ElementPtr Wrapper = Document.CreateElement("div");
			Wrapper->AppendChild(Detach(Document, "c2"));
			Rml::ElementPtr Theme = Document.CreateElement("div");
			Theme->SetClassNames("theme");
			Theme->AppendChild(std::move(Wrapper));
			Document.GetElementById("b")->AppendChild(std::move(Theme));
		})));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectStyle(TEXT("moved two new levels below a themed parent"), Read("c2", "c2k"), 20.0f, Rml::Colourb(255, 0, 0, 255));
	}

	TEST_METHOD(ComputedSubtreeMovedUnderNewWrappersOfAChangingAncestor_TakesTheNewValue)
	{
		using namespace ArcRmlRestyleMovedTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectStyle(TEXT("baseline"), Read("c3", "c3k"), 1.0f, Rml::Colourb(0, 0, 255, 255));

		ASSERT_THAT(IsTrue(View.RunOnDocument([](Rml::ElementDocument& Document) {
			Rml::Element* Host = Document.GetElementById("host");
			if (!Host)
			{
				return;
			}
			Host->SetProperty("--gap", "30px");
			Rml::ElementPtr Inner = Document.CreateElement("div");
			Inner->AppendChild(Detach(Document, "c3"));
			Rml::ElementPtr Outer = Document.CreateElement("div");
			Outer->AppendChild(std::move(Inner));
			Host->AppendChild(std::move(Outer));
		})));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectStyle(TEXT("moved under new wrappers while the ancestor variable changed"), Read("c3", "c3k"), 30.0f, Rml::Colourb(0, 0, 255, 255));
	}

	TEST_METHOD(ComputedSubtreeMovedUnderExistingThemedParent_TakesItsValues)
	{
		using namespace ArcRmlRestyleMovedTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectStyle(TEXT("baseline"), Read("c1", "c1k"), 1.0f, Rml::Colourb(0, 0, 255, 255));

		ASSERT_THAT(IsTrue(View.RunOnDocument([](Rml::ElementDocument& Document) {
			Rml::ElementPtr Moved = Detach(Document, "c1");
			if (Rml::Element* Warm = Document.GetElementById("warm"))
			{
				Warm->AppendChild(std::move(Moved));
			}
		})));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectStyle(TEXT("moved under an existing themed parent"), Read("c1", "c1k"), 12.0f, Rml::Colourb(0, 128, 0, 255));
	}

	TEST_METHOD(ComputedSubtreeMovedUnderExistingWrapperOfThemedParent_TakesItsValues)
	{
		using namespace ArcRmlRestyleMovedTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectStyle(TEXT("baseline"), Read("c2", "c2k"), 1.0f, Rml::Colourb(0, 0, 255, 255));

		ASSERT_THAT(IsTrue(View.RunOnDocument([](Rml::ElementDocument& Document) {
			Rml::ElementPtr Moved = Detach(Document, "c2");
			if (Rml::Element* Wrapper = Document.GetElementById("warmwrap"))
			{
				Wrapper->AppendChild(std::move(Moved));
			}
		})));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectStyle(TEXT("moved under an existing wrapper of a themed parent"), Read("c2", "c2k"), 12.0f, Rml::Colourb(0, 128, 0, 255));
	}

	TEST_METHOD(NewElementsUnderThemedAndPlainParents_TakeTheirValues)
	{
		using namespace ArcRmlRestyleMovedTestsPrivate;
		ASSERT_THAT(IsTrue(Open(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(View.RunOnDocument([](Rml::ElementDocument& Document) {
			Rml::Element* Theme = AppendNewDiv(Document, *Document.GetElementById("b"), "theme");
			Rml::Element* Wrapper = AppendNewDiv(Document, *Theme, nullptr);
			Rml::Element* Themed = AppendNewDiv(Document, *Wrapper, "c");
			Themed->SetId("n1");
			AppendNewDiv(Document, *Themed, "k")->SetId("n1k");
			Rml::Element* Plain = AppendNewDiv(Document, *Document.GetElementById("b"), "c");
			Plain->SetId("n2");
			AppendNewDiv(Document, *Plain, "k")->SetId("n2k");
		})));
		ASSERT_THAT(IsTrue(Settle()));
		ExpectStyle(TEXT("new element under a themed parent"), Read("n1", "n1k"), 20.0f, Rml::Colourb(255, 0, 0, 255));
		ExpectStyle(TEXT("new element under the body"), Read("n2", "n2k"), 1.0f, Rml::Colourb(0, 0, 255, 255));
	}
};
