// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace ArcRmlTabSourceTestsPrivate
{
	const TCHAR* const CommonSheet = TEXT("<link type=\"text/rcss\" href=\"/ArcRmlUITest/UI/Rml/Test/Common.rcss\"/>");

	FString MakeFrame()
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<rml>"));
		Lines.Add(TEXT("<head>"));
		Lines.Add(TEXT("<title>Source Frame</title>"));
		Lines.Add(TEXT("<meta name=\"layer\" content=\"UI.Layer.Menu\"/>"));
		Lines.Add(TEXT("<meta name=\"input-mode\" content=\"menu\"/>"));
		Lines.Add(CommonSheet);
		Lines.Add(TEXT("</head>"));
		Lines.Add(TEXT("<body>"));
		Lines.Add(TEXT("<div class=\"embed\" arc-embed=\"\"></div>"));
		Lines.Add(TEXT("</body>"));
		Lines.Add(TEXT("</rml>"));
		return FString::Join(Lines, TEXT("\n"));
	}

	FString MakeTab(const FString& FramePath)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<rml>"));
		Lines.Add(TEXT("<head>"));
		Lines.Add(TEXT("<title>Source Tab</title>"));
		Lines.Add(FString::Printf(TEXT("<meta name=\"frame\" content=\"%s\"/>"), *FramePath));
		Lines.Add(CommonSheet);
		Lines.Add(TEXT("</head>"));
		Lines.Add(TEXT("<body>"));
		Lines.Add(TEXT("<img id=\"tab-relative\" src=\"TabPic.png\"/>"));
		Lines.Add(TEXT("<img id=\"frame-relative\" src=\"FramePic.png\"/>"));
		Lines.Add(TEXT("</body>"));
		Lines.Add(TEXT("</rml>"));
		return FString::Join(Lines, TEXT("\n"));
	}

	FString MakeDragDocument(const FString& FramePath)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<rml>"));
		Lines.Add(TEXT("<head>"));
		Lines.Add(TEXT("<title>Source Drag</title>"));
		if (FramePath.IsEmpty())
		{
			Lines.Add(TEXT("<meta name=\"layer\" content=\"UI.Layer.Menu\"/>"));
			Lines.Add(TEXT("<meta name=\"input-mode\" content=\"menu\"/>"));
		}
		else
		{
			Lines.Add(FString::Printf(TEXT("<meta name=\"frame\" content=\"%s\"/>"), *FramePath));
		}
		Lines.Add(CommonSheet);
		Lines.Add(TEXT("</head>"));
		Lines.Add(TEXT("<body>"));
		Lines.Add(TEXT("<div id=\"drag-src\" data-drag-type=\"item\" style=\"width: 100px; height: 60px; drag: clone;\"><img src=\"Pic.png\"/></div>"));
		Lines.Add(TEXT("</body>"));
		Lines.Add(TEXT("</rml>"));
		return FString::Join(Lines, TEXT("\n"));
	}

	int32 CountFourPixelImages(const TArray<FArcRmlTestDraw>& Draws)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op != EArcRmlTestDrawOp::Geometry || !Draw.bTextured || Draw.bCallbackTexture)
			{
				continue;
			}
			for (const FArcRmlTestDrawQuad& Quad : Draw.Quads)
			{
				if (FMath::IsNearlyEqual(Quad.Width, 4.0f) && FMath::IsNearlyEqual(Quad.Height, 4.0f))
				{
					++Count;
				}
			}
		}
		return Count;
	}

	FString DescribeSize(const FArcRmlTestLayoutElement* Element)
	{
		return Element ? FString::Printf(TEXT("%.1f x %.1f"), Element->Rect.Size.X, Element->Rect.Size.Y) : FString(TEXT("missing"));
	}
}

