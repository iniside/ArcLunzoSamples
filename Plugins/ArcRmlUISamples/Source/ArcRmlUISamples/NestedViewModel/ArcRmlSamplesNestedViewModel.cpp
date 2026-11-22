// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "NestedViewModel/ArcRmlSamplesNestedViewModel.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesNestedViewModel)

void UArcRmlSamplesNestedChildViewModel::AddPoint()
{
	UE_MVVM_SET_PROPERTY_VALUE(Points, Points + 1);
}

UArcRmlSamplesNestedParentViewModel::UArcRmlSamplesNestedParentViewModel()
{
	Child = CreateDefaultSubobject<UArcRmlSamplesNestedChildViewModel>(TEXT("Child"));
	Child->Name = TEXT("Leader");

	const TCHAR* ChildNames[] = {TEXT("Archer"), TEXT("Healer"), TEXT("Scout")};
	for (const TCHAR* ChildName : ChildNames)
	{
		UArcRmlSamplesNestedChildViewModel* Member = CreateDefaultSubobject<UArcRmlSamplesNestedChildViewModel>(ChildName);
		Member->Name = ChildName;
		Children.Add(Member);
	}
}

namespace ArcRmlSamplesNestedViewModel
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/NestedViewModel/NestedViewModel.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
