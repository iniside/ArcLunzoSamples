// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"

#include "ArcRmlUILog.h"
#include "Dom/JsonObject.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace ArcRmlUITestHelpersPrivate
{
	int32 CountAt(TConstArrayView<FArcRmlDiagnostic> Diagnostics, const FString& MessagePart, const FString& File, int32 Line)
	{
		int32 Count = 0;
		for (const FArcRmlDiagnostic& Diagnostic : Diagnostics)
		{
			if (Diagnostic.Message.Contains(MessagePart) && Diagnostic.File == File && Diagnostic.Line == Line)
			{
				++Count;
			}
		}
		return Count;
	}

	FString Describe(TConstArrayView<FArcRmlDiagnostic> Diagnostics)
	{
		TArray<FString> Lines;
		for (const FArcRmlDiagnostic& Diagnostic : Diagnostics)
		{
			Lines.Add(FString::Printf(TEXT("%s(%d) [%s]: %s"), *Diagnostic.File, Diagnostic.Line, ArcRmlUITestHelpers::GetScopeName(Diagnostic.Scope), *Diagnostic.Message));
		}
		return FString::Printf(TEXT("%d diagnostics: [%s]"), Lines.Num(), *FString::Join(Lines, TEXT(" | ")));
	}

	TArray<FString> List(TConstArrayView<FArcRmlDiagnostic> Diagnostics)
	{
		TArray<FString> Lines;
		for (const FArcRmlDiagnostic& Diagnostic : Diagnostics)
		{
			Lines.Add(FString::Printf(TEXT("%s(%d): %s"), *Diagnostic.File, Diagnostic.Line, *Diagnostic.Message));
		}
		Lines.Sort();
		return Lines;
	}

	TArray<TArray<FVector2f>> SortRegions(const TArray<FVector2f>& Vertices, const TArray<int32>& RegionStarts)
	{
		TArray<TArray<FVector2f>> Regions;
		for (int32 Region = 0; Region < RegionStarts.Num(); ++Region)
		{
			const int32 End = Region + 1 < RegionStarts.Num() ? RegionStarts[Region + 1] : Vertices.Num();
			TArray<FVector2f>& Polygon = Regions.AddDefaulted_GetRef();
			for (int32 Index = RegionStarts[Region]; Index < End; ++Index)
			{
				Polygon.Add(Vertices[Index]);
			}
		}
		Regions.Sort([](const TArray<FVector2f>& A, const TArray<FVector2f>& B) -> bool
		{
			if (A.Num() != B.Num())
			{
				return A.Num() < B.Num();
			}
			for (int32 Index = 0; Index < A.Num(); ++Index)
			{
				if (A[Index].X != B[Index].X)
				{
					return A[Index].X < B[Index].X;
				}
				if (A[Index].Y != B[Index].Y)
				{
					return A[Index].Y < B[Index].Y;
				}
			}
			return false;
		});
		return Regions;
	}

	FString DescribeRegion(const TArray<TArray<FVector2f>>& Regions, int32 Index)
	{
		if (!Regions.IsValidIndex(Index))
		{
			return TEXT("none");
		}
		TArray<FString> Points;
		for (const FVector2f& Vertex : Regions[Index])
		{
			Points.Add(FString::Printf(TEXT("(%g,%g)"), Vertex.X, Vertex.Y));
		}
		return FString::Join(Points, TEXT(" "));
	}
}

FString ArcRmlUITestHelpers::MakePath(const FString& RelativePath)
{
	return FString(TEXT("/ArcRmlUITest/UI/Rml/Test/")) + RelativePath;
}

FString ArcRmlUITestHelpers::GetTestContentDir()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ArcRmlUITest"));
	if (!Plugin.IsValid())
	{
		return FString();
	}
	return FPaths::ConvertRelativePathToFull(Plugin->GetContentDir() / TEXT("UI/Rml/Test"));
}

bool ArcRmlUITestHelpers::OpenView(FArcRmlTestView& View, const FString& Path, float DpRatio)
{
	return View.Open(Path, ViewSize, DpRatio);
}

bool ArcRmlUITestHelpers::OpenFixture(FArcRmlTestView& View, const FString& RelativePath, TArray<FArcRmlTestLayoutElement>& OutLayout)
{
	return OpenView(View, MakePath(RelativePath)) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(OutLayout);
}

