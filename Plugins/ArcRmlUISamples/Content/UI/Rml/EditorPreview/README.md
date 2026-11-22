# Editor preview

A document whose view model is stand-in data from a JSON file, so it shows real-looking values without any C++.

- `<viewmodel name="player" preview="Preview/EditorPreview.json"/>`: a preview model. The path is relative to the
  document. A preview declaration takes no `class`, source attribute, `settable` or `on-close`.
- `Preview/EditorPreview.json`: the top-level keys are the fields (`Name`, `Level`, `Health`, `Accent`,
  `bLowHealth`, `Items`). A colour is `{ "$colour": "#rrggbbaa" }`.
- `$variants`: named overrides of the base data. `LowHealth` changes `Health`, `Accent`, `bLowHealth` and
  `Items`; every other field keeps its base value. A variant names only fields the base data has.
- The data shows unless the build is Shipping or `ArcRml.PreviewHidden` is 1, also in a game with a local player.
  Otherwise, or when the JSON file is missing, the model binds its default values with `ArcResolved` false, and
  `data-if="!ArcResolved"` shows the fallback line.
- Never make live behaviour depend on preview data.

## Run it

In the editor, double-click `EditorPreview.rml` in the Content Browser: the preview opens, and its Variant menu
switches between Base and `LowHealth`.

In the game, play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRml.Preview /ArcRmlUISamples/UI/Rml/EditorPreview/EditorPreview.rml LowHealth
```

Leave out `LowHealth` for the base data. `ArcRml.Close /ArcRmlUISamples/UI/Rml/EditorPreview/EditorPreview.rml` or
the Close button closes it.

## Files

- `EditorPreview.rml`, `EditorPreview.rcss`
- `Preview/EditorPreview.json` (the base data and the `LowHealth` variant)

Guide page: Editor preview.
