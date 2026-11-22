// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "UmgOwnerData/ArcRmlSamplesUmgOwnerData.h"

#include "ArcRmlSamplesLog.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Host/ArcRmlDocumentWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesUmgOwnerData)

namespace ArcRmlSamplesUmgOwnerDataPrivate
{
	const TCHAR* WidgetClassPath = TEXT("/ArcRmlUISamples/UI/Rml/UmgOwnerData/WBP_Sample_UmgOwnerData.WBP_Sample_UmgOwnerData_C");
	const TCHAR* DocumentPath = TEXT("/ArcRmlUISamples/UI/Rml/UmgOwnerData/UmgOwnerData.rml");
	constexpr int32 ZOrder = 1500;
	constexpr int32 MaxHealth = 100;
	constexpr int32 HealAmount = 10;

	TWeakObjectPtr<UUserWidget> OpenWidget;
}

void UArcRmlSamplesUmgOwnerDataViewModel::Heal()
{
	const int32 NewHealth = FMath::Min(Health + ArcRmlSamplesUmgOwnerDataPrivate::HealAmount, ArcRmlSamplesUmgOwnerDataPrivate::MaxHealth);
	UE_MVVM_SET_PROPERTY_VALUE(Health, NewHealth);
}

void UArcRmlSamplesUmgOwnerDataWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Stats = NewObject<UArcRmlSamplesUmgOwnerDataViewModel>(this);
	if (UArcRmlDocumentWidget* Document = Cast<UArcRmlDocumentWidget>(GetWidgetFromName(TEXT("Document"))))
	{
		Document->OnCloseRequested.AddDynamic(this, &UArcRmlSamplesUmgOwnerDataWidget::HandleCloseRequested);
	}
}

void UArcRmlSamplesUmgOwnerDataWidget::HandleCloseRequested()
{
	RemoveFromParent();
	UE_LOGF(LogArcRmlSamples, Display, "ArcRmlSamples UmgOwnerData: closed");
}

namespace ArcRmlSamplesUmgOwnerData
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		if (ArcRmlSamplesUmgOwnerDataPrivate::OpenWidget.IsValid())
		{
			Close(LocalPlayer);
		}
		APlayerController* Player = LocalPlayer ? LocalPlayer->GetPlayerController(nullptr) : nullptr;
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, ArcRmlSamplesUmgOwnerDataPrivate::WidgetClassPath);
		if (!Player || !WidgetClass)
		{
			return FArcRmlDocumentRef();
		}

		UUserWidget* Widget = CreateWidget<UUserWidget>(Player, WidgetClass);
		if (!Widget)
		{
			return FArcRmlDocumentRef();
		}
		Widget->AddToPlayerScreen(ArcRmlSamplesUmgOwnerDataPrivate::ZOrder);
		ArcRmlSamplesUmgOwnerDataPrivate::OpenWidget = Widget;
		return FArcRmlDocumentRef{ArcRmlSamplesUmgOwnerDataPrivate::DocumentPath};
	}

	void Close(ULocalPlayer* LocalPlayer)
	{
		if (UUserWidget* Widget = ArcRmlSamplesUmgOwnerDataPrivate::OpenWidget.Get())
		{
			Widget->RemoveFromParent();
		}
		ArcRmlSamplesUmgOwnerDataPrivate::OpenWidget.Reset();
	}

	bool IsOpen(ULocalPlayer* LocalPlayer)
	{
		const UUserWidget* Widget = ArcRmlSamplesUmgOwnerDataPrivate::OpenWidget.Get();
		return Widget && Widget->IsInViewport();
	}
}