bool ArcRmlUITestHelpers::CanRenderPixels(FAutomationTestBase& TestRunner)
{
	if (FArcRmlTestView::CanRenderPixels())
	{
		return true;
	}
	TestRunner.AddWarning(TEXT("SKIPPED: pixel test not run under -NullRHI; run it with -GpuTests"));
	static bool bLoggedSkip = false;
	if (!bLoggedSkip)
	{
		bLoggedSkip = true;
		UE_LOGF(LogArcRmlUI, Display, "ArcRmlUI.Pixels: skipped under -NullRHI; run them with -RenderOffscreen");
	}
	return false;
}

void ArcRmlUITestHelpers::ExpectLoggedError(FAutomationTestBase& TestRunner, const FString& LoggedTextStart)
{
	TestRunner.AddExpectedErrorPlain(FString(TEXT("LogArcRmlUI: ")) + LoggedTextStart, EAutomationExpectedErrorFlags::Contains, 0);
}

TArray<FArcRmlDiagnostic> ArcRmlUITestHelpers::FindDiagnostics(const FArcRmlTestView& View, const FString& MessagePart)
{
	TArray<FArcRmlDiagnostic> Result;
	for (const FArcRmlDiagnostic& Diagnostic : View.GetViewDiagnostics().Diagnostics)
	{
		if (Diagnostic.Message.Contains(MessagePart))
		{
			Result.Add(Diagnostic);
		}
	}
	return Result;
}

int32 ArcRmlUITestHelpers::CountDiagnosticsAt(const FArcRmlTestView& View, const FString& MessagePart, const FString& File, int32 Line)
{
	return ArcRmlUITestHelpersPrivate::CountAt(View.GetViewDiagnostics().Diagnostics, MessagePart, File, Line);
}

int32 ArcRmlUITestHelpers::CountHeadDiagnosticsAt(const FArcRmlTestView& View, const FString& MessagePart, const FString& File, int32 Line)
{
	return ArcRmlUITestHelpersPrivate::CountAt(View.GetHeadDiagnostics(), MessagePart, File, Line);
}

int32 ArcRmlUITestHelpers::CountHeadDiagnostics(const FArcRmlTestView& View, const FString& MessagePart)
{
	int32 Count = 0;
	for (const FArcRmlDiagnostic& Diagnostic : View.GetHeadDiagnostics())
	{
		if (Diagnostic.Message.Contains(MessagePart))
		{
			++Count;
		}
	}
	return Count;
}

int32 ArcRmlUITestHelpers::CountDiagnosticsAtInScope(const FArcRmlTestView& View, const FString& MessagePart, const FString& File, int32 Line,
	EArcRmlDiagnosticScope Scope)
{
	int32 Count = 0;
	for (const FArcRmlDiagnostic& Diagnostic : FindDiagnostics(View, MessagePart))
	{
		if (Diagnostic.File == File && Diagnostic.Line == Line && Diagnostic.Scope == Scope)
		{
			++Count;
		}
	}
	return Count;
}

const TCHAR* ArcRmlUITestHelpers::GetScopeName(EArcRmlDiagnosticScope Scope)
{
	switch (Scope)
	{
	case EArcRmlDiagnosticScope::Load:
		return TEXT("load");
	case EArcRmlDiagnosticScope::Embed:
		return TEXT("embed");
	case EArcRmlDiagnosticScope::Render:
		return TEXT("render");
	case EArcRmlDiagnosticScope::Click:
		return TEXT("click");
	case EArcRmlDiagnosticScope::Confirm:
		return TEXT("confirm");
	case EArcRmlDiagnosticScope::Input:
		return TEXT("input");
	case EArcRmlDiagnosticScope::None:
		break;
	}
	return TEXT("none");
}

int32 ArcRmlUITestHelpers::CountErrorsOfKind(const FArcRmlTestView& View, EArcRmlDiagnosticKind Kind)
{
	int32 Count = 0;
	for (const FArcRmlDiagnostic& Diagnostic : View.GetViewDiagnostics().Diagnostics)
	{
		if (Diagnostic.Severity == EArcRmlDiagnosticSeverity::Error && Diagnostic.Kind == Kind)
		{
			++Count;
		}
	}
	return Count;
}

FString ArcRmlUITestHelpers::DescribeDiagnostics(const FArcRmlTestView& View)
{
	return ArcRmlUITestHelpersPrivate::Describe(View.GetViewDiagnostics().Diagnostics);
}

