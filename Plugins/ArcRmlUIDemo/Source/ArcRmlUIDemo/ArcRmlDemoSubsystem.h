// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "Containers/Ticker.h"
#include "FieldNotificationId.h"
#include "Host/ArcRmlCloseReason.h"
#include "MVVMViewModelBase.h"
#include "Subsystems/LocalPlayerSubsystem.h"

#include "ArcRmlDemoSubsystem.generated.h"

class UArcRmlDemoDialogViewModel;
class UArcRmlDemoGameViewModel;
class UArcRmlDemoScreenViewModel;
class UArcRmlDemoUIViewModel;
class UArcRmlViewportSubsystem;
class UInputMappingContext;
class UWorld;
struct FArcRmlDemoDialogSpec;
enum class ECommonInputType : uint8;

UCLASS()
class UArcRmlDemoSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Start();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Stop();

	bool IsRunning() const
	{
		return bRunning;
	}

	bool IsTravelPending() const
	{
		return bTravelPending;
	}

	UArcRmlDemoUIViewModel* GetUI() const;

	UArcRmlDemoGameViewModel* GetGame() const;

	UMVVMViewModelBase* FindOrCreateScreenModel(const UClass* Class);

	template <typename T>
	T* FindScreenModel() const
	{
		for (const TObjectPtr<UMVVMViewModelBase>& Model : ScreenModels)
		{
			if (T* Found = Cast<T>(Model.Get()))
			{
				return Found;
			}
		}
		return nullptr;
	}

	UArcRmlDemoScreenViewModel* FindScreenModelByKey(const FString& Screen) const;

	TArray<FString> GetStack() const;
	FString GetTop() const;
	FString GetScreenBelow(const FString& Screen) const;
	void Push(const FString& Screen, const FString& Param = FString());
	void ReplaceTop(const FString& Screen, const FString& Param = FString());
	void ResetTo(TConstArrayView<FString> Screens, const FString& Param = FString());
	void Pop();
	void Back();
	void LeaveScreen();
	void Jump(const FString& Screen);
	void Travel(const TSoftObjectPtr<UWorld>& Map, TConstArrayView<FString> Screens);

	void ShowHubTab(const FString& Document);

	UArcRmlDemoDialogViewModel* OpenDialog(const FArcRmlDemoDialogSpec& Spec, TFunction<void(int32)> OnChoose);
	void ChooseDialog(UArcRmlDemoDialogViewModel* Dialog, int32 Index);

	UArcRmlDemoDialogViewModel* GetDialog() const;

	void ShowToast(const FString& Tone, const FString& Title, const FString& Body = FString());
	void SetInputScheme(const FString& Scheme);
	void ToggleMultiplayer();

	void Later(const UObject* Owner, FName Id, double Seconds, TFunction<void()> Fn);
	void Every(const UObject* Owner, FName Id, double Seconds, TFunction<bool()> Fn);
	void CancelTimer(const UObject* Owner, FName Id);
	void CancelTimers(const UObject* Owner);

private:
	struct FTimer
	{
		TWeakObjectPtr<const UObject> Owner;
		FName Id;
		uint64 Serial = 0;
		double Due = 0.0;
		double Interval = 0.0;
		TFunction<bool()> Fn;
		bool bDead = false;
	};

	UArcRmlViewportSubsystem* GetViewport() const;
	TArray<FArcRmlDocumentRef> GetOpenDocuments() const;
	FArcRmlDocumentRef GetTopDocument() const;
	bool OpenScreen(const FString& Screen, const FString& Param);
	FString CurrentHubTab() const;
	void ResumeAfterTravel();
	void DropDialogChoice();
	void CloseDocuments();
	void PreloadDocuments();
	void DiscardDocuments();
	void RefreshDevBar();
	void AddTimer(const UObject* Owner, FName Id, double Seconds, double Interval, TFunction<bool()> Fn);
	bool HandleTick(float DeltaTime);
	void HandleInputMethodChanged(ECommonInputType InputType);
	void HandleGameFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);
	void AddCommonMapping();
	void RemoveCommonMapping();

	UFUNCTION()
	void HandleScreenActivated(const FArcRmlDocumentRef& Document);

	UFUNCTION()
	void HandleScreenClosed(const FArcRmlDocumentRef& Document, EArcRmlCloseReason Reason);

	UPROPERTY(Transient)
	TObjectPtr<UArcRmlDemoUIViewModel> UIModel;

	UPROPERTY(Transient)
	TObjectPtr<UArcRmlDemoGameViewModel> GameModel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMVVMViewModelBase>> ScreenModels;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> CommonMapping;

	TArray<FString> PreloadedDocuments;
	TArray<FString> PendingScreens;
	TArray<FTimer> Timers;
	FTSTicker::FDelegateHandle TickerHandle;
	FDelegateHandle InputMethodHandle;
	bool bRunning = false;
	bool bTravelPending = false;
};
