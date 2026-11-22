// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/OutputDevice.h"

namespace ArcRmlPulseTestsPrivate
{
	const FIntVector4 StartColour(255, 32, 32, 255);
	const FIntVector4 BaseColour(255, 255, 255, 255);
	const FIntVector4 OneSecondColour(199, 88, 88, 255);
	const int32 AliasBoxTagLine = 3;

	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
	}

	FString ToHex(const FIntVector4& Colour)
	{
		return FString::Printf(TEXT("#%02x%02x%02x%02x"), Colour.X, Colour.Y, Colour.Z, Colour.W);
	}

	bool ParseHex(const FString& Text, FIntVector4& OutColour)
	{
		if (Text.Len() != 9 || !Text.StartsWith(TEXT("#")))
		{
			return false;
		}
		OutColour = FIntVector4(FParse::HexNumber(*Text.Mid(1, 2)), FParse::HexNumber(*Text.Mid(3, 2)), FParse::HexNumber(*Text.Mid(5, 2)),
			FParse::HexNumber(*Text.Mid(7, 2)));
		return true;
	}

	bool IsNear(const FIntVector4& A, const FIntVector4& B)
	{
		return FMath::Abs(A.X - B.X) <= 1 && FMath::Abs(A.Y - B.Y) <= 1 && FMath::Abs(A.Z - B.Z) <= 1 && A.W == B.W;
	}

	bool HasClass(const FArcRmlTestLayoutElement& Element, const FString& Class)
	{
		TArray<FString> Classes;
		Element.Classes.ParseIntoArrayWS(Classes);
		return Classes.Contains(Class);
	}

	class FCollectDevice : public FOutputDevice
	{
	public:
		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			FScopeLock Lock(&Mutex);
			Lines.Add(FString(Message));
		}

		int32 Count(const FString& Part)
		{
			FScopeLock Lock(&Mutex);
			int32 Result = 0;
			for (const FString& Line : Lines)
			{
				Result += Line.Contains(Part) ? 1 : 0;
			}
			return Result;
		}

	private:
		FCriticalSection Mutex;
		TArray<FString> Lines;
	};
}