TArray<FString> ArcRmlUITestHelpers::ListDiagnostics(const FArcRmlTestView& View)
{
	return ArcRmlUITestHelpersPrivate::List(View.GetViewDiagnostics().Diagnostics);
}

FString ArcRmlUITestHelpers::DescribeHeadDiagnostics(const FArcRmlTestView& View)
{
	return ArcRmlUITestHelpersPrivate::Describe(View.GetHeadDiagnostics());
}

TArray<FString> ArcRmlUITestHelpers::ListHeadDiagnostics(const FArcRmlTestView& View)
{
	return ArcRmlUITestHelpersPrivate::List(View.GetHeadDiagnostics());
}

bool ArcRmlUITestHelpers::ReadLayout(const FArcRmlTestView& View, TArray<FArcRmlTestLayoutElement>& OutElements)
{
	return View.ParseLayout(OutElements);
}

const FArcRmlTestLayoutElement* ArcRmlUITestHelpers::FindByDomId(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId)
{
	return Elements.FindByPredicate([&DomId](const FArcRmlTestLayoutElement& Element) -> bool
	{
		return Element.DomId == DomId;
	});
}

const FArcRmlTestLayoutElement* ArcRmlUITestHelpers::FindByPath(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& Path)
{
	return Elements.FindByPredicate([&Path](const FArcRmlTestLayoutElement& Element) -> bool
	{
		return Element.Path == Path;
	});
}

bool ArcRmlUITestHelpers::ReadScrollSize(const FArcRmlTestView& View, const FString& DomId, FVector2f& OutScrollSize)
{
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(View.GetProbe().LayoutJson), Root) || !Root.IsValid())
	{
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Elements = nullptr;
	if (!Root->TryGetArrayField(TEXT("elements"), Elements))
	{
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Elements)
	{
		const TSharedPtr<FJsonObject> Object = Value->AsObject();
		FString ElementId;
		const TArray<TSharedPtr<FJsonValue>>* Size = nullptr;
		if (Object.IsValid() && Object->TryGetStringField(TEXT("dom_id"), ElementId) && ElementId == DomId && Object->TryGetArrayField(TEXT("scroll_size"), Size) && Size->Num() == 2)
		{
			OutScrollSize = FVector2f(static_cast<float>((*Size)[0]->AsNumber()), static_cast<float>((*Size)[1]->AsNumber()));
			return true;
		}
	}
	return false;
}

FString ArcRmlUITestHelpers::GetText(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId)
{
	const FArcRmlTestLayoutElement* Owner = FindByDomId(Elements, DomId);
	if (!Owner)
	{
		return FString();
	}
	FString Text;
	for (const FArcRmlTestLayoutElement& Element : Elements)
	{
		if (Element.Parent == Owner->Path && Element.Kind == TEXT("text"))
		{
			Text += Element.Text;
		}
	}
	return Text;
}

TArray<FArcRmlTestTextLine> ArcRmlUITestHelpers::GetLines(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId)
{
	TArray<FArcRmlTestTextLine> Lines;
	const FArcRmlTestLayoutElement* Owner = FindByDomId(Elements, DomId);
	if (!Owner)
	{
		return Lines;
	}
	for (const FArcRmlTestLayoutElement& Element : Elements)
	{
		if (Element.Parent == Owner->Path && Element.Kind == TEXT("text"))
		{
			Lines.Append(Element.Lines);
		}
	}
	return Lines;
}

