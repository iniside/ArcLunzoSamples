# Struct list

The same rows as Lists, held in an `FArcRmlStructList`. "+1" changes one row, "Add" and "Remove" change the set, "Reverse" changes the order.

- Why it exists: a plain array sends its whole value on every change; an `FArcRmlStructList` sends only the rows that changed (insert, remove, move, update), so a long list stays cheap.
- It is C++ only: `FArcRmlStructList` has no `UFUNCTION` surface, so a Blueprint view model cannot fill it.
- `SetRowType` names the row struct; the view model constructor calls it before any row is set.
- `Set(Key, Row)` adds or replaces the row with that key; `Remove(Key)` deletes it; `SetOrder(Keys)` sets the order.
- After a change the field is announced with `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Rows)`.
- `data-for="r, i : Rows"` with `data-key="r.Id"` reads the rows like an array. The key given to `Set` and the `Id` member are the same text here.
- `call('', 'Bump', i)` passes the row index; the function turns it into a key with `GetKeys()`.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open StructList
```

`ArcRmlSamples.Close` closes it again.

## Files

- `StructList.rml`, `StructList.rcss`
- `Source/ArcRmlUISamples/StructList/ArcRmlSamplesStructList.h`, `.cpp` (the row struct, the view model and the open function)

Guide page: Data binding.
