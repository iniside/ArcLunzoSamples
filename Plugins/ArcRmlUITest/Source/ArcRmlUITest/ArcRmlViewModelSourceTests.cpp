// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlKeptTabTestHelpers.h"
#include "ArcRmlKeptTabTestTypes.h"
#include "ArcRmlValidateTestTypes.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Host/ArcRmlTabEmbed.h"
#include "Host/ArcRmlViewportSubsystem.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "MVVM/ArcRmlModelBinding.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace ArcRmlViewModelSourceTestsPrivate
{
	const TCHAR* const ModelClass = TEXT("/Script/ArcRmlUITest.ArcRmlKeptTabViewModel");
	const TCHAR* const OtherModelClass = TEXT("/Script/ArcRmlUITest.ArcRmlValidateTestViewModel");
	const TCHAR* const CommonSheet = TEXT("<link type=\"text/rcss\" href=\"/ArcRmlUITest/UI/Rml/Test/Common.rcss\"/>");

	FString MakeDocument(const TArray<FString>& HeadLines, const FString& BodyAttributes, const TArray<FString>& BodyLines)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<rml>"));
		Lines.Add(TEXT("<head>"));
		Lines.Add(TEXT("<title>View Model Sources</title>"));
		Lines.Append(HeadLines);
		Lines.Add(CommonSheet);
		Lines.Add(TEXT("</head>"));
		Lines.Add(FString::Printf(TEXT("<body%s>"), *BodyAttributes));
		Lines.Append(BodyLines);
		Lines.Add(TEXT("</body>"));
		Lines.Add(TEXT("</rml>"));
		return FString::Join(Lines, TEXT("\n"));
	}

	TArray<FString> MenuMeta(bool bKeepAlive)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<meta name=\"layer\" content=\"UI.Layer.Menu\"/>"));
		Lines.Add(TEXT("<meta name=\"input-mode\" content=\"menu\"/>"));
		if (bKeepAlive)
		{
			Lines.Add(TEXT("<meta name=\"keep-alive\" content=\"true\"/>"));
		}
		return Lines;
	}

	FString MakeModelTag(const TCHAR* Name, const TCHAR* Class, const TCHAR* Source)
	{
		return FString::Printf(TEXT("<viewmodel name=\"%s\" class=\"%s\" %s/>"), Name, Class, Source);
	}

	TArray<FString> MakeReadLines(const FString& Prefix)
	{
		TArray<FString> Lines;
		Lines.Add(FString::Printf(TEXT("<div id=\"%s-count\">{{ Count }}</div>"), *Prefix));
		Lines.Add(FString::Printf(TEXT("<div id=\"%s-resolved\">{{ ArcResolved ? 'yes' : 'no' }}</div>"), *Prefix));
		return Lines;
	}

	FArcRmlInputBindings MakeBindings()
	{
		FArcRmlInputBindings Bindings;
		Bindings.Confirm.Name = TEXT("confirm");
		Bindings.Confirm.ActionName = TEXT("IA_TestConfirm");
		Bindings.Confirm.Keys.Add(EKeys::Enter);
		Bindings.Back.Name = TEXT("back");
		Bindings.Back.ActionName = TEXT("IA_TestBack");
		Bindings.Back.Keys.Add(EKeys::Escape);
		Bindings.CloseKeys.Add(EKeys::Tab);
		return Bindings;
	}

	FString ResolvedAtStartLine(const FString& DocumentPath, const TCHAR* ModelName)
	{
		return FString::Printf(TEXT("'%s' view model '%s' resolved to"), *DocumentPath, ModelName);
	}

	FString SetAfterStartLine(const FString& DocumentPath, const TCHAR* ModelName)
	{
		return FString::Printf(TEXT("'%s' view model '%s' set manually to"), *DocumentPath, ModelName);
	}
}

