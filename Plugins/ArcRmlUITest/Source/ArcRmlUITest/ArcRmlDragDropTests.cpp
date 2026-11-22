// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/Guid.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace ArcRmlDragDropTestsPrivate
{
	constexpr float Tolerance = 0.5f;

	const FColor CloneColour(20, 180, 220, 255);
	const FColor UntypedCloneColour(220, 20, 180, 255);
	const FColor LocBackgroundColour(30, 30, 30, 255);
	const FColor LocTextColour(10, 230, 90, 255);
	const TCHAR* const TableText = TEXT("Section from table");
	const int32 UntypedLine = 12;

	TArray<const FArcRmlTestDraw*> FindBoxDraws(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		TArray<const FArcRmlTestDraw*> Found;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && !Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
			{
				Found.Add(&Draw);
			}
		}
		return Found;
	}

	TArray<FBox2f> FindTextQuads(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour, const FBox2f& Region)
	{
		TArray<FBox2f> Quads;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op != EArcRmlTestDrawOp::Geometry || !Draw.bTextured || Draw.FirstColourPremultiplied != Colour)
			{
				continue;
			}
			for (const FArcRmlTestDrawQuad& Quad : Draw.Quads)
			{
				const FBox2f Box(FVector2f(Quad.X, Quad.Y), FVector2f(Quad.X + Quad.Width, Quad.Y + Quad.Height));
				if (Region.IsInside(Box.GetCenter()))
				{
					Quads.Add(Box);
				}
			}
		}
		Quads.Sort([](const FBox2f& A, const FBox2f& B) -> bool
		{
			return A.Min.X < B.Min.X;
		});
		return Quads;
	}

	FString DescribeQuads(const TArray<FBox2f>& Quads)
	{
		TArray<FString> Parts;
		for (const FBox2f& Quad : Quads)
		{
			Parts.Add(FString::Printf(TEXT("%.1f+%.1f"), Quad.Min.X, Quad.GetSize().X));
		}
		return FString::Printf(TEXT("%d quads [%s]"), Quads.Num(), *FString::Join(Parts, TEXT(" ")));
	}

	bool SameRun(const TArray<FBox2f>& A, const TArray<FBox2f>& B)
	{
		if (A.Num() != B.Num() || A.IsEmpty())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			const float OffsetA = A[Index].Min.X - A[0].Min.X;
			const float OffsetB = B[Index].Min.X - B[0].Min.X;
			if (!FMath::IsNearlyEqual(OffsetA, OffsetB, Tolerance) || !FMath::IsNearlyEqual(A[Index].GetSize().X, B[Index].GetSize().X, Tolerance))
			{
				return false;
			}
		}
		return true;
	}

	FBox2f RectOf(const FArcRmlTestLayoutElement& Element, FVector2f Offset = FVector2f::ZeroVector)
	{
		const FVector2f Min(Element.Rect.Position.X + Offset.X, Element.Rect.Position.Y + Offset.Y);
		return FBox2f(Min, Min + FVector2f(Element.Rect.Size.X, Element.Rect.Size.Y));
	}

	FString DescribeBounds(const FBox2f& Bounds)
	{
		return FString::Printf(TEXT("(%.2f, %.2f) to (%.2f, %.2f)"), Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y);
	}

	int32 CountVisible(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId)
	{
		int32 Count = 0;
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			if (Element.DomId == DomId && Element.bVisible)
			{
				++Count;
			}
		}
		return Count;
	}

	bool HasClass(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId, const FString& Class)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		if (!Element)
		{
			return false;
		}
		TArray<FString> Classes;
		Element->Classes.ParseIntoArrayWS(Classes);
		return Classes.Contains(Class);
	}

	const TCHAR* const DragAction = TEXT("IA_UISecondary");

	FArcRmlInputBindings MakeKeyboardBindings()
	{
		FArcRmlInputBindings Bindings;
		Bindings.Confirm.Name = TEXT("confirm");
		Bindings.Confirm.ActionName = TEXT("IA_TestConfirm");
		Bindings.Confirm.Keys.Add(EKeys::Enter);
		Bindings.Back.Name = TEXT("back");
		Bindings.Back.ActionName = TEXT("IA_TestBack");
		Bindings.Back.Keys.Add(EKeys::Escape);
		return Bindings;
	}
}

