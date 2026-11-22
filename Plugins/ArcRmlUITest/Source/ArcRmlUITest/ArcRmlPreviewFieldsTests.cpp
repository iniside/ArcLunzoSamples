// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlPreviewFieldsTestTypes.h"
#include "CQTest.h"
#include "Engine/World.h"
#include "IDetailTreeNode.h"
#include "INotifyFieldValueChanged.h"
#include "IPropertyRowGenerator.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "UObject/StructOnScope.h"
#include "Preview/ArcRmlPreviewFields.h"
#include "UObject/Package.h"

namespace ArcRmlPreviewFieldsTestsPrivate
{
	UE::FieldNotification::FFieldId FindField(UArcRmlPreviewFieldsTestViewModel& ViewModel, const TCHAR* Name)
	{
		return ViewModel.GetFieldNotificationDescriptor().GetField(ViewModel.GetClass(), FName(Name));
	}

	void CollectStructListInternals(IDetailTreeNode& Node, TArray<FString>& OutNames)
	{
		const TSharedPtr<IPropertyHandle> Handle = Node.CreatePropertyHandle();
		const FProperty* Property = Handle.IsValid() ? Handle->GetProperty() : nullptr;
		if (Property && Property->GetOwnerStruct() == FArcRmlStructList::StaticStruct())
		{
			OutNames.Add(Property->GetName());
		}
		TArray<TSharedRef<IDetailTreeNode>> Children;
		Node.GetChildren(Children, true);
		for (const TSharedRef<IDetailTreeNode>& Child : Children)
		{
			CollectStructListInternals(*Child, OutNames);
		}
	}

	FArcRmlPreviewFieldsTestRow MakeRow(int32 Count)
	{
		FArcRmlPreviewFieldsTestRow Row;
		Row.Count = Count;
		return Row;
	}
}

