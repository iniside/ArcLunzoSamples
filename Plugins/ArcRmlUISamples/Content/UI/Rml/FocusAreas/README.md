# Focus areas

Arrow keys and the D-pad move focus inside a focus area first, and leave it only at its edge. The line under the
popup shows the last `navnotarget` event: what the document reports when an arrow finds nothing in the area.

- `data-focus-area="left"`: the element is one area; an arrow inside it picks only targets in the same area.
- `data-focus-primary` on `middle` and `autofocus` on "Middle 1": focus starts there, so the first arrow
  already moves it. Without a focused element the first arrow moves nothing.
- Leaving: at the edge of an area an arrow moves focus to the nearest item of another area. The item needs
  `nav: auto` for that direction; buttons get it from the default style sheet.
- `data-focus-contain` on `popup`: `popup-a` and `popup-b` form one container. Left and Right move between
  them, but no arrow leaves the container, so Up from the popup reports `target area: none`.
- `data-event-navnotarget` on `<body>`: `ev.direction`, `ev.area` and `ev.target_area` (the area focus is
  about to leave into, `''` when there is none) are passed to `NoTarget`.
- "Right 2" handles `navnotarget` itself for Down only: `callIf` and `stopPropagation` with the condition
  `ev.direction == 'down'`. Down there would leave into the popup, but the move is cancelled and focus stays;
  Left is not stopped, so it reaches `<body>` and focus leaves into `middle`.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open FocusAreas
```

`ArcRmlSamples.Close` closes it again.

## Files

- `FocusAreas.rml`, `FocusAreas.rcss`
- `Source/ArcRmlUISamples/FocusAreas/ArcRmlSamplesFocusAreas.h`, `.cpp` (the view model and the open function)

Guide page: Navigation.
