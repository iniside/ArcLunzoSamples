// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "SourceContext/ArcRmlSamplesSourceContext.h"

#include "Engine/GameInstance.h"
#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelCollection.h"
#include "Types/MVVMViewModelContext.h"
#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesSourceContext)

namespace ArcRmlSamplesSourceContext
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/SourceContext/SourceContext.rml")};
		UMVVMGameSubsystem* Mvvm = LocalPlayer->GetGameInstance()->GetSubsystem<UMVVMGameSubsystem>();
		if (!Mvvm)
		{
			return FArcRmlDocumentRef();
		}

		FMVVMViewModelContext Context;
		Context.ContextClass = UArcRmlSamplesSourceContextViewModel::StaticClass();
		Context.ContextName = TEXT("VM_SampleContext");
		Mvvm->GetViewModelCollection()->RemoveViewModel(Context);
		UArcRmlSamplesSourceContextViewModel* Instance = NewObject<UArcRmlSamplesSourceContextViewModel>(Mvvm);
		Instance->Label = TEXT("From the GameInstance collection");
		Mvvm->GetViewModelCollection()->AddViewModelInstance(Context, Instance);

		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
