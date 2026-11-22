# View model source: manual

A `manual` declaration creates nothing: the model stays unresolved until game code assigns an object with
`UArcRmlViewportSubsystem::SetViewModel`. `manual` implies `optional` and `settable`.

- `manual="true"`: no source creates the model.
- `SetViewModel(Document, TEXT("item"), First)` before `OpenDocument`: the assignment is kept and is in the first snapshot.
  It reads the head of the document, so the document need not be open. It returns false for a name with no live
  declaration, a declaration that is neither `manual` nor `settable`, or an object that is not the declared class.
- "Swap the instance" calls `Swap`, which makes a new view model and calls `SetViewModel` again while the
  document is open: the set after the open replaces the object and updates every binding. `Generation` shows which instance is shown.
- `Swap(UObject* WorldContextObject)`: the parameter named `WorldContextObject` is filled with the local player
  controller, which gives the function the local player and its `UArcRmlViewportSubsystem`.
- `data-if="ArcResolved"`: the document shows "Waiting for the view model" until the first assignment.
- Blueprint: `BP_SourceManual_Open` is a Blueprint Function Library; its function `Open` (input
  `PlayerController`, output `Document`) constructs the view model with Construct Object from Class, sets
  `Label` to "Instance 1", calls `SetViewModel` with the model name `item` and, when that returns true,
  `OpenDocument`.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open SourceManual
```

`ArcRmlSamples.Close` closes it again. `ArcRmlSamples.OpenBP SourceManual` opens it through the Blueprint `Open`
function instead.

## Files

- `SourceManual.rml`, `SourceManual.rcss`
- `Source/ArcRmlUISamples/SourceManual/ArcRmlSamplesSourceManual.h`, `.cpp` (the view model and the open function)
- `BP_SourceManual_Open` (Blueprint Function Library: function `Open`)

Guide page: View models.
