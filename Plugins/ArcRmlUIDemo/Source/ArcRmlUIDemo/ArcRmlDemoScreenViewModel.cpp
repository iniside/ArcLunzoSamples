// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoScreenViewModel)

void UArcRmlDemoScreenViewModel::Shutdown()
{
	if (UArcRmlDemoGameViewModel* Game = GetGame())
	{
		Game->RemoveAllFieldValueChangedDelegates(this);
	}
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->CancelTimers(this);
	}
}

void UArcRmlDemoScreenViewModel::Back()
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->LeaveScreen();
	}
}

void UArcRmlDemoScreenViewModel::TextInput(const FString& Field, const FString& Value)
{
	if (!Field.IsEmpty())
	{
		WriteText(Field, Value);
	}
}

UArcRmlDemoGameViewModel* UArcRmlDemoScreenViewModel::GetGame() const
{
	const UArcRmlDemoSubsystem* Demo = GetDemo();
	return Demo ? Demo->GetGame() : nullptr;
}

void UArcRmlDemoScreenViewModel::WatchGameField(FName FieldName)
{
	if (UArcRmlDemoGameViewModel* Game = GetGame())
	{
		Game->WatchField(FieldName, FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandleGameFieldChanged));
	}
}

void UArcRmlDemoScreenViewModel::HandleGameFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	GameFieldChanged(FieldId.GetName());
}

void UArcRmlDemoScreenViewModel::Later(FName Id, double Seconds, TFunction<void()> Fn)
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->Later(this, Id, Seconds, MoveTemp(Fn));
	}
}

void UArcRmlDemoScreenViewModel::Every(FName Id, double Seconds, TFunction<bool()> Fn)
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->Every(this, Id, Seconds, MoveTemp(Fn));
	}
}

void UArcRmlDemoScreenViewModel::CancelTimer(FName Id)
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->CancelTimer(this, Id);
	}
}
