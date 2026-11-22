// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoSettingsViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoSettingsViewModel)

namespace ArcRmlDemoSettingsViewModelPrivate
{
	using EType = EArcRmlDemoSettingType;
	using EFormat = EArcRmlDemoSettingFormat;

	const FString SearchField(TEXT("search"));
	const FString CrumbsField(TEXT("crumbs"));
	const FString TabBarField(TEXT("main"));
	const FName ConfirmTimer(TEXT("confirm"));

	FString TypeName(EType Type)
	{
		switch (Type)
		{
		case EType::Text:
			return TEXT("text");
		case EType::Stepper:
			return TEXT("stepper");
		case EType::Toggle:
			return TEXT("toggle");
		case EType::Radio:
			return TEXT("radio");
		case EType::Slider:
			return TEXT("slider");
		case EType::Dropdown:
			return TEXT("dropdown");
		case EType::Checkbox:
			return TEXT("checkbox");
		default:
			return TEXT("keybind");
		}
	}

	FString FormatNumber(const FArcRmlDemoSettingDef& Setting, int32 Number)
	{
		switch (Setting.Format)
		{
		case EFormat::Percent:
			return FString::Printf(TEXT("%d%%"), Number);
		case EFormat::Fps:
			return FString::Printf(TEXT("%d FPS"), Number);
		case EFormat::Tenth:
			return FString::Printf(TEXT("%.1f"), Number / 10.0);
		default:
			return LexToString(Number);
		}
	}

	FString KeyLabel(const FString& Key)
	{
		if (Key == TEXT(" "))
		{
			return TEXT("Space");
		}
		if (Key == TEXT("Backspace"))
		{
			return Key;
		}
		return Key.Len() == 1 && FChar::IsAlnum(Key[0]) ? Key.ToUpper() : FString();
	}

	FString ImpactTone(const FString& Impact)
	{
		if (Impact == TEXT("High"))
		{
			return TEXT("danger");
		}
		if (Impact == TEXT("Medium"))
		{
			return TEXT("warning");
		}
		return Impact == TEXT("Low") ? FString(TEXT("success")) : FString(TEXT("neutral"));
	}
}

void UArcRmlDemoSettingsViewModel::InitScreen(const FString& Param)
{
	Dropdown.Reset();
	Listen.Reset();
	CancelTimer(ArcRmlDemoSettingsViewModelPrivate::ConfirmTimer);
	LoadDraft();
	Refresh();
}

void UArcRmlDemoSettingsViewModel::Back()
{
	if (!Listen.IsEmpty())
	{
		StopListening();
		Refresh();
		return;
	}
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const int32 Changes = CountChanges();
	if (!Demo || Changes == 0)
	{
		Super::Back();
		return;
	}
	FArcRmlDemoDialogSpec Spec = UnsavedDialog;
	Spec.Body = Spec.Body.Replace(TEXT("{count}"), *LexToString(Changes)).Replace(TEXT("{s}"), Changes == 1 ? TEXT("") : TEXT("s"));
	TWeakObjectPtr<UArcRmlDemoSettingsViewModel> WeakThis(this);
	Demo->OpenDialog(Spec, [WeakThis](int32 Choice) -> void
	{
		UArcRmlDemoSettingsViewModel* Self = WeakThis.Get();
		UArcRmlDemoSubsystem* Owner = Self ? Self->GetDemo() : nullptr;
		if (!Owner)
		{
			return;
		}
		if (Choice == 0)
		{
			Self->LoadDraft();
			Self->Refresh();
			Owner->Pop();
		}
		else if (Choice == 2)
		{
			Self->ApplyChanges(true);
		}
	});
}

void UArcRmlDemoSettingsViewModel::Activate(const FString& Field)
{
	const int32 Index = FindSetting(Field);
	if (Index == INDEX_NONE || (!Listen.IsEmpty() && Listen == Field))
	{
		return;
	}
	const FArcRmlDemoSettingDef& Setting = Settings[Index];
	const bool bDropdownHere = Dropdown.IsSet() && Dropdown->Label == Field;
	Touch(Field);
	if (Setting.bLocked)
	{
		Refresh();
		return;
	}
	if (bDropdownHere)
	{
		ResetDropdown();
		Refresh();
		return;
	}
	switch (Setting.Type)
	{
	case EArcRmlDemoSettingType::Toggle:
	case EArcRmlDemoSettingType::Checkbox:
		StepSetting(Setting, 1);
		break;
	case EArcRmlDemoSettingType::Dropdown:
		Dropdown = FDropdown{Field};
		break;
	case EArcRmlDemoSettingType::Keybind:
		Listen = Field;
		break;
	default:
		break;
	}
	Refresh();
}