TEST_CLASS(ArcRmlTabSourceTests, "ArcRmlUI.TabSource")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;
	FString RunFolder;
	FString RunDir;

	BEFORE_EACH()
	{
		RunFolder = FString::Printf(TEXT("TabSource/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RunDir = ArcRmlUITestHelpers::GetTestContentDir() / RunFolder;
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::GetTestContentDir().IsEmpty(), TEXT("the test plugin content directory is unknown")));
	}

	AFTER_EACH()
	{
		View.Close();
		if (RunDir.IsEmpty())
		{
			return;
		}
		IFileManager::Get().DeleteDirectory(*RunDir, false, true);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RunDir), false, false);
		RunDir.Reset();
	}

	bool WriteFile(const FString& Name, const FString& Text)
	{
		return FFileHelper::SaveStringToFile(Text, *(RunDir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool CopyImage(const FString& Name)
	{
		const FString Image = ArcRmlUITestHelpers::GetTestContentDir() / TEXT("CssParity/Probe.png");
		return IFileManager::Get().Copy(*(RunDir / Name), *Image) == COPY_OK;
	}

	TEST_METHOD(RelativeImageInATab_JoinsToTheTabFolderNotTheFrameFolder)
	{
		using namespace ArcRmlTabSourceTestsPrivate;
		const FString FramePath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/FrameDir/Frame.rml"));
		const FString TabPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/TabDir/Tab.rml"));
		ASSERT_THAT(IsTrue(WriteFile(TEXT("FrameDir/Frame.rml"), MakeFrame()) && WriteFile(TEXT("TabDir/Tab.rml"), MakeTab(FramePath)), TEXT("the fixture documents could not be written")));
		ASSERT_THAT(IsTrue(CopyImage(TEXT("TabDir/TabPic.png")) && CopyImage(TEXT("FrameDir/FramePic.png")), TEXT("the fixture images could not be copied")));
		TestRunner->AddExpectedMessagePlain(TEXT("Could not load texture: ") + ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/TabDir/FramePic.png")), ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains, 0);

		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, TabPath)));
		const FArcRmlTestLayoutElement* TabRelative = nullptr;
		for (int32 Attempt = 0; Attempt < 50; ++Attempt)
		{
			ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Elements)));
			TabRelative = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tab-relative"));
			if (TabRelative && TabRelative->Rect.Size.X > 0.0f)
			{
				break;
			}
		}
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TabRelative && FMath::IsNearlyEqual(TabRelative->Rect.Size.X, 4.0f) && FMath::IsNearlyEqual(TabRelative->Rect.Size.Y, 4.0f),
			FString::Printf(TEXT("the image next to the tab is %s, not the 4 x 4 of its file"), *DescribeSize(TabRelative))));
		const FArcRmlTestLayoutElement* FrameRelative = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("frame-relative"));
		ASSERT_THAT(IsTrue(FrameRelative && FrameRelative->Rect.Size.X == 0.0f && FrameRelative->Rect.Size.Y == 0.0f,
			FString::Printf(TEXT("the image next to the frame is %s inside the tab"), *DescribeSize(FrameRelative))));
	}

	void ExpectCloneImageLoads(const FString& OpenPath)
	{
		using namespace ArcRmlTabSourceTestsPrivate;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, OpenPath)));
		const FArcRmlTestLayoutElement* Source = nullptr;
		for (int32 Attempt = 0; Attempt < 50; ++Attempt)
		{
			ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Elements)));
			Source = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("drag-src"));
			if (Source && Source->Rect.Size.X > 0.0f && CountFourPixelImages(View.GetDraws()) >= 1)
			{
				break;
			}
		}
		ASSERT_THAT(IsTrue(Source && CountFourPixelImages(View.GetDraws()) == 1, FString::Printf(TEXT("the source image is not drawn 4 x 4 before the drag: %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		const FVector2f Grab(Source->Rect.Position.X + 50.0f, Source->Rect.Position.Y + 30.0f);
		View.MouseMove(Grab);
		View.MouseDown(Grab);
		View.MouseMove(Grab + FVector2f(200.0f, 4.0f));
		int32 Drawn = 0;
		for (int32 Attempt = 0; Attempt < 50 && Drawn < 2; ++Attempt)
		{
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
			Drawn = CountFourPixelImages(View.GetDraws());
		}
		ASSERT_THAT(AreEqual(2, Drawn, FString::Printf(TEXT("the drag clone image is not drawn 4 x 4 next to its source: %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	TEST_METHOD(RelativeImageInADragClone_JoinsToTheSourceDocumentFolder)
	{
		using namespace ArcRmlTabSourceTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/DragDir/Drag.rml"));
		ASSERT_THAT(IsTrue(WriteFile(TEXT("DragDir/Drag.rml"), MakeDragDocument(FString())), TEXT("the fixture document could not be written")));
		ASSERT_THAT(IsTrue(CopyImage(TEXT("DragDir/Pic.png")), TEXT("the fixture image could not be copied")));
		ExpectCloneImageLoads(Path);
	}

	TEST_METHOD(RelativeImageInADragCloneInATab_JoinsToTheTabFolder)
	{
		using namespace ArcRmlTabSourceTestsPrivate;
		const FString FramePath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/FrameDir/Frame.rml"));
		const FString TabPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/TabDir/Tab.rml"));
		ASSERT_THAT(IsTrue(WriteFile(TEXT("FrameDir/Frame.rml"), MakeFrame()) && WriteFile(TEXT("TabDir/Tab.rml"), MakeDragDocument(FramePath)), TEXT("the fixture documents could not be written")));
		ASSERT_THAT(IsTrue(CopyImage(TEXT("TabDir/Pic.png")), TEXT("the fixture image could not be copied")));
		ExpectCloneImageLoads(TabPath);
	}
};