TEST_CLASS(ArcRmlPreviewFieldsTests, "ArcRmlUI.PreviewFields")
{
	UArcRmlPreviewFieldsTestViewModel* ViewModel = nullptr;

	BEFORE_EACH()
	{
		ViewModel = NewObject<UArcRmlPreviewFieldsTestViewModel>(GetTransientPackage());
	}

	TEST_METHOD(StructEdit_GoesThroughThePropertySetter)
	{
		FArcRmlPreviewShadow Shadow(*ViewModel);
		CastChecked<UArcRmlPreviewFieldsTestViewModel>(Shadow.GetShadow())->Offset = FVector2D(3.0, 4.0);
		Shadow.PushField(TEXT("Offset"));
		ASSERT_THAT(AreEqual(1, ViewModel->OffsetSetterCalls, TEXT("the struct edit wrote the view model memory without its setter")));
		ASSERT_THAT(IsTrue(ViewModel->Offset == FVector2D(3.0, 4.0)));
	}

	TEST_METHOD(ArrayEdit_GoesThroughTheSetFunction)
	{
		FArcRmlPreviewShadow Shadow(*ViewModel);
		CastChecked<UArcRmlPreviewFieldsTestViewModel>(Shadow.GetShadow())->Numbers = { 1, 2, 3 };
		Shadow.PushField(TEXT("Numbers"));
		ASSERT_THAT(AreEqual(1, ViewModel->NumbersSetterCalls, TEXT("the array edit wrote the view model memory without SetNumbers")));
		ASSERT_THAT(AreEqual(3, ViewModel->Numbers.Num()));
	}

	TEST_METHOD(MapEditWithoutSetter_WritesAndBroadcasts)
	{
		int32 Broadcasts = 0;
		const FDelegateHandle Handle = ViewModel->AddFieldValueChangedDelegate(ArcRmlPreviewFieldsTestsPrivate::FindField(*ViewModel, TEXT("Scores")),
			INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&Broadcasts](UObject*, UE::FieldNotification::FFieldId)
			{
				++Broadcasts;
			}));

		FArcRmlPreviewShadow Shadow(*ViewModel);
		CastChecked<UArcRmlPreviewFieldsTestViewModel>(Shadow.GetShadow())->Scores.Add(TEXT("a"), 5);
		Shadow.PushField(TEXT("Scores"));
		ViewModel->RemoveFieldValueChangedDelegate(ArcRmlPreviewFieldsTestsPrivate::FindField(*ViewModel, TEXT("Scores")), Handle);
		ASSERT_THAT(AreEqual(5, ViewModel->Scores.FindRef(TEXT("a"))));
		ASSERT_THAT(AreEqual(1, Broadcasts));
	}

	TEST_METHOD(OutsideChange_IsPulledIntoTheEditedCopy)
	{
		FArcRmlPreviewShadow Shadow(*ViewModel);
		ASSERT_THAT(IsFalse(Shadow.PullField(TEXT("Numbers"))));
		ViewModel->Numbers.Add(9);
		ASSERT_THAT(IsTrue(Shadow.PullField(TEXT("Numbers"))));
		ASSERT_THAT(AreEqual(1, CastChecked<UArcRmlPreviewFieldsTestViewModel>(Shadow.GetShadow())->Numbers.Num()));
		ASSERT_THAT(AreEqual(0, ViewModel->NumbersSetterCalls));
	}

	TEST_METHOD(NestedStructList_EditKeepsItsVersions)
	{
		FArcRmlPreviewShadow Shadow(*ViewModel);
		FArcRmlStructList& Rows = CastChecked<UArcRmlPreviewFieldsTestViewModel>(Shadow.GetShadow())->Group.Rows;
		ASSERT_THAT(IsTrue(Rows.Set(TEXT("a"), ArcRmlPreviewFieldsTestsPrivate::MakeRow(2))));
		Shadow.PushField(TEXT("Group"));

		const FArcRmlStructList& Committed = ViewModel->Group.Rows;
		ASSERT_THAT(AreEqual(1, Committed.Num()));
		ASSERT_THAT(AreEqual(2, Committed.FindAs<FArcRmlPreviewFieldsTestRow>(TEXT("a"))->Count));
		ASSERT_THAT(AreEqual(Rows.GetStructureVersion(), Committed.GetStructureVersion()));
		ASSERT_THAT(IsTrue(Rows.GetRowVersions() == Committed.GetRowVersions()));
	}

	TEST_METHOD(StructListInsideARowStruct_UsesTheRowEditor)
	{
		const TSharedRef<FStructOnScope> Row = MakeShared<FStructOnScope>(FArcRmlPreviewFieldsTestNested::StaticStruct());
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		FPropertyRowGeneratorArgs RowArgs;
		RowArgs.bShouldShowHiddenProperties = true;
		const TSharedRef<IPropertyRowGenerator> RowGenerator = PropertyEditor.CreatePropertyRowGenerator(RowArgs);
		ArcRmlPreviewFields::RegisterRowStructListLayout(*RowGenerator);
		RowGenerator->SetStructure(Row);
		TArray<FString> Internals;
		for (const TSharedRef<IDetailTreeNode>& Node : RowGenerator->GetRootTreeNodes())
		{
			ArcRmlPreviewFieldsTestsPrivate::CollectStructListInternals(*Node, Internals);
		}
		ASSERT_THAT(IsTrue(Internals.IsEmpty(), FString::Printf(TEXT("the row details view edits the list members directly: %s"), *FString::Join(Internals, TEXT(", ")))));

		const TSharedRef<IPropertyRowGenerator> PlainGenerator = PropertyEditor.CreatePropertyRowGenerator(RowArgs);
		PlainGenerator->SetStructure(Row);
		TArray<FString> PlainInternals;
		for (const TSharedRef<IDetailTreeNode>& Node : PlainGenerator->GetRootTreeNodes())
		{
			ArcRmlPreviewFieldsTestsPrivate::CollectStructListInternals(*Node, PlainInternals);
		}
		ASSERT_THAT(IsFalse(PlainInternals.IsEmpty(), TEXT("a details view without the row editor shows no list members, so the walk proves nothing")));
	}

	TEST_METHOD(FieldKinds_AreEditable)
	{
		using namespace ArcRmlPreviewFieldsTestsPrivate;
		ASSERT_THAT(IsTrue(ArcRmlPreviewFields::ClassifyField(*ViewModel, FindField(*ViewModel, TEXT("Scores"))) == EArcRmlPreviewFieldEditor::Details));
		ASSERT_THAT(IsTrue(ArcRmlPreviewFields::ClassifyField(*ViewModel, FindField(*ViewModel, TEXT("Labels"))) == EArcRmlPreviewFieldEditor::Details));
		ASSERT_THAT(IsTrue(ArcRmlPreviewFields::ClassifyField(*ViewModel, FindField(*ViewModel, TEXT("Kind"))) == EArcRmlPreviewFieldEditor::Details,
			TEXT("TSubclassOf is shown as an object value")));
		ASSERT_THAT(IsTrue(ArcRmlPreviewFields::ClassifyField(*ViewModel, FindField(*ViewModel, TEXT("Target"))) == EArcRmlPreviewFieldEditor::Interface));
		ASSERT_THAT(IsTrue(ArcRmlPreviewFields::ClassifyField(*ViewModel, FindField(*ViewModel, TEXT("Group"))) == EArcRmlPreviewFieldEditor::Details));
		ASSERT_THAT(IsTrue(ArcRmlPreviewFields::ClassifyField(*ViewModel, FindField(*ViewModel, TEXT("GetTotal"))) == EArcRmlPreviewFieldEditor::Getter));
	}

	TEST_METHOD(Getter_RunsOnFieldChangeOnly)
	{
		const UE::FieldNotification::FFieldId TotalId = ArcRmlPreviewFieldsTestsPrivate::FindField(*ViewModel, TEXT("GetTotal"));
		ASSERT_THAT(IsTrue(TotalId.IsValid()));
		{
			const FArcRmlPreviewGetter Getter(*ViewModel, TotalId);
			ASSERT_THAT(AreEqual(1, ViewModel->TotalCalls));
			ASSERT_THAT(AreEqual(FString(TEXT("2")), Getter.GetText().ToString()));
			ASSERT_THAT(AreEqual(FString(TEXT("2")), Getter.GetText().ToString()));
			ASSERT_THAT(AreEqual(1, ViewModel->TotalCalls, TEXT("reading the text ran the getter again")));

			ViewModel->Base = 5;
			ViewModel->BroadcastFieldValueChanged(TotalId);
			ASSERT_THAT(AreEqual(2, ViewModel->TotalCalls));
			ASSERT_THAT(AreEqual(FString(TEXT("10")), Getter.GetText().ToString()));
		}
		ViewModel->BroadcastFieldValueChanged(TotalId);
		ASSERT_THAT(AreEqual(2, ViewModel->TotalCalls, TEXT("a destroyed getter still listens to its field")));
	}

	TEST_METHOD(InterfacePicker_ListsObjectsOfItsWorldOnly)
	{
		ASSERT_THAT(AreEqual(0, ArcRmlPreviewFields::FindInterfaceObjects(nullptr, *UArcRmlPreviewFieldsTestTarget::StaticClass()).Num()));

		UWorld* World = UWorld::CreateWorld(EWorldType::None, false);
		UArcRmlPreviewFieldsTestTargetObject* InWorld = NewObject<UArcRmlPreviewFieldsTestTargetObject>(World);
		UArcRmlPreviewFieldsTestTargetObject* Outside = NewObject<UArcRmlPreviewFieldsTestTargetObject>(GetTransientPackage());
		const TArray<UObject*> Objects = ArcRmlPreviewFields::FindInterfaceObjects(World, *UArcRmlPreviewFieldsTestTarget::StaticClass());
		World->DestroyWorld(false);

		ASSERT_THAT(IsTrue(Objects.Contains(InWorld)));
		ASSERT_THAT(IsFalse(Objects.Contains(Outside)));
	}
};
