# Row hotkeys

Three rows. A key (or pad button) marks the row you are on as the favourite, whichever of its controls has focus.

- `data-hotkey-focus="IA_Sample_RowAction"` on a row: when the action fires, the core looks at the focused element and its
  ancestors only, and clicks the first one with this attribute. The row's own `data-event-click` runs, so the row
  handles the key like a click on it.
- `data-hotkey` would search the whole screen and click the first match; `data-hotkey-focus` follows the focus, so the same
  action works on every row.
- The "Use" button inside a row ends its click with `stopPropagation()`, so using it does not also mark the favourite.
- The document's `<meta name="input-mapping">` names `IMC_Sample_RowHotkeys`, which maps `IA_Sample_RowAction` to R and
  to a press of the right thumbstick (pad face left is taken by the DragDrop sample). The context is active while the
  document is open.
- `{{ ArcKeys.IA_Sample_RowAction }}` shows the key or button of the player's last used device.
- Rows mark their state with `data-attrif-favourite="Favourite == 0"`, styled with `.rh-row[favourite]`.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open RowHotkeys
```

Press Down to move between rows, then R. `ArcRmlSamples.Close` closes it again.

## Files

- `RowHotkeys.rml`, `RowHotkeys.rcss`, `IA_Sample_RowAction`, `IMC_Sample_RowHotkeys` (R, pad right thumbstick press)
- `Source/ArcRmlUISamples/RowHotkeys/ArcRmlSamplesRowHotkeys.h`, `.cpp` (the view model and the open function)

Guide page: Input and gamepad.
