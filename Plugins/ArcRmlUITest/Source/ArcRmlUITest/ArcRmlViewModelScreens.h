// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "MVVMViewModelBase.h"
#include "View/MVVMViewModelContextResolver.h"

#include "ArcRmlViewModelScreens.generated.h"

UCLASS()
class UArcRmlScreenViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify)
	FString Label;

	UPROPERTY(BlueprintReadWrite, FieldNotify)
	int32 Level = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify)
	int32 Gold = 0;
};

UCLASS()
class UArcRmlScreenCreatedViewModel : public UArcRmlScreenViewModel
{
	GENERATED_BODY()

public:
	UArcRmlScreenCreatedViewModel();
};

UCLASS()
class UArcRmlScreenResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString Label;

	UPROPERTY(EditAnywhere)
	int32 Level = 0;

	UPROPERTY(EditAnywhere)
	int32 Gold = 0;

	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};

UCLASS()
class AArcRmlScreenPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArcRmlScreenPlayerController();

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UArcRmlScreenViewModel> ScreenViewModel;

protected:
	virtual void BeginPlay() override;
};

UCLASS()
class AArcRmlScreenGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArcRmlScreenGameMode();
};