void UArcRmlDemoSettingsViewModel::Stepped(const FString& Field, int32 Delta)
{
	const int32 Index = FindSetting(Field);
	if (Index == INDEX_NONE)
	{
		return;
	}
	Touch(Field);
	StepSetting(Settings[Index], Delta < 0 ? -1 : 1);
	Refresh();
}

void UArcRmlDemoSettingsViewModel::SliderInput(const FString& Field, const FString& Value)
{
	const int32 Index = FindSetting(Field);
	if (Index == INDEX_NONE || Settings[Index].Type != EArcRmlDemoSettingType::Slider)
	{
		return;
	}
	Touch(Field);
	const FArcRmlDemoSettingDef& Setting = Settings[Index];
	if (!Setting.bLocked)
	{
		FArcRmlDemoSettingValue Number;
		Number.Number = FMath::Clamp(FMath::RoundToInt(FCString::Atof(*Value)), Setting.Min, Setting.Max);
		Draft.Add(Field, Number);
	}
	Refresh();
}

void UArcRmlDemoSettingsViewModel::Selected(const FString& Field, int32 Index)
{
	if (Field == ArcRmlDemoSettingsViewModelPrivate::CrumbsField)
	{
		Dropdown.Reset();
		if (Index == 1)
		{
			SelectTab(0);
			return;
		}
		Back();
		return;
	}
	const int32 SettingIndex = FindSetting(Field);
	if (SettingIndex == INDEX_NONE)
	{
		return;
	}
	Touch(Field);
	const FArcRmlDemoSettingDef& Setting = Settings[SettingIndex];
	if (!Setting.bLocked)
	{
		FArcRmlDemoSettingValue Value;
		Value.Number = FMath::Clamp(Index, 0, FMath::Max(0, Setting.Options.Num() - 1));
		Draft.Add(Field, Value);
		if (Dropdown.IsSet())
		{
			ResetDropdown();
		}
	}
	Refresh();
}

void UArcRmlDemoSettingsViewModel::FieldReset(const FString& Field)
{
	const int32 Index = FindSetting(Field);
	if (Index == INDEX_NONE)
	{
		return;
	}
	ResetDropdown();
	Touch(Field);
	if (!Settings[Index].bLocked)
	{
		Draft.Add(Field, Settings[Index].Default);
	}
	Refresh();
}

void UArcRmlDemoSettingsViewModel::KeyBound(const FString& Field, const FString& Key)
{
	if (Listen.IsEmpty() || Listen != Field)
	{
		return;
	}
	const FString Label = ArcRmlDemoSettingsViewModelPrivate::KeyLabel(Key);
	if (Label.IsEmpty())
	{
		return;
	}
	FArcRmlDemoSettingValue Value;
	Value.Text = Label;
	Draft.Add(Field, Value);
	StopListening();
	Refresh();
}

void UArcRmlDemoSettingsViewModel::KeybindCancel(const FString& Field)
{
	if (Listen.IsEmpty() || Listen != Field)
	{
		return;
	}
	StopListening();
	Refresh();
}

void UArcRmlDemoSettingsViewModel::TabSelected(const FString& Bar, int32 Index)
{
	if (Bar != ArcRmlDemoSettingsViewModelPrivate::TabBarField)
	{
		return;
	}
	Dropdown.Reset();
	StopListening();
	SelectTab(Index);
}

void UArcRmlDemoSettingsViewModel::Apply()
{
	if (Dropdown.IsSet() || !Listen.IsEmpty())
	{
		return;
	}
	ApplyChanges(false);
}

void UArcRmlDemoSettingsViewModel::WriteText(const FString& Field, const FString& Value)
{
	if (Field == ArcRmlDemoSettingsViewModelPrivate::SearchField)
	{
		ARC_RML_DEMO_SET(*this, Query, Value);
		return;
	}
	const int32 Index = FindSetting(Field);
	if (Index == INDEX_NONE || Settings[Index].Type != EArcRmlDemoSettingType::Text)
	{
		return;
	}
	FArcRmlDemoSettingValue Text;
	Text.Text = Value;
	Draft.Add(Field, Text);
	Refresh();
}

