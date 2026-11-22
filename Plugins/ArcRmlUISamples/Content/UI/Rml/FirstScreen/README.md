# First screen

The smallest document: a title and a line of text, opened and closed from C++ or from Blueprint. It has no view model and no
buttons, because a `data-action` button needs a view model on `<body>`.

- `<meta name="layer" content="UI.Layer.Game"/>` puts it on the game layer.
- `<meta name="input-mode" content="game"/>` keeps the input with the game: the document takes no keys or clicks.
- `Samples.rcss` sets the text style; `FirstScreen.rcss` places the panel.
- `ArcRmlSamplesFirstScreen::Open` gets `UArcRmlViewportSubsystem` from the local player and calls
  `OpenDocument` with the document's path; `CloseDocument` with the same path closes it.
- Blueprint: `BP_FirstScreen_Open` is a Blueprint Function Library with two functions. `Open` (input
  `PlayerController`, output `Document`) gets the Arc Rml Viewport Subsystem from the player controller, makes an
  `ArcRmlDocumentRef` with the document's path, calls `OpenDocument` and returns the document when it opened.
  `Close` (inputs `PlayerController`, `Document`) calls `CloseDocument` with that document.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open FirstScreen
```

`ArcRmlSamples.Close` closes it again. `ArcRmlSamples.OpenBP FirstScreen` opens it through the Blueprint `Open`
function instead; `ArcRmlSamples.Close` then calls the Blueprint `Close` function.

## Files

- `FirstScreen.rml`, `FirstScreen.rcss`
- `Source/ArcRmlUISamples/FirstScreen/ArcRmlSamplesFirstScreen.h`, `.cpp` (the open function)
- `BP_FirstScreen_Open` (Blueprint Function Library: functions `Open`, `Close`)

Guide page: Quick start.
