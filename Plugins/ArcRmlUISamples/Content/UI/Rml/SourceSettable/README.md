# View model source: settable

A `settable` declaration accepts a manual assignment with `SetViewModel` on top of its own source. Here the source is
`context`, so the first object comes from the GameInstance MVVM collection and game code can replace it.

- `context="VM_SampleSettable" settable="true"`: the model is the collection's instance until an object is assigned.
- "Replace the instance" calls `Replace`, which makes a new view model and calls
  `UArcRmlViewportSubsystem::SetViewModel(Document, TEXT("item"), Next)` while the document is open: the set replaces the object
  and updates every binding. `Generation` shows which instance is shown.
- A set-manually model is skipped by the retry and by collection changes. `SetViewModel` with `nullptr` removes the
  assignment and sends the model back to its own source, here the collection.
- `SetViewModel` returns false for a declaration that is neither `manual` nor `settable`, so a `context` model
  needs `settable` for this.
- `Replace(UObject* WorldContextObject)`: the parameter named `WorldContextObject` is filled with the local player
  controller, which gives the function the local player and its `UArcRmlViewportSubsystem`.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open SourceSettable
```

`ArcRmlSamples.Close` closes it again.

## Files

- `SourceSettable.rml`, `SourceSettable.rcss`
- `Source/ArcRmlUISamples/SourceSettable/ArcRmlSamplesSourceSettable.h`, `.cpp` (the view model and the open function)

Guide page: View models.
