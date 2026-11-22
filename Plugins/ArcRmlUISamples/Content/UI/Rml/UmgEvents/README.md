# UMG events

A Widget Blueprint whose event graph decides what the document's `data-action` buttons do: `open:` swaps the document
inside the same widget, `close` removes the widget from the screen. No C++ handles the two events.

- `WBP_Sample_UmgEvents` (parent `UserWidget`): a Canvas Panel with a `TextBlock` (40, 40; 480 x 40) and a
  `UArcRmlDocumentWidget` named `Document` (40, 100; 480 x 320, a variable) whose `Document` is `UmgEvents.rml`.
- Event graph: `Event Construct` assigns `On Open Requested` and `On Close Requested` of `Document` to the custom events
  `OnOpenRequested_Event (Path)` and `OnCloseRequested_Event`. The first makes an `ArcRmlDocumentRef` from `Path` and sets
  it as the widget's `Document` (the setter `SetDocument` loads the new document); the second calls `Remove from Parent`.
- `UmgEvents.rml` has a button `data-action="open:/ArcRmlUISamples/UI/Rml/UmgEvents/Second.rml"`; `Second.rml` has one
  back to the first document. Both have a `data-action="close"` button and no view model: `close` and `open:` need no `data-model`. A document widget whose `On Open Requested` or
  `On Close Requested` has no listener logs a warning and does nothing.
- The open function creates the widget with `CreateWidget` for the first local player's controller and adds it with
  `AddToPlayerScreen(1500)`.
- Input (keys, focus, navigation) of the nested document is not part of this sample: use the mouse.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open UmgEvents
```

`ArcRmlSamples.Close` removes it again.

## Files

- `UmgEvents.rml`, `Second.rml`, `UmgEvents.rcss`
- `WBP_Sample_UmgEvents` (Widget Blueprint and its event graph, made by `Scripts/arcrmlui/create_samples_blueprints.py`)
- `Source/ArcRmlUISamples/UmgEvents/ArcRmlSamplesUmgEvents.h`, `.cpp` (the open, close and is-open functions)
