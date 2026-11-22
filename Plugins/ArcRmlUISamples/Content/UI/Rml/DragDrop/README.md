# Drag and drop

Three bag items you can drag with the mouse or carry with the keyboard or a pad: a hand slot that takes the two
weapons (Sword, Shield) and refuses the Potion, a ring slot that refuses every item, and a stash list whose rows take
weapons and potions. The item names are RML; the view
model only stores which name landed in which slot.

- `drag: clone` (`DragDrop.rcss`) with `data-drag-type="weapon"` on a bag item: the item is a drag source and a copy
  follows the pointer. Style the copy with `:drag`. Without `data-drag-type` the drag does not start.
- `data-drop-accept="weapon"` on a target: it accepts a source that shares a token with it. The ring slot
  (`data-drop-accept="ring"`) accepts no bag item, so a drop there does nothing; the stash rows list two tokens
  (`"weapon potion"`).
- Classes the core sets: `drag-source` on the dragged item, `drop-over` on an accepting target under the pointer (or
  focus), `drop-invalid` on a target that does not accept it.
- `data-event-dragdrop="call('', 'Dropped', 'drag:data-item', 'target:data-slot')"`: `'drag:data-item'` passes the
  dragged item's `data-item` value, `'target:data-slot'` the target's `data-slot` value. `Dropped` stores the name in
  that slot and the slot shows it with `{{ Hand }}`.
- `data-drag-action="IA_Sample_Carry"` on `body`: with an item focused, `F` (pad: left face button) picks it up. While
  carrying, arrows move focus and the highlight follows it; `Enter` (pad: accept) on an accepting target drops,
  on any other element it does nothing; `Escape` (pad: back) cancels and returns focus to the item. The action name is
  a UI action of the document's input mapping context
  (`<meta name="input-mapping" content=".../IMC_Sample_DragDrop"/>`).
- `autofocus` on "Sword": focus starts on the first item, so the first arrow or carry works at once.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open DragDrop
```

`ArcRmlSamples.Close` closes it again.

## Files

- `DragDrop.rml`, `DragDrop.rcss`
- `IA_Sample_Carry`, `IMC_Sample_DragDrop` (F, pad left face button)
- `Source/ArcRmlUISamples/DragDrop/ArcRmlSamplesDragDrop.h`, `.cpp` (the view model and the open function)

Guide page: Drag and drop.
