// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlPreviewFieldsTestTypes.h"
#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MVVM/ArcRmlModelBinding.h"
#include "Containers/Ticker.h"
#include "Preview/ArcRmlPreviewEmbed.h"
#include "UObject/StrongObjectPtr.h"

namespace ArcRmlTabTestsPrivate
{
	const TCHAR* const VariantFrame = TEXT("Embeds/VariantFrame.rml");
	const TCHAR* const VariantTab = TEXT("Embeds/VariantTab.rml");
	const TCHAR* const IfFrame = TEXT("Embeds/IfFrame.rml");
	const TCHAR* const IfTab = TEXT("Embeds/IfTab.rml");
	const TCHAR* const NoSheetTab = TEXT("Embeds/NoSheetTab.rml");
	const TCHAR* const ScopeFrame = TEXT("Tabs/ScopeFrame.rml");
	const TCHAR* const ScopeTabA = TEXT("Tabs/ScopeTabA.rml");
	const TCHAR* const ScopeTabB = TEXT("Tabs/ScopeTabB.rml");
	const TCHAR* const AltVariant = TEXT("alt");

	FString DescribeLook(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		if (!Element)
		{
			return TEXT("missing");
		}
		return FString::Printf(TEXT("color %s, background %s, at %.1f,%.1f, size %.1f x %.1f"), *Element->Color, *Element->BackgroundColor, Element->Rect.Position.X,
			Element->Rect.Position.Y, Element->Rect.Size.X, Element->Rect.Size.Y);
	}

	FString MakeDumpDir(const TCHAR* Name)
	{
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), Name));
		IFileManager::Get().DeleteDirectory(*Dir, false, true);
		return Dir;
	}
}

