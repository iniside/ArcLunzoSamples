// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDocumentWidgetTestTypes.h"
#include "ArcRmlDocumentRef.h"
#include "ArcRmlUITestHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Containers/Ticker.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Host/ArcRmlDocumentWidget.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "RmlUi/Core/Element.h"
#include "RmlUi/Core/ElementDocument.h"
#include "Testing/ArcRmlDocumentWidgetTest.h"
#include "Testing/ArcRmlTestTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace ArcRmlDocumentWidgetTestsPrivate
{
	constexpr int32 MaxPumps = 50;
	const FVector2f FullSize(400.0f, 300.0f);
	const FVector2f SmallSize(200.0f, 100.0f);
	const FName ModelName(TEXT("m"));

	FString MakeDocument(const FString& HeadLines, const FString& BodyAttributes, const FString& BodyLines)
	{
		return FString(TEXT("<rml>\n<head>\n\t<title>Document widget</title>\n"))
			+ TEXT("\t<link type=\"text/rcss\" href=\"DocumentWidget.rcss\"/>\n")
			+ HeadLines
			+ TEXT("</head>\n<body") + BodyAttributes + TEXT(">\n")
			+ BodyLines
			+ TEXT("</body>\n</rml>\n");
	}

	FString MakeModelHead(const TCHAR* Source)
	{
		return FString::Printf(TEXT("\t<viewmodel name=\"m\" class=\"/Script/ArcRmlUITest.ArcRmlDocumentWidgetTestViewModel\" %s/>\n"), Source);
	}

	FString ReadText(Rml::ElementDocument& Document, const char* Id)
	{
		Rml::Element* Element = Document.GetElementById(Id);
		if (!Element)
		{
			return FString(TEXT("<missing>"));
		}
		return FString(UTF8_TO_TCHAR(Element->GetInnerRML().c_str())).TrimStartAndEnd();
	}

	FArcRmlDocumentRef MakeRef(const FString& Path)
	{
		FArcRmlDocumentRef Ref;
		Ref.Path = Path;
		return Ref;
	}

	class FLineCounter : public FOutputDevice
	{
	public:
		FLineCounter(const FName InCategory, const FString& InText)
			: Category(InCategory)
			, Text(InText)
		{
			GLog->AddOutputDevice(this);
		}

		virtual ~FLineCounter() override
		{
			GLog->RemoveOutputDevice(this);
		}

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& InCategory) override
		{
			if (InCategory == Category && FCString::Strstr(V, *Text))
			{
				++Count;
			}
		}

		int32 Read()
		{
			GLog->Flush();
			return Count;
		}

	private:
		FName Category;
		FString Text;
		int32 Count = 0;
	};
}

