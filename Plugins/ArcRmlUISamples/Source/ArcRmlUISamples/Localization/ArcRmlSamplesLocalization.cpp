// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Localization/ArcRmlSamplesLocalization.h"

#include "ArcRmlSamplesLog.h"
#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesLocalization)

void UArcRmlSamplesLocalizationViewModel::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	Culture = FInternationalization::Get().GetCurrentCulture()->GetName();
	CultureChangedHandle = FInternationalization::Get().OnCultureChanged().AddUObject(this, &UArcRmlSamplesLocalizationViewModel::HandleCultureChanged);
}

void UArcRmlSamplesLocalizationViewModel::BeginDestroy()
{
	if (CultureChangedHandle.IsValid())
	{
		FInternationalization::Get().OnCultureChanged().Remove(CultureChangedHandle);
		CultureChangedHandle.Reset();
	}
	Super::BeginDestroy();
}

void UArcRmlSamplesLocalizationViewModel::SwitchCulture(const FString& Name)
{
	if (!FInternationalization::Get().SetCurrentCulture(Name))
	{
		UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples: culture '%ls' does not exist in this build", *Name);
	}
}

void UArcRmlSamplesLocalizationViewModel::AddCoin()
{
	UE_MVVM_SET_PROPERTY_VALUE(Coins, Coins + 1);
}

void UArcRmlSamplesLocalizationViewModel::HandleCultureChanged()
{
	UE_MVVM_SET_PROPERTY_VALUE(Culture, FInternationalization::Get().GetCurrentCulture()->GetName());
}

namespace ArcRmlSamplesLocalization
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Localization/Localization.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