TEST_CLASS(ArcRmlPulseTests, "ArcRmlUI.Pulse")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlPulseTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
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

	bool OpenFrozen(const TCHAR* RelativePath)
	{
		ArcRmlPulseTestsPrivate::RunConsole(TEXT("ArcRml.Clock 1000"));
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(RelativePath)) && SettleFrames() && View.GetProbe().bHasDocument;
	}

	bool Advance(double Seconds)
	{
		ArcRmlPulseTestsPrivate::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.3f"), Seconds));
		return SettleFrames();
	}

	bool SetSerial(int64 Value)
	{
		View.SetModelValue(TEXT("pulse"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Serial")), ArcRmlUITestHelpers::MakeInt(Value));
		return SettleFrames();
	}

	const FArcRmlTestLayoutElement* ReadElement(const TCHAR* DomId)
	{
		if (!View.ParseLayout(Elements))
		{
			return nullptr;
		}
		return ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
	}

	void ExpectClass(const TCHAR* Moment, const TCHAR* DomId, bool bExpected)
	{
		const FArcRmlTestLayoutElement* Element = ReadElement(DomId);
		ASSERT_THAT(IsNotNull(Element, FString::Printf(TEXT("%s: no element %s"), Moment, DomId)));
		ASSERT_THAT(AreEqual(bExpected, ArcRmlPulseTestsPrivate::HasClass(*Element, TEXT("flash")),
			FString::Printf(TEXT("%s: classes '%s'; %s"), Moment, *Element->Classes, *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	void ExpectColour(const TCHAR* Moment, const TCHAR* DomId, const FIntVector4& Expected)
	{
		using namespace ArcRmlPulseTestsPrivate;
		const FArcRmlTestLayoutElement* Element = ReadElement(DomId);
		ASSERT_THAT(IsNotNull(Element, FString::Printf(TEXT("%s: no element %s"), Moment, DomId)));
		ASSERT_THAT(IsTrue(HasClass(*Element, TEXT("flash")), FString::Printf(TEXT("%s: %s lacks the class, classes '%s'"), Moment, DomId, *Element->Classes)));
		FIntVector4 Colour;
		ASSERT_THAT(IsTrue(ParseHex(Element->Color, Colour), Element->Color));
		ASSERT_THAT(IsTrue(IsNear(Colour, Expected), FString::Printf(TEXT("%s: %s color is %s, expected %s"), Moment, DomId, *Element->Color, *ToHex(Expected))));
	}

	TEST_METHOD(CounterChanges_AddClassAndRestartAnimationEachTime)
	{
		using namespace ArcRmlPulseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Pulse/PulseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetSerial(1)));
		ExpectClass(TEXT("after 1"), TEXT("pt-box"), true);
		ExpectColour(TEXT("after 1"), TEXT("pt-box"), StartColour);
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectColour(TEXT("1 s after 1"), TEXT("pt-box"), OneSecondColour);

		ASSERT_THAT(IsTrue(SetSerial(2)));
		ExpectClass(TEXT("after 2"), TEXT("pt-box"), true);
		ExpectColour(TEXT("after 2"), TEXT("pt-box"), StartColour);
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectColour(TEXT("1 s after 2"), TEXT("pt-box"), OneSecondColour);

		ASSERT_THAT(IsTrue(SetSerial(3)));
		ExpectClass(TEXT("after 3"), TEXT("pt-box"), true);
		ExpectColour(TEXT("after 3"), TEXT("pt-box"), StartColour);
	}

	TEST_METHOD(FirstValue_SetsNoClass)
	{
		using namespace ArcRmlPulseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Pulse/PulseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClass(TEXT("after open"), TEXT("pt-box"), false);
		const FArcRmlTestLayoutElement* Element = ReadElement(TEXT("pt-box"));
		ASSERT_THAT(IsNotNull(Element));
		FIntVector4 Colour;
		ASSERT_THAT(IsTrue(ParseHex(Element->Color, Colour), Element->Color));
		ASSERT_THAT(IsTrue(IsNear(Colour, BaseColour), FString::Printf(TEXT("color %s"), *Element->Color)));
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectClass(TEXT("a second later"), TEXT("pt-box"), false);
	}

	TEST_METHOD(PreviewVariantReopen_IsNewViewAndNeverPulsesOnItsFirstValue)
	{
		using namespace ArcRmlPulseTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Pulse/PulseTest.rml"));
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Pulse/PulseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClass(TEXT("base"), TEXT("pt-box"), false);

		const TCHAR* const Variants[] = {TEXT("one"), TEXT("two"), TEXT("three")};
		for (const TCHAR* Variant : Variants)
		{
			ASSERT_THAT(IsTrue(View.Preview(Path, Variant), FString::Printf(TEXT("preview %s did not reopen"), Variant)));
			ASSERT_THAT(IsTrue(SettleFrames()));
			ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ExpectClass(Variant, TEXT("pt-box"), false);
			ASSERT_THAT(IsTrue(SetSerial(10)));
			ExpectClass(Variant, TEXT("pt-box"), true);
			ExpectColour(Variant, TEXT("pt-box"), StartColour);
		}
	}

	TEST_METHOD(ComponentProp_PulsesAndRestartsAnimation)
	{
		using namespace ArcRmlPulseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Pulse/PulseCompTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClass(TEXT("after open"), TEXT("pc-box"), false);
		ASSERT_THAT(IsTrue(SetSerial(1)));
		ExpectClass(TEXT("after 1"), TEXT("pc-box"), true);
		ExpectColour(TEXT("after 1"), TEXT("pc-box"), StartColour);
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectColour(TEXT("1 s after 1"), TEXT("pc-box"), OneSecondColour);
		ASSERT_THAT(IsTrue(SetSerial(2)));
		ExpectColour(TEXT("after 2"), TEXT("pc-box"), StartColour);
	}

	TEST_METHOD(PulseWithoutClassName_LogsErrorAndAddsNoView)
	{
		using namespace ArcRmlPulseTestsPrivate;
		TestRunner->AddExpectedErrorPlain(TEXT("data-pulse needs a class name"), EAutomationExpectedErrorFlags::Contains, 2);
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Pulse/PulseEmptyTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Bare = ReadElement(TEXT("pe-bare"));
		ASSERT_THAT(IsNotNull(Bare));
		const FString BareBefore = Bare->Classes;
		const FArcRmlTestLayoutElement* Dash = ReadElement(TEXT("pe-dash"));
		ASSERT_THAT(IsNotNull(Dash));
		const FString DashBefore = Dash->Classes;

		ASSERT_THAT(IsTrue(SetSerial(1)));
		ASSERT_THAT(IsTrue(SetSerial(2)));
		Bare = ReadElement(TEXT("pe-bare"));
		ASSERT_THAT(IsNotNull(Bare));
		ASSERT_THAT(AreEqual(BareBefore, Bare->Classes));
		Dash = ReadElement(TEXT("pe-dash"));
		ASSERT_THAT(IsNotNull(Dash));
		ASSERT_THAT(AreEqual(DashBefore, Dash->Classes));
	}

	TEST_METHOD(ValueChangingOnEveryEvaluation_GetsClassAndDoesNotLoop)
	{
		using namespace ArcRmlPulseTestsPrivate;
		RunConsole(TEXT("ArcRml.Clock live"));
		FCollectDevice Device;
		GLog->AddOutputDevice(&Device);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("Pulse/PulseLiveTest.rml"))) && SettleFrames() && View.GetProbe().bHasDocument,
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClass(TEXT("after open"), TEXT("pl-box"), false);

		View.SetModelValue(TEXT("pulse"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("T")), ArcRmlUITestHelpers::MakeInt(5));
		const bool bSettled = View.FlushUntilSettled(5.0);
		ASSERT_THAT(IsTrue(SettleFrames()));
		GLog->RemoveOutputDevice(&Device);
		ASSERT_THAT(IsTrue(bSettled, TEXT("the view kept asking for updates")));
		ExpectClass(TEXT("after T changed"), TEXT("pl-box"), true);
		ASSERT_THAT(AreEqual(0, Device.Count(TEXT("layout rounds")), TEXT("the layout rounds did not settle")));
		ASSERT_THAT(AreEqual(0, Device.Count(TEXT("update rounds")), TEXT("the update rounds did not settle")));
	}

	TEST_METHOD(ComponentValueProp_ExpandsToTheHostPath)
	{
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(TEXT("Pulse/ExpandValueTest.rml")))));
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Pulse/ExpandValueTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsNotNull(ReadElement(TEXT("vb-input"))));
		int32 Shown = 0;
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			Shown += Element.Text.Contains(TEXT("120 gold")) ? 1 : 0;
		}
		ASSERT_THAT(IsTrue(Shown > 0, TEXT("the input does not show the host value 120 gold")));
	}

	TEST_METHOD(ComponentCheckedProp_ExpandsToTheHostPath)
	{
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(TEXT("Pulse/ExpandCheckedTest.rml")))));
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Pulse/ExpandCheckedTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Input = ReadElement(TEXT("cb-input"));
		ASSERT_THAT(IsNotNull(Input));
		ASSERT_THAT(AreEqual(FString(TEXT("#00ff00ff")), Input->BackgroundColor, TEXT("the checkbox is not checked by the host Flag")));
		View.SetModelValue(TEXT("pulse"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Flag")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(SettleFrames()));
		Input = ReadElement(TEXT("cb-input"));
		ASSERT_THAT(IsNotNull(Input));
		ASSERT_THAT(AreEqual(FString(TEXT("#ff0000ff")), Input->BackgroundColor, TEXT("the checkbox did not follow the host Flag")));
	}

	TEST_METHOD(ComponentAliasWithPathProp_Works)
	{
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(TEXT("Pulse/ExpandAliasTest.rml")))));
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Pulse/ExpandAliasTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsNotNull(ReadElement(TEXT("ab-text"))));
		ASSERT_THAT(AreEqual(FString(TEXT("Sword")), ArcRmlUITestHelpers::GetText(Elements, TEXT("ab-text"))));
	}

	TEST_METHOD(ComponentAliasWithStaticProp_IsCompileErrorAtTheTagLine)
	{
		using namespace ArcRmlPulseTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Pulse/ExpandAliasStaticTest.rml"));
		const FString ComponentPath = ArcRmlUITestHelpers::MakePath(TEXT("Pulse/AliasBox.rml"));
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, FString::Printf(TEXT("%s(%d): $p is used in data-alias-x"), *ComponentPath, AliasBoxTagLine));
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, Path + TEXT(": no usable compiled file"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path)));
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::OpenView(View, Path), TEXT("a static alias prop compiled and opened")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, TEXT("data-alias-x"), ComponentPath, AliasBoxTagLine), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnostics(View, TEXT("data-alias-x")), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
	}
};
