# UMG layout

A document widget placed among other UMG widgets: the slot gives it its size and the surrounding panel scrolls and clips it.

- `WBP_Sample_UmgLayout`: a Canvas Panel with a `ScrollBox` (40, 40; 400 x 300) holding two `TextBlock`s, a `SizeBox`
  (`HeightOverride` 260) with the `UArcRmlDocumentWidget` named `Document` (its `Document` is `UmgLayout.rml`), and a
  third `TextBlock`.
- The document widget's own desired size is zero: the `SizeBox` gives the document its height, and the `ScrollBox` width
  gives it its width. Scroll the box and the document moves and clips with the text around it.
- `UmgLayout.rml` is static, with no view model. The widget closes with the samples "Back to samples" button or
  `ArcRmlSamples.Close`.
- The open function creates the widget with `CreateWidget` for the first local player's controller and adds it with
  `AddToPlayerScreen(1500)`.
- Input (keys, focus, navigation) of the nested document is not part of this sample: use the mouse.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open UmgLayout
```

`ArcRmlSamples.Close` removes it again.

## Files

- `UmgLayout.rml`, `UmgLayout.rcss`
- `WBP_Sample_UmgLayout` (Widget Blueprint, made by `Scripts/arcrmlui/create_samples_blueprints.py`)
- `Source/ArcRmlUISamples/UmgLayout/ArcRmlSamplesUmgLayout.h`, `.cpp` (the open, close and is-open functions)
