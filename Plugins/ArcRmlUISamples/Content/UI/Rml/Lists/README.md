# Lists

A `TArray` of a struct as a Field Notify field, shown as rows. "Add" appends a row, "Remove" on a row deletes it.

- `TArray<FArcRmlSamplesListsRow> Items` is `FieldNotify`. A change to the array is announced with `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Items)`; the whole array is sent again.
- `data-for="r, i : Items"`: one element per row; `r` is the row, `i` its index.
- `data-key="r.Name"` on the same element: a row keeps its element across inserts and removals, matched by that value.
- `call('', 'Remove', i)`: the index travels as an argument of the call; the function checks it.
- Struct members read in the document (`r.Name`, `r.Quantity`) are `UPROPERTY` members of the row struct.
- For long lists that change often, see StructList.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Lists
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Lists.rml`, `Lists.rcss`
- `Source/ArcRmlUISamples/Lists/ArcRmlSamplesLists.h`, `.cpp` (the row struct, the view model and the open function)

Guide page: Data binding.
