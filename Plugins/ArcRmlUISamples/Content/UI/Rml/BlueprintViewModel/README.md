# Blueprint view model

A view model made in Blueprint, with no C++. `BP_SampleVM` has three fields and one function; the document makes
its own instance when it opens. "Add step" calls `Increment`, which adds `Step` to `Count`; "Close" closes
the document.

- `<viewmodel name="counter" class="/ArcRmlUISamples/UI/Rml/BlueprintViewModel/BP_SampleVM.BP_SampleVM_C" create="true"/>`:
  a Blueprint view model is named by its generated class, the asset path followed by `_C`. `create` makes a new
  instance when the document opens.
- `BP_SampleVM` is a Viewmodel Blueprint (parent class `MVVMViewModelBase`) with three Field Notify variables:
  `Title` (String, "Blueprint view model"), `Count` (Integer, 0) and `Step` (Integer, 2). Only Field Notify
  fields can be read from the document: `{{ Title }}`, `{{ Count }}`, `{{ Step }}`.
- `Increment` is a Blueprint function of `BP_SampleVM`: Set `Count` to `Count + Step`. The Set node of a Field
  Notify variable (Set with Broadcast) tells the document that the field changed.
- `data-event-click="call('', 'Increment')"`: calls `Increment` on the model of the element's `data-model`. A
  function can be called when it is not pure and not const; a custom event cannot be called.
- `data-action="close"`: closes the document.
- `BP_BlueprintViewModel_Open` is a Blueprint Function Library. Its function `Open` (input `PlayerController`,
  output `Document`) gets the Arc Rml Viewport Subsystem from the player controller, makes an `ArcRmlDocumentRef`
  with the document's path and calls `OpenDocument`; it returns the document when it opened. The document needs
  nothing else, because it creates its own model.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.OpenBP BlueprintViewModel
```

`ArcRmlSamples.OpenBP` calls the `Open` function of the sample's Blueprint Function Library.
`ArcRmlSamples.Close` closes the document again.

## Files

- `BlueprintViewModel.rml`, `BlueprintViewModel.rcss`
- `BP_SampleVM` (the view model: variables `Title`, `Count`, `Step`; function `Increment`)
- `BP_BlueprintViewModel_Open` (Blueprint Function Library: function `Open`)

Guide page: Blueprint view models.
