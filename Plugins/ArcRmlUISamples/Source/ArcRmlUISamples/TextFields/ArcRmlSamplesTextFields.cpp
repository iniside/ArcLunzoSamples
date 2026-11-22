// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "TextFields/ArcRmlSamplesTextFields.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesTextFields)

namespace ArcRmlSamplesTextFields
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/TextFields/TextFields.rml")};

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

void UArcRmlSamplesTextFieldsViewModel::SetLive(const FString& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(Live, Value);
}

void UArcRmlSamplesTextFieldsViewModel::Commit(const FString& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(Committed, Value);
	UE_MVVM_SET_PROPERTY_VALUE(CommitCount, CommitCount + 1);
}

void UArcRmlSamplesTextFieldsViewModel::SetNotesLive(const FString& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(NotesLive, Value);
}

void UArcRmlSamplesTextFieldsViewModel::CommitNotes(const FString& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(NotesCommitted, Value);
	UE_MVVM_SET_PROPERTY_VALUE(CommitCount, CommitCount + 1);
}
