// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "WorldMarker/ArcRmlSamplesWorldMarker.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesWorldMarker)

void UArcRmlSamplesWorldMarkerViewModel::Track(AActor* InTarget)
{
	Target = InTarget;
	if (!TickerHandle.IsValid())
	{
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UArcRmlSamplesWorldMarkerViewModel::UpdateAnchor));
	}
	UpdateAnchor(0.0f);
}

void UArcRmlSamplesWorldMarkerViewModel::Move()
{
	AActor* Actor = Target.Get();
	if (!Actor)
	{
		return;
	}

	Actor->SetActorLocation(Actor->GetActorLocation() + FVector(0.0, MoveStep, 0.0));
	MoveStep = -MoveStep;
}

void UArcRmlSamplesWorldMarkerViewModel::BeginDestroy()
{
	FTSTicker::RemoveTicker(TickerHandle);
	TickerHandle.Reset();
	Super::BeginDestroy();
}

bool UArcRmlSamplesWorldMarkerViewModel::UpdateAnchor(float DeltaTime)
{
	if (const AActor* Actor = Target.Get())
	{
		const FVector Location = Actor->GetActorLocation();
		UE_MVVM_SET_PROPERTY_VALUE(Anchor, FString::Printf(TEXT("%.1f %.1f %.1f"), Location.X, Location.Y, Location.Z));
	}
	return true;
}

namespace ArcRmlSamplesWorldMarker
{
	const FName TargetTag(TEXT("SampleMarkerTarget"));

	AActor* FindTarget(UWorld* World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TargetTag))
			{
				return *It;
			}
		}
		return nullptr;
	}

	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		UWorld* World = LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
		if (!Viewport || !World)
		{
			return FArcRmlDocumentRef();
		}

		TWeakObjectPtr<AActor> Target = FindTarget(World);
		TWeakObjectPtr<UArcRmlViewportSubsystem> WeakViewport = Viewport;
		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/WorldMarker/WorldMarker.rml")};
		const bool bOpened = Viewport->OpenDocument(Document, [Target, WeakViewport](const FArcRmlDocumentRef& Opened) -> void
		{
			UArcRmlViewportSubsystem* OpenedViewport = WeakViewport.Get();
			UArcRmlSamplesWorldMarkerViewModel* Marker = OpenedViewport ? Cast<UArcRmlSamplesWorldMarkerViewModel>(OpenedViewport->GetViewModel(Opened, TEXT("marker"))) : nullptr;
			if (Marker)
			{
				Marker->Track(Target.Get());
			}
		});
		return bOpened ? Document : FArcRmlDocumentRef();
	}
}
