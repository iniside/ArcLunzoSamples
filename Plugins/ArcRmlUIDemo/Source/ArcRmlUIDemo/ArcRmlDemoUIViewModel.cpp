// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoUIViewModel.h"

#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoUIViewModel)

void UArcRmlDemoUIViewModel::Back()
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->Back();
	}
}

void UArcRmlDemoUIViewModel::ToggleDevBar()
{
	const UArcRmlDemoSubsystem* Demo = GetDemo();
	if (Demo && Demo->IsRunning())
	{
		ARC_RML_DEMO_SET(*this, DevOpen, !DevOpen);
	}
}

void UArcRmlDemoUIViewModel::OpenPause()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (Demo && Demo->IsRunning())
	{
		Demo->Push(TEXT("pause"));
	}
}

void UArcRmlDemoUIViewModel::ToggleGameMenu()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo || !Demo->IsRunning())
	{
		return;
	}
	const FString Top = Demo->GetTop();
	if (Top == TEXT("hud"))
	{
		Demo->Push(TEXT("hub"));
	}
	else if (Top == TEXT("hub"))
	{
		Demo->Pop();
	}
}

void UArcRmlDemoUIViewModel::SetTheme(const FString& InTheme)
{
	const UArcRmlDemoSubsystem* Demo = GetDemo();
	if (Demo && Demo->IsRunning())
	{
		ARC_RML_DEMO_SET(*this, Theme, InTheme);
	}
}

void UArcRmlDemoUIViewModel::SetInputScheme(const FString& Scheme)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (Demo && Demo->IsRunning())
	{
		Demo->SetInputScheme(Scheme);
	}
}

void UArcRmlDemoUIViewModel::Jump(const FString& Screen)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (Demo && Demo->IsRunning())
	{
		Demo->Jump(Screen);
	}
}

void UArcRmlDemoUIViewModel::ToggleMultiplayer()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (Demo && Demo->IsRunning())
	{
		Demo->ToggleMultiplayer();
	}
}

void UArcRmlDemoUIViewModel::SetInput(const FString& InInput)
{
	ARC_RML_DEMO_SET(*this, Input, InInput);
}

void UArcRmlDemoUIViewModel::SetToast(const FArcRmlDemoToast& InToast)
{
	ARC_RML_DEMO_SET(*this, Toast, InToast);
}

void UArcRmlDemoUIViewModel::RefreshDevBar(const TArray<FString>& Stack, bool bMultiplayer)
{
	FArcRmlDemoDevBar Bar;
	Bar.Screens = JumpTargets;
	Bar.Session = bMultiplayer ? TEXT("Session: multiplayer") : TEXT("Session: single player");
	Bar.Current = Stack.IsEmpty() ? FString() : Stack.Last();
	Bar.Status = TEXT("stack: ") + FString::Join(Stack, TEXT(" › "));
	ARC_RML_DEMO_SET(*this, DevBar, Bar);
}
