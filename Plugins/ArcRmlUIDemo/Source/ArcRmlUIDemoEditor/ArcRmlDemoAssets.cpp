// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoAssets.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/StructureEditorUtils.h"
#include "StructUtils/UserDefinedStruct.h"
#include "UObject/Package.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"

bool UArcRmlDemoAssetLibrary::SetVariableFieldNotify(UBlueprint* Blueprint, FName VariableName, bool bFieldNotify)
{
	if (!Blueprint || FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VariableName) == INDEX_NONE)
	{
		return false;
	}
	if (bFieldNotify)
	{
		FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VariableName, nullptr, FBlueprintMetadata::MD_FieldNotify, TEXT(""));
	}
	else
	{
		FBlueprintEditorUtils::RemoveFieldNotifyFromAllMetadata(Blueprint, VariableName);
		FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint, VariableName, nullptr, FBlueprintMetadata::MD_FieldNotify);
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return true;
}

bool UArcRmlDemoAssetLibrary::SetClassDefaultsFromJson(UClass* Class, const FString& Json)
{
	UClass* Parent = Class ? Class->GetSuperClass() : nullptr;
	TSharedPtr<FJsonObject> Values;
	if (!Parent || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Values) || !Values.IsValid())
	{
		return false;
	}
	UObject* Defaults = Class->GetDefaultObject();
	const UObject* ParentDefaults = Parent->GetDefaultObject();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Value : Values->Values)
	{
		const FProperty* Property = Class->FindPropertyByName(FName(*Value.Key));
		if (!Property || !Class->IsChildOf(Property->GetOwnerClass()))
		{
			return false;
		}
	}
	Defaults->Modify();
	for (TFieldIterator<FProperty> It(Class); It; ++It)
	{
		if (!Values->HasField(It->GetAuthoredName()))
		{
			continue;
		}
		if (Parent->IsChildOf(It->GetOwnerClass()))
		{
			It->CopyCompleteValue_InContainer(Defaults, ParentDefaults);
		}
		else
		{
			It->ClearValue_InContainer(Defaults);
		}
	}
	if (!FJsonObjectConverter::JsonObjectToUStruct(Values.ToSharedRef(), Class, Defaults))
	{
		return false;
	}
	Defaults->MarkPackageDirty();
	return true;
}

UUserDefinedStruct* UArcRmlDemoAssetLibrary::EnsureUserDefinedStruct(const FString& AssetPath, const TArray<FName>& MemberNames, const TArray<FEdGraphPinType>& MemberTypes)
{
	const FString AssetName = FPackageName::GetShortName(AssetPath);
	if (MemberNames.IsEmpty() || MemberNames.Num() != MemberTypes.Num() || AssetName.IsEmpty())
	{
		return nullptr;
	}
	UUserDefinedStruct* Struct = LoadObject<UUserDefinedStruct>(nullptr, *(AssetPath + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Struct)
	{
		UPackage* Package = CreatePackage(*AssetPath);
		Struct = FStructureEditorUtils::CreateUserDefinedStruct(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
		if (!Struct)
		{
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Struct);
	}
	for (int32 Index = 0; Index < MemberNames.Num(); ++Index)
	{
		const FString Wanted = MemberNames[Index].ToString();
		const FStructVariableDescription* Found = FStructureEditorUtils::GetVarDesc(Struct).FindByPredicate([Struct, &Wanted](const FStructVariableDescription& Desc)
		{
			return FStructureEditorUtils::GetVariableFriendlyName(Struct, Desc.VarGuid) == Wanted;
		});
		if (!Found)
		{
			if (!FStructureEditorUtils::AddVariable(Struct, MemberTypes[Index]))
			{
				return nullptr;
			}
			const FGuid Added = FStructureEditorUtils::GetVarDesc(Struct).Last().VarGuid;
			if (!FStructureEditorUtils::RenameVariable(Struct, Added, Wanted))
			{
				return nullptr;
			}
		}
		else if (Found->ToPinType() != MemberTypes[Index] && !FStructureEditorUtils::ChangeVariableType(Struct, Found->VarGuid, MemberTypes[Index]))
		{
			return nullptr;
		}
	}
	TArray<FGuid> Stale;
	for (const FStructVariableDescription& Desc : FStructureEditorUtils::GetVarDesc(Struct))
	{
		if (!MemberNames.Contains(FName(*FStructureEditorUtils::GetVariableFriendlyName(Struct, Desc.VarGuid))))
		{
			Stale.Add(Desc.VarGuid);
		}
	}
	for (const FGuid& Guid : Stale)
	{
		if (!FStructureEditorUtils::RemoveVariable(Struct, Guid))
		{
			return nullptr;
		}
	}
	Struct->MarkPackageDirty();
	return Struct;
}

int32 UArcRmlDemoAssetLibrary::RemoveMemberVariablesNotIn(UBlueprint* Blueprint, const TArray<FName>& Keep)
{
	if (!Blueprint)
	{
		return INDEX_NONE;
	}
	TArray<FName> Stale;
	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		if (!Keep.Contains(Variable.VarName))
		{
			Stale.Add(Variable.VarName);
		}
	}
	if (Stale.Num() > 0)
	{
		FBlueprintEditorUtils::BulkRemoveMemberVariables(Blueprint, Stale);
	}
	return Stale.Num();
}
