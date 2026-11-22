# ArcRmlUIDemo

A complete example game UI for the ArcRmlUI plugin, built from RML and RCSS documents plus one view model module:
splash, main menu, settings, save and load, credits, lobby, loading screen, HUD, pause menu, match results, vendor,
crafting and the Game Menu hub (inventory, character, talents, quests, friends, achievements). All data is mock
data; nothing is saved. It runs on Windows (Win64) clients and in the editor, not on dedicated servers.

## Set up

1. Install ArcRmlUI from Fab and enable it in your project.
2. Put the `ArcRmlUIDemo` folder in your project's `Plugins` folder.
3. Open the project. The editor builds the plugin.
4. Open the map `/ArcRmlUIDemo/Demo/Maps/L_ArcRmlDemo` and press Play.

The map's game mode starts the demo on the splash screen. Continue, New game, Load and a found match open the second map,
`/ArcRmlUIDemo/Demo/Maps/L_ArcRmlDemoGame` (a floor, light, sky and fixed camera) with the HUD on top, and quitting to the menu
opens the first map again. The demo stops when you leave both.

## Packaging

With the default Project Settings > ArcRmlUI > Packaging > Package Everything, the demo's documents, fonts and
icons are packaged with no setup. If you turn Package Everything off, add `/ArcRmlUIDemo/UI/Rml` to Paths To
Package; icons that only Blueprint data names (item and achievement icons) ship with that path. Add both demo maps (`L_ArcRmlDemo` and `L_ArcRmlDemoGame`) to the maps you package (or set it
as the game default map) to start the demo in a packaged build.

## Confirm and Back keys

The demo maps Enter, Space and gamepad A to the click action, and Esc and gamepad B to the back action, of your
project's CommonUI input data. If your project already has that input data, nothing more is needed.

If your project has no CommonUI input data, add these lines to `Config/DefaultGame.ini`:

```ini
[/Script/CommonInput.CommonInputSettings]
bEnableEnhancedInputSupport=True
InputData=/ArcRmlUIDemo/Demo/Input/BP_ArcRmlDemoInputData.BP_ArcRmlDemoInputData_C
```

## Other ways to start

In a development or editor build, run the console command `ArcRmlDemo.Start` in any map with a local player, and
`ArcRmlDemo.Stop` to end it. The demo never starts by itself.

The game mode has a Start Screen property (a screen key such as `hud`). Set it on a Blueprint child of the game mode to
start a map straight on that screen; empty starts on the splash. It is ignored when the map is entered by the demo's own travel.

## Controls

- Arrow keys, D-pad or left stick move between items; Enter, Space or gamepad A confirms; Esc or gamepad B goes
  back. The mouse works everywhere.
- Q / E (gamepad LB / RB) switch the main tabs of Settings, Vendor and the Game Menu; Z / C (gamepad LT / RT)
  switch the sub-tabs inside the Game Menu.
- From the HUD, Esc or P (gamepad Start) opens the pause menu (P or Start closes it again), Tab (View) the Game Menu and F9 the
  HUD debug menu. Its Actions buttons open the vendor (V, gamepad X), crafting (C, Y) and, in a multiplayer session, end the match (M, right stick press); those keys work only while the menu is open.
- F10 toggles the Dev bar (mouse only): the theme (Midnight, Ember, Daylight, Slate, Relic), the prompt style (keyboard or
  gamepad) and a jump to any screen.

## How the screens open

Each screen is a document on one of ArcRmlUI's UI layers, opened with `OpenDocument` and an init that sets up the
screen's view model before the screen is shown. Closing the top screen shows the one below again; the demo hears it through
the plugin's `OnDocumentActivated` event and tells that screen's view model. The HUD stays open under the menus, hidden and paused.
Back never closes the splash, main menu, loading screen or HUD. Dialogs are documents on the modal layer.

## Where your data goes

Each screen document has its own view model. The data a screen shows (item rows, quests, friends, recipes, stock, saves,
servers, achievements, talents, settings rows, names, prices, descriptions and icons) is a property of that view
model's C++ class (category `ArcRmlDemo|Data`), and its values are the class defaults of the Blueprint child. Every
document declares the Blueprint child of its class, `BP_ArcRmlDemo<Screen>VM` in `/ArcRmlUIDemo/Demo/ViewModels/`;
open it and edit the values in Class Defaults to change what the screen shows. An icon is a full path with the
extension (`/ArcRmlUIDemo/UI/Rml/icons/sword.svg`); its field has a picker that lists the `.svg` files of
your project. A raster image is a `UTexture2D` asset in a `TSoftObjectPtr<UTexture2D>` property instead; it binds as
`asset:<path>`. The player's bag, equipment, gold and settings live in one shared game model (`VM_ArcRmlDemoGame`)
that several screens read; its item catalogue is in the class defaults of `BP_ArcRmlDemoGameVM`. The demo creates
that model and the UI model from the Blueprint classes named in Project Settings > ArcRmlUI Demo (stored in
`Config/DefaultArcRmlUIDemo.ini`), and the Hub model from the same settings; the Hub class must also be the one
`Hub.rml` declares. To feed the screens from your own game, replace the values or the view model class and keep the
field and function names the documents use.

The demo's fonts are declared by `Content/UI/Rml/Fonts/Fonts.rcss`, which every demo document links. A font is
loaded by an `@font-face` rule; a font file in a folder is not loaded by itself.

`BP_ArcRmlDemoCreditsVM` is a view model made entirely in Blueprint: its parent is `UMVVMViewModelBase`, its `Sections`
variable (an array of the demo's credits row struct) is marked Field Notify and holds the credits in its default value,
and the Credits screen reads it with no C++ behind it.

## Binding a Blueprint view model in RML

A document declares a view model in its `<head>`. A Blueprint class is named by its generated class path:

```xml
<viewmodel name="credits"
           class="/ArcRmlUIDemo/Demo/ViewModels/BP_ArcRmlDemoCreditsVM.BP_ArcRmlDemoCreditsVM_C"
           resolver="/Script/ArcRmlUIDemo.ArcRmlDemoViewModelResolver"/>
```

The `resolver` hands out the demo's instance of that class (one per player while the demo runs). In your own UI use
any other source the plugin accepts instead: `context="..."` (an instance in the game instance's view model
collection), `create` (the plugin makes one instance), `manual` (your code assigns it) or `path`. Only
Blueprint-visible Field Notify variables bind to the document, and a Blueprint function (not an event) can be called
from RML with `call('', 'FunctionName', args)`. A member of a Blueprint struct binds under the name you typed in the
editor.

## Licences

- Fonts (SIL Open Font License): `Content/UI/Rml/Fonts/OFL-*.txt`.
- Icons (Lucide): `Content/UI/Rml/icons/LICENSE-lucide.txt`.
