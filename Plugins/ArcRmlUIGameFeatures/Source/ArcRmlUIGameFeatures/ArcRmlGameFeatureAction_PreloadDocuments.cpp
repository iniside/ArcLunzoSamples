// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlGameFeatureAction_PreloadDocuments.h"

#include "ArcRmlDocumentValidation.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"

#if ARC_WITH_RMLUI
#include "Host/ArcRmlViewportSubsystem.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlGameFeatureAction_PreloadDocuments)

void UArcRmlGameFeatureAction_PreloadDocuments::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);

#if ARC_WITH_RMLUI
	const FGameFeatureStateChangeContext ChangeContext(Context);
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);
	ActiveData.GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this
		, &ThisClass::HandleGameInstanceStart
		, ChangeContext);

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if ((WorldContext.OwningGameInstance != nullptr) && Context.ShouldApplyToWorldContext(WorldContext))
		{
			AddGameInstance(WorldContext.OwningGameInstance
				, ChangeContext);
		}
	}
#endif
}

void UArcRmlGameFeatureAction_PreloadDocuments::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	FPerContextData* ActiveData = ContextData.Find(Context);
	if (ActiveData == nullptr)
	{
		return;
	}

	FWorldDelegates::OnStartGameInstance.Remove(ActiveData->GameInstanceStartHandle);
	for (const FGameInstanceData& GameInstanceData : ActiveData->GameInstances)
	{
		UGameInstance* GameInstance = GameInstanceData.GameInstance.Get();
		if (GameInstance == nullptr)
		{
			continue;
		}

		GameInstance->OnLocalPlayerAddedEvent.Remove(GameInstanceData.LocalPlayerAddedHandle);
		for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
		{
			DiscardDocuments(LocalPlayer);
		}
	}
	ContextData.Remove(Context);
}

#if WITH_EDITOR
EDataValidationResult UArcRmlGameFeatureAction_PreloadDocuments::IsDataValid(FDataValidationContext& Context) const
{
	return CombineDataValidationResults(Super::IsDataValid(Context)
		, ArcRmlDocumentValidation::ValidateDocuments(Documents
			, Context));
}

const TArray<FArcRmlDocumentRef>& UArcRmlGameFeatureAction_PreloadDocuments::GetDocuments() const
{
	return Documents;
}
#endif

void UArcRmlGameFeatureAction_PreloadDocuments::HandleGameInstanceStart(UGameInstance* GameInstance
																		, FGameFeatureStateChangeContext ChangeContext)
{
	const FWorldContext* WorldContext = GameInstance->GetWorldContext();
	if ((WorldContext != nullptr) && ChangeContext.ShouldApplyToWorldContext(*WorldContext))
	{
		AddGameInstance(GameInstance
			, ChangeContext);
	}
}

void UArcRmlGameFeatureAction_PreloadDocuments::AddGameInstance(UGameInstance* GameInstance
																, const FGameFeatureStateChangeContext& ChangeContext)
{
	FPerContextData* ActiveData = ContextData.Find(ChangeContext);
	if (ActiveData == nullptr)
	{
		return;
	}

	const bool bAdded = ActiveData->GameInstances.ContainsByPredicate([GameInstance](const FGameInstanceData& Entry) -> bool
	{
		return Entry.GameInstance.Get() == GameInstance;
	});
	if (bAdded)
	{
		return;
	}

	FGameInstanceData& GameInstanceData = ActiveData->GameInstances.AddDefaulted_GetRef();
	GameInstanceData.GameInstance = GameInstance;
	GameInstanceData.LocalPlayerAddedHandle = GameInstance->OnLocalPlayerAddedEvent.AddUObject(this
		, &ThisClass::PreloadDocuments);

	for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
	{
		PreloadDocuments(LocalPlayer);
	}
}

void UArcRmlGameFeatureAction_PreloadDocuments::PreloadDocuments(ULocalPlayer* LocalPlayer)
{
#if ARC_WITH_RMLUI
	if (UArcRmlViewportSubsystem* Subsystem = LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>())
	{
		for (const FArcRmlDocumentRef& Document : Documents)
		{
			Subsystem->PreloadDocument(Document);
		}
	}
#endif
}

void UArcRmlGameFeatureAction_PreloadDocuments::DiscardDocuments(ULocalPlayer* LocalPlayer)
{
#if ARC_WITH_RMLUI
	if (UArcRmlViewportSubsystem* Subsystem = LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>())
	{
		for (const FArcRmlDocumentRef& Document : Documents)
		{
			Subsystem->DiscardDocument(Document);
		}
	}
#endif
}
