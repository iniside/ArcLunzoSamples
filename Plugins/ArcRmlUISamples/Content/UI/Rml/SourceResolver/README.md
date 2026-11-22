# View model source: resolver

A resolver creates the document's view model. The resolver is a C++ class; its properties are set from attributes of
the `<viewmodel>` declaration.

- `UArcRmlSamplesGreetingResolver` derives from `UMVVMViewModelContextResolver` and overrides `CreateInstance`, which
  returns a new `UArcRmlSamplesSourceResolverViewModel`.
- `resolver="/Script/ArcRmlUISamples.ArcRmlSamplesGreetingResolver"` names the resolver class.
- `Greeting="Hello from the RML attribute"`: every other attribute of the declaration is set on the resolver property
  of the same name before `CreateInstance` runs. A name the resolver does not have is a warning. The names `name`,
  `class`, `context`, `resolver`, `create`, `manual`, `path`, `settable`, `optional`, `preview` and `on-close`
  belong to the declaration, so a resolver property with one of these names cannot be set.
- `CreateInstance` receives a widget owned by the local player, so a resolver can read the player
  (`GetOwningPlayer`); the document shows its name.
- `data-if="ArcResolved"`: `ArcResolved` is true once the view model exists.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open SourceResolver
```

`ArcRmlSamples.Close` closes it again.

## Files

- `SourceResolver.rml`, `SourceResolver.rcss`
- `Source/ArcRmlUISamples/SourceResolver/ArcRmlSamplesSourceResolver.h`, `.cpp` (the view model, the resolver and
  the open function)

Guide page: View models.
