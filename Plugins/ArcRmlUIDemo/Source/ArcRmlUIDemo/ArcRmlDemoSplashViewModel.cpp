// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoSplashViewModel.h"

#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoSplashViewModel)

void UArcRmlDemoSplashViewModel::InitScreen(const FString& Param)
{
	ARC_RML_DEMO_SET(*this, bReady, false);
	Later(TEXT("ready"), ReadySeconds, [this]() -> void
	{
		ARC_RML_DEMO_SET(*this, bReady, true);
	});
}

void UArcRmlDemoSplashViewModel::Back()
{
	Continue();
}

void UArcRmlDemoSplashViewModel::Continue()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (bReady && Demo)
	{
		Demo->ReplaceTop(TEXT("main"));
	}
}

