// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "StructList/ArcRmlSamplesStructList.h"

#include "Algo/Reverse.h"
#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesStructList)

UArcRmlSamplesStructListViewModel::UArcRmlSamplesStructListViewModel()
{
	Rows.SetRowType(FArcRmlSamplesStructListRow::StaticStruct());
	for (int32 Index = 0; Index < 3; ++Index)
	{
		AddRow();
	}
}

void UArcRmlSamplesStructListViewModel::AddRow()
{
	FArcRmlSamplesStructListRow Row;
	Row.Id = FString::Printf(TEXT("row-%d"), NextNumber);
	Row.Name = FString::Printf(TEXT("Item %d"), NextNumber);
	Row.Quantity = NextNumber;
	++NextNumber;
	Rows.Set(Row.Id, Row);
}

void UArcRmlSamplesStructListViewModel::Add()
{
	AddRow();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Rows);
}

void UArcRmlSamplesStructListViewModel::Remove(int32 Index)
{
	if (!Rows.GetKeys().IsValidIndex(Index))
	{
		return;
	}
	const FString Key = Rows.GetKeys()[Index];
	if (Rows.Remove(Key))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Rows);
	}
}

void UArcRmlSamplesStructListViewModel::Bump(int32 Index)
{
	if (!Rows.GetKeys().IsValidIndex(Index))
	{
		return;
	}
	const FString Key = Rows.GetKeys()[Index];
	const FArcRmlSamplesStructListRow* Current = Rows.FindAs<FArcRmlSamplesStructListRow>(Key);
	if (Current)
	{
		FArcRmlSamplesStructListRow Row = *Current;
		++Row.Quantity;
		Rows.Set(Key, Row);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Rows);
	}
}

void UArcRmlSamplesStructListViewModel::Reverse()
{
	TArray<FString> Order = Rows.GetKeys();
	Algo::Reverse(Order);
	Rows.SetOrder(Order);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Rows);
}

namespace ArcRmlSamplesStructList
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/StructList/StructList.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
