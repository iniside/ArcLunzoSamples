# Sounds

UI sounds declared in a style sheet: a click sound declared on a parent, a hover sound, a focus sound, and the back
and close sounds of the document. Each button adds one to the click counter; "Close" closes the document.

- `.sd-group { --sound-click: "asset:/ArcRmlUISamples/UI/Rml/Sounds/MS_Sample_Click.MS_Sample_Click"; }`: a click
  plays the sound of the nearest element that declares it, so every button inside the group plays it.
- `#sd-hover { --sound-hover: ...; }`: plays once when the pointer enters that button, nothing while it moves inside it.
- `#sd-focus { --sound-focus: ...; }`: plays when a key or pad move focuses the button, including a move into an
  area.
- `body { --sound-back: ...; --sound-close: ...; }`: Escape or pad back plays the back sound; the Close button
  (`data-action="close"`) plays the close sound. A back that closes the document plays only the back sound.
- The value is always a quoted `"asset:/Package/Path.Object"` naming a MetaSound Source, written in a style sheet.
  The document compile reports a missing asset, another asset class, an unquoted value or a `var()`.
- `MS_Sample_Click` and `MS_Sample_Back` are short sine tones: a MetaSound Source whose graph starts an AD envelope
  on play, multiplies a sine oscillator by it into the output and finishes when the envelope is done.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Sounds
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Sounds.rml`, `Sounds.rcss`
- `MS_Sample_Click`, `MS_Sample_Back` (MetaSound Sources)
- `Source/ArcRmlUISamples/Sounds/ArcRmlSamplesSounds.h`, `.cpp` (the view model and the open function)

Guide page: Sounds.
