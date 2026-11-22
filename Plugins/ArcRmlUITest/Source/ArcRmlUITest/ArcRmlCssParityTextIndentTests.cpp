// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityTextIndentTestsPrivate
{
	struct FChromeLine
	{
		const TCHAR* Text;
		float X;
	};

	struct FChromeBox
	{
		const TCHAR* DomId;
		TConstArrayView<FChromeLine> Lines;
	};

	constexpr float XTolerance = 0.05f;
	const TCHAR* const SheetName = TEXT("TextIndent");
	const TCHAR* const FixturePath = TEXT("CssParity/Text/TextIndentTest.rml");

	const FChromeLine TiPxLines[] = {{TEXT("Hello"), 30.0f}};
	const FChromeLine TiNegLines[] = {{TEXT("Hello"), -10.0f}};
	const FChromeLine TiPctLines[] = {{TEXT("Hello"), 20.0f}};
	const FChromeLine TiCenterLines[] = {{TEXT("Hello"), 101.141f}};
	const FChromeLine TiRightLines[] = {{TEXT("Hello"), 162.297f}};
	const FChromeLine TiWrapLines[] = {{TEXT("The quick brown"), 60.0f}, {TEXT("fox jumps over the lazy"), 0.0f}, {TEXT("dog again and again"), 0.0f}};
	const FChromeLine TiChildLines[] = {{TEXT("Inherited"), 30.0f}};
	const FChromeLine TiIbLines[] = {{TEXT("Inline block"), 30.0f}};
	const FChromeLine TiTdLines[] = {{TEXT("Cell"), 0.0f}};
	const FChromeLine TiBtnLines[] = {{TEXT("Press"), 0.0f}};

	const FChromeBox Boxes[] = {
		{TEXT("ti-px"), TiPxLines},
		{TEXT("ti-neg"), TiNegLines},
		{TEXT("ti-pct"), TiPctLines},
		{TEXT("ti-center"), TiCenterLines},
		{TEXT("ti-right"), TiRightLines},
		{TEXT("ti-wrap"), TiWrapLines},
		{TEXT("ti-child"), TiChildLines},
		{TEXT("ti-ib"), TiIbLines},
		{TEXT("ti-td"), TiTdLines},
		{TEXT("ti-btn"), TiBtnLines},
	};

	FString SheetBinaryPath()
	{
		return FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/Text"), FString(SheetName) + TEXT(".rcssc"));
	}

	FString DescribeLines(const TArray<FArcRmlTestTextLine>& Lines, float Origin)
	{
		FString Out;
		for (const FArcRmlTestTextLine& Line : Lines)
		{
			Out += FString::Printf(TEXT("['%s' x %.3f] "), *Line.Text, Line.X - Origin);
		}
		return Out;
	}
}

TEST_CLASS(ArcRmlCssParityTextIndentTests, "ArcRmlUI.CssParity.TextIndent")
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

	FString FindMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, const ArcRmlCssParityTextIndentTestsPrivate::FChromeBox& Box)
	{
		using namespace ArcRmlCssParityTextIndentTestsPrivate;
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Box.DomId);
		if (!Element)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Box.DomId);
		}
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Layout, Box.DomId);
		const float Origin = Element->Rect.Position.X;
		bool bSame = Lines.Num() == Box.Lines.Num();
		for (int32 Index = 0; bSame && Index < Lines.Num(); ++Index)
		{
			FString Text = Lines[Index].Text;
			Text.TrimStartAndEndInline();
			bSame = Text == Box.Lines[Index].Text && FMath::IsNearlyEqual(Lines[Index].X - Origin, Box.Lines[Index].X, XTolerance);
		}
		if (bSame)
		{
			return FString();
		}
		FString Chrome;
		for (const FChromeLine& Line : Box.Lines)
		{
			Chrome += FString::Printf(TEXT("['%s' x %.3f] "), Line.Text, Line.X);
		}
		return FString::Printf(TEXT("[%s is %sChrome %s] "), Box.DomId, *DescribeLines(Lines, Origin), *Chrome);
	}

	FString CheckAll(const FString& Stage)
	{
		using namespace ArcRmlCssParityTextIndentTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		if (!View.ParseLayout(Layout))
		{
			return Stage + TEXT(": layout failed: ") + ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		FString Failures;
		for (const FChromeBox& Box : Boxes)
		{
			Failures += FindMismatches(Layout, Box);
		}
		return Failures.IsEmpty() ? FString() : Stage + TEXT(": ") + Failures;
	}

	FString CheckOnBothPaths()
	{
		using namespace ArcRmlCssParityTextIndentTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(FixturePath);
		IFileManager::Get().Delete(*SheetBinaryPath(), false, true, true);
		FArcRmlTestView::DropCompiledDocument(Path);
		FArcRmlTestView::ClearDocumentCaches();
		if (!OpenAndSettle(Path) || !IFileManager::Get().FileExists(*SheetBinaryPath()))
		{
			return TEXT("source open failed: ") + ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		FString Failures = CheckAll(TEXT("from source"));
		if (!Failures.IsEmpty())
		{
			return Failures;
		}
		const FDateTime Compiled = IFileManager::Get().GetTimeStamp(*SheetBinaryPath());
		View.Close();
		FArcRmlTestView::ClearDocumentCaches();
		if (!OpenAndSettle(Path) || IFileManager::Get().GetTimeStamp(*SheetBinaryPath()) != Compiled)
		{
			return TEXT("compiled sheet open failed: ") + ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		return CheckAll(TEXT("from compiled sheet"));
	}

	TEST_METHOD(FirstLine_IndentedByPxPercentNegativeAndAlignment_AsChromeDoes)
	{
		const FString Failures = CheckOnBothPaths();
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}
};
