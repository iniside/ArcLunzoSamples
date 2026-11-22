// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"

#include "ArcRmlTestGameInputWidget.generated.h"

UCLASS(NotBlueprintable)
class UArcRmlTestGameInputWidget final : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
};
