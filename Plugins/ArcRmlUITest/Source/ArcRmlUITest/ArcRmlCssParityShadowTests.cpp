// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityShadowTestsPrivate
{
	const TCHAR* const AnimPath = TEXT("CssParity/Anim/ShadowAnim.rml");
	const TCHAR* const TransPath = TEXT("CssParity/Anim/ShadowTrans.rml");
	const TCHAR* const HoldPath = TEXT("CssParity/Anim/ShadowHold.rml");

	constexpr int32 ColourTolerance = 2;
	constexpr float LengthTolerance = 0.01f;

	struct FShadow
	{
		FIntVector4 Colour = FIntVector4(0, 0, 0, 0);
		float OffsetX = 0.0f;
		float OffsetY = 0.0f;
		float Blur = 0.0f;
		float Spread = 0.0f;
		bool bInset = false;
	};

	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
	}

	void DropCompiledFixture(const TCHAR* RelativeRml)
	{
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(RelativeRml));
		const FString Dir = ArcRmlUITestHelpers::GetTestContentDir();
		const FString Sheet = FPaths::Combine(Dir, FPaths::GetPath(RelativeRml), FPaths::GetBaseFilename(RelativeRml) + TEXT(".rcssc"));
		IFileManager::Get().Delete(*Sheet, false, true, true);
	}

	void SplitTopLevel(const FString& Text, TCHAR Separator, TArray<FString>& OutParts)
	{
		int32 Depth = 0;
		FString Current;
		for (int32 Index = 0; Index < Text.Len(); ++Index)
		{
			const TCHAR Char = Text[Index];
			if (Char == TEXT('('))
			{
				++Depth;
			}
			else if (Char == TEXT(')'))
			{
				--Depth;
			}
			if (Char == Separator && Depth == 0)
			{
				if (!Current.IsEmpty())
				{
					OutParts.Add(Current);
				}
				Current.Reset();
			}
			else
			{
				Current.AppendChar(Char);
			}
		}
		if (!Current.IsEmpty())
		{
			OutParts.Add(Current);
		}
	}

	bool ParseColour(const FString& Text, FIntVector4& OutColour)
	{
		if (Text.StartsWith(TEXT("#")) && (Text.Len() == 7 || Text.Len() == 9))
		{
			const int32 Alpha = Text.Len() == 9 ? FParse::HexNumber(*Text.Mid(7, 2)) : 255;
			OutColour = FIntVector4(FParse::HexNumber(*Text.Mid(1, 2)), FParse::HexNumber(*Text.Mid(3, 2)), FParse::HexNumber(*Text.Mid(5, 2)), Alpha);
			return true;
		}
		int32 Open = INDEX_NONE;
		if (!Text.FindChar(TEXT('('), Open) || !Text.EndsWith(TEXT(")")))
		{
			return false;
		}
		TArray<FString> Numbers;
		Text.Mid(Open + 1, Text.Len() - Open - 2).ParseIntoArray(Numbers, TEXT(","), true);
		if (Numbers.Num() != 3 && Numbers.Num() != 4)
		{
			return false;
		}
		const int32 Alpha = Numbers.Num() == 4 ? FMath::RoundToInt(FCString::Atof(*Numbers[3]) * 255.0f) : 255;
		OutColour = FIntVector4(FCString::Atoi(*Numbers[0]), FCString::Atoi(*Numbers[1]), FCString::Atoi(*Numbers[2]), Alpha);
		return true;
	}

	bool ParseShadows(FString Text, TArray<FShadow>& OutShadows)
	{
		Text.TrimStartAndEndInline();
		if (Text.EndsWith(TEXT(" !important")))
		{
			Text.LeftChopInline(11);
		}
		if (Text.StartsWith(TEXT("drop-shadow(")) && Text.EndsWith(TEXT(")")))
		{
			Text = Text.Mid(12, Text.Len() - 13);
		}
		if (Text == TEXT("none"))
		{
			return true;
		}
		TArray<FString> Items;
		SplitTopLevel(Text, TEXT(','), Items);
		for (FString& Item : Items)
		{
			Item.TrimStartAndEndInline();
			TArray<FString> Tokens;
			SplitTopLevel(Item, TEXT(' '), Tokens);
			FShadow Shadow;
			int32 LengthIndex = 0;
			for (const FString& Token : Tokens)
			{
				if (Token == TEXT("inset"))
				{
					Shadow.bInset = true;
				}
				else if (Token.StartsWith(TEXT("#")) || Token.Contains(TEXT("(")))
				{
					if (!ParseColour(Token, Shadow.Colour))
					{
						return false;
					}
				}
				else
				{
					const float Value = FCString::Atof(*Token);
					switch (LengthIndex)
					{
						case 0:
							Shadow.OffsetX = Value;
							break;
						case 1:
							Shadow.OffsetY = Value;
							break;
						case 2:
							Shadow.Blur = Value;
							break;
						case 3:
							Shadow.Spread = Value;
							break;
						default:
							return false;
					}
					++LengthIndex;
				}
			}
			OutShadows.Add(Shadow);
		}
		return true;
	}

	bool IsColourNear(const FIntVector4& A, const FIntVector4& B)
	{
		return FMath::Abs(A.X - B.X) <= ColourTolerance && FMath::Abs(A.Y - B.Y) <= ColourTolerance && FMath::Abs(A.Z - B.Z) <= ColourTolerance
			&& FMath::Abs(A.W - B.W) <= ColourTolerance;
	}

	bool IsShadowNear(const FShadow& A, const FShadow& B)
	{
		return IsColourNear(A.Colour, B.Colour) && FMath::IsNearlyEqual(A.OffsetX, B.OffsetX, LengthTolerance)
			&& FMath::IsNearlyEqual(A.OffsetY, B.OffsetY, LengthTolerance) && FMath::IsNearlyEqual(A.Blur, B.Blur, LengthTolerance)
			&& FMath::IsNearlyEqual(A.Spread, B.Spread, LengthTolerance) && A.bInset == B.bInset;
	}

	bool AreShadowsNear(const TArray<FShadow>& A, const TArray<FShadow>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (!IsShadowNear(A[Index], B[Index]))
			{
				return false;
			}
		}
		return true;
	}

	FString ToText(const FIntVector4& Colour)
	{
		return FString::Printf(TEXT("rgba(%d, %d, %d, %d)"), Colour.X, Colour.Y, Colour.Z, Colour.W);
	}

	FString ToText(const TArray<FShadow>& Shadows)
	{
		FString Text;
		for (const FShadow& Shadow : Shadows)
		{
			Text += FString::Printf(TEXT("[%s %g %g %g %g%s]"), *ToText(Shadow.Colour), Shadow.OffsetX, Shadow.OffsetY, Shadow.Blur, Shadow.Spread,
				Shadow.bInset ? TEXT(" inset") : TEXT(""));
		}
		return Text.IsEmpty() ? FString(TEXT("[none]")) : Text;
	}

	const FArcRmlTestLayoutElement* FindTextChild(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& ParentPath)
	{
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			if (Element.Kind == TEXT("text") && Element.Parent == ParentPath)
			{
				return &Element;
			}
		}
		return nullptr;
	}

	const FArcRmlTestLayoutElement* FindTagChild(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& ParentPath, const TCHAR* Tag)
	{
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			if (Element.Parent == ParentPath && Element.Tag == Tag)
			{
				return &Element;
			}
		}
		return nullptr;
	}
}