TArray<FArcRmlTestDrawQuad> ArcRmlUITestHelpers::GetGlyphQuads(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
{
	TArray<FArcRmlTestDrawQuad> Quads;
	for (const FArcRmlTestDraw& Draw : Draws)
	{
		if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
		{
			Quads.Append(Draw.Quads);
		}
	}
	Quads.Sort([](const FArcRmlTestDrawQuad& A, const FArcRmlTestDrawQuad& B) -> bool
	{
		return A.X < B.X;
	});
	return Quads;
}

FString ArcRmlUITestHelpers::GetFocusDomId(const FArcRmlTestView& View)
{
	TArray<FArcRmlTestLayoutElement> Elements;
	if (View.GetProbe().FocusPath.IsEmpty() || !View.ParseLayout(Elements))
	{
		return FString();
	}
	const FArcRmlTestLayoutElement* Focus = FindByPath(Elements, View.GetProbe().FocusPath);
	return Focus ? Focus->DomId : FString();
}

FString ArcRmlUITestHelpers::GetNavFocusDomId(const FArcRmlTestView& View)
{
	TArray<FArcRmlTestLayoutElement> Elements;
	if (View.GetProbe().NavFocusPath.IsEmpty() || !View.ParseLayout(Elements))
	{
		return FString();
	}
	const FArcRmlTestLayoutElement* Focus = FindByPath(Elements, View.GetProbe().NavFocusPath);
	return Focus ? Focus->DomId : FString();
}

int32 ArcRmlUITestHelpers::CountCalls(const FArcRmlTestView& View, const FString& FunctionName, const FString& Argument)
{
	int32 Count = 0;
	for (const FArcRmlViewModelCall& Call : View.GetCalls())
	{
		if (Call.FunctionName != FunctionName)
		{
			continue;
		}
		const bool bArgumentMatches = Argument.IsEmpty()
			|| Call.Arguments.ContainsByPredicate([&Argument](const FArcRmlValue& Value) -> bool { return Value.String == Argument; });
		if (bArgumentMatches)
		{
			++Count;
		}
	}
	return Count;
}

FArcRmlValue ArcRmlUITestHelpers::MakeBool(bool bValue)
{
	FArcRmlValue Value;
	Value.Kind = EArcRmlValueKind::Bool;
	Value.bBool = bValue;
	return Value;
}

FArcRmlValue ArcRmlUITestHelpers::MakeInt(int64 Value)
{
	FArcRmlValue Result;
	Result.Kind = EArcRmlValueKind::Int;
	Result.Int = Value;
	return Result;
}

FArcRmlValue ArcRmlUITestHelpers::MakeString(const FString& Value)
{
	FArcRmlValue Result;
	Result.Kind = EArcRmlValueKind::String;
	Result.String = Value;
	return Result;
}

TArray<FArcRmlPathEntry> ArcRmlUITestHelpers::MakeFieldPath(const FString& Field)
{
	TArray<FArcRmlPathEntry> Path;
	Path.Add(FArcRmlPathEntry{Field, INDEX_NONE});
	return Path;
}

TArray<FArcRmlPathEntry> ArcRmlUITestHelpers::MakeRowFieldPath(const FString& ListField, int32 Row, const FString& Field)
{
	TArray<FArcRmlPathEntry> Path;
	Path.Add(FArcRmlPathEntry{ListField, INDEX_NONE});
	Path.Add(FArcRmlPathEntry{FString(), Row});
	Path.Add(FArcRmlPathEntry{Field, INDEX_NONE});
	return Path;
}

FString ArcRmlUITestHelpers::DescribeHitRegionMismatch(const FArcRmlTestView& View)
{
	const FArcRmlViewSnapshot& Snapshot = View.GetSnapshot();
	const FArcRmlTestProbe& Probe = View.GetProbe();
	if (!Snapshot.InteractiveRegions.IsValid())
	{
		return TEXT("no snapshot regions were published");
	}
	Rml::HitRegionList Resolved;
	Snapshot.InteractiveRegions->Resolve(Resolved);
	TArray<FVector2f> PublishedVertices;
	for (const Rml::Vector2f& Vertex : Resolved.vertices)
	{
		PublishedVertices.Add(FVector2f(Vertex.x, Vertex.y));
	}
	const TArray<TArray<FVector2f>> Published =
		ArcRmlUITestHelpersPrivate::SortRegions(PublishedVertices, TArray<int32>(Resolved.offsets.data(), static_cast<int32>(Resolved.offsets.size())));
	const TArray<TArray<FVector2f>> Rebuilt =
		ArcRmlUITestHelpersPrivate::SortRegions(Probe.ReferenceInteractiveVertices, Probe.ReferenceInteractiveRegionStarts);
	const int32 Shared = FMath::Min(Published.Num(), Rebuilt.Num());
	int32 First = 0;
	while (First < Shared && Published[First] == Rebuilt[First])
	{
		++First;
	}
	if (First == Shared && Published.Num() == Rebuilt.Num())
	{
		return FString();
	}
	return FString::Printf(TEXT("published %d regions, a full rebuild %d; sorted region %d published [%s], rebuilt [%s]"), Published.Num(), Rebuilt.Num(), First,
		*ArcRmlUITestHelpersPrivate::DescribeRegion(Published, First), *ArcRmlUITestHelpersPrivate::DescribeRegion(Rebuilt, First));
}
