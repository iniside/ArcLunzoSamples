# UMG owner data

A document in a Widget Blueprint whose view model belongs to the widget that holds it: the document declares `path="Stats"`
and finds the model through its owning `UUserWidget`, so the open function sets nothing on the document.

- `UArcRmlSamplesUmgOwnerDataWidget` (C++, the Widget Blueprint's parent class) has `UPROPERTY(BlueprintReadOnly) Stats`, a
  `UArcRmlSamplesUmgOwnerDataViewModel` (Field Notify `Health`, function `Heal`) made in `NativeOnInitialized`. The same
  function binds the document's `OnCloseRequested` to remove the widget from the screen.
- `WBP_Sample_UmgOwnerData`: a Canvas Panel with a `TextBlock` (40, 40; 480 x 40) and a `UArcRmlDocumentWidget` named
  `Document` (40, 100; 480 x 320) whose `Document` is `UmgOwnerData.rml`.
- `<viewmodel name="stats" class="..." path="Stats"/>`: `path` is evaluated against the owning widget, so `Stats` is read
  from the widget. `data-event-click="call('', 'Heal')"` calls the model; `data-action="close"` fires `OnCloseRequested`.
- In the Widget Blueprint Designer `NativeOnInitialized` does not run, `Stats` stays empty and the document loads with the
  model unresolved.
- The open function creates the widget with `CreateWidget` for the first local player's controller and adds it with
  `AddToPlayerScreen(1500)`.
- Input (keys, focus, navigation) of the nested document is not part of this sample: use the mouse.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open UmgOwnerData
```

`ArcRmlSamples.Close` removes it again.

## Files

- `UmgOwnerData.rml`, `UmgOwnerData.rcss`
- `WBP_Sample_UmgOwnerData` (Widget Blueprint)
- `Source/ArcRmlUISamples/UmgOwnerData/ArcRmlSamplesUmgOwnerData.h`, `.cpp` (the widget class, the view model and the open, close and is-open functions)
