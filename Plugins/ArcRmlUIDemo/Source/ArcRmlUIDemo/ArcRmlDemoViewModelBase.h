// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlStructList.h"
#include "MVVMViewModelBase.h"
#include "UObject/UnrealType.h"

#include <type_traits>

#include "ArcRmlDemoViewModelBase.generated.h"

#define ARC_RML_DEMO_SET(ViewModel, Member, Value) \
	(ViewModel).SetField((ViewModel).Member, (Value), GET_MEMBER_NAME_CHECKED(std::remove_cvref_t<decltype(ViewModel)>, Member))

class UArcRmlDemoSubsystem;

UCLASS(Abstract)
class UArcRmlDemoViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UArcRmlDemoSubsystem* GetDemo() const;

	FDelegateHandle WatchField(FName FieldName, FFieldValueChangedDelegate Delegate)
	{
		return AddFieldValueChangedDelegate(GetFieldNotificationDescriptor().GetField(GetClass(), FieldName), MoveTemp(Delegate));
	}

	template <typename T>
	void SetField(T& Field, const std::type_identity_t<T>& Value, FName FieldName)
	{
		const FProperty* Property = GetClass()->FindPropertyByName(FieldName);
		if (Property && Property->Identical(&Field, &Value))
		{
			return;
		}
		Field = Value;
		NotifyField(FieldName);
	}

	void NotifyField(FName FieldName)
	{
		BroadcastFieldValueChanged(GetFieldNotificationDescriptor().GetField(GetClass(), FieldName));
	}

	template <typename RowType>
	void SetList(FArcRmlStructList& List, FName FieldName, const TArray<FString>& Keys, const TArray<RowType>& Rows)
	{
		bool bChanged = false;
		TArray<FString> Stale;
		for (const FString& Key : List.GetKeys())
		{
			if (!Keys.Contains(Key))
			{
				Stale.Add(Key);
			}
		}
		for (const FString& Key : Stale)
		{
			bChanged |= List.Remove(Key);
		}
		for (int32 Index = 0; Index < Keys.Num(); ++Index)
		{
			bChanged |= List.Set(Keys[Index], Rows[Index]);
		}
		const uint64 Before = List.GetStructureVersion();
		List.SetOrder(Keys);
		bChanged |= Before != List.GetStructureVersion();
		if (bChanged)
		{
			NotifyField(FieldName);
		}
	}
};
