// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "ArcRmlDocumentRef.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"

#include "ArcRmlGameFeatureAction_PreloadDocuments.generated.h"

class FDataValidationContext;
class UGameInstance;
class ULocalPlayer;

UCLASS(MinimalAPI, meta = (DisplayName = "Preload RML Documents"))
class UArcRmlGameFeatureAction_PreloadDocuments final : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	ARCRMLUIGAMEFEATURES_API const TArray<FArcRmlDocumentRef>& GetDocuments() const;
#endif

private:
	UPROPERTY(EditAnywhere, Category = "RmlUI", meta = (TitleProperty = "Path"))
	TArray<FArcRmlDocumentRef> Documents;

	struct FGameInstanceData
	{
		TWeakObjectPtr<UGameInstance> GameInstance;
		FDelegateHandle LocalPlayerAddedHandle;
	};

	struct FPerContextData
	{
		FDelegateHandle GameInstanceStartHandle;
		TArray<FGameInstanceData> GameInstances;
	};

	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;

	void HandleGameInstanceStart(UGameInstance* GameInstance
								 , FGameFeatureStateChangeContext ChangeContext);

	void AddGameInstance(UGameInstance* GameInstance
						 , const FGameFeatureStateChangeContext& ChangeContext);

	void PreloadDocuments(ULocalPlayer* LocalPlayer);

	void DiscardDocuments(ULocalPlayer* LocalPlayer);
};