TEST_CLASS(ArcRmlDragDropTests, "ArcRmlUI.DragDrop")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;
	FString RuntimeDir;
	FString RuntimePackageName;
	FString RuntimeTablePath;
	FString RuntimeTableFile;
	FString PreviousCulture;
	bool bCultureChanged = false;
	FVector2f Grab = FVector2f::ZeroVector;
	FVector2f Moved = FVector2f::ZeroVector;
	FBox2f SourceRect = FBox2f(ForceInit);

	AFTER_EACH()
	{
		if (bCultureChanged)
		{
			FInternationalization::Get().SetCurrentCulture(PreviousCulture);
			bCultureChanged = false;
		}
		View.Close();
		if (!RuntimeDir.IsEmpty())
		{
			IAssetRegistry& Registry = IAssetRegistry::GetChecked();
			if (UPackage* Package = FindPackage(nullptr, *RuntimePackageName))
			{
				ResetLoaders(Package);
				Registry.PackageDeleted(Package);
			}
			IFileManager::Get().DeleteDirectory(*RuntimeDir, false, true);
			TArray<FString> Files;
			Files.Add(RuntimeTableFile);
			Registry.ScanModifiedAssetFiles(Files);
			FStringTableRegistry::Get().UnregisterStringTable(FName(*RuntimeTablePath));
			IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RuntimeDir), false, false);
			RuntimeDir.Reset();
		}
	}

	bool OpenDrag()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Drag/DragTest.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ParseLayout(Elements);
	}

	bool Read()
	{
		return View.Flush() && View.Flush() && View.ParseLayout(Elements);
	}

	FVector2f CentreOf(const FString& DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		check(Element);
		return FVector2f(Element->Rect.Position.X + Element->Rect.Size.X * 0.5f, Element->Rect.Position.Y + Element->Rect.Size.Y * 0.5f);
	}

	FString DescribeCalls() const
	{
		TArray<FString> Parts;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			TArray<FString> Arguments;
			for (const FArcRmlValue& Argument : Call.Arguments)
			{
				Arguments.Add(Argument.String);
			}
			Parts.Add(FString::Printf(TEXT("%s(%s) drag %s target %s"), *Call.FunctionName, *FString::Join(Arguments, TEXT(", ")),
				*Call.DragData.FindRef(TEXT("data-slot")), *Call.ElementData.FindRef(TEXT("data-slot"))));
		}
		return FString::Printf(TEXT("%d calls: %s"), Parts.Num(), *FString::Join(Parts, TEXT("; ")));
	}

	void StartDrag(const FString& SourceId)
	{
		Grab = CentreOf(SourceId) - FVector2f(20.0f, 10.0f);
		View.MouseMove(Grab);
		View.MouseDown(Grab);
		View.MouseMove(Grab + FVector2f(6.0f, 4.0f));
	}

	TEST_METHOD(Classes_FollowSourceAndTargetsUnderThePointer)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;

		StartDrag(TEXT("src-a"));
		View.MouseMove(CentreOf(TEXT("tgt-ok")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("src-a"), TEXT("drag-source")), TEXT("the dragged source has no drag-source class")));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the accepting target under the pointer has no drop-over class")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-invalid"))));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-no"), TEXT("drop-invalid")), TEXT("a target away from the pointer has a class")));

		View.MouseMove(CentreOf(TEXT("tgt-no")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the target the pointer left kept drop-over")));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("tgt-no"), TEXT("drop-invalid")), TEXT("the non-accepting target under the pointer has no drop-invalid class")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-no"), TEXT("drop-over"))));

		View.MouseMove(CentreOf(TEXT("empty")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-no"), TEXT("drop-invalid")), TEXT("the target the pointer left kept drop-invalid")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("zone"), TEXT("drop-over")) || HasClass(Elements, TEXT("zone"), TEXT("drop-invalid")), TEXT("an element without data-drop-accept got a class")));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("src-a"), TEXT("drag-source"))));

		View.MouseMove(CentreOf(TEXT("tgt-ok")));
		View.MouseUp(CentreOf(TEXT("tgt-ok")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("src-a"), TEXT("drag-source")), TEXT("the source kept drag-source after the drop")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the target kept drop-over after the drop")));
	}

	TEST_METHOD(DropOnAcceptingTarget_RecordsOneCallWithResolvedArguments)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;

		StartDrag(TEXT("src-a"));
		View.MouseMove(CentreOf(TEXT("tgt-ok")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the accepting target is not marked before the drop")));
		View.ClearCalls();

		View.MouseUp(CentreOf(TEXT("tgt-ok")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("A")), DescribeCalls()));
		const FArcRmlViewModelCall* Found = View.GetCalls().FindByPredicate([](const FArcRmlViewModelCall& Candidate) -> bool
		{
			return Candidate.FunctionName == TEXT("Dropped");
		});
		ASSERT_THAT(IsNotNull(Found, DescribeCalls()));
		const FArcRmlViewModelCall& Call = *Found;
		ASSERT_THAT(AreEqual(2, Call.Arguments.Num(), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("drag:data-slot")), Call.Arguments[0].String, DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("target:data-slot")), Call.Arguments[1].String, DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("A")), Call.DragData.FindRef(TEXT("data-slot")), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("T1")), Call.ElementData.FindRef(TEXT("data-slot")), DescribeCalls()));
	}

	TEST_METHOD(DropOnNonAcceptingTargetOrEmptySpace_RecordsNoCall)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		StartDrag(TEXT("src-a"));
		View.MouseMove(CentreOf(TEXT("tgt-no")));
		View.MouseUp(CentreOf(TEXT("tgt-no")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));

		StartDrag(TEXT("src-a"));
		View.MouseMove(CentreOf(TEXT("empty")));
		View.MouseUp(CentreOf(TEXT("empty")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));

		StartDrag(TEXT("src-a"));
		View.MouseMove(CentreOf(TEXT("tgt-ok")));
		View.MouseUp(CentreOf(TEXT("tgt-ok")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
	}

	TEST_METHOD(UntypedSource_ReportsAndNeitherDragsNorShowsAClone)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Drag/DragTest.rml"));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("data-drag-type")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, FString::Printf(TEXT("%s(%d): drag source"), *Path, UntypedLine));
		StartDrag(TEXT("src-untyped"));
		View.MouseMove(Grab + FVector2f(12.0f, 8.0f));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAtInScope(View, TEXT("data-drag-type"), Path, UntypedLine, EArcRmlDiagnosticScope::Input),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountErrorsOfKind(View, EArcRmlDiagnosticKind::UntypedDragSource), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, FindBoxDraws(View.GetDraws(), UntypedCloneColour).Num(), TEXT("a clone of the untyped source is drawn")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("src-untyped"), TEXT("drag-source")), TEXT("the untyped source is marked as dragged")));

		View.MouseUp(Grab + FVector2f(12.0f, 8.0f));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dragged"), FString()), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Clicked"), FString()), DescribeCalls()));
	}

	TEST_METHOD(SourceRemovedMidDrag_CancelsTheDrag)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;

		StartDrag(TEXT("src-a"));
		const FVector2f Target = CentreOf(TEXT("tgt-ok"));
		View.MouseMove(Target);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(1, FindBoxDraws(View.GetDraws(), CloneColour).Num(), TEXT("the clone is not drawn while dragging")));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the accepting target is not marked")));
		ASSERT_THAT(AreEqual(1, CountVisible(Elements, TEXT("src-a")), TEXT("the source is not shown before its removal")));

		View.SetModelValue(TEXT("dd"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showSrc")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, CountVisible(Elements, TEXT("src-a")), TEXT("the source was not removed")));
		ASSERT_THAT(AreEqual(0, FindBoxDraws(View.GetDraws(), CloneColour).Num(), TEXT("the clone is still drawn after its source was removed")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the target kept drop-over after the source was removed")));

		View.MouseUp(Target);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
	}

	TEST_METHOD(CloneDraw_MovesWithThePointerAtTheGrabOffset)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;
		const FArcRmlTestLayoutElement* Source = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("src-a"));
		ASSERT_THAT(IsNotNull(Source));
		const FBox2f Rect = RectOf(*Source);

		StartDrag(TEXT("src-a"));
		for (const FVector2f& Delta : {FVector2f(100.0f, 50.0f), FVector2f(180.0f, 140.0f)})
		{
			View.MouseMove(Grab + Delta);
			ASSERT_THAT(IsTrue(Read()));
			const TArray<const FArcRmlTestDraw*> Clones = FindBoxDraws(View.GetDraws(), CloneColour);
			ASSERT_THAT(AreEqual(1, Clones.Num(), TEXT("exactly one clone draw")));
			const FBox2f Expected(Rect.Min + Delta, Rect.Max + Delta);
			const FBox2f& Bounds = Clones[0]->Bounds;
			ASSERT_THAT(IsTrue(Bounds.Min.Equals(Expected.Min, Tolerance) && Bounds.Max.Equals(Expected.Max, Tolerance),
				FString::Printf(TEXT("pointer moved by (%.0f, %.0f): clone %s, expected %s"), Delta.X, Delta.Y, *DescribeBounds(Bounds), *DescribeBounds(Expected))));
		}
		View.MouseUp(Grab + FVector2f(180.0f, 140.0f));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, FindBoxDraws(View.GetDraws(), CloneColour).Num(), TEXT("the clone is drawn after the release")));
	}

	void ExpectNoLiveDrag(const TCHAR* When)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		ASSERT_THAT(AreEqual(0, FindBoxDraws(View.GetDraws(), CloneColour).Num(), FString::Printf(TEXT("%s: the clone is still drawn"), When)));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("src-a"), TEXT("drag-source")), FString::Printf(TEXT("%s: the source kept drag-source"), When)));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), FString::Printf(TEXT("%s: the target kept drop-over"), When)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("A")), FString::Printf(TEXT("%s: %s"), When, *DescribeCalls())));
	}

	void StartDragOverAcceptingTarget()
	{
		using namespace ArcRmlDragDropTestsPrivate;
		StartDrag(TEXT("src-a"));
		View.MouseMove(CentreOf(TEXT("tgt-ok")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(1, FindBoxDraws(View.GetDraws(), CloneColour).Num(), TEXT("the clone is not drawn while dragging")));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the accepting target is not marked")));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("src-a"), TEXT("drag-source")), TEXT("the source is not marked")));
		View.ClearCalls();
	}

	TEST_METHOD(PointerCaptureLost_CancelsTheDrag)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;
		StartDragOverAcceptingTarget();

		View.CaptureLost();
		ASSERT_THAT(IsTrue(Read()));
		ExpectNoLiveDrag(TEXT("after the capture loss"));

		View.MouseMove(CentreOf(TEXT("tgt-no")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-no"), TEXT("drop-invalid")), TEXT("a later move still drags")));
		ASSERT_THAT(AreEqual(0, FindBoxDraws(View.GetDraws(), CloneColour).Num(), TEXT("a later move draws a clone")));
		View.MouseUp(CentreOf(TEXT("tgt-no")));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
	}

	TEST_METHOD(PressDuringALiveDrag_EndsItBeforeTheNewPress)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;
		StartDragOverAcceptingTarget();

		FArcRmlInputEvent Press;
		Press.Type = EArcRmlInputType::MouseDown;
		Press.ViewId = View.GetViewId();
		Press.Position = ArcRmlPointer::ToRmlPoint(CentreOf(TEXT("empty")));
		View.RawInput(MoveTemp(Press));
		ASSERT_THAT(IsTrue(Read()));
		ExpectNoLiveDrag(TEXT("after the second press"));

		FArcRmlInputEvent Release;
		Release.Type = EArcRmlInputType::MouseUp;
		Release.ViewId = View.GetViewId();
		Release.Position = ArcRmlPointer::ToRmlPoint(CentreOf(TEXT("tgt-ok")));
		View.RawInput(MoveTemp(Release));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("A")), DescribeCalls()));
	}

	void ExpectDragStillLive(const TCHAR* When)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		ASSERT_THAT(AreEqual(1, FindBoxDraws(View.GetDraws(), CloneColour).Num(), FString::Printf(TEXT("%s: the clone is not drawn"), When)));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), FString::Printf(TEXT("%s: the target lost drop-over"), When)));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("A")), FString::Printf(TEXT("%s: %s"), When, *DescribeCalls())));
	}

	void ExpectOneDropAndEnd()
	{
		using namespace ArcRmlDragDropTestsPrivate;
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), TEXT("drag:data-slot")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("A")), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, FindBoxDraws(View.GetDraws(), CloneColour).Num(), TEXT("the clone is drawn after the release")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the target kept drop-over after the release")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("src-a"), TEXT("drag-source")), TEXT("the source kept drag-source after the release")));
	}

	TEST_METHOD(OtherButtonPressAndReleaseMidDrag_PrimaryReleaseStillDropsOnce)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		StartDragOverAcceptingTarget();
		const FVector2f Target = CentreOf(TEXT("tgt-ok"));

		View.MouseDown(Target, 1);
		View.MouseUp(Target, 1);
		ASSERT_THAT(IsTrue(Read()));
		ExpectDragStillLive(TEXT("after the other button"));

		View.MouseUp(Target, 0);
		ASSERT_THAT(IsTrue(Read()));
		ExpectOneDropAndEnd();
	}

	TEST_METHOD(SecondTouchMidTouchDrag_FirstTouchReleaseStillDropsOnce)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;
		const uint32 FirstTouch = 0;
		const uint32 SecondTouch = 1;
		const FVector2f Target = CentreOf(TEXT("tgt-ok"));
		Grab = CentreOf(TEXT("src-a")) - FVector2f(20.0f, 10.0f);

		View.TouchStart(Grab, FirstTouch);
		View.TouchMove(Grab + FVector2f(6.0f, 4.0f), FirstTouch);
		View.TouchMove(Target, FirstTouch);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the touch drag did not reach the accepting target")));
		View.ClearCalls();

		const FVector2f Elsewhere = CentreOf(TEXT("empty"));
		View.TouchStart(Elsewhere, SecondTouch);
		View.TouchMove(Elsewhere + FVector2f(5.0f, 5.0f), SecondTouch);
		View.TouchEnd(Elsewhere + FVector2f(5.0f, 5.0f), SecondTouch);
		View.TouchMove(Target + FVector2f(2.0f, 2.0f), FirstTouch);
		ASSERT_THAT(IsTrue(Read()));
		ExpectDragStillLive(TEXT("after the second touch"));

		View.TouchEnd(Target + FVector2f(2.0f, 2.0f), FirstTouch);
		ASSERT_THAT(IsTrue(Read()));
		ExpectOneDropAndEnd();
	}

	TEST_METHOD(SourceWrapperRemovedMidDrag_ClearsClassesAndEndsOnce)
	{
		ASSERT_THAT(IsTrue(OpenDrag(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		using namespace ArcRmlDragDropTestsPrivate;

		StartDrag(TEXT("src-b"));
		const FVector2f Target = CentreOf(TEXT("tgt-ok"));
		View.MouseMove(Target);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the accepting target is not marked")));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("src-b"), TEXT("drag-source")), TEXT("the source is not marked")));
		View.ClearCalls();

		View.SetModelValue(TEXT("dd"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showWrap")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, CountVisible(Elements, TEXT("src-b")), TEXT("the source was not removed with its wrapper")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("tgt-ok"), TEXT("drop-over")), TEXT("the target kept drop-over after the wrapper was removed")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("B")), DescribeCalls()));

		View.MouseUp(Target);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("B")), DescribeCalls()));
	}

	TEST_METHOD(CloneOfTextInheritingTheHtmlFont_DrawsAsItsSource)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Drag/DragFontTest.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.ParseLayout(Elements),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Source = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("font-src"));
		ASSERT_THAT(IsNotNull(Source));
		SourceRect = RectOf(*Source);

		StartDrag(TEXT("font-src"));
		Moved = FVector2f(60.0f, 200.0f);
		View.MouseMove(Grab + Moved);
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(2, FindBoxDraws(View.GetDraws(), LocBackgroundColour).Num(), TEXT("the source and its clone are drawn")));
		const TArray<FBox2f> SourceQuads = FindTextQuads(View.GetDraws(), LocTextColour, SourceRect);
		const TArray<FBox2f> CloneQuads = FindTextQuads(View.GetDraws(), LocTextColour, FBox2f(SourceRect.Min + Moved, SourceRect.Max + Moved));
		ASSERT_THAT(IsTrue(SameRun(SourceQuads, CloneQuads), FString::Printf(TEXT("the clone does not draw with the source's html font size and colour: source %s, clone %s"),
			*DescribeQuads(SourceQuads), *DescribeQuads(CloneQuads))));
		View.MouseUp(Grab + Moved);
		ASSERT_THAT(IsTrue(View.Flush()));
	}

	TEST_METHOD(CloneOfAnUnstyledHeading_KeepsTheUserAgentDefaultsAsItsSource)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Drag/DragUserAgentTest.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.ParseLayout(Elements),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Source = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ua-src"));
		ASSERT_THAT(IsNotNull(Source));
		SourceRect = RectOf(*Source);
		const FBox2f ChromeSource(FVector2f(40.0f, 59.9062f), FVector2f(440.0f, 90.9062f));
		ASSERT_THAT(IsTrue(SourceRect.Min.Equals(ChromeSource.Min, 1.0f) && SourceRect.Max.Equals(ChromeSource.Max, 1.0f),
			FString::Printf(TEXT("the h2 source is %s, Chrome %s"), *DescribeBounds(SourceRect), *DescribeBounds(ChromeSource))));

		StartDrag(TEXT("ua-src"));
		Moved = FVector2f(60.0f, 200.0f);
		View.MouseMove(Grab + Moved);
		ASSERT_THAT(IsTrue(Read()));
		const TArray<const FArcRmlTestDraw*> Backgrounds = FindBoxDraws(View.GetDraws(), LocBackgroundColour);
		ASSERT_THAT(AreEqual(2, Backgrounds.Num(), TEXT("the source and its clone are drawn")));
		const FBox2f ExpectedClone(SourceRect.Min + Moved, SourceRect.Max + Moved);
		const FBox2f* Clone = nullptr;
		for (const FArcRmlTestDraw* Draw : Backgrounds)
		{
			if (Draw->Bounds.Min.Equals(ExpectedClone.Min, Tolerance) && Draw->Bounds.Max.Equals(ExpectedClone.Max, Tolerance))
			{
				Clone = &Draw->Bounds;
			}
		}
		ASSERT_THAT(IsNotNull(Clone, *FString::Printf(TEXT("no clone background with the h2's user-agent box at %s: draws %s and %s"), *DescribeBounds(ExpectedClone),
			*DescribeBounds(Backgrounds[0]->Bounds), *DescribeBounds(Backgrounds[1]->Bounds))));
		const TArray<FBox2f> SourceQuads = FindTextQuads(View.GetDraws(), LocTextColour, SourceRect);
		const TArray<FBox2f> CloneQuads = FindTextQuads(View.GetDraws(), LocTextColour, ExpectedClone);
		ASSERT_THAT(IsTrue(SameRun(SourceQuads, CloneQuads), FString::Printf(TEXT("the clone does not draw with the h2's user-agent font size and weight: source %s, clone %s"),
			*DescribeQuads(SourceQuads), *DescribeQuads(CloneQuads))));
		View.MouseUp(Grab + Moved);
		ASSERT_THAT(IsTrue(View.Flush()));
	}

	TEST_METHOD(CloneOfTranslatedText_FollowsACultureChange)
	{
		TestCommandBuilder.Until(TEXT("ArcRmlUI string tables settled"), []() -> bool
		{
			return FArcRmlTestView::AreStringTablesSettled();
		}, FTimespan::FromSeconds(60));
		const FString RunFolder = FString::Printf(TEXT("LocRuntime/D%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		const FString DocumentPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/LocTest.rml"));

		TestCommandBuilder
			.Then(TEXT("copy the drag fixture next to a string table"), [this, RunFolder]()
			{
				const FString ContentDir = ArcRmlUITestHelpers::GetTestContentDir();
				ASSERT_THAT(IsFalse(ContentDir.IsEmpty()));
				RuntimeDir = ContentDir / RunFolder;
				RuntimePackageName = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Loc/ST_LocTest"));
				RuntimeTablePath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Loc/ST_LocTest.ST_LocTest"));
				RuntimeTableFile = RuntimeDir / TEXT("Loc/ST_LocTest.uasset");
				IFileManager& FileManager = IFileManager::Get();
				ASSERT_THAT(IsTrue(FileManager.MakeDirectory(*(RuntimeDir / TEXT("Loc")), true)
					&& FileManager.Copy(*(RuntimeDir / TEXT("LocTest.rml")), *(ContentDir / TEXT("Drag/DragLocTemplate.rml"))) == COPY_OK
					&& FileManager.Copy(*RuntimeTableFile, *(ContentDir / TEXT("LocLoaded/Loc/ST_LocTest.uasset"))) == COPY_OK, TEXT("the runtime files could not be copied")));
				TArray<FString> Files;
				Files.Add(RuntimeTableFile);
				IAssetRegistry::GetChecked().ScanFilesSynchronous(Files, false);
			})
			.Until(TEXT("the fixture's string table loads"), [this]() -> bool
			{
				return FArcRmlTestView::GetStringTableState(RuntimeTablePath) == EArcRmlStringTableState::Loaded;
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view and drag the translated source"), [this, DocumentPath]()
			{
				using namespace ArcRmlDragDropTestsPrivate;
				ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, DocumentPath) && View.FlushUntilSettled() && View.ParseLayout(Elements),
					ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(AreEqual(FString(TableText), ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-src"))));
				const FArcRmlTestLayoutElement* Source = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("loc-src"));
				ASSERT_THAT(IsNotNull(Source));
				SourceRect = RectOf(*Source);

				StartDrag(TEXT("loc-src"));
				Moved = FVector2f(60.0f, 120.0f);
				View.MouseMove(Grab + Moved);
				ASSERT_THAT(IsTrue(Read()));
				ASSERT_THAT(AreEqual(2, FindBoxDraws(View.GetDraws(), LocBackgroundColour).Num(), TEXT("the source and its clone are drawn")));
				const TArray<FBox2f> SourceQuads = FindTextQuads(View.GetDraws(), LocTextColour, SourceRect);
				const TArray<FBox2f> CloneQuads = FindTextQuads(View.GetDraws(), LocTextColour, FBox2f(SourceRect.Min + Moved, SourceRect.Max + Moved));
				ASSERT_THAT(IsTrue(SameRun(SourceQuads, CloneQuads), FString::Printf(TEXT("before the change: source %s, clone %s"), *DescribeQuads(SourceQuads), *DescribeQuads(CloneQuads))));
			})
			.Then(TEXT("change the culture to LEET"), [this]()
			{
				PreviousCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
				bCultureChanged = true;
				ASSERT_THAT(IsTrue(FInternationalization::Get().SetCurrentCulture(TEXT("LEET")), TEXT("the LEET culture could not be set")));
			})
			.Until(TEXT("the source shows the LEET text"), [this]() -> bool
			{
				return View.Flush() && View.ParseLayout(Elements) && ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-src")) != ArcRmlDragDropTestsPrivate::TableText;
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the clone shows the same text as its source"), [this]()
			{
				using namespace ArcRmlDragDropTestsPrivate;
				ASSERT_THAT(IsTrue(Read()));
				const FString LeetSection = FText::FromStringTable(FName(*RuntimeTablePath), TEXT("Section")).ToString();
				ASSERT_THAT(AreEqual(LeetSection, ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-src"))));
				const TArray<FBox2f> SourceQuads = FindTextQuads(View.GetDraws(), LocTextColour, SourceRect);
				const TArray<FBox2f> CloneQuads = FindTextQuads(View.GetDraws(), LocTextColour, FBox2f(SourceRect.Min + Moved, SourceRect.Max + Moved));
				ASSERT_THAT(IsTrue(SameRun(SourceQuads, CloneQuads), FString::Printf(TEXT("after the change: source %s, clone %s"), *DescribeQuads(SourceQuads), *DescribeQuads(CloneQuads))));
				View.MouseUp(Grab + Moved);
				ASSERT_THAT(IsTrue(View.Flush()));
			})
			.Then(TEXT("restore the previous culture"), [this]()
			{
				bCultureChanged = false;
				ASSERT_THAT(IsTrue(FInternationalization::Get().SetCurrentCulture(PreviousCulture), TEXT("the previous culture could not be restored")));
			})
			.Until(TEXT("the source shows the restored text"), [this]() -> bool
			{
				return View.Flush() && View.ParseLayout(Elements) && ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-src")) == ArcRmlDragDropTestsPrivate::TableText;
			}, FTimespan::FromSeconds(30));
	}

	bool OpenNavDrag(bool bGamepad)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		if (!bGamepad)
		{
			View.SetInputBindings(MakeKeyboardBindings());
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Drag/NavDragTest.rml"));
		if (!FArcRmlTestView::DropCompiledDocument(Path) || !ArcRmlUITestHelpers::OpenView(View, Path) || !View.FlushUntilSettled() || !View.GetProbe().bHasDocument)
		{
			return false;
		}
		if (bGamepad)
		{
			View.SetInputType(ECommonInputType::Gamepad);
		}
		else
		{
			View.RawNavFocusStart(false);
		}
		return View.Flush() && View.Flush() && Read();
	}

	bool Press(const FKey& Key)
	{
		View.PressKey(Key);
		return View.Flush() && View.Flush() && Read();
	}

	bool PickUp()
	{
		View.UIAction(ArcRmlDragDropTestsPrivate::DragAction);
		return View.Flush() && View.Flush() && Read();
	}

	bool PadAccept()
	{
		View.GamepadAcceptDown();
		View.GamepadAcceptUp();
		return View.Flush() && View.Flush() && Read();
	}

	FString NavFocus() const
	{
		return ArcRmlUITestHelpers::GetNavFocusDomId(View);
	}

	void ExpectCarriedOn(const FString& DomId, const FString& When)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		ASSERT_THAT(AreEqual(DomId, NavFocus(), FString::Printf(TEXT("%s: navigation focus"), *When)));
		const FArcRmlTestLayoutElement* Slot = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Slot, When));
		const TArray<const FArcRmlTestDraw*> Clones = FindBoxDraws(View.GetDraws(), CloneColour);
		ASSERT_THAT(AreEqual(1, Clones.Num(), FString::Printf(TEXT("%s: clone draws"), *When)));
		const FVector2f Expected = RectOf(*Slot).GetCenter();
		const FVector2f Actual = Clones[0]->Bounds.GetCenter();
		ASSERT_THAT(IsTrue(Actual.Equals(Expected, Tolerance),
			FString::Printf(TEXT("%s: clone centre (%.1f, %.1f), expected the centre of '%s' (%.1f, %.1f)"), *When, Actual.X, Actual.Y, *DomId, Expected.X, Expected.Y)));
	}

	void ExpectNoCarry(const FString& When)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		ASSERT_THAT(AreEqual(0, FindBoxDraws(View.GetDraws(), CloneColour).Num(), FString::Printf(TEXT("%s: the clone is still drawn"), *When)));
		for (const TCHAR* const Id : {TEXT("nd-a"), TEXT("nd-ok"), TEXT("nd-no"), TEXT("nd-r1")})
		{
			ASSERT_THAT(IsFalse(HasClass(Elements, Id, TEXT("drag-source")) || HasClass(Elements, Id, TEXT("drop-over")) || HasClass(Elements, Id, TEXT("drop-invalid")),
				FString::Printf(TEXT("%s: '%s' kept a drag class"), *When, Id)));
		}
	}

	void RunPickUp()
	{
		using namespace ArcRmlDragDropTestsPrivate;
		ASSERT_THAT(AreEqual(FString(TEXT("nd-a")), NavFocus(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectNoCarry(TEXT("before the pick up"));

		View.UIAction(TEXT("IA_UIOther"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ExpectNoCarry(TEXT("after another UI action"));

		ASSERT_THAT(IsTrue(PickUp()));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("nd-a"), TEXT("drag-source")), TEXT("the focused source is not marked")));
		ExpectCarriedOn(TEXT("nd-a"), TEXT("after the pick up"));
	}

	void RunMoveAcrossAreas(const FKey& Into, const FKey& Down, const FKey& Up)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		ASSERT_THAT(IsTrue(PickUp()));

		ASSERT_THAT(IsTrue(Press(Into)));
		ExpectCarriedOn(TEXT("nd-ok"), TEXT("into the other area"));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("nd-ok"), TEXT("drop-over")), TEXT("the accepting slot has no drop-over")));

		ASSERT_THAT(IsTrue(Press(Down)));
		ExpectCarriedOn(TEXT("nd-no"), TEXT("onto the non-accepting slot"));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("nd-no"), TEXT("drop-invalid")), TEXT("the non-accepting slot has no drop-invalid")));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("nd-no"), TEXT("drop-over"))));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("nd-ok"), TEXT("drop-over")), TEXT("the slot focus left kept drop-over")));

		ASSERT_THAT(IsTrue(Press(Up)));
		ExpectCarriedOn(TEXT("nd-ok"), TEXT("back onto the accepting slot"));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("nd-ok"), TEXT("drop-over"))));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("nd-no"), TEXT("drop-invalid")), TEXT("the slot focus left kept drop-invalid")));
	}

	void RunScrolledMove(const FKey& Into, const FKey& Right, const FKey& Down)
	{
		using namespace ArcRmlDragDropTestsPrivate;
		ASSERT_THAT(IsTrue(PickUp()));
		ASSERT_THAT(IsTrue(Press(Into)));
		ExpectCarriedOn(TEXT("nd-ok"), TEXT("into the other area"));
		ASSERT_THAT(IsTrue(Press(Right)));
		ASSERT_THAT(IsTrue(NavFocus().StartsWith(TEXT("nd-r")), FString::Printf(TEXT("navigation focus '%s' is not in the list"), *NavFocus())));
		ExpectCarriedOn(NavFocus(), TEXT("into the list"));

		for (int32 Step = 0; Step < 10 && NavFocus() != TEXT("nd-r6"); ++Step)
		{
			ASSERT_THAT(IsTrue(Press(Down)));
			ExpectCarriedOn(NavFocus(), FString::Printf(TEXT("down step %d"), Step));
		}
		ASSERT_THAT(AreEqual(FString(TEXT("nd-r6")), NavFocus()));
		const FArcRmlTestLayoutElement* List = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("nd-list"));
		ASSERT_THAT(IsNotNull(List));
		ASSERT_THAT(IsTrue(List->Scroll.Y > 1.0f, FString::Printf(TEXT("navigation did not scroll the list, scroll %.1f"), List->Scroll.Y)));
		ExpectCarriedOn(TEXT("nd-r6"), TEXT("after the scroll"));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("nd-r6"), TEXT("drop-over"))));
	}

	void ExpectDropped(const FString& Target)
	{
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("A")), DescribeCalls()));
		const FArcRmlViewModelCall* Found = View.GetCalls().FindByPredicate([](const FArcRmlViewModelCall& Candidate) -> bool
		{
			return Candidate.FunctionName == TEXT("Dropped");
		});
		ASSERT_THAT(IsNotNull(Found, DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("A")), Found->DragData.FindRef(TEXT("data-slot")), DescribeCalls()));
		ASSERT_THAT(AreEqual(Target, Found->ElementData.FindRef(TEXT("data-slot")), DescribeCalls()));
		ExpectNoCarry(TEXT("after the drop"));
	}

	void RunAccept(const FKey& Into, const FKey& Down, const FKey& Up, TFunction<bool()> Accept)
	{
		ASSERT_THAT(IsTrue(PickUp()));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Accept()));
		ExpectCarriedOn(TEXT("nd-a"), TEXT("accept on the source itself"));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));

		ASSERT_THAT(IsTrue(Press(Into)));
		ASSERT_THAT(IsTrue(Press(Down)));
		ExpectCarriedOn(TEXT("nd-no"), TEXT("onto the non-accepting slot"));
		ASSERT_THAT(IsTrue(Accept()));
		ExpectCarriedOn(TEXT("nd-no"), TEXT("accept on the non-accepting slot"));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("A")), DescribeCalls()));

		ASSERT_THAT(IsTrue(Press(Up)));
		ExpectCarriedOn(TEXT("nd-ok"), TEXT("back onto the accepting slot"));
		ASSERT_THAT(IsTrue(Accept()));
		ExpectDropped(TEXT("T1"));
	}

	void RunCancel(const FKey& Into, TFunction<bool()> Back)
	{
		ASSERT_THAT(IsTrue(PickUp()));
		ASSERT_THAT(IsTrue(Press(Into)));
		ExpectCarriedOn(TEXT("nd-ok"), TEXT("into the other area"));
		View.ClearCalls();

		ASSERT_THAT(IsTrue(Back()));
		ExpectNoCarry(TEXT("after back"));
		ASSERT_THAT(AreEqual(FString(TEXT("nd-a")), NavFocus(), TEXT("back did not refocus the source")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("DragEnded"), TEXT("A")), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("Dropped"), FString()), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests(), TEXT("back closed the view")));
	}

	TEST_METHOD(Pad_UIActionPicksUpTheFocusedSource)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunPickUp();
	}

	TEST_METHOD(Pad_DpadIntoAnotherAreaMovesCloneAndHoverWithFocus)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunMoveAcrossAreas(EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Up);
	}

	TEST_METHOD(Pad_CloneSitsOnTheSlotAfterNavigationScrolls)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunScrolledMove(EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_DPad_Down);
	}

	TEST_METHOD(Pad_AcceptDropsOnAnAcceptingTargetOnly)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunAccept(EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Up, [this]() -> bool
		{
			return PadAccept();
		});
	}

	TEST_METHOD(Pad_BackCancelsAndRefocusesTheSource)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunCancel(EKeys::Gamepad_DPad_Down, [this]() -> bool
		{
			View.Back();
			return View.Flush() && View.Flush() && Read();
		});
	}

	TEST_METHOD(Keyboard_UIActionPicksUpTheFocusedSource)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunPickUp();
	}

	TEST_METHOD(Keyboard_ArrowsIntoAnotherAreaMoveCloneAndHoverWithFocus)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunMoveAcrossAreas(EKeys::Down, EKeys::Down, EKeys::Up);
	}

	TEST_METHOD(Keyboard_CloneSitsOnTheSlotAfterNavigationScrolls)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunScrolledMove(EKeys::Down, EKeys::Right, EKeys::Down);
	}

	TEST_METHOD(Keyboard_EnterDropsOnAnAcceptingTargetOnly)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunAccept(EKeys::Down, EKeys::Down, EKeys::Up, [this]() -> bool
		{
			return Press(EKeys::Enter);
		});
	}

	TEST_METHOD(Keyboard_EscapeCancelsAndRefocusesTheSource)
	{
		ASSERT_THAT(IsTrue(OpenNavDrag(false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		RunCancel(EKeys::Down, [this]() -> bool
		{
			return Press(EKeys::Escape);
		});
	}
};