TEST_CLASS(ArcRmlCssParityShadowTests, "ArcRmlUI.CssParity.ShadowAnimation")
{
	FArcRmlTestView View;
	double Elapsed = 0.0;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlCssParityShadowTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
	}

	bool SettleFrames()
	{
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			FPlatformProcess::Sleep(0.05f);
			if (!View.Flush())
			{
				return false;
			}
		}
		return true;
	}

	bool StartFixture(const TCHAR* RelativePath)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		DropCompiledFixture(RelativePath);
		RunConsole(TEXT("ArcRml.Clock 1000"));
		Elapsed = 0.0;
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(RelativePath)) && SettleFrames() && View.GetProbe().bHasDocument
			&& View.GetViewDiagnostics().Diagnostics.IsEmpty();
	}

	bool AdvanceTo(double Seconds)
	{
		ArcRmlCssParityShadowTestsPrivate::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.4f"), Seconds - Elapsed));
		Elapsed = Seconds;
		return SettleFrames();
	}

	bool SetRootClass(const TCHAR* Class)
	{
		View.SetRootClass(Class, true);
		return SettleFrames();
	}

	void ExpectShadowsNow(const TCHAR* DomId, const TCHAR* Property, const TCHAR* Chrome)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		const FString* Running = Element->Animated.Find(Property);
		ASSERT_THAT(IsTrue(Running != nullptr, FString::Printf(TEXT("%s has no running %s at %.2f s, Chrome %s"), DomId, Property, Elapsed, Chrome)));
		TArray<FShadow> Actual;
		TArray<FShadow> Expected;
		ASSERT_THAT(IsTrue(ParseShadows(*Running, Actual), *Running));
		ASSERT_THAT(IsTrue(ParseShadows(Chrome, Expected), Chrome));
		ASSERT_THAT(IsTrue(AreShadowsNear(Actual, Expected),
			FString::Printf(TEXT("%s %s at %.2f s is %s (%s), Chrome %s"), DomId, Property, Elapsed, *ToText(Actual), **Running, *ToText(Expected))));
		ASSERT_THAT(IsFalse(Expected.IsEmpty()));
		const FArcRmlTestLayoutElement* SiteElement = Element;
		const TCHAR* SiteKey = FCString::Strcmp(Property, TEXT("box-shadow")) == 0 ? TEXT("box_shadow") : TEXT("drop_shadow");
		if (FCString::Strcmp(Property, TEXT("text-shadow")) == 0)
		{
			SiteElement = FindTextChild(Elements, Element->Path);
			SiteKey = TEXT("text_shadow");
		}
		ASSERT_THAT(IsNotNull(SiteElement));
		const FString* Site = SiteElement->SiteColours.Find(SiteKey);
		ASSERT_THAT(IsTrue(Site != nullptr, FString::Printf(TEXT("%s has no %s site colour"), DomId, SiteKey)));
		FIntVector4 SiteColour;
		ASSERT_THAT(IsTrue(ParseColour(*Site, SiteColour), *Site));
		ASSERT_THAT(IsTrue(IsColourNear(SiteColour, Expected[0].Colour),
			FString::Printf(TEXT("%s %s site colour at %.2f s is %s, Chrome %s"), DomId, SiteKey, Elapsed, *ToText(SiteColour), *ToText(Expected[0].Colour))));
	}

	void ExpectShadows(const TCHAR* DomId, const TCHAR* Property, double Seconds, const TCHAR* Chrome)
	{
		ASSERT_THAT(IsTrue(AdvanceTo(Seconds)));
		ExpectShadowsNow(DomId, Property, Chrome);
	}

	void ExpectSiteColour(const TCHAR* DomId, const TCHAR* SiteKey, const FIntVector4& Chrome)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		const FString* Site = Element->SiteColours.Find(SiteKey);
		ASSERT_THAT(IsTrue(Site != nullptr, FString::Printf(TEXT("%s has no %s site colour"), DomId, SiteKey)));
		FIntVector4 Colour;
		ASSERT_THAT(IsTrue(ParseColour(*Site, Colour), *Site));
		ASSERT_THAT(IsTrue(IsColourNear(Colour, Chrome),
			FString::Printf(TEXT("%s %s at %.2f s is %s, Chrome %s"), DomId, SiteKey, Elapsed, *ToText(Colour), *ToText(Chrome))));
	}

	void ExpectBackground(const TCHAR* DomId, const FIntVector4& Chrome)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		FIntVector4 Colour;
		ASSERT_THAT(IsTrue(ParseColour(Element->BackgroundColor, Colour), Element->BackgroundColor));
		ASSERT_THAT(IsTrue(IsColourNear(Colour, Chrome),
			FString::Printf(TEXT("%s background-color at %.2f s is %s, Chrome %s"), DomId, Elapsed, *ToText(Colour), *ToText(Chrome))));
	}

	void ExpectElementColour(const TCHAR* DomId, double Seconds, const FIntVector4& Chrome)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		ASSERT_THAT(IsTrue(AdvanceTo(Seconds)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		FIntVector4 Colour;
		ASSERT_THAT(IsTrue(ParseColour(Element->Color, Colour), Element->Color));
		ASSERT_THAT(IsTrue(IsColourNear(Colour, Chrome),
			FString::Printf(TEXT("%s color at %.2f s is %s, Chrome %s"), DomId, Seconds, *ToText(Colour), *ToText(Chrome))));
	}

	void ExpectTextNodeColour(const TCHAR* ParentDomId, bool bSpan, const FIntVector4& Chrome)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Parent = ArcRmlUITestHelpers::FindByDomId(Elements, ParentDomId);
		ASSERT_THAT(IsNotNull(Parent));
		const FArcRmlTestLayoutElement* Owner = bSpan ? FindTagChild(Elements, Parent->Path, TEXT("span")) : Parent;
		ASSERT_THAT(IsNotNull(Owner));
		const FArcRmlTestLayoutElement* Text = FindTextChild(Elements, Owner->Path);
		ASSERT_THAT(IsNotNull(Text));
		const FString* Site = Text->SiteColours.Find(TEXT("text_shadow"));
		ASSERT_THAT(IsTrue(Site != nullptr, FString::Printf(TEXT("%s %s has no text_shadow site colour"), ParentDomId, bSpan ? TEXT("span") : TEXT("text"))));
		FIntVector4 Colour;
		ASSERT_THAT(IsTrue(ParseColour(*Site, Colour), *Site));
		ASSERT_THAT(IsTrue(IsColourNear(Colour, Chrome),
			FString::Printf(TEXT("%s %s text_shadow at %.2f s is %s, Chrome %s"), ParentDomId, bSpan ? TEXT("span") : TEXT("text"), Elapsed, *ToText(Colour), *ToText(Chrome))));
	}

	TEST_METHOD(BoxShadow_InterpolatesOffsetsBlurSpreadAndColour)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-basic"), TEXT("box-shadow"), 0.25, TEXT("rgb(191, 0, 64) 1px 2px 3px 0.5px"));
		ExpectShadows(TEXT("k-basic"), TEXT("box-shadow"), 0.5, TEXT("rgb(128, 0, 128) 2px 4px 6px 1px"));
		ExpectShadows(TEXT("k-basic"), TEXT("box-shadow"), 0.75, TEXT("rgb(64, 0, 191) 3px 6px 9px 1.5px"));
	}

	TEST_METHOD(ShorterShadowList_IsPaddedWithATransparentZeroShadow)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-one-two"), TEXT("box-shadow"), 0.25, TEXT("rgb(191, 0, 64) 2px 2px 0px 0px, rgba(0, 128, 0, 0.25) 1px 1px 1px 0px"));
		ExpectShadows(TEXT("k-one-two"), TEXT("box-shadow"), 0.5, TEXT("rgb(128, 0, 128) 2px 2px 0px 0px, rgba(0, 128, 0, 0.5) 2px 2px 2px 0px"));
		ExpectShadows(TEXT("k-one-two"), TEXT("box-shadow"), 0.75, TEXT("rgb(64, 0, 191) 2px 2px 0px 0px, rgba(0, 128, 0, 0.75) 3px 3px 3px 0px"));
	}

	TEST_METHOD(LongerStartList_FadesItsInsetShadowOutAsTheCounterpartOfThePadding)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-two-one"), TEXT("box-shadow"), 0.25, TEXT("rgb(191, 0, 64) 3px 3px 0.5px 0px, rgba(0, 128, 0, 0.75) 3px 3px 3px 0px inset"));
		ExpectShadows(TEXT("k-two-one"), TEXT("box-shadow"), 0.5, TEXT("rgb(128, 0, 128) 4px 4px 1px 0px, rgba(0, 128, 0, 0.5) 2px 2px 2px 0px inset"));
		ExpectShadows(TEXT("k-two-one"), TEXT("box-shadow"), 0.75, TEXT("rgb(64, 0, 191) 5px 5px 1.5px 0px, rgba(0, 128, 0, 0.25) 1px 1px 1px 0px inset"));
	}

	TEST_METHOD(InsetMismatch_SwapsTheWholeValueAtTheMidpoint)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-inset"), TEXT("box-shadow"), 0.25, TEXT("rgb(255, 0, 0) 2px 2px 0px 0px"));
		ExpectShadows(TEXT("k-inset"), TEXT("box-shadow"), 0.5, TEXT("rgb(0, 0, 255) 2px 2px 0px 0px inset"));
		ExpectShadows(TEXT("k-inset"), TEXT("box-shadow"), 0.75, TEXT("rgb(0, 0, 255) 2px 2px 0px 0px inset"));
	}

	TEST_METHOD(NoneBoxShadow_IsAnEmptyListThatFadesIn)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-none"), TEXT("box-shadow"), 0.25, TEXT("rgba(0, 0, 255, 0.25) 1px 1px 1px 0.5px"));
		ExpectShadows(TEXT("k-none"), TEXT("box-shadow"), 0.5, TEXT("rgba(0, 0, 255, 0.5) 2px 2px 2px 1px"));
		ExpectShadows(TEXT("k-none"), TEXT("box-shadow"), 0.75, TEXT("rgba(0, 0, 255, 0.75) 3px 3px 3px 1.5px"));
	}

	TEST_METHOD(NoneBoxShadow_FadesAnInsetShadowOutKeepingTheInset)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-inset-none"), TEXT("box-shadow"), 0.25, TEXT("rgba(0, 0, 255, 0.75) 3px 3px 3px 1.5px inset"));
		ExpectShadows(TEXT("k-inset-none"), TEXT("box-shadow"), 0.5, TEXT("rgba(0, 0, 255, 0.5) 2px 2px 2px 1px inset"));
		ExpectShadows(TEXT("k-inset-none"), TEXT("box-shadow"), 0.75, TEXT("rgba(0, 0, 255, 0.25) 1px 1px 1px 0.5px inset"));
	}

	TEST_METHOD(CurrentcolorInAKeyframe_ResolvesAgainstTheElementColourWhileRunning)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-current"), TEXT("box-shadow"), 0.25, TEXT("rgb(0, 191, 64) 1px 1px 1px 0px"));
		ExpectShadows(TEXT("k-current"), TEXT("box-shadow"), 0.5, TEXT("rgb(0, 128, 128) 2px 2px 2px 0px"));
		ExpectShadows(TEXT("k-current"), TEXT("box-shadow"), 0.75, TEXT("rgb(0, 64, 191) 3px 3px 3px 0px"));
	}

	TEST_METHOD(BoxShadowWithoutAColour_UsesCurrentcolor)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-nocolor"), TEXT("box-shadow"), 0.25, TEXT("rgb(255, 128, 0) 1px 1px 0px 0px"));
		ExpectShadows(TEXT("k-nocolor"), TEXT("box-shadow"), 0.5, TEXT("rgb(255, 128, 0) 2px 2px 0px 0px"));
		ExpectShadows(TEXT("k-nocolor"), TEXT("box-shadow"), 0.75, TEXT("rgb(255, 128, 0) 3px 3px 0px 0px"));
	}

	TEST_METHOD(UnsetBlurAndSpread_CountAsZeroAgainstASetOne)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-unset"), TEXT("box-shadow"), 0.25, TEXT("rgb(191, 0, 64) 2.5px 2.5px 1px 0.5px"));
		ExpectShadows(TEXT("k-unset"), TEXT("box-shadow"), 0.5, TEXT("rgb(128, 0, 128) 3px 3px 2px 1px"));
		ExpectShadows(TEXT("k-unset"), TEXT("box-shadow"), 0.75, TEXT("rgb(64, 0, 191) 3.5px 3.5px 3px 1.5px"));
	}

	TEST_METHOD(EmAgainstPx_InterpolatesInPixels)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-units"), TEXT("box-shadow"), 0.25, TEXT("rgb(255, 0, 0) 12px 0px 0px 0px"));
		ExpectShadows(TEXT("k-units"), TEXT("box-shadow"), 0.5, TEXT("rgb(255, 0, 0) 16px 0px 0px 0px"));
		ExpectShadows(TEXT("k-units"), TEXT("box-shadow"), 0.75, TEXT("rgb(255, 0, 0) 20px 0px 0px 0px"));
	}

	TEST_METHOD(ColorMixShadowColour_InterpolatesLikeChrome)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-mix"), TEXT("box-shadow"), 0.25, TEXT("rgb(115, 61, 109) 1px 1px 0px 0px"));
		ExpectShadows(TEXT("k-mix"), TEXT("box-shadow"), 0.5, TEXT("rgb(98, 88, 88) 2px 2px 0px 0px"));
		ExpectShadows(TEXT("k-mix"), TEXT("box-shadow"), 0.75, TEXT("rgb(72, 109, 62) 3px 3px 0px 0px"));
	}

	TEST_METHOD(TranslucentShadowColour_InterpolatesPremultiplied)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-alpha"), TEXT("box-shadow"), 0.25, TEXT("rgba(96, 0, 159, 0.4) 1px 1px 0px 0px"));
		ExpectShadows(TEXT("k-alpha"), TEXT("box-shadow"), 0.5, TEXT("rgba(43, 0, 213, 0.6) 2px 2px 0px 0px"));
		ExpectShadows(TEXT("k-alpha"), TEXT("box-shadow"), 0.75, TEXT("rgba(16, 0, 239, 0.8) 3px 3px 0px 0px"));
	}

	TEST_METHOD(TextShadow_InterpolatesLikeBoxShadow)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("kt-basic"), TEXT("text-shadow"), 0.25, TEXT("rgb(191, 0, 64) 1px 1px 0.5px"));
		ExpectShadows(TEXT("kt-basic"), TEXT("text-shadow"), 0.5, TEXT("rgb(128, 0, 128) 2px 2px 1px"));
		ExpectShadows(TEXT("kt-basic"), TEXT("text-shadow"), 0.75, TEXT("rgb(64, 0, 191) 3px 3px 1.5px"));
	}

	TEST_METHOD(TextShadowList_PadsTheShorterListWithATransparentZeroShadow)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("kt-one-two"), TEXT("text-shadow"), 0.25, TEXT("rgb(191, 0, 64) 1.25px 1.25px 0px, rgba(0, 128, 0, 0.25) 1px 1px 0.5px"));
		ExpectShadows(TEXT("kt-one-two"), TEXT("text-shadow"), 0.5, TEXT("rgb(128, 0, 128) 1.5px 1.5px 0px, rgba(0, 128, 0, 0.5) 2px 2px 1px"));
		ExpectShadows(TEXT("kt-one-two"), TEXT("text-shadow"), 0.75, TEXT("rgb(64, 0, 191) 1.75px 1.75px 0px, rgba(0, 128, 0, 0.75) 3px 3px 1.5px"));
	}

	TEST_METHOD(TextShadowFromNone_FadesIn)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("kt-none"), TEXT("text-shadow"), 0.25, TEXT("rgba(0, 0, 255, 0.25) 0.5px 0.5px 0.5px"));
		ExpectShadows(TEXT("kt-none"), TEXT("text-shadow"), 0.5, TEXT("rgba(0, 0, 255, 0.5) 1px 1px 1px"));
		ExpectShadows(TEXT("kt-none"), TEXT("text-shadow"), 0.75, TEXT("rgba(0, 0, 255, 0.75) 1.5px 1.5px 1.5px"));
	}

	TEST_METHOD(TextShadowUnsetBlur_CountsAsZeroAgainstASetOne)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("kt-unset"), TEXT("text-shadow"), 0.25, TEXT("rgb(191, 0, 64) 2.5px 2.5px 0.5px"));
		ExpectShadows(TEXT("kt-unset"), TEXT("text-shadow"), 0.5, TEXT("rgb(128, 0, 128) 3px 3px 1px"));
		ExpectShadows(TEXT("kt-unset"), TEXT("text-shadow"), 0.75, TEXT("rgb(64, 0, 191) 3.5px 3.5px 1.5px"));
	}

	TEST_METHOD(TextShadowCurrentcolor_ResolvesAgainstTheElementColourWhileRunning)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("kt-current"), TEXT("text-shadow"), 0.25, TEXT("rgb(0, 191, 64) 1px 1px 0.5px"));
		ExpectShadows(TEXT("kt-current"), TEXT("text-shadow"), 0.5, TEXT("rgb(0, 128, 128) 2px 2px 1px"));
		ExpectShadows(TEXT("kt-current"), TEXT("text-shadow"), 0.75, TEXT("rgb(0, 64, 191) 3px 3px 1.5px"));
	}

	TEST_METHOD(DropShadow_InterpolatesItsShadow)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-filter"), TEXT("filter"), 0.25, TEXT("drop-shadow(rgb(191, 0, 64) 1px 1px 0.5px)"));
		ExpectShadows(TEXT("k-filter"), TEXT("filter"), 0.5, TEXT("drop-shadow(rgb(128, 0, 128) 2px 2px 1px)"));
		ExpectShadows(TEXT("k-filter"), TEXT("filter"), 0.75, TEXT("drop-shadow(rgb(64, 0, 191) 3px 3px 1.5px)"));
	}

	TEST_METHOD(DropShadowFromNone_FadesIn)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-filter-none"), TEXT("filter"), 0.25, TEXT("drop-shadow(rgba(0, 0, 255, 0.25) 1px 1px 0.5px)"));
		ExpectShadows(TEXT("k-filter-none"), TEXT("filter"), 0.5, TEXT("drop-shadow(rgba(0, 0, 255, 0.5) 2px 2px 1px)"));
		ExpectShadows(TEXT("k-filter-none"), TEXT("filter"), 0.75, TEXT("drop-shadow(rgba(0, 0, 255, 0.75) 3px 3px 1.5px)"));
	}

	TEST_METHOD(DropShadowCurrentcolor_ResolvesAgainstTheElementColourWhileRunning)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-filter-current"), TEXT("filter"), 0.25, TEXT("drop-shadow(rgb(0, 191, 64) 1px 1px 0.5px)"));
		ExpectShadows(TEXT("k-filter-current"), TEXT("filter"), 0.5, TEXT("drop-shadow(rgb(0, 128, 128) 2px 2px 1px)"));
		ExpectShadows(TEXT("k-filter-current"), TEXT("filter"), 0.75, TEXT("drop-shadow(rgb(0, 64, 191) 3px 3px 1.5px)"));
	}

	TEST_METHOD(OvershootingEasing_ClampsTheBlurAtZero)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-over"), TEXT("box-shadow"), 0.25, TEXT("rgb(255, 0, 0) 10px 10px 0px 0px"));
		ExpectShadows(TEXT("k-over"), TEXT("box-shadow"), 0.5, TEXT("rgb(255, 0, 0) 10px 10px 2.2px 0px"));
		ExpectShadows(TEXT("k-over"), TEXT("box-shadow"), 0.75, TEXT("rgb(255, 0, 0) 10px 10px 6.68412px 0px"));
	}

	TEST_METHOD(OvershootingEasing_ClampsTheDropShadowBlurAtZero)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-over-filter"), TEXT("filter"), 0.25, TEXT("drop-shadow(rgb(255, 0, 0) 10px 10px 0px)"));
		ExpectShadows(TEXT("k-over-filter"), TEXT("filter"), 0.5, TEXT("drop-shadow(rgb(255, 0, 0) 10px 10px 2.2px)"));
		ExpectShadows(TEXT("k-over-filter"), TEXT("filter"), 0.75, TEXT("drop-shadow(rgb(255, 0, 0) 10px 10px 6.68412px)"));
	}

	TEST_METHOD(NegativeSpread_InterpolatesThroughZero)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectShadows(TEXT("k-negspread"), TEXT("box-shadow"), 0.25, TEXT("rgb(191, 0, 64) 0px 0px 0px -2px"));
		ExpectShadows(TEXT("k-negspread"), TEXT("box-shadow"), 0.5, TEXT("rgb(128, 0, 128) 0px 0px 0px 0px"));
		ExpectShadows(TEXT("k-negspread"), TEXT("box-shadow"), 0.75, TEXT("rgb(64, 0, 191) 0px 0px 0px 2px"));
	}

	TEST_METHOD(ColourAnimatedAlongsideTheShadow_ResolvesCurrentcolorInTheSameUpdate)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		ASSERT_THAT(IsTrue(StartFixture(AnimPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectElementColour(TEXT("k-colouranim"), 0.25, FIntVector4(191, 64, 0, 255));
		ExpectShadowsNow(TEXT("k-colouranim"), TEXT("box-shadow"), TEXT("rgb(48, 16, 191) 0px 0px 0px 0px"));
		ExpectElementColour(TEXT("k-colouranim"), 0.5, FIntVector4(128, 128, 0, 255));
		ExpectShadowsNow(TEXT("k-colouranim"), TEXT("box-shadow"), TEXT("rgb(64, 64, 128) 0px 0px 0px 0px"));
		ExpectElementColour(TEXT("k-colouranim"), 0.75, FIntVector4(64, 191, 0, 255));
		ExpectShadowsNow(TEXT("k-colouranim"), TEXT("box-shadow"), TEXT("rgb(48, 143, 64) 0px 0px 0px 0px"));
	}

	TEST_METHOD(BoxShadowTransition_RunsShadowByShadow)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::TransPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		ExpectShadows(TEXT("t-basic"), TEXT("box-shadow"), 0.25, TEXT("rgb(191, 0, 64) 1px 2px 3px 0.5px"));
		ExpectShadows(TEXT("t-basic"), TEXT("box-shadow"), 0.5, TEXT("rgb(128, 0, 128) 2px 4px 6px 1px"));
		ExpectShadows(TEXT("t-basic"), TEXT("box-shadow"), 0.75, TEXT("rgb(64, 0, 191) 3px 6px 9px 1.5px"));
	}

	TEST_METHOD(BoxShadowTransitionFromNone_FadesIn)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::TransPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		ExpectShadows(TEXT("t-none"), TEXT("box-shadow"), 0.25, TEXT("rgba(0, 0, 255, 0.25) 1px 1px 1px 0.5px"));
		ExpectShadows(TEXT("t-none"), TEXT("box-shadow"), 0.5, TEXT("rgba(0, 0, 255, 0.5) 2px 2px 2px 1px"));
		ExpectShadows(TEXT("t-none"), TEXT("box-shadow"), 0.75, TEXT("rgba(0, 0, 255, 0.75) 3px 3px 3px 1.5px"));
	}

	TEST_METHOD(BoxShadowTransitionWithInsetMismatch_DoesNotInterpolate)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::TransPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		for (const double Seconds : {0.25, 0.5, 0.75})
		{
			ASSERT_THAT(IsTrue(AdvanceTo(Seconds)));
			ExpectSiteColour(TEXT("t-inset"), TEXT("box_shadow"), FIntVector4(0, 0, 255, 255));
		}
	}

	TEST_METHOD(CurrentcolorBoxShadowTransition_ResolvesAgainstTheElementColour)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::TransPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		ExpectShadows(TEXT("t-current"), TEXT("box-shadow"), 0.25, TEXT("rgb(0, 191, 64) 1px 1px 1px 0px"));
		ExpectShadows(TEXT("t-current"), TEXT("box-shadow"), 0.5, TEXT("rgb(0, 128, 128) 2px 2px 2px 0px"));
		ExpectShadows(TEXT("t-current"), TEXT("box-shadow"), 0.75, TEXT("rgb(0, 64, 191) 3px 3px 3px 0px"));
	}

	TEST_METHOD(TextShadowTransition_RunsShadowByShadow)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityShadowTestsPrivate::TransPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		ExpectShadows(TEXT("t-text"), TEXT("text-shadow"), 0.25, TEXT("rgb(191, 0, 64) 1px 1px 0.5px"));
		ExpectShadows(TEXT("t-text"), TEXT("text-shadow"), 0.5, TEXT("rgb(128, 0, 128) 2px 2px 1px"));
		ExpectShadows(TEXT("t-text"), TEXT("text-shadow"), 0.75, TEXT("rgb(64, 0, 191) 3px 3px 1.5px"));
	}

	TEST_METHOD(HeldForwardsAnimation_FollowsTheColourOfCurrentcolor)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		ASSERT_THAT(IsTrue(StartFixture(HoldPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(1.5)));
		ExpectSiteColour(TEXT("h-fwd"), TEXT("box_shadow"), FIntVector4(255, 0, 0, 255));
		ExpectBackground(TEXT("h-bg-fwd"), FIntVector4(255, 0, 0, 255));
		ExpectSiteColour(TEXT("h-filter-fwd"), TEXT("drop_shadow"), FIntVector4(255, 0, 0, 255));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		ExpectSiteColour(TEXT("h-fwd"), TEXT("box_shadow"), FIntVector4(0, 255, 0, 255));
		ExpectBackground(TEXT("h-bg-fwd"), FIntVector4(0, 255, 0, 255));
		ExpectSiteColour(TEXT("h-filter-fwd"), TEXT("drop_shadow"), FIntVector4(0, 255, 0, 255));
	}

	TEST_METHOD(PausedAnimation_FollowsTheColourOfCurrentcolor)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		ASSERT_THAT(IsTrue(StartFixture(HoldPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(1.5)));
		ExpectSiteColour(TEXT("h-paused"), TEXT("box_shadow"), FIntVector4(128, 0, 128, 255));
		ExpectBackground(TEXT("h-bg-paused"), FIntVector4(128, 0, 128, 255));
		ExpectSiteColour(TEXT("h-filter-paused"), TEXT("drop_shadow"), FIntVector4(128, 0, 128, 255));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		ExpectSiteColour(TEXT("h-paused"), TEXT("box_shadow"), FIntVector4(0, 128, 128, 255));
		ExpectBackground(TEXT("h-bg-paused"), FIntVector4(0, 128, 128, 255));
		ExpectSiteColour(TEXT("h-filter-paused"), TEXT("drop_shadow"), FIntVector4(0, 128, 128, 255));
	}

	TEST_METHOD(PausedTextShadow_ChildrenInheritTheResolvedColour)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		ASSERT_THAT(IsTrue(StartFixture(HoldPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(1.5)));
		ExpectTextNodeColour(TEXT("h-text"), false, FIntVector4(128, 0, 128, 255));
		ExpectTextNodeColour(TEXT("h-text"), true, FIntVector4(128, 0, 128, 255));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		ExpectTextNodeColour(TEXT("h-text"), false, FIntVector4(0, 128, 128, 255));
		ExpectTextNodeColour(TEXT("h-text"), true, FIntVector4(0, 128, 128, 255));
	}

	TEST_METHOD(PausedCurrentcolorAgainstColorMix_ResamplesWhenTheColourChanges)
	{
		using namespace ArcRmlCssParityShadowTestsPrivate;
		ASSERT_THAT(IsTrue(StartFixture(HoldPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(1.5)));
		ExpectSiteColour(TEXT("h-shadow-mix"), TEXT("box_shadow"), FIntVector4(188, 28, 94, 255));
		ExpectBackground(TEXT("h-mix-a"), FIntVector4(188, 28, 94, 255));
		ASSERT_THAT(IsTrue(SetRootClass(TEXT("go"))));
		ExpectSiteColour(TEXT("h-shadow-mix"), TEXT("box_shadow"), FIntVector4(130, 148, 115, 255));
		ExpectBackground(TEXT("h-mix-a"), FIntVector4(130, 148, 115, 255));
	}
};
