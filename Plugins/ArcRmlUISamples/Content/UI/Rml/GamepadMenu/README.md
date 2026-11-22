# Gamepad menu

A menu driven by keyboard and gamepad: three buttons, one verb of the menu's own, and Back closes it.

- `<meta name="input-mode" content="menu"/>`: the document takes keyboard, gamepad and mouse input while it is open.
- Arrows and the D-pad move the focus between the buttons. A `<button>` is a form control, so it takes focus and is a
  navigation stop by default; `autofocus` on "Play" is where the focus starts. A `div` or `span` needs
  `tab-index: auto; nav: auto` to become one.
- Confirm is always Enter, Space or the gamepad's accept button and presses the focused button. The CommonUI click
  action only supplies its key label, `{{ ArcKeys.confirm }}`.
- Back is the back action of the project's CommonUI input data. A Back that no `data-event-back` handler stops
  closes the document.
- `<meta name="input-mapping" content="/ArcRmlUISamples/UI/Rml/GamepadMenu/IMC_Sample_GamepadMenu"/>`: the mapping
  context the document adds while it is active. It maps one action, `IA_Sample_MenuInfo` (I, gamepad Y), which
  arrives as a UI action.
- `data-hotkey="IA_Sample_MenuInfo"`: when that action fires, the element is clicked, so "Info" calls
  `ToggleInfo`. `{{ ArcKeys.IA_Sample_MenuInfo }}` shows the action's key, named by the action's asset name.
- `{{ ArcKeys.confirm }}` and `{{ ArcKeys.back }}` show the keys of Confirm and Back for the current input device.
- Keep Confirm and Back out of a document's mapping context: they belong to one project-wide context.

## Project setup

The menu needs a CommonUI setup in the project: a CommonUI game viewport client, Enhanced Input support with an
`InputData` Blueprint whose click and back actions are Confirm and Back, and a mapping context with their keys added
at start. A menu document whose Back action has no key logs a warning when it opens. A project without such a setup
can use the input assets under `/ArcRmlUISamples/Input/` with these lines:

```ini title="Config/DefaultEngine.ini"
[/Script/Engine.Engine]
GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient
```

```ini title="Config/DefaultGame.ini"
[/Script/CommonInput.CommonInputSettings]
bEnableEnhancedInputSupport=True
InputData=/ArcRmlUISamples/Input/BP_SampleInputData.BP_SampleInputData_C
```

```ini title="Config/DefaultInput.ini"
[/Script/Engine.InputSettings]
DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput
DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent

[/Script/EnhancedInput.EnhancedInputDeveloperSettings]
+DefaultMappingContexts=(InputMappingContext="/ArcRmlUISamples/Input/IMC_Sample_Common.IMC_Sample_Common",Priority=0,bAddImmediately=True,bRegisterWithUserSettings=False)
```

`IMC_Sample_Common` maps Enter, Space and gamepad A to `IA_Sample_Confirm`, Escape and gamepad B to
`IA_Sample_Back`. Restart the editor after changing these lines.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open GamepadMenu
```

Move with the arrows or the D-pad, press Enter or gamepad A, press I or gamepad Y for "Info", and Escape or gamepad B
to close it. `ArcRmlSamples.Close` closes it too.

## Files

- `GamepadMenu.rml`, `GamepadMenu.rcss`
- `IA_Sample_MenuInfo`, `IMC_Sample_GamepadMenu` (the menu's own action and mapping context)
- `Source/ArcRmlUISamples/GamepadMenu/ArcRmlSamplesGamepadMenu.h`, `.cpp` (the view model and the open function)
- `/ArcRmlUISamples/Input/`: `IA_Sample_Confirm`, `IA_Sample_Back`, `IMC_Sample_Common`, `BP_SampleInputData`
  (the project setup above)

Guide page: Input and gamepad.
