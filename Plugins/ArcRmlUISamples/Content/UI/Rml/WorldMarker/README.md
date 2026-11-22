# World marker

A label that stays over a cube in the level. "Move" moves the cube sideways and the label follows it; a second
"Move" moves it back. The label text is RML; the view model only stores the cube's position.

- `data-world="Anchor"` on `#wm-marker`: the element is placed over the world point in `Anchor`, a string of three
  numbers in centimetres (`"600.0 0.0 0.0"`). The document moves the element with `transform: translate` every time
  the point or the camera changes.
- `position: absolute; left: 0; top: 0` on `.wm-marker`: the translate starts from the element's own layout position,
  so that position must be the top-left corner of the screen.
- `transform: translate(-50%, -100%)` on the child `.wm-label`: centres the label over the point and puts it above.
  The anchored element's own `transform` is set by `data-world`, so an offset of your own goes on a child.
- The label is hidden before the first position arrives, while `Anchor` is empty and while the point is behind the
  camera.
- `data-model="marker"` on `body`: `data-world` needs a model, like every data attribute.
- The view model updates `Anchor` from the cube's location every frame with an `FTSTicker`, started when the document
  opens (the open function finds the cube by its tag `SampleMarkerTarget` and passes it in) and removed in
  `BeginDestroy`. `UE_MVVM_SET_PROPERTY_VALUE` sends the value only when it changed.
- `call('', 'Move')`: moves the cube 200 cm along Y, then back on the next press.
- The cube is a placed `StaticMeshActor` (`/Engine/BasicShapes/Cube`, mobility Movable, tag `SampleMarkerTarget`) in
  `L_ArcRmlSamples`, in front of the start camera.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open WorldMarker
```

`ArcRmlSamples.Close` closes it again.

## Files

- `WorldMarker.rml`, `WorldMarker.rcss`
- `Source/ArcRmlUISamples/WorldMarker/ArcRmlSamplesWorldMarker.h`, `.cpp` (the view model and the open function)

Guide page: World markers.
