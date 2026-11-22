# Converter

A plain HTML page as a web developer has it, and what the HTML to RML converter makes of it. The page is a menu with a
heading, a paragraph with a line break, three buttons and a linked local style sheet whose `font-family` is
`'Arc Samples Sans'`, a family the project loads. It also holds what RML cannot take: a `<meta charset>`, two Google
Fonts `<link>` tags, a `<script>` and an `onclick`.

- `Html/Page.html` and `Html/page.css` are the input, kept in their own folder so the converter output sits beside them.
- `Page.rml` and `page.rcss` are the converter output: the `<link>` points at `page.rcss` (the sheet is renamed, `<link>`
  loads `.rcss` only), the removed tags are gone and `<br>` is the self-closed `<br/>` XML needs. The stylesheet text is
  copied unchanged.
- The page links no shared sheet, so it shows the plain converted look.

## Convert

Run the converter commandlet on the page, writing into this folder:

```
UnrealEditor-Cmd ArcRmlUIHost.uproject -run=ArcRmlConvert -Source=<path>/Html/Page.html -Dest=<path>/Converter -Name=Page
```

The log names every removal and ends with a summary (disk paths shortened to package paths):

```
/ArcRmlUISamples/UI/Rml/Converter/Html/Page.html(4): warning removed-meta: removed <meta charset="utf-8">: RML reads only the ArcRmlUI <meta> names layer, input-mode, hide-lower, keep-alive, frame, input-mapping, input-mapping-priority, any-input
/ArcRmlUISamples/UI/Rml/Converter/Html/Page.html(6): warning removed-link: removed <link rel="preconnect" href="https://fonts.googleapis.com">: RML loads only local style sheets; for a font, provide a .ttf/.otf under a content root and an @font-face
/ArcRmlUISamples/UI/Rml/Converter/Html/Page.html(7): warning removed-link: removed <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Roboto&display=swap">: RML loads no remote style sheets; for a font, provide a .ttf/.otf under a content root and an @font-face
/ArcRmlUISamples/UI/Rml/Converter/Html/Page.html(9): warning removed-script: removed <script>: RML runs no scripts
/ArcRmlUISamples/UI/Rml/Converter/Html/Page.html(15): warning removed-handler: removed 'onclick' on <button>: RML runs no inline handlers, and the attribute would make the element take pointer input
ArcRmlConvert: wrote '/ArcRmlUISamples/UI/Rml/Converter/Page.rml'
ArcRmlConvert: wrote '/ArcRmlUISamples/UI/Rml/Converter/page.rcss'
ArcRmlConvert: wrote 2 files, 0 errors, 5 warnings
```

The `<br>` void tag raises no line: it is rewritten without a message.

## Run it

Play any map and type in the console:

```
ArcRml.Open /ArcRmlUISamples/UI/Rml/Converter/Page.rml
```

The page shows the heading, the two-line paragraph and three blue buttons on a dark panel.

## Files

- `Html/Page.html`, `Html/page.css`
- `Page.rml`, `page.rcss`