TEST_CLASS(ArcRmlViewModelSourceTests, "ArcRmlUI.ViewModelSources")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;
	FString RunFolder;
	FString RunDir;

	BEFORE_EACH()
	{
		RunFolder = FString::Printf(TEXT("ViewModels/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RunDir = ArcRmlUITestHelpers::GetTestContentDir() / RunFolder;
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::GetTestContentDir().IsEmpty(), TEXT("the test plugin content directory is unknown")));
	}

	AFTER_EACH()
	{
		View.Close();
		if (RunDir.IsEmpty())
		{
			return;
		}
		IFileManager::Get().DeleteDirectory(*RunDir, false, true);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RunDir), false, false);
		RunDir.Reset();
	}

	FString PathOf(const TCHAR* Name) const
	{
		return ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/") + Name);
	}

	bool WriteFile(const TCHAR* Name, const FString& Text)
	{
		return Assert.IsTrue(FFileHelper::SaveStringToFile(Text, *(RunDir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM),
			TEXT("the fixture document could not be written"));
	}

	bool WriteManualDocument()
	{
		using namespace ArcRmlViewModelSourceTestsPrivate;
		TArray<FString> Head = MenuMeta(false);
		Head.Add(MakeModelTag(TEXT("m"), ModelClass, TEXT("manual")));
		return WriteFile(TEXT("Manual.rml"), MakeDocument(Head, TEXT(" data-model=\"m\""), MakeReadLines(TEXT("m"))));
	}

	bool WriteCreatedDocument()
	{
		using namespace ArcRmlViewModelSourceTestsPrivate;
		TArray<FString> Head = MenuMeta(true);
		Head.Add(MakeModelTag(TEXT("c"), ModelClass, TEXT("create")));
		return WriteFile(TEXT("Created.rml"), MakeDocument(Head, TEXT(" data-model=\"c\""), MakeReadLines(TEXT("c"))));
	}

	bool WriteFrameAndTab()
	{
		using namespace ArcRmlViewModelSourceTestsPrivate;
		TArray<FString> FrameHead = MenuMeta(false);
		FrameHead.Add(MakeModelTag(TEXT("fm"), ModelClass, TEXT("manual")));
		TArray<FString> FrameBody;
		FrameBody.Add(TEXT("<div id=\"embed\" arc-embed=\"\" style=\"width: 600px; height: 400px;\"></div>"));
		TArray<FString> TabHead;
		TabHead.Add(FString::Printf(TEXT("<meta name=\"frame\" content=\"%s\"/>"), *PathOf(TEXT("Frame.rml"))));
		TabHead.Add(MakeModelTag(TEXT("tm"), ModelClass, TEXT("manual")));
		TArray<FString> TabBody = MakeReadLines(TEXT("t"));
		TabBody.Add(TEXT("<div data-model=\"fm\"><div id=\"f-count\">{{ Count }}</div></div>"));
		return WriteFile(TEXT("Frame.rml"), MakeDocument(FrameHead, TEXT(" data-model=\"fm\""), FrameBody))
			&& WriteFile(TEXT("Tab.rml"), MakeDocument(TabHead, TEXT(" data-model=\"tm\""), TabBody));
	}

	bool Read()
	{
		return View.Flush() && View.ParseLayout(Elements);
	}

	bool Open(const TCHAR* Name)
	{
		View.SetInputBindings(ArcRmlViewModelSourceTestsPrivate::MakeBindings());
		return Assert.IsTrue(ArcRmlUITestHelpers::OpenView(View, PathOf(Name)), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View))
			&& Assert.IsTrue(View.FlushUntilSettled() && View.Flush(), ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Read();
	}

	bool Pump()
	{
		FTSTicker::GetCoreTicker().Tick(0.0f);
		return View.Flush() && View.Flush() && Read();
	}

	FString TextOf(const TCHAR* DomId) const
	{
		return ArcRmlUITestHelpers::GetText(Elements, DomId);
	}

	void SetFrameObject(const TCHAR* ModelName, UObject* ViewModel)
	{
		View.SwapWith(View.GetProbe().EmbeddedPath, [ModelName, ViewModel](const TSharedRef<FArcRmlUIThread>&, uint32, FArcRmlModelBinding& FrameBinding, FArcRmlTabModels&) -> bool
		{
			FrameBinding.SetViewModelObject(ModelName, ViewModel);
			return false;
		});
	}

	UObject* GetFrameObject(const TCHAR* ModelName)
	{
		UObject* Result = nullptr;
		View.SwapWith(View.GetProbe().EmbeddedPath, [ModelName, &Result](const TSharedRef<FArcRmlUIThread>&, uint32, FArcRmlModelBinding& FrameBinding, FArcRmlTabModels&) -> bool
		{
			Result = FrameBinding.GetViewModelObject(ModelName);
			return false;
		});
		return Result;
	}

	bool CloseAndWait()
	{
		View.PressKey(EKeys::Tab);
		for (int32 Attempt = 0; Attempt < 300; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			if (!View.Flush())
			{
				return false;
			}
			if (View.IsParked() || View.GetProbe().bRemoved)
			{
				return true;
			}
		}
		return Assert.IsTrue(false, TEXT("the leave never ended"));
	}

	TEST_METHOD(ManualAssignmentBeforeStart_IsInTheFirstSnapshot)
	{
		using namespace ArcRmlViewModelSourceTestsPrivate;
		ASSERT_THAT(IsTrue(WriteFrameAndTab()));
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		Model->SetCount(4);
		ASSERT_THAT(IsTrue(Open(TEXT("Frame.rml"))));
		UArcRmlKeptTabViewModel* const ModelObject = Model.Get();
		View.SwapWith(PathOf(TEXT("Tab.rml")), [ModelObject](const TSharedRef<FArcRmlUIThread>&, uint32, FArcRmlModelBinding&, FArcRmlTabModels& TabModels) -> bool
		{
			TabModels.SetOnBindingCreated(FArcRmlOnTabBindingCreated::CreateLambda([ModelObject](FArcRmlModelBinding& TabBinding)
			{
				if (TabBinding.AcceptsAssignment(TEXT("tm")))
				{
					TabBinding.SetViewModelObject(TEXT("tm"), ModelObject);
				}
			}));
			return false;
		});

		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		ASSERT_THAT(IsTrue(View.Embed(PathOf(TEXT("Tab.rml"))), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && Read()));
		const FString TabPath = PathOf(TEXT("Tab.rml"));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(*ResolvedAtStartLine(TabPath, TEXT("tm"))), Log.Describe()));
		ASSERT_THAT(AreEqual(0, Log.CountContaining(*SetAfterStartLine(TabPath, TEXT("tm"))), Log.Describe()));
		ASSERT_THAT(AreEqual(FString(TEXT("4")), TextOf(TEXT("t-count")), TEXT("the tab does not show the object assigned before its binding started")));
		ASSERT_THAT(AreEqual(FString(TEXT("yes")), TextOf(TEXT("t-resolved")), TEXT("the model assigned before its binding started is not resolved")));
	}

	TEST_METHOD(ManualAssignmentAfterStart_RerunsTheBindingsAndSetsArcResolved)
	{
		ASSERT_THAT(IsTrue(WriteManualDocument()));
		ASSERT_THAT(IsTrue(Open(TEXT("Manual.rml"))));
		ASSERT_THAT(AreEqual(FString(TEXT("no")), TextOf(TEXT("m-resolved")), TEXT("a manual model with no assignment is resolved")));
		ASSERT_THAT(AreEqual(FString(TEXT("0")), TextOf(TEXT("m-count")), TEXT("an unresolved manual model does not show its class defaults")));

		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		Model->SetCount(5);
		ArcRmlKeptTabTestHelpers::FLogCapture Log;
		SetFrameObject(TEXT("m"), Model.Get());
		ASSERT_THAT(IsTrue(Pump()));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(*ArcRmlViewModelSourceTestsPrivate::SetAfterStartLine(PathOf(TEXT("Manual.rml")), TEXT("m"))), Log.Describe()));
		ASSERT_THAT(AreEqual(FString(TEXT("5")), TextOf(TEXT("m-count")), TEXT("the bindings did not run again for the assigned object")));
		ASSERT_THAT(AreEqual(FString(TEXT("yes")), TextOf(TEXT("m-resolved")), TEXT("the assignment did not set ArcResolved")));

		Model->SetCount(6);
		ASSERT_THAT(IsTrue(Pump()));
		ASSERT_THAT(AreEqual(FString(TEXT("6")), TextOf(TEXT("m-count")), TEXT("a field change of the assigned object does not reach the document")));
	}

	TEST_METHOD(CreateInstance_KeepsItsIdentityAcrossParkAndReopen_AndIsNewAfterAReload)
	{
		ASSERT_THAT(IsTrue(WriteCreatedDocument()));
		ASSERT_THAT(IsTrue(Open(TEXT("Created.rml"))));
		const TStrongObjectPtr<UObject> First(GetFrameObject(TEXT("c")));
		ASSERT_THAT(IsTrue(First.IsValid() && First->IsA<UArcRmlKeptTabViewModel>(), TEXT("the create model has no instance of its class after the open")));
		ASSERT_THAT(AreEqual(FString(TEXT("yes")), TextOf(TEXT("c-resolved")), TEXT("the created instance is not resolved")));

		ASSERT_THAT(IsTrue(CloseAndWait() && View.IsParked(), TEXT("the keep-alive document did not park")));
		ASSERT_THAT(IsTrue(View.Open(PathOf(TEXT("Created.rml")), ArcRmlUITestHelpers::ViewSize, 1.0f), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(ArcRmlKeptTabTestHelpers::SettleFrames(View) && Read()));
		ASSERT_THAT(IsTrue(GetFrameObject(TEXT("c")) == First.Get(), TEXT("park and reopen replaced the created instance")));

		ASSERT_THAT(IsTrue(View.Reload(), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && Read()));
		UObject* const AfterReload = GetFrameObject(TEXT("c"));
		ASSERT_THAT(IsTrue(AfterReload && AfterReload->IsA<UArcRmlKeptTabViewModel>(), TEXT("the reloaded document has no created instance")));
		ASSERT_THAT(IsTrue(AfterReload != First.Get(), TEXT("the reload kept the old created instance")));
	}

	TEST_METHOD(NullClearWithoutAnAssignment_KeepsTheCreatedInstanceAndItsState)
	{
		ASSERT_THAT(IsTrue(WriteCreatedDocument()));
		ASSERT_THAT(IsTrue(Open(TEXT("Created.rml"))));
		UArcRmlKeptTabViewModel* const Created = Cast<UArcRmlKeptTabViewModel>(GetFrameObject(TEXT("c")));
		ASSERT_THAT(IsNotNull(Created, TEXT("the create model has no instance")));
		Created->SetCount(3);
		ASSERT_THAT(IsTrue(Pump()));
		ASSERT_THAT(AreEqual(FString(TEXT("3")), TextOf(TEXT("c-count"))));

		SetFrameObject(TEXT("c"), nullptr);
		ASSERT_THAT(IsTrue(Pump()));
		ASSERT_THAT(IsTrue(GetFrameObject(TEXT("c")) == Created, TEXT("a null clear without an assignment replaced the created instance")));
		ASSERT_THAT(AreEqual(FString(TEXT("3")), TextOf(TEXT("c-count")), TEXT("a null clear without an assignment lost the created instance's state")));
	}

	TEST_METHOD(TabReadsItsFramesManualModel)
	{
		ASSERT_THAT(IsTrue(WriteFrameAndTab()));
		ASSERT_THAT(IsTrue(Open(TEXT("Tab.rml"))));
		ASSERT_THAT(AreEqual(PathOf(TEXT("Tab.rml")), View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		Model->SetCount(9);
		SetFrameObject(TEXT("fm"), Model.Get());
		ASSERT_THAT(IsTrue(Pump()));
		ASSERT_THAT(AreEqual(FString(TEXT("9")), TextOf(TEXT("f-count")), TEXT("the tab does not read the object assigned to its frame's manual model")));
		ASSERT_THAT(AreEqual(FString(TEXT("no")), TextOf(TEXT("t-resolved")), TEXT("the frame's assignment reached the tab's own manual model")));
	}
};

TEST_CLASS(ArcRmlViewModelApiTests, "ArcRmlUI.ViewModelApi")
{
	FString RunFolder;
	FString RunDir;
	TStrongObjectPtr<ULocalPlayer> LocalPlayer;
	TStrongObjectPtr<UArcRmlViewportSubsystem> Subsystem;

	BEFORE_EACH()
	{
		RunFolder = FString::Printf(TEXT("ViewModels/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RunDir = ArcRmlUITestHelpers::GetTestContentDir() / RunFolder;
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::GetTestContentDir().IsEmpty(), TEXT("the test plugin content directory is unknown")));
		LocalPlayer.Reset(NewObject<ULocalPlayer>(GEngine));
		Subsystem.Reset(NewObject<UArcRmlViewportSubsystem>(LocalPlayer.Get()));

		using namespace ArcRmlViewModelSourceTestsPrivate;
		TArray<FString> Head = MenuMeta(false);
		Head.Add(MakeModelTag(TEXT("m"), ModelClass, TEXT("manual")));
		Head.Add(MakeModelTag(TEXT("s"), OtherModelClass, TEXT("context=\"VM_Api\" settable")));
		Head.Add(MakeModelTag(TEXT("c"), ModelClass, TEXT("context=\"VM_Api\"")));
		TArray<FString> TwinHead = MenuMeta(false);
		TwinHead.Add(MakeModelTag(TEXT("a"), ModelClass, TEXT("manual")));
		TwinHead.Add(MakeModelTag(TEXT("b"), ModelClass, TEXT("context=\"VM_Api\" settable")));
		const TArray<FString> Body{TEXT("<div>api</div>")};
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(MakeDocument(Head, FString(), Body), *(RunDir / TEXT("Api.rml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
			&& FFileHelper::SaveStringToFile(MakeDocument(TwinHead, FString(), Body), *(RunDir / TEXT("Twin.rml")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM),
			TEXT("the fixture documents could not be written")));
	}

	AFTER_EACH()
	{
		Subsystem.Reset();
		LocalPlayer.Reset();
		if (RunDir.IsEmpty())
		{
			return;
		}
		IFileManager::Get().DeleteDirectory(*RunDir, false, true);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RunDir), false, false);
		RunDir.Reset();
	}

	FArcRmlDocumentRef DocumentOf(const TCHAR* Name) const
	{
		return FArcRmlDocumentRef{ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/") + Name)};
	}

	void ExpectRefusal()
	{
		TestRunner->AddExpectedMessagePlain(TEXT("not set"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	}

	TEST_METHOD(SetViewModel_UnknownName_ReturnsFalse)
	{
		ExpectRefusal();
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsFalse(Subsystem->SetViewModel(DocumentOf(TEXT("Api.rml")), FName(TEXT("nosuch")), Model.Get()), TEXT("an unknown model name was accepted")));
	}

	TEST_METHOD(SetViewModel_DeclarationNeitherManualNorSettable_ReturnsFalse)
	{
		ExpectRefusal();
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsFalse(Subsystem->SetViewModel(DocumentOf(TEXT("Api.rml")), FName(TEXT("c")), Model.Get()), TEXT("a context model without 'settable' took an assignment")));
	}

	TEST_METHOD(SetViewModel_ObjectOfAnotherClass_ReturnsFalse)
	{
		ExpectRefusal();
		const TStrongObjectPtr<UArcRmlValidateTestViewModel> Model(NewObject<UArcRmlValidateTestViewModel>());
		ASSERT_THAT(IsFalse(Subsystem->SetViewModel(DocumentOf(TEXT("Api.rml")), FName(TEXT("m")), Model.Get()), TEXT("an object of another class was accepted")));
	}

	TEST_METHOD(SetViewModel_ManualAndSettableDeclarationsOfADocumentNotOpen_ReturnTrue)
	{
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		const TStrongObjectPtr<UArcRmlValidateTestViewModel> Other(NewObject<UArcRmlValidateTestViewModel>());
		ASSERT_THAT(IsTrue(Subsystem->SetViewModel(DocumentOf(TEXT("Api.rml")), FName(TEXT("m")), Model.Get()), TEXT("a manual model of a document not open was refused")));
		ASSERT_THAT(IsTrue(Subsystem->SetViewModel(DocumentOf(TEXT("Api.rml")), FName(TEXT("s")), Other.Get()), TEXT("a settable model of a document not open was refused")));
		ASSERT_THAT(IsTrue(Subsystem->SetViewModel(DocumentOf(TEXT("Api.rml")), FName(TEXT("m")), nullptr), TEXT("clearing a manual model was refused")));
		ASSERT_THAT(IsNull(Subsystem->GetViewModel(DocumentOf(TEXT("Api.rml")), FName(TEXT("m"))), TEXT("a document that is not open returned a live view model")));
	}

	TEST_METHOD(SetViewModelByClass_TwoMatchingDeclarations_Fails)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("set by class"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsFalse(Subsystem->SetViewModelByClass(DocumentOf(TEXT("Twin.rml")), Model.Get()), TEXT("two matching declarations did not make the call fail")));
	}

	TEST_METHOD(SetViewModelByClass_OneSettableMatch_SucceedsAndIgnoresDeclarationsThatAreNotSettable)
	{
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Model(NewObject<UArcRmlKeptTabViewModel>());
		ASSERT_THAT(IsTrue(Subsystem->SetViewModelByClass(DocumentOf(TEXT("Api.rml")), Model.Get()),
			TEXT("the one manual declaration of the class was not found, or the context declaration of the same class made the call ambiguous")));
	}
};
