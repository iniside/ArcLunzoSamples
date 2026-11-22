// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Images/ArcRmlSamplesImages.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesImages)

void UArcRmlSamplesImagesViewModel::PostInitProperties()
{
	Super::PostInitProperties();
	Icon = FirstIcon;
	Picture = FirstPicture;
}

void UArcRmlSamplesImagesViewModel::Swap()
{
	bShowingSecond = !bShowingSecond;
	UE_MVVM_SET_PROPERTY_VALUE(Icon, bShowingSecond ? SecondIcon : FirstIcon);
	UE_MVVM_SET_PROPERTY_VALUE(Picture, bShowingSecond ? SecondPicture : FirstPicture);
}

namespace ArcRmlSamplesImages
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Images/Images.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
