# Themes

One document with three colour sets. Three buttons ("Default", "Dark", "High contrast") switch the set; the panel,
the text and the button colours change at once. The labels are RML; the view model only stores the theme name.

- `:root` in `Themes.rcss` declares the custom properties (`--panel`, `--text`, `--accent`, `--button`); the rules
  read them with `var(--panel)`.
- `body.theme-dark` and `body.theme-contrast` redeclare the properties. Every element below the body inherits the new
  values, so no other rule changes.
- `data-class-theme-dark="Theme == 'dark'"` on `body`: the class is on while the expression is true. The view model's
  `Theme` string drives both classes; the default theme has no class.
- `call('', 'SetTheme', 'dark')`: the button passes the theme name; `SetTheme(const FString& Name)` stores it.
- `var(--muted, #9aa6ba)` on the sentence: only `body.theme-contrast` declares `--muted`; in the other themes the fallback
  after the comma is used. A `var()` without a fallback on an undeclared property
  makes the property invalid at computed-value time: it takes its inherited or initial value.
- `@property` registers a custom property with a type and an initial value; the samples do not need it.
  `@starting-style` is not supported: its rules are dropped.
- `autofocus` on "Default": focus starts on the first button.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Themes
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Themes.rml`, `Themes.rcss`
- `Source/ArcRmlUISamples/Themes/ArcRmlSamplesThemes.h`, `.cpp` (the view model and the open function)

Guide page: Styling.