TArray<int32> UArcRmlDemoSettingsViewModel::TabRows() const
{
	TArray<int32> Rows;
	for (int32 Index = 0; Index < Settings.Num(); ++Index)
	{
		if (Settings[Index].Tab == Tab)
		{
			Rows.Add(Index);
		}
	}
	return Rows;
}

int32 UArcRmlDemoSettingsViewModel::FindSetting(const FString& Label) const
{
	return Label.IsEmpty() ? INDEX_NONE : Settings.IndexOfByPredicate([&Label](const FArcRmlDemoSettingDef& Def) -> bool { return Def.Label == Label; });
}

int32 UArcRmlDemoSettingsViewModel::RowOf(const FString& Label) const
{
	const TArray<int32> Rows = TabRows();
	for (int32 Row = 0; Row < Rows.Num(); ++Row)
	{
		if (Settings[Rows[Row]].Label == Label)
		{
			return Row;
		}
	}
	return INDEX_NONE;
}

FArcRmlDemoSettingValue UArcRmlDemoSettingsViewModel::DraftOf(const FArcRmlDemoSettingDef& Setting) const
{
	const FArcRmlDemoSettingValue* Value = Draft.Find(Setting.Label);
	return Value ? *Value : AppliedOf(Setting);
}

FArcRmlDemoSettingValue UArcRmlDemoSettingsViewModel::AppliedOf(const FArcRmlDemoSettingDef& Setting) const
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	const FArcRmlDemoAppliedSetting* Applied = Game ? Game->FindAppliedSetting(Setting.Label) : nullptr;
	if (!Applied)
	{
		return Setting.Default;
	}
	FArcRmlDemoSettingValue Value;
	Value.Number = Applied->Number;
	Value.Text = Applied->Text;
	return Value;
}

int32 UArcRmlDemoSettingsViewModel::CountChanges(const FString& OnlyTab) const
{
	int32 Changes = 0;
	for (const FArcRmlDemoSettingDef& Setting : Settings)
	{
		if ((OnlyTab.IsEmpty() || Setting.Tab == OnlyTab) && !(DraftOf(Setting) == AppliedOf(Setting)))
		{
			++Changes;
		}
	}
	return Changes;
}

void UArcRmlDemoSettingsViewModel::LoadDraft()
{
	Draft.Reset();
	for (const FArcRmlDemoSettingDef& Setting : Settings)
	{
		Draft.Add(Setting.Label, AppliedOf(Setting));
	}
}

void UArcRmlDemoSettingsViewModel::Touch(const FString& Label)
{
	if (Dropdown.IsSet() && Dropdown->Label != Label)
	{
		Dropdown.Reset();
	}
	if (!Listen.IsEmpty() && Listen != Label)
	{
		StopListening();
	}
	const int32 Row = RowOf(Label);
	if (Row != INDEX_NONE)
	{
		ARC_RML_DEMO_SET(*this, SelectedRow, Row);
	}
}

void UArcRmlDemoSettingsViewModel::StepSetting(const FArcRmlDemoSettingDef& Setting, int32 Direction)
{
	if (Setting.bLocked)
	{
		return;
	}
	FArcRmlDemoSettingValue Value = DraftOf(Setting);
	switch (Setting.Type)
	{
	case EArcRmlDemoSettingType::Stepper:
	case EArcRmlDemoSettingType::Radio:
		Value.Number = FMath::Clamp(Value.Number + Direction, 0, Setting.Options.Num() - 1);
		break;
	case EArcRmlDemoSettingType::Toggle:
	case EArcRmlDemoSettingType::Checkbox:
		Value.Number = Value.Number != 0 ? 0 : 1;
		break;
	default:
		return;
	}
	Draft.Add(Setting.Label, Value);
}

void UArcRmlDemoSettingsViewModel::CloseDropdown(const FString& Field)
{
	if (Dropdown.IsSet() && Dropdown->Label == Field)
	{
		ResetDropdown();
		Refresh();
	}
}

