// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace ArcRmlChromeDiffTestsPrivate
{
	const TCHAR* const ChromePath = TEXT("C:/Program Files/Google/Chrome/Application/chrome.exe");
	const TCHAR* const FixtureFolder = TEXT("ChromeDiff");
	const TCHAR* const MissingFixtures = TEXT("NoFixtures");
	const TCHAR* const OutputStart = TEXT("<pre id=\"__out\">");
	const TCHAR* const MeasureScript = TEXT("<pre id=\"__out\"></pre><script>")
		TEXT("document.fonts.ready.then(function(){var o={};document.querySelectorAll('[id]').forEach(function(e){if(e.id==='__out'||o.hasOwnProperty(e.id)){return;}")
		TEXT("var r=e.getBoundingClientRect();o[e.id]=[r.left+window.scrollX,r.top+window.scrollY,r.width,r.height];});")
		TEXT("document.getElementById('__out').textContent=JSON.stringify({viewport:[window.innerWidth,window.innerHeight],boxes:o});});</script>\n");
	constexpr float Tolerance = 1.0f;
	constexpr double ChromeTimeoutSeconds = 60.0;

	FString GetOutputDir(const FString& Fixture)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ArcRml"), FixtureFolder, Fixture));
	}

	FString ToFileUrl(const FString& Path)
	{
		return TEXT("file:///") + Path.Replace(TEXT("\\"), TEXT("/")).Replace(TEXT(" "), TEXT("%20"));
	}

	bool WriteMeasurePage(const FString& DumpPage, const FString& MeasurePage, FString& OutError)
	{
		FString Page;
		if (!FFileHelper::LoadFileToString(Page, *DumpPage))
		{
			OutError = FString::Printf(TEXT("the layout dump wrote no page at %s"), *DumpPage);
			return false;
		}
		const int32 HtmlEnd = Page.Find(TEXT("</html>"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (HtmlEnd == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("%s has no </html>"), *DumpPage);
			return false;
		}
		Page.InsertAt(HtmlEnd, MeasureScript);
		if (!FFileHelper::SaveStringToFile(Page, *MeasurePage, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			OutError = FString::Printf(TEXT("cannot write %s"), *MeasurePage);
			return false;
		}
		return true;
	}

	bool RunChrome(const FString& MeasurePage, const FString& ProfileDir, FIntPoint WindowSize, FString& OutDom, FString& OutError)
	{
		if (!FPaths::FileExists(ChromePath))
		{
			OutError = FString::Printf(TEXT("Google Chrome is not installed at %s; ArcRmlUI.ChromeDiff needs it"), ChromePath);
			return false;
		}
		void* ReadPipe = nullptr;
		void* WritePipe = nullptr;
		if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
		{
			OutError = TEXT("cannot create the pipe for Chrome's output");
			return false;
		}
		const FString Params = FString::Printf(TEXT("--headless=new --disable-gpu --allow-file-access-from-files --virtual-time-budget=10000 --no-first-run --user-data-dir=\"%s\" --window-size=%d,%d --dump-dom \"%s\""), *ProfileDir,
			WindowSize.X, WindowSize.Y, *ToFileUrl(MeasurePage));
		FProcHandle Process = FPlatformProcess::CreateProc(ChromePath, *Params, false, true, true, nullptr, 0, nullptr, WritePipe);
		if (!Process.IsValid())
		{
			FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
			OutError = FString::Printf(TEXT("cannot start %s"), ChromePath);
			return false;
		}
		const double Deadline = FPlatformTime::Seconds() + ChromeTimeoutSeconds;
		bool bTimedOut = false;
		while (FPlatformProcess::IsProcRunning(Process))
		{
			OutDom += FPlatformProcess::ReadPipe(ReadPipe);
			if (FPlatformTime::Seconds() > Deadline)
			{
				FPlatformProcess::TerminateProc(Process, true);
				bTimedOut = true;
				break;
			}
			FPlatformProcess::Sleep(0.01f);
		}
		OutDom += FPlatformProcess::ReadPipe(ReadPipe);
		FPlatformProcess::CloseProc(Process);
		FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
		if (bTimedOut)
		{
			OutError = FString::Printf(TEXT("Chrome did not finish %s within %.0f s"), *MeasurePage, ChromeTimeoutSeconds);
			return false;
		}
		return true;
	}

	bool ParseChromeBoxes(const FString& Dom, FIntPoint& OutViewport, TMap<FString, FBox2f>& OutBoxes, FString& OutError)
	{
		const int32 Start = Dom.Find(OutputStart);
		const int32 End = Start == INDEX_NONE ? INDEX_NONE : Dom.Find(TEXT("</pre>"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("Chrome's DOM has no measured boxes: %s"), *Dom.Left(2000));
			return false;
		}
		const int32 JsonStart = Start + FCString::Strlen(OutputStart);
		const FString Json = Dom.Mid(JsonStart, End - JsonStart).Replace(TEXT("&lt;"), TEXT("<")).Replace(TEXT("&gt;"), TEXT(">")).Replace(TEXT("&amp;"), TEXT("&"));
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			OutError = FString::Printf(TEXT("Chrome's boxes are not JSON: %s"), *Json.Left(2000));
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Viewport = nullptr;
		const TSharedPtr<FJsonObject>* Boxes = nullptr;
		if (!Root->TryGetArrayField(TEXT("viewport"), Viewport) || Viewport->Num() != 2 || !Root->TryGetObjectField(TEXT("boxes"), Boxes))
		{
			OutError = FString::Printf(TEXT("Chrome's output has no viewport or boxes: %s"), *Json.Left(2000));
			return false;
		}
		OutViewport = FIntPoint(FMath::RoundToInt((*Viewport)[0]->AsNumber()), FMath::RoundToInt((*Viewport)[1]->AsNumber()));
		OutBoxes.Reset();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Boxes)->Values)
		{
			const TArray<TSharedPtr<FJsonValue>>& Values = Pair.Value->AsArray();
			if (Values.Num() != 4)
			{
				OutError = FString::Printf(TEXT("Chrome's box of %s has %d values"), *Pair.Key, Values.Num());
				return false;
			}
			const FVector2f Min(static_cast<float>(Values[0]->AsNumber()), static_cast<float>(Values[1]->AsNumber()));
			OutBoxes.Add(Pair.Key, FBox2f(Min, Min + FVector2f(static_cast<float>(Values[2]->AsNumber()), static_cast<float>(Values[3]->AsNumber()))));
		}
		return true;
	}

	bool MeasureInChrome(const FString& MeasurePage, TMap<FString, FBox2f>& OutBoxes, FString& OutError)
	{
		const FString ProfileDir = GetOutputDir(TEXT("ChromeProfile"));
		FIntPoint WindowSize = ArcRmlUITestHelpers::ViewSize;
		FIntPoint Viewport = FIntPoint::ZeroValue;
		for (int32 Attempt = 0; Attempt < 2; ++Attempt)
		{
			FString Dom;
			if (!RunChrome(MeasurePage, ProfileDir, WindowSize, Dom, OutError) || !ParseChromeBoxes(Dom, Viewport, OutBoxes, OutError))
			{
				return false;
			}
			if (Viewport == ArcRmlUITestHelpers::ViewSize)
			{
				return true;
			}
			WindowSize += ArcRmlUITestHelpers::ViewSize - Viewport;
		}
		OutError = FString::Printf(TEXT("Chrome's viewport stays %dx%d, not %dx%d"), Viewport.X, Viewport.Y, ArcRmlUITestHelpers::ViewSize.X, ArcRmlUITestHelpers::ViewSize.Y);
		return false;
	}

	TMap<FString, FBox2f> CollectOurBoxes(const TArray<FArcRmlTestLayoutElement>& Layout)
	{
		TMap<FString, FBox2f> Boxes;
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.bInDom && !Element.DomId.IsEmpty() && !Boxes.Contains(Element.DomId))
			{
				Boxes.Add(Element.DomId, FBox2f(Element.Rect.Position, Element.Rect.Position + Element.Rect.Size));
			}
		}
		return Boxes;
	}

	FString DescribeBox(const FBox2f& Box)
	{
		const FVector2f Size = Box.GetSize();
		return FString::Printf(TEXT("%.2f,%.2f %.2fx%.2f"), Box.Min.X, Box.Min.Y, Size.X, Size.Y);
	}

	bool IsNear(const FBox2f& A, const FBox2f& B)
	{
		const FVector2f SizeA = A.GetSize();
		const FVector2f SizeB = B.GetSize();
		return FMath::Abs(A.Min.X - B.Min.X) <= Tolerance && FMath::Abs(A.Min.Y - B.Min.Y) <= Tolerance && FMath::Abs(SizeA.X - SizeB.X) <= Tolerance
			&& FMath::Abs(SizeA.Y - SizeB.Y) <= Tolerance;
	}

	TArray<FString> DescribeMismatches(const TMap<FString, FBox2f>& Ours, const TMap<FString, FBox2f>& Chrome)
	{
		TArray<FString> Ids;
		Ours.GetKeys(Ids);
		for (const TPair<FString, FBox2f>& Pair : Chrome)
		{
			Ids.AddUnique(Pair.Key);
		}
		Ids.Sort();
		TArray<FString> Mismatches;
		for (const FString& Id : Ids)
		{
			const FBox2f* Our = Ours.Find(Id);
			const FBox2f* Theirs = Chrome.Find(Id);
			if (!Our)
			{
				Mismatches.Add(FString::Printf(TEXT("%s is not in our layout, Chrome %s"), *Id, *DescribeBox(*Theirs)));
			}
			else if (!Theirs)
			{
				Mismatches.Add(FString::Printf(TEXT("%s is not in Chrome's layout, ours %s"), *Id, *DescribeBox(*Our)));
			}
			else if (!IsNear(*Our, *Theirs))
			{
				Mismatches.Add(FString::Printf(TEXT("%s ours %s, Chrome %s"), *Id, *DescribeBox(*Our), *DescribeBox(*Theirs)));
			}
		}
		return Mismatches;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FArcRmlChromeDiffTest, "ArcRmlUI.ChromeDiff", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

void FArcRmlChromeDiffTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	using namespace ArcRmlChromeDiffTestsPrivate;
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), FixtureFolder, TEXT("*.rml")), true, false);
	Files.RemoveAll([](const FString& File) -> bool
	{
		return FPaths::GetExtension(File) != TEXT("rml");
	});
	Files.Sort();
	for (const FString& File : Files)
	{
		OutBeautifiedNames.Add(FPaths::GetBaseFilename(File));
		OutTestCommands.Add(File);
	}
	if (Files.IsEmpty())
	{
		OutBeautifiedNames.Add(MissingFixtures);
		OutTestCommands.Add(MissingFixtures);
	}
}

