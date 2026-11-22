// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "Containers/Ticker.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesWorldMarker.generated.h"

class AActor;
class ULocalPlayer;

UCLASS()
class UArcRmlSamplesWorldMarkerViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Anchor;

	void Track(AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Move();

	virtual void BeginDestroy() override;

private:
	bool UpdateAnchor(float DeltaTime);

	TWeakObjectPtr<AActor> Target;
	FTSTicker::FDelegateHandle TickerHandle;
	double MoveStep = 200.0;
};

namespace ArcRmlSamplesWorldMarker
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
