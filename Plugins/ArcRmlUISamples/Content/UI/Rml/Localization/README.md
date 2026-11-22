# Localization

A panel whose texts come from a String Table and change language in place. Two buttons ("English", "Polski") switch
the culture; the title, the sentences, the labels and the "Add a coin" button change at once, the plain grey line
never does. "Add a coin" counts up, and the count picks its plural form for the current language: "1 coin", "2 coins"
in English, "1 moneta", "2 monety", "5 monet" in Polish.

- `loc="Key"` on an element that holds plain text: the text is the entry `Key` of the String Table beside the file,
  `Loc/ST_Localization`. The text written in the `.rml` is the source (English) text.
- The plain line has no `loc`, so it is shown as written in every culture.
- `{{ Coins | arcplural('Coins') }}`: the entry `Coins` holds the plural forms,
  `{0}|plural(one={0} coin,other={0} coins)`; the Polish entry adds `few` and `many`. `{0}` is the number.
- `{{ Culture }}` shows the culture name the view model reads from the engine. The view model listens to the
  engine's culture change, so a change from anywhere (the buttons, game code, the console command `culture=pl`)
  updates the readout.
- `call('', 'SwitchCulture', 'pl')`: `SwitchCulture(const FString& Name)` calls
  `FInternationalization::Get().SetCurrentCulture(Name)`. ArcRmlUI translates every open document again in place;
  the document is not reopened, so focus and the count stay.
- `data-class-selected="Culture == 'pl'"` marks the button of the current culture.
- A text with `loc` must be the whole text of its element: no child element and no `{{ }}` inside it, and an
  attribute (a tooltip, a placeholder) cannot be localized.
- `autofocus` on "English": focus starts on the first culture button.

## Edit the texts

1. Change the text in `Localization.rml`, then right-click the file (or its folder) in the Content Browser and choose
   **Sync Strings**. It adds new keys to `ST_Localization` and updates changed texts. The plural entry `Coins` is
   edited in the String Table itself; Sync Strings only checks it.
2. Gather, translate and compile. The route depends on where the document lives.

   **Documents in your own project** (`/Game/UI/Rml/...`): the string table is a content asset, so the project's
   Game localization target can gather it. Open **Tools > Localization Dashboard**, select the Game target and, in
   its settings, enable **Gather from Packages** and add an include path that covers your documents (the default
   Game target gathers no packages). Click **Gather Text**, translate the texts (the edit-translations button in the
   culture's row, tooltip "Edit translations for this culture.", or the exported `.po` file), then click
   **Compile Text**.

   **This sample's plugin target**: the dashboard lists only the Game and Engine targets, not a plugin's
   `LocalizationTargets`, so run the engine command from the project folder:

   ```
   UnrealEditor-Cmd.exe <Project>.uproject -run=GatherText -config=Plugins/ArcRmlUISamples/Config/Localization/ArcRmlUISamples.ini
   ```

   It gathers the plugin's String Tables, writes the `.manifest` and `.archive`, and exports
   `Content/Localization/ArcRmlUISamples/<culture>/ArcRmlUISamples.po`. Write the translations as the `msgstr`
   lines of `pl/ArcRmlUISamples.po` and run the same command again: it imports the `.po` and compiles
   `Content/Localization/ArcRmlUISamples/<culture>/ArcRmlUISamples.locres`.

## Packaging

A packaged game ships only the cultures listed in the project's packaging settings. In `Config/DefaultGame.ini`:

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+CulturesToStage=pl
```

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Localization
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Localization.rml`, `Localization.rcss`
- `Loc/ST_Localization` (the String Table)
- `Content/Localization/ArcRmlUISamples/` (the gathered and compiled texts, `en` and `pl`)
- `Source/ArcRmlUISamples/Localization/ArcRmlSamplesLocalization.h`, `.cpp` (the view model and the open function)

Guide page: Localization.
