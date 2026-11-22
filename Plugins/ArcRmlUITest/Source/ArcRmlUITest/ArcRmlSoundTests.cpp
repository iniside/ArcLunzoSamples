// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "CommonInputTypeEnum.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"

namespace ArcRmlSoundTestsPrivate
{
	const TCHAR* const SoundFolder = TEXT("/ArcRmlUITest/UI/Sound/");

	FString SoundPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s%s.%s"), SoundFolder, Name, Name);
	}

	FVector2f GetCenter(const FArcRmlTestLayoutElement& Element)
	{
		return Element.Rect.Position + Element.Rect.Size * 0.5f;
	}

	FString Join(const TArray<FString>& Sounds)
	{
		return FString::Join(Sounds, TEXT(", "));
	}

	class FAssetReferenceCollector final : public FOutputDevice
	{
	public:
		FAssetReferenceCollector()
		{
			GLog->AddOutputDevice(this);
		}

		virtual ~FAssetReferenceCollector() override
		{
			GLog->RemoveOutputDevice(this);
		}

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (FCString::Strstr(Message, TEXT("asset reference '")))
			{
				FScopeLock Lock(&LinesLock);
				Lines.Add(FString(Message));
			}
		}

		virtual bool CanBeUsedOnAnyThread() const override
		{
			return true;
		}

		virtual bool CanBeUsedOnMultipleThreads() const override
		{
			return true;
		}

		int32 Count(const FString& File, const FString& Reference) const
		{
			FScopeLock Lock(&LinesLock);
			int32 Matches = 0;
			for (const FString& Line : Lines)
			{
				if (Line.Contains(File) && Line.Contains(Reference))
				{
					++Matches;
				}
			}
			return Matches;
		}

		FString Describe() const
		{
			FScopeLock Lock(&LinesLock);
			return FString::Printf(TEXT("%d reference lines: %s"), Lines.Num(), *FString::Join(Lines, TEXT(" | ")));
		}

	private:
		mutable FCriticalSection LinesLock;
		TArray<FString> Lines;
	};
}

