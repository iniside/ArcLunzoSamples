# Inner RML

The same kinds of text shown two ways: `{{ }}` shows it as text, `data-rml` parses it as markup.

- `data-rml="Banner"` replaces the element's children with the model string parsed as RML, again each time the string
  changes. `Banner` is a string the game wrote, `Welcome back, <span class="ir-gold">traveller</span>`, and the
  `<span>` becomes an element that `InnerRml.rcss` styles.
- The text field holds `Typed`, which starts as `<b>x</b>`. `{{ Typed }}` shows the characters `<b>x</b>` as they are.
  `data-rml="Typed"` turns them into a `<b>` element holding `x`, which `.ir-line b` in the stylesheet colours blue.
  Type into the field and both lines follow every key.
- Nothing is escaped in `data-rml`. Text a player typed (a name, a chat line, a note) can hold markup, so it would
  become elements: a stray `<` is read as markup and a player could insert their own `<img>` or styled `<span>`.
  Show player text with `{{ }}` only, and use `data-rml` for strings your own game or data wrote.
- A localised text (`loc`) in `data-rml` is a compile error.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open InnerRml
```

`ArcRmlSamples.Close` closes it again.

## Files

- `InnerRml.rml`, `InnerRml.rcss`
- `Source/ArcRmlUISamples/InnerRml/ArcRmlSamplesInnerRml.h`, `.cpp` (the view model and the open function)

Guide page: Data binding.
