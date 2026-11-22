// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityAtRuleTestsPrivate
{
	struct FColourCase
	{
		const TCHAR* Id;
		const TCHAR* Chrome;
	};

	struct FFixture
	{
		const TCHAR* Path;
		TConstArrayView<const TCHAR*> SheetNames;
		TConstArrayView<FColourCase> Cases;
	};

	const FColourCase StatementCases[] = {
		{TEXT("first-1"), TEXT("#010101ff")},
		{TEXT("first-2"), TEXT("#020202ff")},
		{TEXT("first-3"), TEXT("#030303ff")},
		{TEXT("mid-1"), TEXT("#040404ff")},
		{TEXT("mid-2"), TEXT("#050505ff")},
		{TEXT("mid-3"), TEXT("#060606ff")},
	};
	const TCHAR* const StatementSheets[] = {TEXT("LayerFirst"), TEXT("LayerMiddle")};

	const FColourCase EscapeCases[] = {
		{TEXT("p1"), TEXT("#111111ff")},
		{TEXT("after-p"), TEXT("#121212ff")},
		{TEXT("m1"), TEXT("#212121ff")},
		{TEXT("m2"), TEXT("#212121ff")},
		{TEXT("m3"), TEXT("#222222ff")},
		{TEXT("after-m"), TEXT("#232323ff")},
	};
	const TCHAR* const EscapeSheets[] = {TEXT("EscapePretty"), TEXT("EscapeMinified")};

	const FColourCase LayerCascadeCases[] = {
		{TEXT("stmt"), TEXT("#0b0b0bff")},
		{TEXT("blk"), TEXT("#0c0c0cff")},
		{TEXT("spec"), TEXT("#1b1b1bff")},
		{TEXT("unl"), TEXT("#2f2f2fff")},
		{TEXT("imp"), TEXT("#3a3a3aff")},
		{TEXT("mix"), TEXT("#4a4a4aff")},
		{TEXT("uimp"), TEXT("#5a5a5aff")},
		{TEXT("ln"), TEXT("#6a6a6aff")},
		{TEXT("nest"), TEXT("#7b7b7bff")},
		{TEXT("nx"), TEXT("#8b8b8bff")},
		{TEXT("dot"), TEXT("#9b9b9bff")},
		{TEXT("sib"), TEXT("#abababff")},
		{TEXT("nimp"), TEXT("#cacacaff")},
	};
	const TCHAR* const LayerCascadeSheets[] = {TEXT("LayerCascade")};

	const FColourCase LayerSheetsCases[] = {
		{TEXT("xs"), TEXT("#4b4b4bff")},
		{TEXT("ord"), TEXT("#0b0b0bff")},
		{TEXT("late"), TEXT("#1b1b1bff")},
		{TEXT("unl"), TEXT("#2a2a2aff")},
		{TEXT("xs-imp"), TEXT("#3a3a3aff")},
	};
	const TCHAR* const LayerSheetsSheets[] = {TEXT("LayerSheetsA"), TEXT("LayerSheetsB")};

	const FFixture StatementFixture = {TEXT("CssParity/AtRules/StatementTest.rml"), StatementSheets, StatementCases};
	const FFixture EscapeFixture = {TEXT("CssParity/AtRules/EscapeTest.rml"), EscapeSheets, EscapeCases};
	const FFixture LayerCascadeFixture = {TEXT("CssParity/AtRules/LayerCascadeTest.rml"), LayerCascadeSheets, LayerCascadeCases};
	const FFixture LayerSheetsFixture = {TEXT("CssParity/AtRules/LayerSheetsTest.rml"), LayerSheetsSheets, LayerSheetsCases};

	FString SheetBinaryPath(const TCHAR* Name)
	{
		return FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/AtRules"), FString(Name) + TEXT(".rcssc"));
	}

	FString FindMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FColourCase> Cases)
	{
		FString Failures;
		for (const FColourCase& Case : Cases)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
			if (!Element)
			{
				Failures += FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
				continue;
			}
			if (!Element->BackgroundColor.Equals(Case.Chrome, ESearchCase::IgnoreCase))
			{
				Failures += FString::Printf(TEXT("[%s is %s, Chrome %s] "), Case.Id, *Element->BackgroundColor, Case.Chrome);
			}
		}
		return Failures;
	}
}

