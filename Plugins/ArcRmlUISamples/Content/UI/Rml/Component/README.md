# Component

A component file `Card.rml` imported twice by `dc-import`: once with static props, once with a prop bound to the view model.

- `<component>` with `<prop name="title" default="Card"/>` and `<prop name="accentColor" .../>`: the body reads a prop as `$title`.
- A camelCase prop is written in kebab-case on the import: `accent-color="#ff9933"` sets `accentColor`.
- A prop value is static text (`title="Static title"`) or one whole-value expression (`title="{{ Title }}"`); text and `{{ }}` are not mixed.
- `name="/ArcRmlUISamples/UI/Rml/Component/Card"` is the absolute package path of the component file; a name that does not start with `/` is an error.
- `<slot/>` receives the children of the import; `<slot name="footer"/>` receives only the children that carry `slot="footer"`.
- Components are expanded when the document compiles. The component's styles are in the document's own style sheet, `Component.rcss`.
- The import inside the `create` model reads the model's fields in its props and children (`{{ Count }}`, `call('', 'Increment')`).

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Component
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Component.rml`, `Card.rml`, `Component.rcss`
- `Source/ArcRmlUISamples/Component/ArcRmlSamplesComponent.h`, `.cpp` (the view model and the open function)

Guide page: Components.
