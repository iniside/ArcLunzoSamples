# Nested view models

A party: a leader and members. Each of them is a view model of its own that the game updates on its own, and the
party view model holds them. The document shows the leader and the member list, and calls a function on one of them.
"Leader +1" adds a point to the leader, "Healer +1" to the second member.

The sample's C++ names are generic: the party is `UArcRmlSamplesNestedParentViewModel`, the leader is its `Child` and
the members are its `Children` (`UArcRmlSamplesNestedChildViewModel`, fields `Name` and `Points`).

## Why nested view models and not a struct array

A struct array (see the StructList sample) holds plain data rows. A nested view model is an object: it announces its
own changes, so the game updates one member on its own, and the document can call a function on it (`AddPoint` here).

- `TObjectPtr<UArcRmlSamplesNestedChildViewModel> Child` and `TArray<TObjectPtr<...>> Children` are `FieldNotify`
  fields of the parent. A field that holds a view model object, or an array of them, can be read and called.
- `{{ Child.Name }}`, `{{ Child.Points }}`: the leader's fields, read through the parent's field name.
- `data-for="c : Children"`: one row per member in the array.
- `call('Child', 'AddPoint')`: the first argument is the path of the view model to call, from the model of the
  element's nearest `data-model`; `''` is that model itself. `call('Children[1]', 'AddPoint')` calls the second element of the array.
- The child announces its own change (`UE_MVVM_SET_PROPERTY_VALUE(Points, ...)`), and the document updates the
  value read through the parent.
- The children are subobjects of the parent: it creates them in its constructor with `CreateDefaultSubobject`, so the
  `create` source makes the parent and its children together. An object inspector shows them as transient objects
  inside the parent, not as objects of their own.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open NestedViewModel
```

`ArcRmlSamples.Close` closes it again.

## Files

- `NestedViewModel.rml`, `NestedViewModel.rcss`
- `Source/ArcRmlUISamples/NestedViewModel/ArcRmlSamplesNestedViewModel.h`, `.cpp` (the child and parent view models
  and the open function)

Guide page: Data binding.
