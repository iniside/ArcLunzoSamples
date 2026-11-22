# Events

Events calling view model functions. The line "Last event" shows what the model received.

- `call('', 'Pick', 'Red', 1)`: a call with arguments; the count must match the function's parameters.
- `callIf(ev.button == 0, ...)` on `mouseup`: the call runs only for the left button (`1` is the right one).
- `callIf(ev.key == 'ArrowUp', ...)` on `keydown`: the call runs only for that key; the box takes focus when the
  document opens (`autofocus`), or click it.
- `stopPropagation()`: "Inner, stops" counts only the inner click; "Inner, bubbles" counts the inner and the outer
  one. `stopPropagation(condition)` stops only when the condition is true.
- `data-event-back` on `<body>`: the first Back calls `ArmBack` and stops the event, so the document stays
  open; the second Back is not stopped and closes it. Back is the back action of the project's CommonUI input data.
- `ShowCaller(UObject* WorldContextObject)`: a parameter named `WorldContextObject` is filled with the local
  player controller, so `call('', 'ShowCaller')` passes no argument for it.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Events
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Events.rml`, `Events.rcss`
- `Source/ArcRmlUISamples/Events/ArcRmlSamplesEvents.h`, `.cpp` (the view model and the open function)

Guide page: Events.
