// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Testing/ArcRmlTestView.h"

class FAutomationTestBase;

namespace ArcRmlUITestHelpers
{
	inline const FIntPoint ViewSize(1280, 720);

	FString MakePath(const FString& RelativePath);
	FString GetTestContentDir();
	bool OpenView(FArcRmlTestView& View, const FString& Path, float DpRatio = 1.0f);
	bool OpenFixture(FArcRmlTestView& View, const FString& RelativePath, TArray<FArcRmlTestLayoutElement>& OutLayout);
	bool CanRenderPixels(FAutomationTestBase& TestRunner);
	void ExpectLoggedError(FAutomationTestBase& TestRunner, const FString& LoggedTextStart);

	TArray<FArcRmlDiagnostic> FindDiagnostics(const FArcRmlTestView& View, const FString& MessagePart);
	int32 CountDiagnosticsAt(const FArcRmlTestView& View, const FString& MessagePart, const FString& File, int32 Line);
	int32 CountDiagnosticsAtInScope(const FArcRmlTestView& View, const FString& MessagePart, const FString& File, int32 Line, EArcRmlDiagnosticScope Scope);
	const TCHAR* GetScopeName(EArcRmlDiagnosticScope Scope);
	int32 CountErrorsOfKind(const FArcRmlTestView& View, EArcRmlDiagnosticKind Kind);
	FString DescribeDiagnostics(const FArcRmlTestView& View);
	TArray<FString> ListDiagnostics(const FArcRmlTestView& View);
	int32 CountHeadDiagnosticsAt(const FArcRmlTestView& View, const FString& MessagePart, const FString& File, int32 Line);
	int32 CountHeadDiagnostics(const FArcRmlTestView& View, const FString& MessagePart);
	FString DescribeHeadDiagnostics(const FArcRmlTestView& View);
	TArray<FString> ListHeadDiagnostics(const FArcRmlTestView& View);

	bool ReadLayout(const FArcRmlTestView& View, TArray<FArcRmlTestLayoutElement>& OutElements);
	const FArcRmlTestLayoutElement* FindByDomId(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId);
	const FArcRmlTestLayoutElement* FindByPath(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& Path);
	bool ReadScrollSize(const FArcRmlTestView& View, const FString& DomId, FVector2f& OutScrollSize);
	FString GetText(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId);
	TArray<FArcRmlTestTextLine> GetLines(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& DomId);
	TArray<FArcRmlTestDrawQuad> GetGlyphQuads(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour);
	FString GetFocusDomId(const FArcRmlTestView& View);
	FString GetNavFocusDomId(const FArcRmlTestView& View);

	int32 CountCalls(const FArcRmlTestView& View, const FString& FunctionName, const FString& Argument);
	FString DescribeHitRegionMismatch(const FArcRmlTestView& View);

	FArcRmlValue MakeBool(bool bValue);
	FArcRmlValue MakeInt(int64 Value);
	FArcRmlValue MakeString(const FString& Value);
	TArray<FArcRmlPathEntry> MakeFieldPath(const FString& Field);
	TArray<FArcRmlPathEntry> MakeRowFieldPath(const FString& ListField, int32 Row, const FString& Field);
}