void UArcRmlDemoSettingsViewModel::ResetDropdown()
{
	if (Dropdown.IsSet())
	{
		const int32 Row = RowOf(Dropdown->Label);
		if (Row != INDEX_NONE)
		{
			ARC_RML_DEMO_SET(*this, SelectedRow, Row);
		}
	}
	Dropdown.Reset();
}

void UArcRmlDemoSettingsViewModel::StopListening()
{
	Listen.Reset();
}

void UArcRmlDemoSettingsViewModel::SelectTab(int32 Index)
{
	if (!TabDefs.IsValidIndex(Index))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, Tab, TabDefs[Index].Key);
	ARC_RML_DEMO_SET(*this, SelectedRow, 0);
	Refresh();
}

void UArcRmlDemoSettingsViewModel::ApplyChanges(bool bLeave)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo)
	{
		return;
	}
	if (CountChanges() == 0)
	{
		if (bLeave)
		{
			Demo->Pop();
		}
		return;
	}
	if (CountChanges(ConfirmTab) > 0)
	{
		OpenConfirmDialog(bLeave);
		return;
	}
	CommitDraft();
	if (bLeave)
	{
		Demo->Pop();
	}
}

void UArcRmlDemoSettingsViewModel::CommitDraft()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Demo || !Game)
	{
		return;
	}
	TArray<FArcRmlDemoAppliedSetting> Applied;
	for (const FArcRmlDemoSettingDef& Setting : Settings)
	{
		const FArcRmlDemoSettingValue Value = DraftOf(Setting);
		FArcRmlDemoAppliedSetting& Entry = Applied.AddDefaulted_GetRef();
		Entry.Label = Setting.Label;
		Entry.Number = Value.Number;
		Entry.Text = Value.Text;
	}
	Game->SetAppliedSettings(Applied);
	if (const FArcRmlDemoAppliedSetting* PromptStyle = Game->FindAppliedSetting(TEXT("Prompt style")))
	{
		if (PromptStyle->Number != 0)
		{
			Demo->SetInputScheme(PromptStyle->Number == 2 ? TEXT("gamepad") : TEXT("keyboard"));
		}
	}
	Demo->ShowToast(TEXT("success"), TEXT("Settings applied"));
	Refresh();
}

void UArcRmlDemoSettingsViewModel::RevertTab(const FString& RevertedTab)
{
	for (const FArcRmlDemoSettingDef& Setting : Settings)
	{
		if (Setting.Tab == RevertedTab)
		{
			Draft.Add(Setting.Label, AppliedOf(Setting));
		}
	}
	Refresh();
}

void UArcRmlDemoSettingsViewModel::OpenConfirmDialog(bool bLeave)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo)
	{
		return;
	}
	TWeakObjectPtr<UArcRmlDemoSettingsViewModel> WeakThis(this);
	UArcRmlDemoDialogViewModel* Dialog = Demo->OpenDialog(ConfirmDialog, [WeakThis, bLeave](int32 Choice) -> void
	{
		UArcRmlDemoSettingsViewModel* Self = WeakThis.Get();
		UArcRmlDemoSubsystem* Owner = Self ? Self->GetDemo() : nullptr;
		if (!Owner)
		{
			return;
		}
		Self->CancelTimer(ArcRmlDemoSettingsViewModelPrivate::ConfirmTimer);
		if (Choice != 1)
		{
			Self->RevertTab(Self->ConfirmTab);
			return;
		}
		Self->CommitDraft();
		if (bLeave)
		{
			Owner->Pop();
		}
	});
	if (!Dialog)
	{
		return;
	}
	ConfirmLeft = FMath::Max(1, ConfirmSeconds);
	const TFunction<void(UArcRmlDemoDialogViewModel&)> Show = [this](UArcRmlDemoDialogViewModel& Shown) -> void
	{
		Shown.SetCountdown(ConfirmLeft * 100.0f / FMath::Max(1, ConfirmSeconds), TEXT("Reverting in"), FString::Printf(TEXT("%d s"), ConfirmLeft));
	};
	Show(*Dialog);
	TWeakObjectPtr<UArcRmlDemoDialogViewModel> WeakDialog(Dialog);
	Every(ArcRmlDemoSettingsViewModelPrivate::ConfirmTimer, 1.0, [this, WeakDialog, Show]() -> bool
	{
		UArcRmlDemoDialogViewModel* Shown = WeakDialog.Get();
		UArcRmlDemoSubsystem* Owner = GetDemo();
		if (!Shown || !Owner || Owner->GetDialog() != Shown)
		{
			return false;
		}
		if (--ConfirmLeft > 0)
		{
			Show(*Shown);
			return true;
		}
		Shown->Choose(0);
		Owner->ShowToast(TEXT("warning"), TEXT("Display settings reverted"), FString::Printf(TEXT("No confirmation within %d seconds."), ConfirmSeconds));
		return false;
	});
}

