// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlKeptTabTestTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlKeptTabTestTypes)

void UArcRmlKeptTabViewModel::Ping()
{
}

void UArcRmlKeptTabViewModel::OnLeftA()
{
}

void UArcRmlKeptTabViewModel::OnLeftB()
{
}

void UArcRmlKeptTabViewModel::SetCount(int32 InCount)
{
	UE_MVVM_SET_PROPERTY_VALUE(Count, InCount);
}

int32 UArcRmlKeptTabViewModel::GetCount() const
{
	return Count;
}
