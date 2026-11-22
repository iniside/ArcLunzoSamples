# Back and parent

Back in a child area returns to its parent area; Back in an area without a parent goes on to `<body>` and closes the
document when nothing stops it. The counters show the backs the document heard and the clicks on "Done" and "Use".

- `data-focus-parent="list"` on the `detail` area: Back in the detail returns focus to the list item that
  Right left from. Back in the `list` area, which has no parent, closes the document.
- `data-focus-exit="#bp-done"`: when Back leaves the detail, the "Done" button is clicked first, so `Exit` runs and
  "Done clicked" goes up. A direct click on "Done" runs `Exit` too, but does not leave the area: only Back moves focus
  back to the list.
- `data-event-back="call('', 'CountBack')"` on `<body>`: every Back that is not a return to the parent reaches it
  and is counted. It does not call `stopPropagation()`, so Back in the list still closes the document.
- The "Back" button lies outside every area. Its click calls `PressBack`, which takes the local player's
  `UArcRmlViewportSubsystem` through its `WorldContextObject` parameter and calls `PostBack` with the sample's
  document: the same Back as the key, so it closes the document.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open BackAndParent
```

`ArcRmlSamples.Close` closes it again.

## Files

- `BackAndParent.rml`, `BackAndParent.rcss`
- `Source/ArcRmlUISamples/BackAndParent/ArcRmlSamplesBackAndParent.h`, `.cpp` (the view model and the open function)

Guide page: Navigation.
