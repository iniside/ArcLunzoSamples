# Tailwind

A screen styled only with Tailwind CSS 4 utility classes: a panel with a heading, a paragraph, three buttons with
`hover:` and `focus:` variants, a list with `divide-y`, a three-column grid, rounded corners, a shadow and colours
from the default palette. The document links the shared samples sheet and the sheet Tailwind builds, nothing else.

- `tailwind.css` is the Tailwind input: `@import "tailwindcss" source(none);` turns off the automatic file scan,
  `@source "./Tailwind.rml";` names the document whose classes Tailwind keeps, and `@theme` sets `--font-sans` and
  `--font-mono` to `'Arc Samples Sans'`, a family the project loads (no system font ships with a game).
- `Tailwind.rcss` is the built sheet. `<link>` loads `.rcss` files, so the output is named `.rcss`.
- `<meta name="layer" content="UI.Layer.Menu"/>` and `<meta name="input-mode" content="menu"/>` let the document take
  the mouse and keys, so `hover:` and `focus:` show; `autofocus` on the first button focuses it on open.
- `ArcRmlSamplesTailwind::Open` opens the document as the first screen sample does: `UArcRmlViewportSubsystem` from
  the local player, then `OpenDocument`.

## Build the sheet

Download the Tailwind CSS 4 standalone CLI for your platform from the Tailwind CSS GitHub releases (no Node needed),
then run in this folder:

```
tailwindcss -i tailwind.css -o Tailwind.rcss
```

Add `--watch` to rebuild the sheet on every save of `Tailwind.rml`.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Tailwind
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Tailwind.rml`, `tailwind.css`, `Tailwind.rcss`
- `Source/ArcRmlUISamples/Tailwind/ArcRmlSamplesTailwind.h`, `.cpp` (the open function)

Guide page: Tailwind CSS.