TEST_CLASS(ArcRmlTabTests, "ArcRmlUI.Tabs")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool FlushAndRead()
	{
		return View.Flush() && View.ParseLayout(Elements);
	}

	TEST_METHOD(EmbedSwapsTabAndClearEmbedRestoresFrame)
	{
		const FString FramePath = ArcRmlUITestHelpers::MakePath(TEXT("Tabs/TabFrame.rml"));
		const FString TabAPath = ArcRmlUITestHelpers::MakePath(TEXT("Tabs/TabA.rml"));
		const FString TabBPath = ArcRmlUITestHelpers::MakePath(TEXT("Tabs/TabB.rml"));

		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, FramePath)));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetProbe().EmbeddedPath.IsEmpty()));
		ASSERT_THAT(AreEqual(FString(TEXT("Frame")), ArcRmlUITestHelpers::GetText(Elements, TEXT("frame-title"))));

		ASSERT_THAT(IsTrue(View.Embed(TabAPath), TEXT("the frame did not accept tab A")));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(TabAPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("Tab A")), ArcRmlUITestHelpers::GetText(Elements, TEXT("tab-a"))));

		ASSERT_THAT(IsTrue(View.Embed(TabBPath), TEXT("the frame did not accept tab B")));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(TabBPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* KeptA = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tab-a"));
		ASSERT_THAT(IsTrue(KeptA && !KeptA->bVisible, TEXT("tab A is not kept hidden after the swap")));
		ASSERT_THAT(AreEqual(FString(TEXT("Tab B")), ArcRmlUITestHelpers::GetText(Elements, TEXT("tab-b"))));
		ASSERT_THAT(IsTrue(View.GetProbe().ModelNames.Contains(TEXT("taba")), FString::Join(View.GetProbe().ModelNames, TEXT(", "))));
		ASSERT_THAT(IsTrue(View.GetProbe().ModelNames.Contains(TEXT("tabb")), FString::Join(View.GetProbe().ModelNames, TEXT(", "))));

		View.ClearEmbed();
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(IsTrue(View.GetProbe().EmbeddedPath.IsEmpty(), View.GetProbe().EmbeddedPath));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tab-b")), TEXT("tab B is still in the document after ClearEmbed")));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tab-a")), TEXT("kept tab A is still in the document after ClearEmbed")));
		ASSERT_THAT(IsFalse(View.GetProbe().ModelNames.Contains(TEXT("taba")), FString::Join(View.GetProbe().ModelNames, TEXT(", "))));
		ASSERT_THAT(IsFalse(View.GetProbe().ModelNames.Contains(TEXT("tabb")), FString::Join(View.GetProbe().ModelNames, TEXT(", "))));
		ASSERT_THAT(AreEqual(FString(TEXT("Frame")), ArcRmlUITestHelpers::GetText(Elements, TEXT("frame-title"))));
		ASSERT_THAT(AreEqual(FramePath, View.GetProbe().DocumentPath));
	}

	TEST_METHOD(DataActionOpenOfSiblingTab_SwapsTab)
	{
		const FString TabAPath = ArcRmlUITestHelpers::MakePath(TEXT("Tabs/TabA.rml"));
		const FString TabBPath = ArcRmlUITestHelpers::MakePath(TEXT("Tabs/TabB.rml"));

		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, TabAPath)));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(TabAPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.ClickElement(TEXT("#open-b"));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the click failed")));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(TabBPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("Tab B")), ArcRmlUITestHelpers::GetText(Elements, TEXT("tab-b"))));
		const FArcRmlTestLayoutElement* KeptA = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tab-a"));
		ASSERT_THAT(IsTrue(KeptA && !KeptA->bVisible, TEXT("tab A is not kept hidden after the swap")));
		ASSERT_THAT(IsTrue(View.GetProbe().ModelNames.Contains(TEXT("taba")), FString::Join(View.GetProbe().ModelNames, TEXT(", "))));
		ASSERT_THAT(IsTrue(View.GetProbe().ModelNames.Contains(TEXT("tabb")), FString::Join(View.GetProbe().ModelNames, TEXT(", "))));
		ASSERT_THAT(AreEqual(0, View.GetOpenRequests().Num()));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests()));
	}

	TEST_METHOD(EditorPreviewSwap_EmbedsTheSiblingWithItsClassModel)
	{
		const FString FramePath = ArcRmlUITestHelpers::MakePath(TEXT("Tabs/TabFrame.rml"));
		const FString TabAPath = ArcRmlUITestHelpers::MakePath(TEXT("Tabs/TabA.rml"));
		const FString ClassTabPath = ArcRmlUITestHelpers::MakePath(TEXT("Tabs/ClassTab.rml"));

		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, TabAPath)));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(TabAPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		TestRunner->AddExpectedErrorPlain(TEXT("only tabs of"), EAutomationExpectedErrorFlags::Contains, 1);
		ASSERT_THAT(IsFalse(ArcRmlPreviewEmbed::ReadTab(ClassTabPath, ArcRmlUITestHelpers::MakePath(TEXT("Embeds/VariantFrame.rml"))).IsSet(),
			TEXT("a tab of another frame was accepted")));
		TOptional<FArcRmlDocumentHead> Head = ArcRmlPreviewEmbed::ReadTab(ClassTabPath, FramePath);
		ASSERT_THAT(IsTrue(Head.IsSet()));

		const TStrongObjectPtr<UArcRmlPreviewFieldsTestViewModel> Model(NewObject<UArcRmlPreviewFieldsTestViewModel>());
		Model->Base = 42;
		int32 Found = 0;
		const bool bSwapped = View.SwapWith(ClassTabPath, [&Head, &FramePath, &ClassTabPath, &Model, &Found](const TSharedRef<FArcRmlUIThread>& Thread, uint32 ViewId, FArcRmlModelBinding& Frame,
			FArcRmlTabModels& Tab) -> bool
		{
			return ArcRmlPreviewEmbed::SwapTab(Thread, ViewId, Frame, Tab, FramePath, ClassTabPath, MoveTemp(Head->Declarations), FString(), FArcRmlValue(),
				FArcRmlOnDocumentAction(), [&Model, &Found](const FArcRmlViewModelDeclaration& Declaration) -> UObject*
				{
					++Found;
					return Declaration.Name == TEXT("classtab") ? Model.Get() : nullptr;
				});
		});
		ASSERT_THAT(IsTrue(bSwapped));
		ASSERT_THAT(AreEqual(1, Found, TEXT("the class model of the swapped-in tab was not asked for")));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		FTSTicker::GetCoreTicker().Tick(0.0f);
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(ClassTabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* KeptA = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tab-a"));
		ASSERT_THAT(IsTrue(KeptA && !KeptA->bVisible, TEXT("tab A is not kept hidden after the preview swap")));
		ASSERT_THAT(AreEqual(FString(TEXT("42")), ArcRmlUITestHelpers::GetText(Elements, TEXT("class-base")),
			FString::Printf(TEXT("the swapped-in tab did not bind its class model: '%s', models %s"), *ArcRmlUITestHelpers::GetText(Elements, TEXT("class-base")),
				*FString::Join(View.GetProbe().ModelNames, TEXT(", ")))));
	}

	TEST_METHOD(PreviewOfEmbeddedTab_KeepsFrameVariant)
	{
		using namespace ArcRmlTabTestsPrivate;
		const FString FramePath = ArcRmlUITestHelpers::MakePath(VariantFrame);
		const FString TabPath = ArcRmlUITestHelpers::MakePath(VariantTab);
		ASSERT_THAT(IsTrue(View.Open(FramePath, ArcRmlUITestHelpers::ViewSize, 1.0f, AltVariant)));
		ASSERT_THAT(IsTrue(View.Embed(TabPath)));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(FString(TEXT("Frame Alt")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vf-title")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("Tab Base")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vt-label"))));

		ASSERT_THAT(IsTrue(View.Preview(TabPath, AltVariant), TEXT("the tab preview did not reopen the view")));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("Tab Alt")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vt-label"))));
		ASSERT_THAT(AreEqual(FString(TEXT("Frame Alt")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vf-title")), TEXT("the tab preview dropped the frame variant")));
	}

	TEST_METHOD(PreviewOfFrameWithEmbed_ReopensEmbed)
	{
		using namespace ArcRmlTabTestsPrivate;
		const FString FramePath = ArcRmlUITestHelpers::MakePath(VariantFrame);
		const FString TabPath = ArcRmlUITestHelpers::MakePath(VariantTab);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, FramePath)));
		ASSERT_THAT(IsTrue(View.Embed(TabPath, AltVariant)));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(FString(TEXT("Tab Alt")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vt-label")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(View.Preview(FramePath, AltVariant), TEXT("the frame preview did not reopen the view")));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(FString(TEXT("Frame Alt")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vf-title")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, TEXT("the frame preview reopened the frame without its embed")));
		ASSERT_THAT(AreEqual(FString(TEXT("Tab Alt")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vt-label")), TEXT("the reopened embed lost its variant")));
	}

	TEST_METHOD(ReloadOfFrameWithEmbed_KeepsBothVariants)
	{
		using namespace ArcRmlTabTestsPrivate;
		const FString FramePath = ArcRmlUITestHelpers::MakePath(VariantFrame);
		const FString TabPath = ArcRmlUITestHelpers::MakePath(VariantTab);
		ASSERT_THAT(IsTrue(View.Open(FramePath, ArcRmlUITestHelpers::ViewSize, 1.0f, AltVariant)));
		ASSERT_THAT(IsTrue(View.Embed(TabPath, AltVariant)));
		ASSERT_THAT(IsTrue(FlushAndRead()));

		ASSERT_THAT(IsTrue(View.Reload(), TEXT("the reload did not reopen the view")));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("Tab Alt")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vt-label"))));
		ASSERT_THAT(AreEqual(FString(TEXT("Frame Alt")), ArcRmlUITestHelpers::GetText(Elements, TEXT("vf-title")), TEXT("the reload dropped the frame variant")));
	}

	TEST_METHOD(ClearEmbedAfterWrapperRemoved_LeavesOnlyTheFrameSheet)
	{
		using namespace ArcRmlTabTestsPrivate;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(IfFrame))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		const FArcRmlTestLayoutElement* Marker = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-marker"));
		ASSERT_THAT(IsNotNull(Marker, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const float FrameWidth = Marker->Rect.Size.X;
		ASSERT_THAT(IsTrue(FrameWidth > 300.0f, FString::Printf(TEXT("frame marker width %.1f"), FrameWidth)));

		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(IfTab))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		const FArcRmlTestLayoutElement* TabContent = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-tab"));
		ASSERT_THAT(IsNotNull(TabContent));
		ASSERT_THAT(IsNear(180.0f, TabContent->Rect.Size.X, 0.5f, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		Marker = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-marker"));
		ASSERT_THAT(IsNotNull(Marker));
		ASSERT_THAT(IsNear(FrameWidth, Marker->Rect.Size.X, 0.5f, TEXT("the tab sheet's rule for the frame marker reached an element outside the embed wrapper")));
		const FString EmbeddedDir = MakeDumpDir(TEXT("ClearAfterWrapperEmbedded"));
		View.DumpLayout(EmbeddedDir);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		FString EmbeddedHtml;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(EmbeddedHtml, *FPaths::Combine(EmbeddedDir, TEXT("IfFrame.html"))), TEXT("the layout dump wrote no html")));
		ASSERT_THAT(IsTrue(EmbeddedHtml.Contains(TEXT("IfTab.rcss")), TEXT("the dump lacks the tab sheet while the tab is embedded, so its absence later proves nothing")));

		View.SetModelValue(TEXT("ifframe"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showEmbed")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(IsTrue(View.GetProbe().EmbeddedPath.IsEmpty(), TEXT("the data-if did not destroy the embed wrapper")));
		Marker = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-marker"));
		ASSERT_THAT(IsNotNull(Marker));
		ASSERT_THAT(IsNear(FrameWidth, Marker->Rect.Size.X, 0.5f, TEXT("destroying the wrapper changed the frame marker")));
		const FString AppliedDir = MakeDumpDir(TEXT("ClearAfterWrapperApplied"));
		View.DumpLayout(AppliedDir);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		FString AppliedHtml;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(AppliedHtml, *FPaths::Combine(AppliedDir, TEXT("IfFrame.html"))), TEXT("the layout dump wrote no html")));
		ASSERT_THAT(IsFalse(AppliedHtml.Contains(TEXT("IfTab.rcss")), TEXT("the tab sheet outlived its destroyed wrapper")));

		View.ClearEmbed();
		ASSERT_THAT(IsTrue(FlushAndRead()));
		Marker = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-marker"));
		ASSERT_THAT(IsNotNull(Marker));
		ASSERT_THAT(IsNear(FrameWidth, Marker->Rect.Size.X, 0.5f, TEXT("ClearEmbed changed the frame marker")));
		const FString RestoredDir = MakeDumpDir(TEXT("ClearAfterWrapperRestored"));
		View.DumpLayout(RestoredDir);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		FString RestoredHtml;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(RestoredHtml, *FPaths::Combine(RestoredDir, TEXT("IfFrame.html"))), TEXT("the layout dump wrote no html")));
		ASSERT_THAT(IsTrue(RestoredHtml.Contains(TEXT("Common.rcss")), TEXT("the dump lacks the frame sheet")));
		ASSERT_THAT(IsFalse(RestoredHtml.Contains(TEXT("IfTab.rcss")), TEXT("the tab sheet is listed after ClearEmbed")));
	}

	TEST_METHOD(ReshownEmbedContainer_EmbedsTheTabAgain)
	{
		using namespace ArcRmlTabTestsPrivate;
		const FString TabPath = ArcRmlUITestHelpers::MakePath(IfTab);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(IfFrame))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		const FArcRmlTestLayoutElement* Marker = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-marker"));
		ASSERT_THAT(IsNotNull(Marker, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const float FrameWidth = Marker->Rect.Size.X;
		ASSERT_THAT(IsTrue(View.Embed(TabPath)));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(FString(TEXT("If Tab")), ArcRmlUITestHelpers::GetText(Elements, TEXT("if-tab")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.SetModelValue(TEXT("ifframe"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showEmbed")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(IsTrue(View.GetProbe().EmbeddedPath.IsEmpty(), TEXT("the data-if did not destroy the embed wrapper")));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-tab"))));

		View.SetModelValue(TEXT("ifframe"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showEmbed")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, TEXT("the shown container did not get the tab again")));
		ASSERT_THAT(AreEqual(FString(TEXT("If Tab")), ArcRmlUITestHelpers::GetText(Elements, TEXT("if-tab")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* TabContent = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-tab"));
		ASSERT_THAT(IsNotNull(TabContent));
		ASSERT_THAT(IsNear(180.0f, TabContent->Rect.Size.X, 0.5f, TEXT("the tab sheet is not applied after the re-embed")));
		Marker = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("if-marker"));
		ASSERT_THAT(IsNotNull(Marker));
		ASSERT_THAT(IsNear(FrameWidth, Marker->Rect.Size.X, 0.5f, TEXT("the re-embedded tab sheet reached the frame marker outside the wrapper")));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountErrorsOfKind(View, EArcRmlDiagnosticKind::Load), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(SwapRestylesOnlyTheTabSubtree_FrameElementsKeepTheirValues)
	{
		using namespace ArcRmlTabTestsPrivate;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(ScopeFrame))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		const FString HoverLook = DescribeLook(Elements, TEXT("sf-hover"));
		const FString PlainLook = DescribeLook(Elements, TEXT("sf-plain"));
		ASSERT_THAT(IsFalse(HoverLook == TEXT("missing") || PlainLook == TEXT("missing"), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(ScopeTabA))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		const FArcRmlTestLayoutElement* TabA = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("st-a"));
		ASSERT_THAT(IsNotNull(TabA, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsNear(180.0f, TabA->Rect.Size.X, 0.5f, TEXT("the tab sheet did not style the tab subtree")));
		ASSERT_THAT(IsFalse(TabA->BackgroundColor == TEXT("#123456ff"), TEXT("a frame rule matched an element inside the tab")));
		const FArcRmlTestLayoutElement* FramePlain = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sf-plain"));
		ASSERT_THAT(IsNotNull(FramePlain));
		ASSERT_THAT(IsFalse(TabA->Color == FramePlain->Color, FString::Printf(TEXT("the tab inherited the frame's color %s"), *FramePlain->Color)));
		ASSERT_THAT(AreEqual(HoverLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("embedding changed a frame element")));
		ASSERT_THAT(AreEqual(PlainLook, DescribeLook(Elements, TEXT("sf-plain")), TEXT("embedding changed a frame element")));

		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(ScopeTabB))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		const FArcRmlTestLayoutElement* TabB = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("st-b"));
		ASSERT_THAT(IsNotNull(TabB, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsNear(170.0f, TabB->Rect.Size.X, 0.5f, TEXT("the second tab sheet did not style the tab subtree")));
		ASSERT_THAT(IsFalse(TabB->BackgroundColor == TEXT("#123456ff"), TEXT("a frame rule matched an element inside the second tab")));
		ASSERT_THAT(AreEqual(HoverLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("the swap changed a frame element")));
		ASSERT_THAT(AreEqual(PlainLook, DescribeLook(Elements, TEXT("sf-plain")), TEXT("the swap changed a frame element")));

		View.ClearEmbed();
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(HoverLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("clearing the embed changed a frame element")));
		ASSERT_THAT(AreEqual(PlainLook, DescribeLook(Elements, TEXT("sf-plain")), TEXT("clearing the embed changed a frame element")));
	}

	TEST_METHOD(TabRuleSelectingFrameElement_NeverAppliesAcrossHoverAndReclass)
	{
		using namespace ArcRmlTabTestsPrivate;
		const FVector2f Away(640.0f, 600.0f);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(ScopeFrame))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		const FArcRmlTestLayoutElement* Hover = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sf-hover"));
		ASSERT_THAT(IsNotNull(Hover, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Hit = Hover->Rect.Position + Hover->Rect.Size * 0.5f;
		const FString RestLook = DescribeLook(Elements, TEXT("sf-hover"));

		View.MouseMove(Hit);
		ASSERT_THAT(IsTrue(FlushAndRead()));
		const FString HoveredLook = DescribeLook(Elements, TEXT("sf-hover"));
		ASSERT_THAT(IsTrue(HoveredLook != RestLook, TEXT("the frame's own :hover rule did not apply, so the hover probe is not valid")));
		View.MouseMove(Away);
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(RestLook, DescribeLook(Elements, TEXT("sf-hover"))));

		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(ScopeTabA))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(IsNotNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("st-a")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(RestLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("a tab rule applied to a frame element at embed")));

		View.MouseMove(Hit);
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(HoveredLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("a tab rule applied to a frame element when it was hovered")));
		View.MouseMove(Away);
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(RestLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("a tab rule stayed on a frame element after the hover ended")));

		View.SetRootClass(TEXT("root-on"), true);
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(RestLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("a tab rule applied to a frame element when the root class changed")));

		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(ScopeTabB))));
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(RestLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("a tab rule applied to a frame element at the swap")));
		View.MouseMove(Hit);
		ASSERT_THAT(IsTrue(FlushAndRead()));
		ASSERT_THAT(AreEqual(HoveredLook, DescribeLook(Elements, TEXT("sf-hover")), TEXT("a tab rule applied to a frame element when it was hovered after the swap")));
	}

	TEST_METHOD(LayoutDumpWithAppliedTabSheet_ListsTabSheet)
	{
		using namespace ArcRmlTabTestsPrivate;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(IfFrame))));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Embed(ArcRmlUITestHelpers::MakePath(IfTab))));
		ASSERT_THAT(IsTrue(View.Flush()));

		const FString Dir = MakeDumpDir(TEXT("LayoutDumpApplied"));
		View.DumpLayout(Dir);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		FString Html;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(Html, *FPaths::Combine(Dir, TEXT("IfFrame.html"))), TEXT("the layout dump wrote no html")));
		ASSERT_THAT(IsTrue(Html.Contains(TEXT("Common.rcss")), TEXT("the dump lacks the frame sheet")));
		ASSERT_THAT(IsTrue(Html.Contains(TEXT("IfTab.rcss")), TEXT("the dump lacks the applied tab sheet")));
	}

	TEST_METHOD(LayoutDumpWithUnappliedTabSheet_OmitsTabLinks)
	{
		using namespace ArcRmlTabTestsPrivate;
		const FString TabPath = ArcRmlUITestHelpers::MakePath(NoSheetTab);
		TestRunner->AddExpectedErrorPlain(TEXT("NoSheetMissing.rcss"), EAutomationExpectedErrorFlags::Contains, 0);
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, TabPath + TEXT(": no usable style sheet"));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(IfFrame))));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(IsTrue(View.Embed(TabPath)));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, TEXT("a tab without a usable sheet must still be embedded")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("no usable style sheet")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		const FString Dir = MakeDumpDir(TEXT("LayoutDumpUnapplied"));
		View.DumpLayout(Dir);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		FString Html;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(Html, *FPaths::Combine(Dir, TEXT("IfFrame.html"))), TEXT("the layout dump wrote no html")));
		ASSERT_THAT(IsTrue(Html.Contains(TEXT("Common.rcss")), TEXT("the dump lacks the frame sheet")));
		ASSERT_THAT(IsFalse(Html.Contains(TEXT("NoSheetMissing.rcss")), TEXT("the dump lists the links of a tab whose sheet was not applied")));
	}

	TEST_METHOD(LocalAssignmentToAStoreModel_IsRejected)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/LocalWriteTest.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClickElement(TEXT("#lw-button"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Shown = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("lw-shown"));
		ASSERT_THAT(IsTrue(Shown && Shown->bVisible, TEXT("a data-event assignment changed a store model, which only game-thread deltas may change")));
	}
};
