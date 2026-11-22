// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "BackAndParent/ArcRmlSamplesBackAndParent.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesBackAndParent)

namespace ArcRmlSamplesBackAndParent
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/BackAndParent/BackAndParent.rml")};

	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}

void UArcRmlSamplesBackAndParentViewModel::Select(const FString& Item)
{
	UE_MVVM_SET_PROPERTY_VALUE(Selected, Item);
}

void UArcRmlSamplesBackAndParentViewModel::Use()
{
	UE_MVVM_SET_PROPERTY_VALUE(UseCount, UseCount + 1);
}

void UArcRmlSamplesBackAndParentViewModel::Exit()
{
	UE_MVVM_SET_PROPERTY_VALUE(ExitCount, ExitCount + 1);
}

void UArcRmlSamplesBackAndParentViewModel::CountBack()
{
	UE_MVVM_SET_PROPERTY_VALUE(BackCount, BackCount + 1);
}

void UArcRmlSamplesBackAndParentViewModel::PressBack(UObject* WorldContextObject)
{
	const APlayerController* Player = Cast<APlayerController>(WorldContextObject);
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
	if (Viewport)
	{
		Viewport->PostBack(ArcRmlSamplesBackAndParent::Document);
	}
}
