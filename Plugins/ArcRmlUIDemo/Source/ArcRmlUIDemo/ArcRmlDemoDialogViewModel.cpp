// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoDialogViewModel.h"

#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoDialogViewModel)

void UArcRmlDemoDialogViewModel::Configure(const FArcRmlDemoDialogSpec& Spec, TFunction<void(int32)> InOnChoose)
{
	OnChoose = MoveTemp(InOnChoose);
	CancelIndex = Spec.CancelIndex;
	ARC_RML_DEMO_SET(*this, Title, Spec.Title);
	ARC_RML_DEMO_SET(*this, Body, Spec.Body);
	ARC_RML_DEMO_SET(*this, Variant, Spec.Variant);
	ARC_RML_DEMO_SET(*this, Buttons, Spec.Buttons);
	ARC_RML_DEMO_SET(*this, Focused, Spec.FocusIndex);
}

void UArcRmlDemoDialogViewModel::SetBody(const FString& InBody)
{
	ARC_RML_DEMO_SET(*this, Body, InBody);
}

void UArcRmlDemoDialogViewModel::SetCountdown(float Pct, const FString& Label, const FString& Value)
{
	ARC_RML_DEMO_SET(*this, bCountdown, true);
	ARC_RML_DEMO_SET(*this, CountdownPct, Pct);
	ARC_RML_DEMO_SET(*this, CountdownLabel, Label);
	ARC_RML_DEMO_SET(*this, CountdownValue, Value);
}

TFunction<void(int32)> UArcRmlDemoDialogViewModel::TakeCallback()
{
	TFunction<void(int32)> Callback = MoveTemp(OnChoose);
	OnChoose.Reset();
	return Callback;
}

void UArcRmlDemoDialogViewModel::Choose(int32 Index)
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->ChooseDialog(this, Index);
	}
}

void UArcRmlDemoDialogViewModel::Cancel()
{
	Choose(CancelIndex);
}

void UArcRmlDemoDialogViewModel::Closed()
{
	if (const TFunction<void(int32)> Callback = TakeCallback())
	{
		Callback(CancelIndex);
	}
}