TEST_CLASS(ArcRmlCssParityAtRuleTests, "ArcRmlUI.CssParity.AtRules")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenAndSettle(const FString& Path)
	{
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork;
	}

	bool SheetBinariesExist(const ArcRmlCssParityAtRuleTestsPrivate::FFixture& Fixture)
	{
		using namespace ArcRmlCssParityAtRuleTestsPrivate;
		for (const TCHAR* Name : Fixture.SheetNames)
		{
			if (!IFileManager::Get().FileExists(*SheetBinaryPath(Name)))
			{
				return false;
			}
		}
		return true;
	}

	bool OpenFromSource(const ArcRmlCssParityAtRuleTestsPrivate::FFixture& Fixture)
	{
		using namespace ArcRmlCssParityAtRuleTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture.Path);
		for (const TCHAR* Name : Fixture.SheetNames)
		{
			IFileManager::Get().Delete(*SheetBinaryPath(Name), false, true, true);
		}
		FArcRmlTestView::DropCompiledDocument(Path);
		FArcRmlTestView::ClearDocumentCaches();
		return OpenAndSettle(Path) && SheetBinariesExist(Fixture);
	}

	bool ReopenFromCompiledSheets(const ArcRmlCssParityAtRuleTestsPrivate::FFixture& Fixture)
	{
		using namespace ArcRmlCssParityAtRuleTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture.Path);
		TArray<FDateTime> Compiled;
		for (const TCHAR* Name : Fixture.SheetNames)
		{
			Compiled.Add(IFileManager::Get().GetTimeStamp(*SheetBinaryPath(Name)));
		}
		View.Close();
		FArcRmlTestView::ClearDocumentCaches();
		if (!OpenAndSettle(Path))
		{
			return false;
		}
		for (int32 Index = 0; Index < Fixture.SheetNames.Num(); ++Index)
		{
			if (IFileManager::Get().GetTimeStamp(*SheetBinaryPath(Fixture.SheetNames[Index])) != Compiled[Index])
			{
				return false;
			}
		}
		return true;
	}

	FString CheckOnBothPaths(const ArcRmlCssParityAtRuleTestsPrivate::FFixture& Fixture)
	{
		using namespace ArcRmlCssParityAtRuleTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		if (!OpenFromSource(Fixture) || !View.ParseLayout(Layout))
		{
			return TEXT("source open failed: ") + ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		FString Failures = FindMismatches(Layout, Fixture.Cases);
		if (!Failures.IsEmpty())
		{
			return TEXT("from source: ") + Failures;
		}
		Layout.Reset();
		if (!ReopenFromCompiledSheets(Fixture) || !View.ParseLayout(Layout))
		{
			return TEXT("compiled sheet open failed: ") + ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		Failures = FindMismatches(Layout, Fixture.Cases);
		return Failures.IsEmpty() ? FString() : TEXT("from compiled sheets: ") + Failures;
	}

	TEST_METHOD(StatementLayerFirstAndMidSheet_ApplyEveryRuleAsChromeDoes)
	{
		const FString Failures = CheckOnBothPaths(ArcRmlCssParityAtRuleTestsPrivate::StatementFixture);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CascadeLayers_OrderImportantNestingAndUnlayered_ResolveAsChromeDoes)
	{
		const FString Failures = CheckOnBothPaths(ArcRmlCssParityAtRuleTestsPrivate::LayerCascadeFixture);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CascadeLayers_AcrossLinkedSheets_ResolveAsChromeDoes)
	{
		const FString Failures = CheckOnBothPaths(ArcRmlCssParityAtRuleTestsPrivate::LayerSheetsFixture);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(EscapedAtSignInSelectors_PrettyAndMinified_ApplyAsChromeDoes)
	{
		const FString Failures = CheckOnBothPaths(ArcRmlCssParityAtRuleTestsPrivate::EscapeFixture);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}
};