FString UArcRmlDemoSettingsViewModel::ValueLabel(const FArcRmlDemoSettingDef& Setting, const FArcRmlDemoSettingValue& Value) const
{
	switch (Setting.Type)
	{
	case EArcRmlDemoSettingType::Toggle:
	case EArcRmlDemoSettingType::Checkbox:
		return Value.Number != 0 ? TEXT("On") : TEXT("Off");
	case EArcRmlDemoSettingType::Radio:
	case EArcRmlDemoSettingType::Stepper:
	case EArcRmlDemoSettingType::Dropdown:
		return Setting.Options.IsValidIndex(Value.Number) ? Setting.Options[Value.Number] : FString();
	case EArcRmlDemoSettingType::Slider:
		return ArcRmlDemoSettingsViewModelPrivate::FormatNumber(Setting, Value.Number);
	case EArcRmlDemoSettingType::Keybind:
		return Value.Text.IsEmpty() ? TEXT("Unbound") : Value.Text;
	default:
		return Value.Text;
	}
}

FArcRmlDemoSettingRow UArcRmlDemoSettingsViewModel::MakeRow(const FArcRmlDemoSettingDef& Setting, int32 RowIndex,
	const TMap<FString, TArray<FString>>& KeyUse) const
{
	using namespace ArcRmlDemoSettingsViewModelPrivate;

	const FArcRmlDemoSettingValue Value = DraftOf(Setting);
	FArcRmlDemoSettingRow Row;
	Row.Row = RowIndex;
	Row.Label = Setting.Label;
	Row.Field = Setting.Label;
	Row.Description = Setting.Description;
	Row.Type = TypeName(Setting.Type);
	Row.bChanged = !(Value == AppliedOf(Setting));
	Row.bLocked = Setting.bLocked;
	Row.Placeholder = Setting.Placeholder;
	Row.Tag = Setting.Tag;
	Row.TagTone = Setting.TagTone.IsEmpty() ? FString(TEXT("warning")) : Setting.TagTone;

	switch (Setting.Type)
	{
	case EType::Toggle:
		Row.bOn = Value.Number != 0;
		break;
	case EType::Checkbox:
		Row.bOn = Value.Number != 0;
		Row.ValueLabel = Setting.CheckLabel;
		break;
	case EType::Slider:
		Row.Value = Value.Number;
		Row.Min = Setting.Min;
		Row.Max = Setting.Max;
		Row.Step = FMath::Max(1, Setting.Step);
		Row.ValueLabel = FormatNumber(Setting, Value.Number);
		break;
	case EType::Stepper:
		Row.Index = Value.Number;
		Row.Count = Setting.Options.Num();
		Row.ValueLabel = ValueLabel(Setting, Value);
		break;
	case EType::Radio:
		Row.Index = Value.Number;
		Row.Options = Setting.Options;
		break;
	case EType::Dropdown:
		Row.Index = Value.Number;
		Row.Options = Setting.Options;
		Row.ValueLabel = ValueLabel(Setting, Value);
		Row.bOpen = Dropdown.IsSet() && Dropdown->Label == Setting.Label;
		break;
	case EType::Text:
		Row.ValueLabel = Value.Text;
		break;
	case EType::Keybind:
	{
		Row.ValueLabel = Value.Text;
		const TArray<FString>* Users = Value.Text.IsEmpty() ? nullptr : KeyUse.Find(Value.Text);
		const bool bClash = Users && Users->Num() > 1;
		Row.KeyState = Listen == Setting.Label ? TEXT("listening") : Value.Text.IsEmpty() ? TEXT("unbound") : bClash ? TEXT("conflict") : TEXT("bound");
		if (bClash)
		{
			TArray<FString> Others = *Users;
			Others.Remove(Setting.Label);
			Row.ConflictLabel = TEXT("Also bound to ") + FString::Join(Others, TEXT(", "));
		}
		break;
	}
	default:
		break;
	}
	return Row;
}

