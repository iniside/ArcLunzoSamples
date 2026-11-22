# Focus request

Three "Go to" buttons that move focus to a text field the view model names, not the player.

- `data-focus-request="NameFocusSerial"` on a focusable element: the bound value is a counter. The first value never
  focuses; every later change to a value that is not empty focuses the element.
- Each button calls a view model function that adds 1 to its counter (`data-event-click="call('', 'FocusName')"`), so
  pressing it again focuses the field again, even when the player moved away in between.
- The focus moves like a navigation move: the element gets `:focus-visible` (here a blue background), the element scrolls into view and its focus area
  becomes the active one. A request on an element that is hidden, disabled or not focusable does nothing.
- It is the web's `element.focus()` for a focus the game starts, for example a "first invalid field" after a failed
  submit. It ignores area confinement.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open FocusRequest
```

`ArcRmlSamples.Close` closes it again.

## Files

- `FocusRequest.rml`, `FocusRequest.rcss`
- `Source/ArcRmlUISamples/FocusRequest/ArcRmlSamplesFocusRequest.h`, `.cpp` (the view model and the open function)

Guide page: Navigation.
