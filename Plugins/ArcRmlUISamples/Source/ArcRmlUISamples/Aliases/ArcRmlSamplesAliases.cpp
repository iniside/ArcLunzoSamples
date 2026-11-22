// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Aliases/ArcRmlSamplesAliases.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesAliases)

namespace ArcRmlSamplesAliases
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Aliases/Aliases.rml")};

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

UArcRmlSamplesAliasesViewModel::UArcRmlSamplesAliasesViewModel()
{
	FArcRmlSamplesAliasesHero& Archer = Heroes.AddDefaulted_GetRef();
	Archer.Name = TEXT("Mira");
	Archer.Title = TEXT("Archer");
	Archer.Level = 12;

	FArcRmlSamplesAliasesHero& Healer = Heroes.AddDefaulted_GetRef();
	Healer.Name = TEXT("Tomas");
	Healer.Title = TEXT("Healer");
	Healer.Level = 8;

	FArcRmlSamplesAliasesHero& Scout = Heroes.AddDefaulted_GetRef();
	Scout.Name = TEXT("Edda");
	Scout.Title = TEXT("Scout");
	Scout.Level = 15;

	Selected = Heroes[0];
}

void UArcRmlSamplesAliasesViewModel::SelectNext()
{
	SelectedIndex = (SelectedIndex + 1) % Heroes.Num();
	Selected = Heroes[SelectedIndex];
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Selected);
}
