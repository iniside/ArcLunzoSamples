# UMG widget

A Widget Blueprint that shows an RML document: `UArcRmlDocumentWidget`, placed in a UMG tree beside a UMG `TextBlock`.

- `WBP_Sample_UmgWidget`: a Canvas Panel with a `TextBlock` (40, 40; 480 x 40) and a `UArcRmlDocumentWidget` named
  `Document` (40, 100; 480 x 320) whose `Document` is `UmgWidget.rml`. The document fills the widget's slot.
- `<viewmodel name="counter" ... manual="true"/>`: the document makes nothing; the open function calls
  `SetViewModel(TEXT("counter"), ViewModel)` on the widget. `manual` models are the ones a widget can be given.
- `data-event-click="call('', 'Increment')"`: calls the view model. `data-action="close"`: the widget opens and closes
  nothing itself; it fires `OnCloseRequested`, and the open function bound it to remove the widget from the screen.
- The open function creates the widget with `CreateWidget` for the first local player's controller and adds it with
  `AddToPlayerScreen(1500)`, above the layer host (1000) and below the Back button (2000).
- Input (keys, focus, navigation) of the nested document is not part of this sample: use the mouse.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open UmgWidget
```

`ArcRmlSamples.Close` removes it again.

## Files

- `UmgWidget.rml`, `UmgWidget.rcss`
- `WBP_Sample_UmgWidget` (Widget Blueprint, made by `Scripts/arcrmlui/create_samples_blueprints.py`)
- `Source/ArcRmlUISamples/UmgWidget/ArcRmlSamplesUmgWidget.h`, `.cpp` (the view model and the open, close and is-open functions)
