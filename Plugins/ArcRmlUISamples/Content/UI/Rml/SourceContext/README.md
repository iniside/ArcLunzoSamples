# View model source: context

A `context` source takes the view model from the GameInstance MVVM collection. The game registers the instance; the
document finds it.

- `context="VM_SampleContext"`: `FindViewModelInstance({class, context})` in the GameInstance MVVM collection, with
  the declaration's `class` and this context name.
- The open function gets `UMVVMGameSubsystem` from the game instance, calls
  `GetViewModelCollection()->AddViewModelInstance` with `FMVVMViewModelContext` (`ContextClass`, `ContextName`)
  before `OpenDocument`, so the first snapshot shows the instance. It removes an earlier instance of the same context first.
- Blueprint: `BP_SourceContext_Open` is a Blueprint Function Library; its function `Open` (input
  `PlayerController`, output `Document`) does the same with nodes: Get MVVM Game Subsystem, Get View Model
  Collection, Make MVVMViewModelContext (`ContextClass` the view model class, `ContextName` `VM_SampleContext`),
  Remove View Model, Construct Object from Class with the subsystem as outer, Set `Label`, Add View Model Instance,
  then `OpenDocument`.
- `data-if="ArcResolved"`: `ArcResolved` is true once the view model exists. A `context` model that is not in the
  collection yet is found by the retry (every second and on resume) and when the collection changes.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open SourceContext
```

`ArcRmlSamples.Close` closes it again. `ArcRmlSamples.OpenBP SourceContext` opens it through the Blueprint `Open`
function instead.

## Files

- `SourceContext.rml`, `SourceContext.rcss`
- `Source/ArcRmlUISamples/SourceContext/ArcRmlSamplesSourceContext.h`, `.cpp` (the view model and the open function)
- `BP_SourceContext_Open` (Blueprint Function Library: function `Open`)

Guide page: View models.