TEST_CLASS(ArcRmlSoundTests, "ArcRmlUI.Sound")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	FString SheetBinary(const TCHAR* Name) const
	{
		return FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), FString::Printf(TEXT("Sound/%s.rcssc"), Name));
	}

	bool OpenAndFlush(const TCHAR* Name)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("Sound/%s.rml"), Name));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument;
	}

	bool SettleFrames()
	{
		return View.Flush() && View.Flush();
	}

	bool CenterOf(const FString& DomId, FVector2f& OutCenter)
	{
		if (!View.ParseLayout(Elements))
		{
			return false;
		}
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		if (!Element)
		{
			return false;
		}
		OutCenter = ArcRmlSoundTestsPrivate::GetCenter(*Element);
		return true;
	}

	bool MoveMouseTo(const FString& DomId)
	{
		FVector2f Center;
		if (!CenterOf(DomId, Center))
		{
			return false;
		}
		View.MouseMove(Center);
		return SettleFrames();
	}

	bool TakeOverWithGamepad()
	{
		View.SetInputType(ECommonInputType::Gamepad);
		return SettleFrames();
	}

	bool Press(const FKey& Key)
	{
		View.PressKey(Key);
		return SettleFrames();
	}

	bool ClickCloseAndSettle()
	{
		View.ClearPlayedSounds();
		View.ClickElement(TEXT("#sn-close"));
		return SettleFrames() && View.GetCloseRequests() == 1;
	}

	bool OpenFrameAndEmbedTab()
	{
		const FString TabPath = ArcRmlUITestHelpers::MakePath(TEXT("Sound/SoundTab.rml"));
		return FArcRmlTestView::DropCompiledDocument(TabPath) && OpenAndFlush(TEXT("SoundFrame")) && View.Embed(TabPath) && SettleFrames()
			&& View.GetProbe().EmbeddedPath == TabPath;
	}

	FString Played() const
	{
		return ArcRmlSoundTestsPrivate::Join(View.GetPlayedSounds());
	}

	FString DescribeState() const
	{
		return FString::Printf(TEXT("played [%s]; nav focus '%s'; %s"), *Played(), *ArcRmlUITestHelpers::GetNavFocusDomId(View), *ArcRmlUITestHelpers::DescribeDiagnostics(View));
	}

	void ExpectCompileError(const TCHAR* Name, const FString& Reason)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("Sound/%s.rml"), Name));
		const FString Sheet = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("Sound/%s.rcss"), Name));
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, Sheet);
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, FString(TEXT("RmlUi: Cannot load style sheet ")) + Sheet);
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, FString(TEXT("RmlUi: Failed to load style sheet ")) + Sheet);
		IFileManager::Get().Delete(*SheetBinary(Name), false, false, true);
		FArcRmlTestView::ClearDocumentCaches();
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path), TEXT("the compiled document could not be dropped")));
		ArcRmlUITestHelpers::OpenView(View, Path);
		View.Flush();
		const int32 Reported = ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, Reason, Sheet, 6) + ArcRmlUITestHelpers::CountDiagnosticsAt(View, Reason, Sheet, 6);
		ASSERT_THAT(AreEqual(1, Reported, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View) + ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		IFileManager::Get().Delete(*SheetBinary(Name), false, false, true);
		ASSERT_THAT(IsFalse(IFileManager::Get().FileExists(*SheetBinary(Name)), TEXT("an invalid style sheet was compiled")));
	}

	TEST_METHOD(MouseClick_PlaysTheThemeClickSoundOnce)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(MoveMouseTo(TEXT("sn-btn")), TEXT("the mouse move flush failed")));
		View.ClearPlayedSounds();

		FVector2f Center;
		ASSERT_THAT(IsTrue(CenterOf(TEXT("sn-btn"), Center)));
		View.MouseClick(Center);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Click")), Played(), DescribeState()));
	}

	TEST_METHOD(AcceptKey_PlaysTheThemeClickSoundOnce)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("sn-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeState()));
		View.ClearPlayedSounds();

		ASSERT_THAT(IsTrue(Press(EKeys::Enter), TEXT("enter flush failed")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Click")), Played(), DescribeState()));
	}

	TEST_METHOD(HoverOnEntry_PlaysOnceAndNothingInsideTheElement)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), Played(), TEXT("the document played a sound on open")));

		ASSERT_THAT(IsTrue(MoveMouseTo(TEXT("sn-icon")), TEXT("the move onto the icon flush failed")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Hover")), Played(), DescribeState()));
		View.ClearPlayedSounds();

		ASSERT_THAT(IsTrue(MoveMouseTo(TEXT("sn-label")), TEXT("the move onto the label flush failed")));
		ASSERT_THAT(AreEqual(FString(), Played(), FString::Printf(TEXT("moving from the icon to the label inside the button played a sound; %s"), *DescribeState())));
	}

	TEST_METHOD(ContainerWithOnlyAnInheritedValue_PlaysNothingOnHover)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Group = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sn-group"));
		ASSERT_THAT(IsNotNull(Group));
		View.MouseMove(Group->Rect.Position + FVector2f(Group->Rect.Size.X * 0.2f, Group->Rect.Size.Y * 0.5f));
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Hover")), Played(), DescribeState()));
		View.ClearPlayedSounds();

		ASSERT_THAT(IsTrue(MoveMouseTo(TEXT("sn-inner")), TEXT("the move onto the inner container flush failed")));
		ASSERT_THAT(AreEqual(FString(), Played(), FString::Printf(TEXT("a container that only inherits the hover sound played it; %s"), *DescribeState())));
	}

	TEST_METHOD(ContainerWithOnlyAnInheritedValue_PlaysNothingOnPadFocus)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		for (int32 Step = 0; Step < 4 && ArcRmlUITestHelpers::GetNavFocusDomId(View) != TEXT("sn-group"); ++Step)
		{
			ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down), TEXT("pad down flush failed")));
		}
		ASSERT_THAT(AreEqual(FString(TEXT("sn-group")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeState()));
		View.ClearPlayedSounds();

		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Right), TEXT("pad right flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("sn-inner")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeState()));
		ASSERT_THAT(AreEqual(FString(), Played(), FString::Printf(TEXT("a focusable container that only inherits the focus sound played it; %s"), *DescribeState())));
	}

	TEST_METHOD(PadMove_PlaysTheFocusSoundAndTheTakeoverPlaysNothing)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("sn-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeState()));
		ASSERT_THAT(AreEqual(FString(), Played(), FString::Printf(TEXT("the gamepad takeover recovery played a sound; %s"), *DescribeState())));

		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down), TEXT("pad down flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("sn-gone")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeState()));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Focus")), Played(), DescribeState()));
	}

	TEST_METHOD(HostFocusRecovery_PlaysNothing)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down), TEXT("pad down flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("sn-gone")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeState()));
		View.ClearPlayedSounds();

		View.SetModelValue(TEXT("snd"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showGone")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(SettleFrames(), TEXT("flush after removing the focused element failed")));
		const FString Recovered = ArcRmlUITestHelpers::GetNavFocusDomId(View);
		ASSERT_THAT(IsFalse(Recovered.IsEmpty() || Recovered == TEXT("sn-gone"), FString::Printf(TEXT("the host did not recover the focus; %s"), *DescribeState())));
		ASSERT_THAT(AreEqual(FString(), Played(), FString::Printf(TEXT("the host focus recovery played a sound; %s"), *DescribeState())));
	}

	TEST_METHOD(BackThatClosesTheView_PlaysOnlyTheBackSound)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Escape), TEXT("escape flush failed")));
		ASSERT_THAT(AreEqual(1, View.GetCloseRequests(), TEXT("back did not close the view")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Back")), Played(), DescribeState()));
	}

	TEST_METHOD(BackToTheParentArea_PlaysOnlyTheBackSound)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundArea")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("leaf")), View.GetProbe().ActiveArea, DescribeState()));
		View.ClearPlayedSounds();

		ASSERT_THAT(IsTrue(Press(EKeys::Escape), TEXT("escape flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("top")), View.GetProbe().ActiveArea, DescribeState()));
		ASSERT_THAT(AreEqual(FString(TEXT("sa-top-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeState()));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests(), TEXT("back closed the view instead of returning to the parent area")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Back")), Played(), DescribeState()));
	}

	TEST_METHOD(CloseAction_PlaysTheCloseSound)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClickElement(TEXT("#sn-close"));
		ASSERT_THAT(IsTrue(SettleFrames(), TEXT("flush after the close click failed")));
		ASSERT_THAT(AreEqual(1, View.GetCloseRequests(), TEXT("the close action did not close the view")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Close")), Played(), DescribeState()));
	}

	TEST_METHOD(OverrideSheet_ChangesTheClickAsset)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundOverride")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(MoveMouseTo(TEXT("sn-btn")), TEXT("the mouse move flush failed")));
		View.ClearPlayedSounds();

		FVector2f Center;
		ASSERT_THAT(IsTrue(CenterOf(TEXT("sn-btn"), Center)));
		View.MouseClick(Center);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Alt")), Played(), DescribeState()));
		View.ClearPlayedSounds();

		View.ClickElement(TEXT("#sn-close"));
		ASSERT_THAT(IsTrue(SettleFrames(), TEXT("flush after the close click failed")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Focus")), Played(), FString::Printf(TEXT("the close after the override did not play the override's --sound-close; %s"), *DescribeState())));
	}

#if WITH_EDITOR
	TEST_METHOD(OverrideSheetAddedToTheView_ChangesTheCloseSound)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.AddOverrideStyleSheet(ArcRmlUITestHelpers::MakePath(TEXT("Sound/SoundCloseTheme.rcss")));
		ASSERT_THAT(IsTrue(SettleFrames(), TEXT("flush after the override sheet failed")));

		ASSERT_THAT(IsTrue(ClickCloseAndSettle(), TEXT("the close action did not close the view")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Alt")), Played(), FString::Printf(TEXT("the close after the override sheet did not play its --sound-close; %s"), *DescribeState())));
	}
#endif

	TEST_METHOD(EmbeddedTabSheet_DoesNotChangeTheFrameCloseSound)
	{
		ASSERT_THAT(IsTrue(OpenFrameAndEmbedTab(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(ClickCloseAndSettle(), TEXT("the close action did not close the view")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Close")), Played(), FString::Printf(TEXT("the tab sheet's body --sound-close changed the frame close sound; %s"), *DescribeState())));
	}

	TEST_METHOD(ClearedEmbed_RestoresTheFrameCloseSound)
	{
		ASSERT_THAT(IsTrue(OpenFrameAndEmbedTab(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearEmbed();
		ASSERT_THAT(IsTrue(SettleFrames(), TEXT("flush after clearing the embed failed")));
		ASSERT_THAT(IsTrue(View.GetProbe().EmbeddedPath.IsEmpty(), View.GetProbe().EmbeddedPath));

		ASSERT_THAT(IsTrue(ClickCloseAndSettle(), TEXT("the close action did not close the view")));
		ASSERT_THAT(AreEqual(ArcRmlSoundTestsPrivate::SoundPath(TEXT("MS_Close")), Played(), FString::Printf(TEXT("the close after clearing the embed did not play the frame's --sound-close; %s"), *DescribeState())));
	}

	TEST_METHOD(SoundCuePath_IsACompileError)
	{
		ExpectCompileError(TEXT("BadCue"), TEXT("is a /Script/Engine.SoundCue, not a MetaSoundSource"));
	}

	TEST_METHOD(Texture2DPath_IsACompileError)
	{
		ExpectCompileError(TEXT("BadTexture"), TEXT("is a /Script/Engine.Texture2D, not a MetaSoundSource"));
	}

	TEST_METHOD(MissingPath_IsACompileError)
	{
		ExpectCompileError(TEXT("BadMissing"), TEXT("does not exist"));
	}

	TEST_METHOD(VarValue_IsACompileError)
	{
		ExpectCompileError(TEXT("BadVar"), TEXT("var() is not allowed in a sound property"));
	}

	TEST_METHOD(CompileReport_ListsTheSoundReferences)
	{
		IFileManager::Get().Delete(*SheetBinary(TEXT("SoundTest")), false, false, true);
		FArcRmlTestView::ClearDocumentCaches();
		ArcRmlSoundTestsPrivate::FAssetReferenceCollector Collector;
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*SheetBinary(TEXT("SoundTest"))), TEXT("the style sheet was not compiled at open")));
		for (const TCHAR* const Name : {TEXT("MS_Click"), TEXT("MS_Hover"), TEXT("MS_Focus"), TEXT("MS_Back"), TEXT("MS_Close")})
		{
			const FString Reference = FString::Printf(TEXT("asset reference '%s'"), *ArcRmlSoundTestsPrivate::SoundPath(Name));
			ASSERT_THAT(AreEqual(1, Collector.Count(TEXT("Sound/SoundTest.rcss("), Reference), FString::Printf(TEXT("%s is not reported once; %s"), Name, *Collector.Describe())));
		}
	}

	TEST_METHOD(LoadSounds_ListsTheDocumentsSounds)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("SoundTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SettleFrames()));
		const TArray<FString>& Loaded = View.GetLoadedSounds();
		ASSERT_THAT(AreEqual(5, Loaded.Num(), ArcRmlSoundTestsPrivate::Join(Loaded)));
		for (const TCHAR* const Name : {TEXT("MS_Click"), TEXT("MS_Hover"), TEXT("MS_Focus"), TEXT("MS_Back"), TEXT("MS_Close")})
		{
			ASSERT_THAT(IsTrue(Loaded.Contains(ArcRmlSoundTestsPrivate::SoundPath(Name)), FString::Printf(TEXT("%s is not loaded: %s"), Name, *ArcRmlSoundTestsPrivate::Join(Loaded))));
		}
	}
};
