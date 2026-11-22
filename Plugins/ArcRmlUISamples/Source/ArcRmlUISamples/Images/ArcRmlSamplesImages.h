// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "ArcRmlImageRef.h"
#include "Engine/Texture2D.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesImages.generated.h"

class ULocalPlayer;

UCLASS(Abstract, Blueprintable)
class UArcRmlSamplesImagesViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FArcRmlImageRef Icon;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	TSoftObjectPtr<UTexture2D> Picture;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Swap();

	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, Category = "Samples")
	FArcRmlImageRef FirstIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Samples")
	FArcRmlImageRef SecondIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Samples")
	TSoftObjectPtr<UTexture2D> FirstPicture;

	UPROPERTY(EditDefaultsOnly, Category = "Samples")
	TSoftObjectPtr<UTexture2D> SecondPicture;

private:
	bool bShowingSecond = false;
};

namespace ArcRmlSamplesImages
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
