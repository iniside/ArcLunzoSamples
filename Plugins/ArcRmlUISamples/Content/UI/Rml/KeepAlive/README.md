# Keep alive

A menu with four buttons and two screens it opens: `Kept`, which stays alive when it closes, and `Preloaded`,
which is loaded before its first open. Back on either screen closes it and the menu shows again.

- `<meta name="keep-alive" content="true"/>` in `Kept.rml`'s `<head>`: closing the screen parks it instead of
  destroying it. Open it again and it comes back as you left it: the count, the text in the field, the scroll position
  of the list, the classes and `data-if` state, and the view model with its values. Focus starts again at the
  `autofocus` button and the enter animation plays again.
- A parked screen is not open: it draws nothing, takes no input and does not hide the screens below it.
- "Preload" calls `PreloadDocument` with `Preloaded.rml`. The screen is loaded, laid out and parked without being
  shown, so "Open preloaded" shows it at once. It starts once the local player controller exists (and its pawn, when
  the game mode has a default pawn), one preload at a time. A preloaded screen parks on every close, although it has
  no `keep-alive` meta. Opened before "Preload", it opens and closes like any screen.
- "Discard both" calls `DiscardDocument` for both screens: a parked screen is destroyed, so the next open loads it
  again and `Kept` starts at count 0 with an empty field. A discarded preloaded screen loses its preload mark: one
  still loading is dropped, and one that is open closes for good instead of parking. An open screen that is not
  preloaded, or one never loaded, is left alone.
- The menu's buttons call `call('', 'Preload')` and the like. Each view model function takes a
  `UObject* WorldContextObject` (that parameter takes no argument in RML), which is the local player controller, and
  reaches `UArcRmlViewportSubsystem` through its local player. `PreloadDocument` and `OpenDocument` are also
  Blueprint nodes; `DiscardDocument` is C++ only.
- In a Game Feature, the companion plugin's `UArcRmlGameFeatureAction_PreloadDocuments` preloads its documents on
  activation and discards them on deactivation. The console command `ArcRml.Discard <Path>` discards one parked screen.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open KeepAlive
```

`ArcRmlSamples.Close` closes the menu again.

## Files

- `Menu.rml`, `Kept.rml`, `Preloaded.rml`, `KeepAlive.rcss`
- `Source/ArcRmlUISamples/KeepAlive/ArcRmlSamplesKeepAlive.h`, `.cpp` (the two view models and the open function)

Guide page: Keep-alive and preload.
