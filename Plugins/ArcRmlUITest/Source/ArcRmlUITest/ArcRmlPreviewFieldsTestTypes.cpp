// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlPreviewFieldsTestTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlPreviewFieldsTestTypes)

int32 UArcRmlPreviewFieldsTestViewModel::GetTotal() const
{
	++TotalCalls;
	return Base * 2;
}

void UArcRmlPreviewFieldsTestViewModel::SetOffset(const FVector2D& InOffset)
{
	++OffsetSetterCalls;
	UE_MVVM_SET_PROPERTY_VALUE(Offset, InOffset);
}

FVector2D UArcRmlPreviewFieldsTestViewModel::GetOffset() const
{
	return Offset;
}

void UArcRmlPreviewFieldsTestViewModel::SetNumbers(const TArray<int32>& InNumbers)
{
	++NumbersSetterCalls;
	UE_MVVM_SET_PROPERTY_VALUE(Numbers, InNumbers);
}
