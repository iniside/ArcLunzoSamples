# Form controls

A checkbox, a radio group, a range and a select that show model values and send each change back to the view model.
The readout under them shows what the view model holds.

- Checkbox: `data-checked="bSubtitles"` checks the box when the bool is true. `data-event-change="call('',
  'SetSubtitles', ev.checked)"` sends the new state back: `ev.checked` is the box's state after the toggle.
- Radio group: the three radios share `name="class"`, so checking one unchecks the others (inside the nearest
  `<form>`, else the whole document). Each has `data-checked="ClassName"`: with a string, a radio is checked when its
  `value` equals the string. `ev.value` of the picked radio is its `value`.
- Range: `data-value="Volume"` sets the slider; `data-event-input` sends every step while it moves. Left and Right
  (or the D-pad) change the value by `step`; Up and Down move focus to the next control.
- Select: `data-attr-value="Quality"` selects the option whose `value` matches; `data-event-change` sends the pick.
  Enter, Space or pad accept opens the list, Up and Down move in it, Enter picks, Escape or pad back closes it without a
  pick. Up and Down on a closed select move focus.
- `<label for="fc-subtitles">`: clicking the label toggles the checkbox.
- `data-checked` and `data-value` only show the model value: model variables are read-only, so a click changes the
  control and the `call()` in `data-event-input` / `data-event-change` writes the value back. A value written by a
  binding fires no event.
- Enter, Space and pad accept toggle a checkbox and pick a radio.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open FormControls
```

`ArcRmlSamples.Close` closes it again.

## Files

- `FormControls.rml`, `FormControls.rcss`
- `Source/ArcRmlUISamples/FormControls/ArcRmlSamplesFormControls.h`, `.cpp` (the view model and the open function)

Guide page: Forms.
