// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "ArcRmlUITestHelpers.h"
#include "HAL/CriticalSection.h"
#include "Misc/OutputDevice.h"

namespace ArcRmlKeptTabTestHelpers
{
	extern const TCHAR* const Frame;
	extern const TCHAR* const TabA;
	extern const TCHAR* const TabB;
	extern const TCHAR* const CloseA;
	extern const TCHAR* const CloseB;
	extern const TCHAR* const Conflict;

	class FLogCapture final : public FOutputDevice
	{
	public:
		FLogCapture();
		virtual ~FLogCapture() override;

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override;
		virtual bool CanBeUsedOnAnyThread() const override;
		virtual bool CanBeUsedOnMultipleThreads() const override;

		int32 CountContaining(const TCHAR* Needle) const;
		FString Describe() const;

	private:
		mutable FCriticalSection Lock;
		TArray<FString> Lines;
	};

	void DropFixtures();
	void RunConsole(const FString& Command);
	TArray<const FArcRmlTestLayoutElement*> FindAll(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId);
	bool HasClass(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId, const FString& Class);
	TArray<const FArcRmlTestDraw*> FindFlatDraws(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour);
	bool SettleFrames(FArcRmlTestView& View);
	bool OpenFrame(FArcRmlTestView& View, bool bPad);
	bool Visit(FArcRmlTestView& View, const TCHAR* Relative);
}
