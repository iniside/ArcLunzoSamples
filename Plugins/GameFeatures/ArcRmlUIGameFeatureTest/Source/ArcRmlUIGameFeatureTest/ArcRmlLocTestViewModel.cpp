// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlLocTestViewModel.h"

#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlLocTestViewModel)

DEFINE_LOG_CATEGORY_STATIC(LogArcRmlLocTest, Log, All);

void UArcRmlLocTestViewModel::Initialize()
{
	CultureChangedHandle = FInternationalization::Get().OnCultureChanged().AddUObject(this, &UArcRmlLocTestViewModel::HandleCultureChanged);
	HandleCultureChanged();
}

void UArcRmlLocTestViewModel::Shutdown()
{
	FInternationalization::Get().OnCultureChanged().Remove(CultureChangedHandle);
	CultureChangedHandle.Reset();
}

void UArcRmlLocTestViewModel::SwitchCulture(const FString& Name)
{
	if (!FInternationalization::Get().SetCurrentCulture(Name))
	{
		UE_LOGF(LogArcRmlLocTest, Warning, "ArcRmlLocTest: culture '%ls' does not exist in this build", *Name);
		return;
	}
	UE_LOGF(LogArcRmlLocTest, Log, "ArcRmlLocTest: culture set to '%ls'", *Name);
}

void UArcRmlLocTestViewModel::HandleCultureChanged()
{
	UE_MVVM_SET_PROPERTY_VALUE(Culture, FInternationalization::Get().GetCurrentCulture()->GetName());
}
