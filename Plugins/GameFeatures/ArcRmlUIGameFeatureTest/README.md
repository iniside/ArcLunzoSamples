# ArcRmlUIGameFeatureTest

Game Feature plugin (built-in, starts `Active`) that checks
`UArcRmlGameFeatureAction_PackageDocuments` in a packaged build with
Package Everything off. The action and the cook provider that reads it come from the companion plugin
`ArcRmlUIHost/Plugins/ArcRmlUIGameFeatures`, which this plugin lists as a dependency.

- `Content/ArcRmlUIGameFeatureTest.uasset`: the GameFeatureData. Its one
  "Package RML Documents" action lists `/ArcRmlUIGameFeatureTest/UI/Rml/GameFeatureTest.rml`
  and the folders `/ArcRmlUIGameFeatureTest/UI/Rml/Dependencies` and
  `/ArcRmlUIGameFeatureTest/UI/Rml/Localization`.
  The pages use the plugin's own assets: the MetaSoundSources
  `UI/Sound/MS_DependenciesClick` and `MS_DependenciesHover`, the texture
  `UI/Textures/T_DependenciesAsset` and the String Tables `UI/Rml/Dependencies/Loc/ST_Dependencies`
  and `UI/Rml/Localization/Loc/ST_Localization` (written by Sync Strings from the pages' `.rml`).
- `Source/ArcRmlUIGameFeatureTest`: the Localization page's view model
  `UArcRmlLocTestViewModel` (field `Culture`, call `SwitchCulture(Name)`), registered under
  context `LocTest` by the game instance subsystem `UArcRmlLocTestSubsystem`.
- `Content/UI/Rml/Localization/Localization.rml`: a title, two sentences and three menu labels
  from `ST_Localization`, one plain line without `loc`, and a small menu that switches the
  culture to `en`, `pl`, `fr`, `zh-Hans`, `ar`, `LEET` or `keys`. Barlow has no Chinese or Arabic
  letters, so `Localization.rcss` adds two fallback faces copied from the engine's
  `Engine/Content/Slate/Fonts`: `DroidSansFallback.ttf` (Apache 2.0, `LICENSE-DroidSans.txt`) and
  `NotoNaskhArabicUI-Regular.ttf` (OFL, `LICENSE-Noto.txt`).
- Localization target `ArcRmlUIGameFeatureTest` (`LocalizationTargets` in the `.uplugin`,
  `Config/Localization/ArcRmlUIGameFeatureTest.ini`): gathers the plugin's String Tables and
  writes `Content/Localization/ArcRmlUIGameFeatureTest/<culture>/` (`.archive`, `.po`, `.locres`)
  for `en` (native), `pl`, `fr`, `zh-Hans` and `ar`. The translations live in the
  `.po` files. To regenerate after a text change:
  1. right-click the page's `.rml` in the Content Browser and choose **Sync Strings**;
  2. `UnrealEditor-Cmd.exe <full path>/ArcRmlUIHost.uproject -run=GatherText -config=<full path>/Plugins/GameFeatures/ArcRmlUIGameFeatureTest/Config/Localization/ArcRmlUIGameFeatureTest.ini`;
  3. translate the new texts in each `<culture>/ArcRmlUIGameFeatureTest.po`;
  4. step 2 again, which imports the `.po` files and compiles the `.locres` files.

  The packaged game needs these cultures and the full ICU data in
  `ArcRmlUIHost/Config/DefaultGame.ini`:

  ```ini
  [/Script/UnrealEd.ProjectPackagingSettings]
  InternationalizationPreset=All
  +CulturesToStage=pl
  +CulturesToStage=fr
  +CulturesToStage=zh-Hans
  +CulturesToStage=ar
  ```

  `LEET` (letters swapped for digits and signs and the text wrapped in `‡`, so `Lorem ipsum`
  reads `‡L0r3m !p$um‡`) and `keys`
  (each text shows its String Table namespace and key) are the engine's debug cultures; they
  need no translation data but exist only in Debug, Development and Test builds, not Shipping.
- `Content/UI/Rml/GameFeatureTest.rml` links `GameFeatureTest.rcss` and shows
  `GameFeatureTest.svg`; the cook ships all three through the action. Its text, like the
  Dependencies page's (`Shared/Shared.rcss`), is in `'Barlow Condensed'`, the face this plugin
  ships, so it renders without any other plugin's fonts. A generic family such as `sans-serif`
  would resolve to the host's `GenericFontFamilies` mapping, whose files are in other plugins,
  and the cook reports that as an error.
- `Content/UI/Rml/Dependencies/Dependencies.rml` uses every kind of dependency the cook
  follows, one numbered cell each. Files outside the selected folder ship only because the
  cook follows the references:
  1. `<img src>` PNG `Shared/Images/Element.png` (outside);
  2. `<svg src>` `Dependencies/Images/Vector.svg`;
  3. `background: url()` in `Shared/Shared.rcss`, a linked sheet outside the folder, naming
     `Shared/Images/Background.png`;
  4. `decorator: image()` in `Dependencies.rcss` naming `Shared/Images/ImageFunction.png` (outside);
  5. a named `@decorator` with `image-src` `Dependencies/Images/Named.png`;
  6. an `@spritesheet` (`Dependencies/Images/Sprites.png`) with two `img sprite`;
  7. `fill-image` `Dependencies/Images/Fill.png` on a `<progress>`;
  8. `image()` in a static `style` attribute naming `Shared/Images/Inline.png` (outside);
  9. the `asset:` texture `UI/Textures/T_DependenciesAsset`;
  10. `--sound-click` / `--sound-hover` in `Shared.rcss` naming the two MetaSoundSources;
  11. a `loc` text from `ST_Dependencies`;
  12. the face `UI/Rml/Fonts/BarlowCondensed-Medium.ttf` of this plugin (OFL, copied from
      `ArcRmlUITest`; no demo file has this family, so in the package only this plugin's copy
      can draw it).
- `Content/UI/Rml/NotPackaged/NotPackaged.rml` is in no selection and must not ship.
  The document is valid; it is only absent from every selection, so the package must not
  contain it.

## Check

1. In `ArcRmlUIHost/Config/DefaultGame.ini`, under `[/Script/ArcRmlUI.ArcRmlUISettings]`:

   ```ini
   bPackageEverything=False
   ```

   with no `PathsToPackage`, so only the game feature action selects files.
2. Package the project for Windows into `ArcRmlUIHost/Packaged` (from the engine root):

   ```
   Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -nop4 -project=<full path>\ArcRmlUIHost.uproject -target=ArcRmlUIHost -platform=Win64 -clientconfig=Development -map=/ArcRmlUIDemo/Demo/Maps/L_ArcRmlDemo -build -cook -zenstore -stage -pak -manifests -prereqs -package -archive -archivedirectory=<full path>\Packaged
   ```
3. The cook log has
   `ArcRmlUI cook: game feature action of plugin 'ArcRmlUIGameFeatureTest' (/ArcRmlUIGameFeatureTest/ArcRmlUIGameFeatureTest.ArcRmlUIGameFeatureTest): 3 paths, 9 files`
   and no `LogArcRmlUI` error. The cook log also holds one line per dependency of each written
   document,
   `ArcRmlUI cook: Windows: '/ArcRmlUIGameFeatureTest/UI/Rml/Dependencies/Dependencies.rml' <kind> '<path>' <state>`:
   `packaged` for both `style sheet`s (`Shared/Shared.rcss`, `Dependencies/Dependencies.rcss`) and every
   `element image`, `style sheet image`, `decorator image` and `fill-image` above;
   `fonts '/ArcRmlUIGameFeatureTest/UI/Rml/Fonts/' packaged (1 files)`;
   `font family ''Barlow Condensed'' packaged (/ArcRmlUIGameFeatureTest/UI/Rml/Fonts/BarlowCondensed-Medium.ttf)`
   (also for `GameFeatureTest.rml`); `added to cook` for the `texture`
   `T_DependenciesAsset`, the `sound`s `MS_DependenciesClick` and `MS_DependenciesHover` and the
   `string table` `ST_Dependencies`. No line says `NOT packaged`.
4. `ArcRmlUIHost/Packaged/Windows/Manifest_UFSFiles_Win64.txt` lists, under
   `ArcRmlUIHost/Plugins/GameFeatures/ArcRmlUIGameFeatureTest/Content/`:
   - `UI/Rml/GameFeatureTest.rmlc`, `GameFeatureTest.rcssc` and `GameFeatureTest.svg`;
   - `UI/Rml/Dependencies/Dependencies.rmlc`, `Dependencies.rcssc`, `Images/Vector.svg`,
     `Images/Named.png`, `Images/Sprites.png`, `Images/Fill.png`;
   - `UI/Rml/Shared/Shared.rcssc`, `Shared/Images/Element.png`, `Background.png`,
     `ImageFunction.png`, `Inline.png`;
   - `UI/Rml/Fonts/BarlowCondensed-Medium.ttf`;
   - the cooked `UI/Sound/MS_DependenciesClick`, `MS_DependenciesHover`,
     `UI/Textures/T_DependenciesAsset` and `UI/Rml/Dependencies/Loc/ST_Dependencies` packages;
   - `UI/Rml/Localization/Localization.rmlc`, `Localization.rcssc` and the cooked
     `UI/Rml/Localization/Loc/ST_Localization` package;

   and no `NotPackaged` line.
5. In the packaged game console: `ArcRml.Open /ArcRmlUIGameFeatureTest/UI/Rml/GameFeatureTest.rml`.
   Expected: a centred panel with the check mark image and "Game Feature packaging test" in the
   narrow Barlow face.
   `ArcRml.Open /ArcRmlUIGameFeatureTest/UI/Rml/NotPackaged/NotPackaged.rml` must fail to load.
6. In the packaged game console: `ArcRml.Open /ArcRmlUIGameFeatureTest/UI/Rml/Dependencies/Dependencies.rml`.
   Expected, one cell each: (1) a magenta framed square, (2) a blue star on white, (3) a teal
   checker, (4) an orange framed square, (5) a purple framed square, (6) a red circle and a blue
   square, (7) a green striped bar about two thirds full, (8) a yellow framed square, (9) a cyan
   square with a white cross, (10) a round "sound" button that plays a (silent) sound on click and
   hover, (11) the yellow text "Text from the String Table ST_Dependencies" (the key
   `DependenciesLocText` instead means the table did not ship), (12) "Barlow Condensed 0123" in
   the narrow Barlow face. No `LogArcRmlUI` error.
7. In the packaged game console: `ArcRml.Open /ArcRmlUIGameFeatureTest/UI/Rml/Localization/Localization.rml`.
   Expected: the title "Localization test", "Welcome, traveler!", the blacksmith sentence, the
   yellow labels "New game", "Options", "Quit", the grey plain line, and "Culture: en" with the
   `en` button lit. Each click changes the six texts in place, the plain line never changes, and
   the log has `ArcRmlUI: culture '<name>' loaded`:
   - `pl`: "Test lokalizacji", "Witaj, wędrowcze!", "Nowa gra", "Opcje", "Wyjdź" (Polish
     letters in Barlow);
   - `fr`: "Test de localisation", "Bienvenue, voyageur !", "Nouvelle partie", "Quitter";
   - `zh-Hans`: "本地化测试", "欢迎你，旅行者！", "新游戏", "选项", "退出" (Droid Sans Fallback);
   - `ar`: "اختبار الترجمة", "مرحبًا أيها المسافر!" (Noto Naskh Arabic). ArcRmlUI lays out
     every line left to right (no bidi), so this checks that the translation ships and the
     glyphs draw joined, not the right-to-left layout;
   - `LEET`: leet letters in `‡…‡`; `keys`: each text shows its key and namespace (the `.rml` path), such as
     `LocTitle, /ArcRmlUIGameFeatureTest/UI/Rml/Localization/Localization.rml`;
   - `en`: the source texts come back.

   The English text under `pl`, `fr`, `zh-Hans` or `ar` means that culture's `.locres` did not
   ship; keys instead of text under `en` mean `ST_Localization` did not ship; a warning
   `culture '<name>' does not exist in this build` means the ICU data for it was not staged.