FString UArcRmlDemoSettingsViewModel::CrumbRoot() const
{
	const UArcRmlDemoSubsystem* Demo = GetDemo();
	return Demo && Demo->GetScreenBelow(GetScreenKey()) == TEXT("pause") ? TEXT("Pause") : TEXT("Main menu");
}

void UArcRmlDemoSettingsViewModel::Refresh()
{
	TArray<FArcRmlDemoTabRow> TabRowsOut;
	int32 Active = 0;
	for (int32 Index = 0; Index < TabDefs.Num(); ++Index)
	{
		TabRowsOut.Add(TabDefs[Index]);
		if (TabDefs[Index].Key == Tab)
		{
			Active = Index;
		}
	}

	TMap<FString, TArray<FString>> KeyUse;
	for (const FArcRmlDemoSettingDef& Setting : Settings)
	{
		const FArcRmlDemoSettingValue Value = DraftOf(Setting);
		if (Setting.Type == EArcRmlDemoSettingType::Keybind && !Value.Text.IsEmpty())
		{
			KeyUse.FindOrAdd(Value.Text).Add(Setting.Label);
		}
	}

	const TArray<int32> Rows = TabRows();
	TArray<FArcRmlDemoSettingGroup> NewGroups;
	const FArcRmlDemoSettingDef* Detail = nullptr;
	bool bResettable = false;
	for (int32 Row = 0; Row < Rows.Num(); ++Row)
	{
		const FArcRmlDemoSettingDef& Setting = Settings[Rows[Row]];
		if (NewGroups.IsEmpty() || NewGroups.Last().Title != Setting.Group)
		{
			NewGroups.AddDefaulted_GetRef().Title = Setting.Group;
		}
		NewGroups.Last().Rows.Add(MakeRow(Setting, Row, KeyUse));
		bResettable |= !Setting.bLocked && !(DraftOf(Setting) == Setting.Default);
		if (Row == SelectedRow)
		{
			Detail = &Setting;
		}
	}
	if (!Detail && !Rows.IsEmpty())
	{
		Detail = &Settings[Rows[0]];
	}

	const int32 Changes = CountChanges();
	const FString TabLabel = TabDefs.IsValidIndex(Active) ? TabDefs[Active].Label : FString();
	ARC_RML_DEMO_SET(*this, Tabs, TabRowsOut);
	ARC_RML_DEMO_SET(*this, ActiveTab, Active);
	ARC_RML_DEMO_SET(*this, Groups, NewGroups);
	ARC_RML_DEMO_SET(*this, Crumbs, TArray<FString>({CrumbRoot(), TEXT("Settings"), TabLabel}));
	ARC_RML_DEMO_SET(*this, ChangesLabel, FString::Printf(TEXT("%d %s"), Changes, Changes == 1 ? TEXT("unsaved change") : TEXT("unsaved changes")));
	ARC_RML_DEMO_SET(*this, bDropdownOpen, Dropdown.IsSet());
	ARC_RML_DEMO_SET(*this, bListening, !Listen.IsEmpty());
	ARC_RML_DEMO_SET(*this, bCanReset, bResettable);
	ARC_RML_DEMO_SET(*this, bCanApply, Changes > 0);
	if (Detail)
	{
		ARC_RML_DEMO_SET(*this, DetailLabel, Detail->Label);
		ARC_RML_DEMO_SET(*this, DetailLong, Detail->Long);
		ARC_RML_DEMO_SET(*this, DetailPreview, Detail->Preview.IsEmpty() ? TEXT("Preview · ") + Detail->Label : Detail->Preview);
		ARC_RML_DEMO_SET(*this, DetailCurrent, ValueLabel(*Detail, DraftOf(*Detail)));
		ARC_RML_DEMO_SET(*this, DetailDefault, Detail->DefaultLabel);
		ARC_RML_DEMO_SET(*this, DetailImpact, Detail->Impact);
		ARC_RML_DEMO_SET(*this, DetailImpactTone, ArcRmlDemoSettingsViewModelPrivate::ImpactTone(Detail->Impact));
	}
}
