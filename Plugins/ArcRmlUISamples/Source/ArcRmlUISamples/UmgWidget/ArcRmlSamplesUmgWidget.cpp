// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "UmgWidget/ArcRmlSamplesUmgWidget.h"

#include "ArcRmlSamplesLog.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Host/ArcRmlDocumentWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesUmgWidget)

namespace ArcRmlSamplesUmgWidgetPrivate
{
	const TCHAR* WidgetClassPath = TEXT("/ArcRmlUISamples/UI/Rml/UmgWidget/WBP_Sample_UmgWidget.WBP_Sample_UmgWidget_C");
	const TCHAR* DocumentPath = TEXT("/ArcRmlUISamples/UI/Rml/UmgWidget/UmgWidget.rml");
	constexpr int32 ZOrder = 1500;

	TWeakObjectPtr<UUserWidget> OpenWidget;
}

void UArcRmlSamplesUmgWidgetViewModel::Increment()
{
	const int32 NewCount = Count + 1;
	UE_MVVM_SET_PROPERTY_VALUE(Count, NewCount);
}

void UArcRmlSamplesUmgWidgetViewModel::HandleCloseRequested()
{
	if (UUserWidget* Widget = Host.Get())
	{
		Widget->RemoveFromParent();
	}
	UE_LOGF(LogArcRmlSamples, Display, "ArcRmlSamples UmgWidget: closed");
}

namespace ArcRmlSamplesUmgWidget
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		if (ArcRmlSamplesUmgWidgetPrivate::OpenWidget.IsValid())
		{
			Close(LocalPlayer);
		}
		APlayerController* Player = LocalPlayer ? LocalPlayer->GetPlayerController(nullptr) : nullptr;
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, ArcRmlSamplesUmgWidgetPrivate::WidgetClassPath);
		if (!Player || !WidgetClass)
		{
			return FArcRmlDocumentRef();
		}

		UUserWidget* Widget = CreateWidget<UUserWidget>(Player, WidgetClass);
		UArcRmlDocumentWidget* Document = Widget ? Cast<UArcRmlDocumentWidget>(Widget->GetWidgetFromName(TEXT("Document"))) : nullptr;
		if (!Document)
		{
			return FArcRmlDocumentRef();
		}

		UArcRmlSamplesUmgWidgetViewModel* ViewModel = NewObject<UArcRmlSamplesUmgWidgetViewModel>(Widget);
		ViewModel->Host = Widget;
		Document->SetViewModel(TEXT("counter"), ViewModel);
		Document->OnCloseRequested.AddDynamic(ViewModel, &UArcRmlSamplesUmgWidgetViewModel::HandleCloseRequested);
		Widget->AddToPlayerScreen(ArcRmlSamplesUmgWidgetPrivate::ZOrder);
		ArcRmlSamplesUmgWidgetPrivate::OpenWidget = Widget;
		return FArcRmlDocumentRef{ArcRmlSamplesUmgWidgetPrivate::DocumentPath};
	}

	void Close(ULocalPlayer* LocalPlayer)
	{
		if (UUserWidget* Widget = ArcRmlSamplesUmgWidgetPrivate::OpenWidget.Get())
		{
			Widget->RemoveFromParent();
		}
		ArcRmlSamplesUmgWidgetPrivate::OpenWidget.Reset();
	}

	bool IsOpen(ULocalPlayer* LocalPlayer)
	{
		const UUserWidget* Widget = ArcRmlSamplesUmgWidgetPrivate::OpenWidget.Get();
		return Widget && Widget->IsInViewport();
	}
}