TEST_CLASS(ArcRmlDocumentWidgetTests, "ArcRmlUI.DocumentWidget")
{
	FString RunDir;
	FString RunPackage;
	TStrongObjectPtr<UArcRmlDocumentWidgetTestHost> Host;
	TStrongObjectPtr<UArcRmlDocumentWidgetTestListener> Listener;
	TStrongObjectPtr<UArcRmlDocumentWidgetTestViewModel> Model;
	UArcRmlDocumentWidget* Widget = nullptr;
	FArcRmlTestProbe Probe;

	BEFORE_EACH()
	{
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::GetTestContentDir().IsEmpty(), TEXT("the test plugin content directory is unknown")));
		const FString RunFolder = FString::Printf(TEXT("DocumentWidget/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RunDir = ArcRmlUITestHelpers::GetTestContentDir() / RunFolder;
		RunPackage = ArcRmlUITestHelpers::MakePath(RunFolder);
		TestRunner->AddExpectedMessagePlain(TEXT("has no owning player"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

		Host.Reset(NewObject<UArcRmlDocumentWidgetTestHost>(GetTransientPackage(), NAME_None, RF_Transient));
		Host->WidgetTree = NewObject<UWidgetTree>(Host.Get(), TEXT("WidgetTree"), RF_Transient);
		Widget = Host->WidgetTree->ConstructWidget<UArcRmlDocumentWidget>(UArcRmlDocumentWidget::StaticClass(), TEXT("Document"));
		Host->WidgetTree->RootWidget = Widget;
		Listener.Reset(NewObject<UArcRmlDocumentWidgetTestListener>(GetTransientPackage(), NAME_None, RF_Transient));
		Model.Reset(NewObject<UArcRmlDocumentWidgetTestViewModel>(GetTransientPackage(), NAME_None, RF_Transient));
	}

	AFTER_EACH()
	{
		if (Widget)
		{
			Widget->ReleaseSlateResources(true);
			Widget = nullptr;
		}
		Host.Reset();
		Listener.Reset();
		Model.Reset();
		if (RunDir.IsEmpty())
		{
			return;
		}
		IFileManager::Get().DeleteDirectory(*RunDir, false, true);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RunDir), false, false);
		RunDir.Reset();
	}

	FString WriteFixture(const FString& Name, const FString& Text)
	{
		const FString SheetFile = RunDir / TEXT("DocumentWidget.rcss");
		if (!IFileManager::Get().FileExists(*SheetFile)
			&& !Assert.IsTrue(FFileHelper::SaveStringToFile(FString(TEXT("body { margin: 0; padding: 0; }\n#fill { display: block; width: 100%; height: 10px; }\n")), *SheetFile,
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM), *SheetFile))
		{
			return FString();
		}
		if (!Assert.IsTrue(FFileHelper::SaveStringToFile(Text, *(RunDir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM), *(RunDir / Name)))
		{
			return FString();
		}
		return RunPackage / Name;
	}

	FString WritePlain(const FString& Name, const TCHAR* Label)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		return WriteFixture(Name, MakeDocument(FString(), FString(),
			FString::Printf(TEXT("\t<div id=\"fill\"></div>\n\t<p id=\"label\">%s</p>\n"), Label)));
	}

	FString WriteModel(const FString& Name, const TCHAR* Source)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		return WriteFixture(Name, MakeDocument(MakeModelHead(Source), TEXT(" data-model=\"m\""), TEXT("\t<p id=\"value\">{{ Text }}</p>\n")));
	}

	FString WriteActions()
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		const FString OpenTarget = RunPackage / TEXT("Other/../Second.rml");
		return WriteFixture(TEXT("Actions.rml"), MakeDocument(FString(), FString(),
			FString::Printf(TEXT("\t<button id=\"open\" data-action=\"open:%s\">Open</button>\n\t<button id=\"close\" data-action=\"close\">Close</button>\n"), *OpenTarget)));
	}

	bool Settle(const FVector2f Size)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		bool bPreviousIdle = false;
		for (int32 Pass = 0; Pass < MaxPumps; ++Pass)
		{
			FTSTicker::GetCoreTicker().Tick(0.0f);
			const bool bIdle = ArcRmlDocumentWidgetTest::Pump(*Widget, Size, Probe) && Probe.bHasDocument && !Probe.bLayoutPending && !Probe.bPendingAsyncWork;
			if (bIdle && bPreviousIdle)
			{
				return true;
			}
			bPreviousIdle = bIdle;
		}
		return false;
	}

	bool RunOnDocument(const FVector2f Size, TFunction<void(Rml::ElementDocument&)> Body)
	{
		return Settle(Size) && ArcRmlDocumentWidgetTest::Pump(*Widget, Size, Probe, MoveTemp(Body)) && Probe.bHasDocument;
	}

	FString ReadOne(const char* Id)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		const TSharedRef<FString> Text = MakeShared<FString>(TEXT("<not read>"));
		RunOnDocument(FullSize, [Text, Id](Rml::ElementDocument& Document)
		{
			*Text = ReadText(Document, Id);
		});
		return *Text;
	}

	void Click(const char* Id)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		const TSharedRef<bool> bClicked = MakeShared<bool>(false);
		ASSERT_THAT(IsTrue(RunOnDocument(FullSize, [bClicked, Id](Rml::ElementDocument& Document)
		{
			if (Rml::Element* Element = Document.GetElementById(Id))
			{
				Element->Click();
				*bClicked = true;
			}
		}), Probe.DocumentPath));
		ASSERT_THAT(IsTrue(*bClicked, FString(UTF8_TO_TCHAR(Id))));
	}

	TEST_METHOD(ZeroSize_LoadsNothing)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		const FString First = WritePlain(TEXT("First.rml"), TEXT("First"));
		Widget->SetDocument(MakeRef(First));
		ASSERT_THAT(IsFalse(ArcRmlDocumentWidgetTest::Pump(*Widget, FVector2f::ZeroVector, Probe), TEXT("a 0 x 0 widget loaded its document")));
		ASSERT_THAT(IsFalse(Probe.bHasDocument));
		ASSERT_THAT(IsTrue(Settle(FullSize), TEXT("the document did not load at 400 x 300")));
		ASSERT_THAT(AreEqual(First, Probe.DocumentPath));
	}

	TEST_METHOD(Load_DocumentTakesWidgetSize)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WritePlain(TEXT("First.rml"), TEXT("First"))));
		const TSharedRef<float> Width = MakeShared<float>(-1.0f);
		const TFunction<void(Rml::ElementDocument&)> ReadWidth = [Width](Rml::ElementDocument& Document)
		{
			Rml::Element* Fill = Document.GetElementById("fill");
			*Width = Fill ? Fill->GetOffsetWidth() : -1.0f;
		};

		ASSERT_THAT(IsTrue(RunOnDocument(FullSize, ReadWidth)));
		ASSERT_THAT(AreEqual(400, Probe.Size.X));
		ASSERT_THAT(AreEqual(300, Probe.Size.Y));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(*Width, 400.0f), FString::Printf(TEXT("fill width %f"), *Width)));

		ASSERT_THAT(IsTrue(RunOnDocument(SmallSize, ReadWidth)));
		ASSERT_THAT(AreEqual(200, Probe.Size.X));
		ASSERT_THAT(AreEqual(100, Probe.Size.Y));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(*Width, 200.0f), FString::Printf(TEXT("fill width %f"), *Width)));
	}

	TEST_METHOD(SetDocument_SwapsDocument)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WritePlain(TEXT("First.rml"), TEXT("First"))));
		const FString Second = WritePlain(TEXT("Second.rml"), TEXT("Second"));
		ASSERT_THAT(AreEqual(FString(TEXT("First")), ReadOne("label")));

		Widget->SetDocument(MakeRef(Second));
		ASSERT_THAT(AreEqual(FString(TEXT("Second")), ReadOne("label")));
		ASSERT_THAT(AreEqual(Second, Probe.DocumentPath));
	}

	TEST_METHOD(SetDocument_EmptyPath_ShowsNothing)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WritePlain(TEXT("First.rml"), TEXT("First"))));
		ASSERT_THAT(IsTrue(Settle(FullSize)));

		Widget->SetDocument(FArcRmlDocumentRef());
		const bool bPumped = ArcRmlDocumentWidgetTest::Pump(*Widget, FullSize, Probe);
		ASSERT_THAT(IsTrue(!bPumped || !Probe.bHasDocument, Probe.DocumentPath));
	}

	TEST_METHOD(SetDocument_SamePath_KeepsDocument)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		const FString First = WritePlain(TEXT("First.rml"), TEXT("First"));
		Widget->SetDocument(MakeRef(First));
		ASSERT_THAT(IsTrue(RunOnDocument(FullSize, [](Rml::ElementDocument& Document)
		{
			if (Rml::Element* Label = Document.GetElementById("label"))
			{
				Label->SetAttribute("data-marker", Rml::String("kept"));
			}
		})));

		Widget->SetDocument(MakeRef(First));
		const TSharedRef<FString> Marker = MakeShared<FString>();
		ASSERT_THAT(IsTrue(RunOnDocument(FullSize, [Marker](Rml::ElementDocument& Document)
		{
			if (Rml::Element* Label = Document.GetElementById("label"))
			{
				*Marker = UTF8_TO_TCHAR(Label->GetAttribute<Rml::String>("data-marker", Rml::String()).c_str());
			}
		})));
		ASSERT_THAT(AreEqual(FString(TEXT("kept")), *Marker));
	}

	TEST_METHOD(ReloadDocument_LoadsAgain)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WritePlain(TEXT("First.rml"), TEXT("First"))));
		ASSERT_THAT(IsTrue(RunOnDocument(FullSize, [](Rml::ElementDocument& Document)
		{
			if (Rml::Element* Label = Document.GetElementById("label"))
			{
				Label->SetAttribute("data-marker", Rml::String("kept"));
			}
		})));

		Widget->ReloadDocument();
		const TSharedRef<FString> Marker = MakeShared<FString>(TEXT("<not read>"));
		ASSERT_THAT(IsTrue(RunOnDocument(FullSize, [Marker](Rml::ElementDocument& Document)
		{
			Rml::Element* Label = Document.GetElementById("label");
			*Marker = Label ? FString(UTF8_TO_TCHAR(Label->GetAttribute<Rml::String>("data-marker", Rml::String()).c_str())) : FString(TEXT("<missing>"));
		})));
		ASSERT_THAT(AreEqual(FString(), *Marker));
	}

	TEST_METHOD(SetViewModel_BeforeLoad_ShowsValue)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WriteModel(TEXT("Model.rml"), TEXT("manual=\"true\""))));
		Model->SetText(TEXT("Before load"));
		Widget->SetViewModel(ModelName, Model.Get());
		ASSERT_THAT(AreEqual(FString(TEXT("Before load")), ReadOne("value")));
	}

	TEST_METHOD(SetViewModel_AfterLoad_ShowsValue)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WriteModel(TEXT("Model.rml"), TEXT("manual=\"true\""))));
		ASSERT_THAT(IsTrue(Settle(FullSize)));
		Model->SetText(TEXT("After load"));
		Widget->SetViewModel(ModelName, Model.Get());
		ASSERT_THAT(AreEqual(FString(TEXT("After load")), ReadOne("value")));
	}

	TEST_METHOD(SetViewModel_SurvivesReloadAndSetDocument)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WriteModel(TEXT("Model.rml"), TEXT("manual=\"true\""))));
		const FString Other = WriteModel(TEXT("ModelOther.rml"), TEXT("manual=\"true\""));
		Model->SetText(TEXT("Stored"));
		Widget->SetViewModel(ModelName, Model.Get());
		ASSERT_THAT(AreEqual(FString(TEXT("Stored")), ReadOne("value")));

		Widget->ReloadDocument();
		ASSERT_THAT(AreEqual(FString(TEXT("Stored")), ReadOne("value")));

		Widget->SetDocument(MakeRef(Other));
		ASSERT_THAT(AreEqual(FString(TEXT("Stored")), ReadOne("value")));
		ASSERT_THAT(AreEqual(Other, Probe.DocumentPath));
	}

	TEST_METHOD(SetViewModel_UndeclaredName_Warns)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WritePlain(TEXT("First.rml"), TEXT("First"))));
		ASSERT_THAT(IsTrue(Settle(FullSize)));
		TestRunner->AddExpectedMessagePlain(TEXT("declares no view model 'missing'"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Widget->SetViewModel(FName(TEXT("missing")), Model.Get());
	}

	TEST_METHOD(OpenAction_BroadcastsNormalisedPath)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WriteActions()));
		Widget->OnOpenRequested.AddDynamic(Listener.Get(), &UArcRmlDocumentWidgetTestListener::HandleOpenRequested);
		Click("open");
		ASSERT_THAT(AreEqual(1, Listener->OpenPaths.Num()));
		ASSERT_THAT(AreEqual(RunPackage / TEXT("Second.rml"), Listener->OpenPaths[0]));
	}

	TEST_METHOD(CloseAction_Broadcasts)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WriteActions()));
		Widget->OnCloseRequested.AddDynamic(Listener.Get(), &UArcRmlDocumentWidgetTestListener::HandleCloseRequested);
		Click("close");
		ASSERT_THAT(AreEqual(1, Listener->CloseCount));
	}

	TEST_METHOD(OpenAction_NoListener_Warns)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WriteActions()));
		TestRunner->AddExpectedMessagePlain(TEXT("has no OnOpenRequested listener for open:"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Click("open");
	}

	TEST_METHOD(CloseAction_NoListener_Warns)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WriteActions()));
		TestRunner->AddExpectedMessagePlain(TEXT("has no OnCloseRequested listener for close"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Click("close");
	}

	TEST_METHOD(OnClose_RunsOnSetDocumentAndReload)
	{
		using namespace ArcRmlDocumentWidgetTestsPrivate;
		Widget->SetDocument(MakeRef(WriteModel(TEXT("ModelOnClose.rml"), TEXT("manual=\"true\" on-close=\"Closed\""))));
		const FString Second = WritePlain(TEXT("Second.rml"), TEXT("Second"));
		Widget->SetViewModel(ModelName, Model.Get());
		ASSERT_THAT(IsTrue(Settle(FullSize)));
		FLineCounter Counter(FName(TEXT("LogArcRmlUI")), TEXT("[preview] call('', 'Closed') on 'm'"));

		Widget->ReloadDocument();
		ASSERT_THAT(IsTrue(Settle(FullSize)));
		Widget->SetDocument(MakeRef(Second));
		ASSERT_THAT(IsTrue(Settle(FullSize)));
		ASSERT_THAT(AreEqual(2, Counter.Read()));
	}
};
