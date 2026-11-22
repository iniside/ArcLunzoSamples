# Frame and tabs

A frame document with a row of two tab buttons and the documents `TabA` and `TabB` shown inside it, one at a time.
Each tab has a count you raise with "Add one"; switch away and back and the count is still there. The labels are RML;
the view models only store the shown index and the counts.

- `<meta name="frame" content="/ArcRmlUISamples/UI/Rml/FrameTabs/Frame.rml"/>` in a tab's `<head>`: the document is a
  tab of that frame. Opening the tab opens the frame (when it is not open yet) and shows the tab in the frame's one
  `arc-embed` element (`<div class="ft-embed" arc-embed="">`). A tab's head takes no `layer` or `input-mode`; the
  frame's decide.
- Switching is an ordinary `OpenDocument` of the other tab. A tab button calls
  `call('', 'ShowTab', 1)`; `ShowTab(UObject* WorldContextObject, int32 Index)` gets the local player controller in
  `WorldContextObject` (that parameter takes no argument in RML), reaches `UArcRmlViewportSubsystem` through its
  local player and opens `TabA.rml` or `TabB.rml`. The shown tab is hidden and the other one is shown in its place.
- Visited tabs are kept: a tab you leave stays in the frame, hidden, with its view model, field text and `data-if`
  state, until the frame closes. Showing it again does not create it again, so `Count` keeps its value. Closing the
  frame drops every tab; the next open starts at 0.
- Tabs are isolated, like a page in an `<iframe>`: no frame rule matches inside a tab, and inherited and custom
  properties start again from their initial values at the tab. That is why each tab links `Samples.rcss` and its own
  `Tab.rcss`; a `:root` or `html` rule in a tab sheet refers to the tab.
- The frame's model (`frame`) and the tabs' models (`taba`, `tabb`) have different names, as they must. A `call()` goes to the
  tab model with that name, else to the frame, and a tab may not declare a name a kept tab declares differently. The two tabs use one view model class under two names, so each has its own count.
- `ArcRmlSamples.Open FrameTabs` opens `TabA.rml`; the open function returns the frame, since the frame stays open
  while the tabs switch.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open FrameTabs
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Frame.rml`, `Frame.rcss` (the frame), `TabA.rml`, `TabB.rml`, `Tab.rcss` (the tabs)
- `Source/ArcRmlUISamples/FrameTabs/ArcRmlSamplesFrameTabs.h`, `.cpp` (the frame and tab view models and the open function)

Guide page: Frames and tabs.
