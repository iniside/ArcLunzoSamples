# Animations

A panel that fades in when it opens and fades out when it closes, and a "Hits" counter that flashes each time you press
"Hit". The texts are RML; the view model only counts.

- The host puts the class `enter` on `body` when the document is shown and `leave` when it closes. `body.enter` and
  `body.leave` in `Animations.rcss` start a `@keyframes` animation (`an-enter`, `an-leave`, opacity).
- The leave ends when the body's animation ends (here 0.3 s), at the latest after `UArcRmlUISettings::LeaveTimeoutSeconds`
  (default 0.5 s); then the document is removed. Keep a leave animation under that time or it is cut off. `forwards` holds
  the last frame until the document goes.
- `@starting-style` is not supported: its rules are dropped. Use an `@keyframes` animation on `body.enter` for an
  entrance.
- `data-pulse-flash="Hits"` on the counter: each change of `Hits` removes the class `flash` and adds it back in the same
  update, so `.an-hits.flash { animation: an-flash ... }` restarts every time. The class stays until the next change.
- The first value of the field never pulses; several changes before one UI update give one pulse; a view created in the
  update that changes the value does not pulse. Inside a `data-for` list use keyed rows (`data-key`), else a row reads
  whichever item sits at its index.
- `call('', 'Hit')` raises `Hits`; the text is `Hits: {{ Hits }}`.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Animations
```

`ArcRmlSamples.Close` closes it again, or press "Close".

## Files

- `Animations.rml`, `Animations.rcss`
- `Source/ArcRmlUISamples/Animations/ArcRmlSamplesAnimations.h`, `.cpp` (the view model and the open function)

Guide page: Styling.