bool FArcRmlChromeDiffTest::RunTest(const FString& Parameters)
{
	using namespace ArcRmlChromeDiffTestsPrivate;
	if (Parameters == MissingFixtures)
	{
		AddError(FString::Printf(TEXT("no .rml fixture in %s"), *FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), FixtureFolder)));
		return false;
	}
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;
	const FString OutputDir = GetOutputDir(FPaths::GetBaseFilename(Parameters));
	if (!ArcRmlUITestHelpers::OpenFixture(View, FString(FixtureFolder) / Parameters, Layout))
	{
		AddError(FString::Printf(TEXT("cannot open %s: %s"), *Parameters, *ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.Close();
		return false;
	}
	View.DumpLayout(OutputDir);
	const bool bDumped = View.Flush() && View.Flush();
	View.Close();
	const FString DumpPage = FPaths::Combine(OutputDir, FPaths::GetBaseFilename(Parameters) + TEXT(".html"));
	const FString MeasurePage = FPaths::Combine(OutputDir, FPaths::GetBaseFilename(Parameters) + TEXT(".chrome.html"));
	FString Error;
	TMap<FString, FBox2f> ChromeBoxes;
	if (!bDumped || !WriteMeasurePage(DumpPage, MeasurePage, Error) || !MeasureInChrome(MeasurePage, ChromeBoxes, Error))
	{
		AddError(Error.IsEmpty() ? FString::Printf(TEXT("the layout dump of %s did not finish"), *Parameters) : Error);
		return false;
	}
	for (const FString& Mismatch : DescribeMismatches(CollectOurBoxes(Layout), ChromeBoxes))
	{
		AddError(Mismatch);
	}
	return !HasAnyErrors();
}
