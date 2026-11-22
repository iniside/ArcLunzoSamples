// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogArcRmlTest, Log, All);

namespace ArcRmlTestCommandsPrivate
{
	void HandleAfterCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 2)
		{
			UE_LOGF(LogArcRmlTest, Warning, "ArcRmlTest.After: usage ArcRmlTest.After <seconds> <command>");
			return;
		}
		const float Seconds = FMath::Max(0.0f, FCString::Atof(*Args[0]));
		FString Command;
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			Command += (Index > 1 ? TEXT(" ") : TEXT("")) + Args[Index];
		}
		// Hitches (shader compiles in offscreen runs) count at most MaxStepSeconds, so a delay measures running frames, not stalls.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Command, Remaining = Seconds](float DeltaTime) mutable -> bool
		{
			constexpr float MaxStepSeconds = 0.25f;
			Remaining -= FMath::Min(DeltaTime, MaxStepSeconds);
			if (Remaining > 0.0f)
			{
				return true;
			}
			if (GEngine)
			{
				GEngine->DeferredCommands.Add(Command);
			}
			return false;
		}));
	}

	void HandleDelayQuitCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!GEngine || Args.IsEmpty())
		{
			UE_LOGF(LogArcRmlTest, Warning, "ArcRmlTest.DelayQuit: usage ArcRmlTest.DelayQuit <seconds>");
			return;
		}
		int32 Delayed = 0;
		for (FString& Command : GEngine->DeferredCommands)
		{
			if (Command.TrimStartAndEnd().Equals(TEXT("Quit"), ESearchCase::IgnoreCase))
			{
				Command.Reset();
				++Delayed;
			}
		}
		if (Delayed > 0)
		{
			TArray<FString> QuitArgs = {Args[0], TEXT("Quit")};
			HandleAfterCommand(QuitArgs, World);
		}
		UE_LOGF(LogArcRmlTest, Log, "ArcRmlTest.DelayQuit: %d queued Quit command(s) moved %ls s later", Delayed, *Args[0]);
	}

	FAutoConsoleCommandWithWorldAndArgs DelayQuitCommand(TEXT("ArcRmlTest.DelayQuit"),
		TEXT("Moves the Quit commands still queued in this frame's deferred commands (the -ExecCmds list) the given number of seconds later"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleDelayQuitCommand));

	FAutoConsoleCommandWithWorldAndArgs AfterCommand(TEXT("ArcRmlTest.After"), TEXT("Runs a console command after the given number of seconds: ArcRmlTest.After <seconds> <command>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleAfterCommand));
}

#endif
