// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "SourceResolver/ArcRmlSamplesSourceResolver.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesSourceResolver)

UObject* UArcRmlSamplesGreetingResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (ExpectedType && !UArcRmlSamplesSourceResolverViewModel::StaticClass()->IsChildOf(ExpectedType))
	{
		return nullptr;
	}

	UArcRmlSamplesSourceResolverViewModel* ViewModel = NewObject<UArcRmlSamplesSourceResolverViewModel>(GetTransientPackage());
	ViewModel->Greeting = Greeting;
	const APlayerController* Player = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	ViewModel->PlayerName = Player ? Player->GetName() : FString(TEXT("no player"));
	return ViewModel;
}

namespace ArcRmlSamplesSourceResolver
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/SourceResolver/SourceResolver.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
