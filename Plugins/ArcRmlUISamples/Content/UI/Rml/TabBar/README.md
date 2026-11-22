# Tab bar

Three tabs in a bar that `Q` / `E` (pad: left / right shoulder) switch, and each tab's content in its own focus area.
The tab names and the content are RML; the view model only stores the selected index.

- `data-hotkey-cycle="IA_Sample_PrevTab IA_Sample_NextTab"` on the bar: the two names are UI actions of the
  document's input mapping context (`<meta name="input-mapping" content=".../IMC_Sample_TabBar"/>`). The items are the
  focusable elements inside the bar (here the three `<button>`s); the step wraps from the last to the first.
- `data-attrif-selected="Selected == 0"`: the selected tab carries the `selected` attribute, styled with
  `.tb-tab[selected]`. The cycle starts from the focused tab when focus is in the bar, else from the `selected` one.
- `data-event-click="call('', 'Select', 0)"`: a cycle step clicks the next tab, so a mouse click, pad accept and
  `Q` / `E` all run the same `Select`. The content blocks switch with `data-if="Selected == 0"`.
- `data-focus-enter="items"` on a tab: its click moves focus into the area `items`, once `Select` has run and the
  area is shown. It is read only from a click (a mouse click, pad accept or a cycle step), never from a key handler
  that calls the view model.
- Focus in the bar: `E` focuses the next tab, then clicks it, so focus goes on into that tab's content area.
- Focus outside the bar (in a content area): `E` only clicks the tab after the `selected` one; it does not focus
  it first. The click's `data-focus-enter` then decides where focus goes.
- `data-focus-area="bar"` with `data-focus-primary` and `autofocus` on "Items": focus starts on the first tab. Up from
  a content area leaves into the bar.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open TabBar
```

`ArcRmlSamples.Close` closes it again.

## Files

- `TabBar.rml`, `TabBar.rcss`
- `IA_Sample_PrevTab`, `IA_Sample_NextTab`, `IMC_Sample_TabBar` (Q / E, pad shoulders)
- `Source/ArcRmlUISamples/TabBar/ArcRmlSamplesTabBar.h`, `.cpp` (the view model and the open function)

Guide page: Navigation.
