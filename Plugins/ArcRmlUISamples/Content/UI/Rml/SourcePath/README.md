# View model source: path

A `path` source reads the view model from an object property chain, starting at the widget the binding creates
for the player controller.

- `path="GetOwningPlayer.SampleViewModel"`: evaluated against the binding's `UArcRmlResolverContextWidget` (the
  `UUserWidget` the resolver uses, created for the player controller), so the roots are `UUserWidget` getters
  such as `GetOwningPlayer`, `GetOwningPlayerPawn` and `GetOwningLocalPlayer`.
- `SampleViewModel` is a `BlueprintReadOnly` property of `AArcRmlSamplesPlayerController`, the project's
  player controller. Each segment is looked up on the live object's class, so a member of the game's controller
  subclass resolves without a casting getter. A segment is a Blueprint-visible object property or a const zero-argument
  `BlueprintCallable` function returning an object.
- The controller creates the view model in its constructor with `CreateDefaultSubobject`.
- A path needs a player; without one the model stays unresolved. A null on the way leaves the model unresolved and
  the retry evaluates the path again.
- The validator walks the path from `UUserWidget`, whose `GetOwningPlayer` returns `APlayerController`, which has
  no `SampleViewModel`: it reports a `view-model-path` warning on this sample's last segment.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open SourcePath
```

`ArcRmlSamples.Close` closes it again.

## Files

- `SourcePath.rml`, `SourcePath.rcss`
- `Source/ArcRmlUISamples/SourcePath/ArcRmlSamplesSourcePath.h`, `.cpp` (the view model and the open function)
- `Source/ArcRmlUISamples/ArcRmlSamplesGameMode.h`, `.cpp` (the player controller: its `SampleViewModel` property and
  where it is created)

Guide page: View models.
