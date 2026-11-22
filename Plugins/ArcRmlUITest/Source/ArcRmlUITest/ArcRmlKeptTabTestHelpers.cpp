// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlKeptTabTestHelpers.h"

#include "CommonInputTypeEnum.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/OutputDeviceRedirector.h"

const TCHAR* const ArcRmlKeptTabTestHelpers::Frame = TEXT("Tabs/KeepFrame.rml");
const TCHAR* const ArcRmlKeptTabTestHelpers::TabA = TEXT("Tabs/KeepTabA.rml");
const TCHAR* const ArcRmlKeptTabTestHelpers::TabB = TEXT("Tabs/KeepTabB.rml");
const TCHAR* const ArcRmlKeptTabTestHelpers::CloseA = TEXT("Tabs/KeepCloseA.rml");
const TCHAR* const ArcRmlKeptTabTestHelpers::CloseB = TEXT("Tabs/KeepCloseB.rml");
const TCHAR* const ArcRmlKeptTabTestHelpers::Conflict = TEXT("Tabs/KeepConflict.rml");

ArcRmlKeptTabTestHelpers::FLogCapture::FLogCapture()
{
	GLog->AddOutputDevice(this);
}

ArcRmlKeptTabTestHelpers::FLogCapture::~FLogCapture()
{
	GLog->RemoveOutputDevice(this);
}

void ArcRmlKeptTabTestHelpers::FLogCapture::Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category)
{
	if (FCString::Strstr(Message, TEXT("ArcRmlUI")))
	{
		FScopeLock ScopeLock(&Lock);
		Lines.Add(Message);
	}
}

bool ArcRmlKeptTabTestHelpers::FLogCapture::CanBeUsedOnAnyThread() const
{
	return true;
}

bool ArcRmlKeptTabTestHelpers::FLogCapture::CanBeUsedOnMultipleThreads() const
{
	return true;
}

int32 ArcRmlKeptTabTestHelpers::FLogCapture::CountContaining(const TCHAR* Needle) const
{
	GLog->Flush();
	FScopeLock ScopeLock(&Lock);
	int32 Count = 0;
	for (const FString& Line : Lines)
	{
		Count += Line.Contains(Needle) ? 1 : 0;
	}
	return Count;
}

FString ArcRmlKeptTabTestHelpers::FLogCapture::Describe() const
{
	GLog->Flush();
	FScopeLock ScopeLock(&Lock);
	return FString::Join(Lines, TEXT(" | "));
}

void ArcRmlKeptTabTestHelpers::DropFixtures()
{
	for (const TCHAR* const Path : {Frame, TabA, TabB, CloseA, CloseB, Conflict})
	{
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(Path));
	}
}

void ArcRmlKeptTabTestHelpers::RunConsole(const FString& Command)
{
	IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
}

TArray<const FArcRmlTestLayoutElement*> ArcRmlKeptTabTestHelpers::FindAll(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId)
{
	TArray<const FArcRmlTestLayoutElement*> Found;
	for (const FArcRmlTestLayoutElement& Element : Elements)
	{
		if (Element.DomId == DomId)
		{
			Found.Add(&Element);
		}
	}
	return Found;
}

bool ArcRmlKeptTabTestHelpers::HasClass(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId, const FString& Class)
{
	const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
	if (!Element)
	{
		return false;
	}
	TArray<FString> Names;
	Element->Classes.ParseIntoArrayWS(Names);
	return Names.Contains(Class);
}

TArray<const FArcRmlTestDraw*> ArcRmlKeptTabTestHelpers::FindFlatDraws(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
{
	TArray<const FArcRmlTestDraw*> Found;
	for (const FArcRmlTestDraw& Draw : Draws)
	{
		if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && !Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
		{
			Found.Add(&Draw);
		}
	}
	return Found;
}

bool ArcRmlKeptTabTestHelpers::SettleFrames(FArcRmlTestView& View)
{
	for (int32 Attempt = 0; Attempt < 4; ++Attempt)
	{
		FPlatformProcess::Sleep(0.05f);
		if (!View.Flush())
		{
			return false;
		}
	}
	return true;
}

bool ArcRmlKeptTabTestHelpers::OpenFrame(FArcRmlTestView& View, bool bPad)
{
	DropFixtures();
	if (!ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(Frame)))
	{
		return false;
	}
	if (bPad)
	{
		View.SetInputType(ECommonInputType::Gamepad);
	}
	return View.FlushUntilSettled() && View.Flush();
}

bool ArcRmlKeptTabTestHelpers::Visit(FArcRmlTestView& View, const TCHAR* Relative)
{
	const FString Path = ArcRmlUITestHelpers::MakePath(Relative);
	return View.Embed(Path) && View.Flush() && View.Flush() && View.GetProbe().EmbeddedPath == Path;
}
