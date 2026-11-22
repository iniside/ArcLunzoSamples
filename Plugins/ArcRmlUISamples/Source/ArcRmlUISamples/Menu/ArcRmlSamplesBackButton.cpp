// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Menu/ArcRmlSamplesBackButton.h"

#if !UE_BUILD_SHIPPING

#include "ArcRmlSamplesLog.h"
#include "ArcRmlSamplesRunner.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace ArcRmlSamplesBackButton
{
	constexpr int32 ZOrder = 2000;

	class SBackButton : public SButton
	{
	public:
		SLATE_BEGIN_ARGS(SBackButton) {}
			SLATE_EVENT(FOnClicked, OnClicked)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			SButton::Construct(SButton::FArguments()
				.IsFocusable(false)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked(InArgs._OnClicked)
				[
					SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("Back to samples")))
				]);
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			FReply Reply = SButton::OnMouseButtonDown(MyGeometry, MouseEvent);
			// A reply without a focus target makes Slate focus the game SViewport under the cursor (SlateApplication.cpp:5487-5503).
			const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetUserFocusedWidget(MouseEvent.GetUserIndex());
			if (Reply.IsEventHandled() && Focused.IsValid())
			{
				Reply.SetUserFocus(Focused.ToSharedRef(), EFocusCause::SetDirectly);
			}
			return Reply;
		}

		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			// SButton clicks only while hovered (SButton.cpp:425); a press with no move before it, as ArcRml.SimulateKey sends, never hovers it.
			const bool bUnhoveredClick = !IsHovered() && IsPressed() && HasMouseCapture() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
				&& MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition());
			FReply Reply = SButton::OnMouseButtonUp(MyGeometry, MouseEvent);
			if (bUnhoveredClick)
			{
				ExecuteOnClick();
			}
			return Reply;
		}
	};

	FReply HandleClicked(TWeakObjectPtr<ULocalPlayer> WeakPlayer)
	{
		ULocalPlayer* LocalPlayer = WeakPlayer.Get();
		if (!LocalPlayer)
		{
			return FReply::Handled();
		}
		const FArcRmlDocumentRef Closed = ArcRmlSamplesRunner::CloseLastOpened(LocalPlayer, LocalPlayer->GetWorld());
		if (Closed.IsSet())
		{
			UE_LOGF(LogArcRmlSamples, Display, "ArcRmlSamples menu: closed %ls", *Closed.Path);
		}
		return FReply::Handled();
	}

	TSharedPtr<SWidget> Add(ULocalPlayer* LocalPlayer)
	{
		UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
		if (!ViewportClient)
		{
			return nullptr;
		}
		const TSharedRef<SWidget> Widget = SNew(SBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(16.0f)
			[
				SNew(SBox)
				.MinDesiredWidth(120.0f)
				.MinDesiredHeight(32.0f)
				[
					SNew(SBackButton)
					.OnClicked(FOnClicked::CreateStatic(&HandleClicked, TWeakObjectPtr<ULocalPlayer>(LocalPlayer)))
				]
			];
		ViewportClient->AddViewportWidgetForPlayer(LocalPlayer, Widget, ZOrder);
		return Widget;
	}

	void Remove(ULocalPlayer* LocalPlayer, const TSharedPtr<SWidget>& Widget)
	{
		UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
		if (ViewportClient && Widget.IsValid())
		{
			ViewportClient->RemoveViewportWidgetForPlayer(LocalPlayer, Widget.ToSharedRef());
		}
	}
}

#endif
