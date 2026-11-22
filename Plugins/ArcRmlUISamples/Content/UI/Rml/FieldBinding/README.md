# Field binding

A C++ view model with one field of each simple type (bool, int, float, text, colour), shown in the document.
"Add one" calls `Increment`, which changes every field; "Close" closes the document.

- `<viewmodel name="fields" class="/Script/ArcRmlUISamples.ArcRmlSamplesFieldBindingViewModel" create="true"/>`:
  the document makes its own view model when it opens.
- `data-model="fields"` on `<body>`: expressions inside read that model.
- `{{ Count }}`: text; `{{ Progress | format(1) }}` and `{{ Message | to_upper }}`: two transforms.
- `data-if`: shows an element only while the expression is true.
- `data-class-fb-odd`: adds the class `fb-odd` while the expression is true.
- `data-style-width`, `data-style-background-color`: set one style property from the model.
- `data-attr-value`: sets the `value` attribute of the `<progress>` element.
- `data-event-click="call('', 'Increment')"`: calls a `BlueprintCallable` function of the model.
- `data-action="close"`: closes the document.
- Every field is `FieldNotify` and every write uses `UE_MVVM_SET_PROPERTY_VALUE`, which tells the document that
  the field changed. A plain assignment would not show.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open FieldBinding
```

`ArcRmlSamples.Close` closes it again.

## Files

- `FieldBinding.rml`, `FieldBinding.rcss`
- `Source/ArcRmlUISamples/FieldBinding/ArcRmlSamplesFieldBinding.h`, `.cpp` (the view model and the open function)

Guide page: Data binding.
