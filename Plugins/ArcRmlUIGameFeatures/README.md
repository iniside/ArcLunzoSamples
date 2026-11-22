# ArcRmlUIGameFeatures

Game Feature actions for ArcRmlUI. The ArcRmlUI plugin from Fab does not enable the engine's GameFeatures plugin; this
plugin adds the two actions for projects that use Game Features.

- **Preload RML Documents** (`UArcRmlGameFeatureAction_PreloadDocuments`): lists documents. While the Game Feature is
  active, each document is built and parked for every local player, so its first open is warm. Deactivating the
  Game Feature discards them. Nothing happens on a dedicated server. When Package Everything is off, the cook packages
  the listed documents, from any plugin, with their frames and the documents their `open:` actions name, on each platform
  that enables the Game Feature plugin (log line `ArcRmlUI cook: seed '<path>' from game feature preload action of plugin
  '<name>' (<asset>)`).
- **Package RML Documents** (`UArcRmlGameFeatureAction_PackageDocuments`): lists package paths (folders or files) inside
  the Game Feature's own plugin, for example `/MyFeature/UI/Rml/Hud.rml`. The cook packages them when the Game Feature
  plugin is enabled on the platform, so a project that turned off Project Settings > ArcRmlUI > Packaging >
  Package Everything does not have to list them in Paths To Package. The action does nothing at runtime. The cook
  log has one line per action, `ArcRmlUI cook: game feature action of plugin '<name>' (<asset>): <n> paths, <n> files`,
  and an error for a path outside the plugin.

## Set up

1. Install ArcRmlUI from Fab and enable it in your project.
2. Put the `ArcRmlUIGameFeatures` folder in your project's `Plugins` folder.
3. Open the project. The editor builds the plugin and enables it with the plugins it needs (ArcRmlUI and GameFeatures).
4. In a Game Feature plugin's `GameFeatureData` asset, add the action with "Add Action".

A Game Feature plugin that uses the actions should list `ArcRmlUIGameFeatures` under `Plugins` in its `.uplugin`.

## GameFeatureData Asset Manager rule

Enabling GameFeatures makes the engine log `Asset manager settings do not include a rule for assets of type
GameFeatureData` unless the project has a rule for that type. A project that already uses Game Features has one. If yours
does not, add this to `Config/DefaultGame.ini` (the rule Lyra uses):

```ini
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="GameFeatureData",AssetBaseClass="/Script/GameFeatures.GameFeatureData",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path="/Game/Unused")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=AlwaysCook))
```

Do not add it when the project already has a `GameFeatureData` entry: the Asset Manager then reports multiple entries
for that type.

## Licence

MIT, see `LICENSE` at the root of the repository. ArcRmlUI itself is a separate product with its own licence on Fab.
