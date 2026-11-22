# Text fields

A text input and a textarea that show what is typed while typing and what was committed when the field was left. The
display text is RML; the view model only stores the strings and counts the commits.

- `data-attr-value="Live"` sets the field's `value` attribute from the model. `data-value` only shows a model value
  (model variables are read-only), so the text is written back by hand.
- `data-event-input="call('', 'SetLive', ev.value)"`: `input` fires for every edit (typing, delete, paste, cut), so
  "Live" follows the field.
- `data-event-change="call('', 'Commit', ev.value)"`: `change` fires when focus leaves the field, or on Enter in the
  input, when the text differs from the one at focus or at the last `change`. "Committed" and the commit count change
  then. Enter in the textarea inserts a line break instead.
- Pad back in a text field you are typing in ends typing: `change` fires, so the text is committed, and the field keeps
  focus without a caret. The D-pad then moves focus from it, and a second pad back is the screen's normal Back (here it
  closes the screen). Pad accept or Enter on the field types again. Keyboard Escape is a plain `keydown`
  (`ev.key == 'Escape'`): the text and the focus stay, as in Chrome.
- A value written through `data-attr-value` fires no event. On a focused field the write does not become the
  committed value, so `change` still fires when the field is left; echoing the field's own text writes the same
  value and changes nothing, so the caret stays.
- `placeholder="..."` is shown while the field is empty and styled with `::placeholder`.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open TextFields
```

`ArcRmlSamples.Close` closes it again.

## Files

- `TextFields.rml`, `TextFields.rcss`
- `Source/ArcRmlUISamples/TextFields/ArcRmlSamplesTextFields.h`, `.cpp` (the view model and the open function)

Guide page: Text fields.
