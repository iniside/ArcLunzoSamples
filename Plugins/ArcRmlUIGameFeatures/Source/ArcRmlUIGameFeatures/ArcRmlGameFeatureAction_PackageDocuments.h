// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "GameFeatureAction.h"

#include "ArcRmlGameFeatureAction_PackageDocuments.generated.h"

UCLASS(MinimalAPI, BlueprintType, meta = (DisplayName = "Package RML Documents"))
class UArcRmlGameFeatureAction_PackageDocuments final : public UGameFeatureAction
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	ARCRMLUIGAMEFEATURES_API const TArray<FString>& GetPathsToPackage() const;
#endif

private:
#if WITH_EDITORONLY_DATA
	// Package paths of folders or files inside this game feature's own mount, e.g. /MyFeature/UI/Rml/Hud.
	UPROPERTY(EditAnywhere, Category = "Packaging")
	TArray<FString> PathsToPackage;
#endif
};
