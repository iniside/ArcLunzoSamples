# Aliases

A card that shows the selected hero through a short name, and a button that selects the next one.

- `data-alias-sel="Selected"` on `.al-card` makes `sel` stand for `Selected` on that element and everything inside
  it. `{{ sel.Name }}` reads `Selected.Name`, `{{ sel.Title }}` reads `Selected.Title`.
- `Selected` is a struct property of the view model. The line under the card writes the full path,
  `{{ Selected.Name }}`, and always shows the same text as the card.
- "Next hero" calls `SelectNext`, which copies the next hero into `Selected` and broadcasts the field change. The card
  and the line under it update together: reads through an alias follow the data as any other read does.
- The alias is resolved when the card is created. A model variable with the same name as the alias wins over it and the
  alias is reported as shadowed, so pick an alias name no variable has.
- In a component the alias path must come from a prop that holds a whole `{{ path }}` value.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Aliases
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Aliases.rml`, `Aliases.rcss`
- `Source/ArcRmlUISamples/Aliases/ArcRmlSamplesAliases.h`, `.cpp` (the view model and the open function)

Guide page: Data binding.
