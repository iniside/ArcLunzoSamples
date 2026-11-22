# ArcRmlUI performance fixtures

Synthetic heavy pages for measuring ArcRmlUI. Start the game (Play in the editor, or
`UnrealEditor.exe ArcRmlUIHost.uproject -game -log`), open the console, enter `log LogArcRmlUI Verbose` so the `ArcRmlUI perf:` lines show, then open each page
with `ArcRml.Open <path>`. Close with `ArcRml.Close <path>`.

| Page | Path | Load |
|---|---|---|
| Slot grid | `/ArcRmlUITest/UI/Rml/Test/Perf/PerfSlotGrid.rml` | 3000 slots in a CSS grid, each an SVG icon and two texts (about 12000 elements). Hover moves the hover chain. |
| Deep nesting | `/ArcRmlUITest/UI/Rml/Test/Perf/PerfDeepNesting.rml` | 4 trees of 40 nested flex containers (row and column alternating); the leaf width animates, so every frame relays out all 40 levels. |
| Data-for list | `/ArcRmlUITest/UI/Rml/Test/Perf/PerfDataForList.rml` | The live mock list view model (`ArcRmlMockListViewModel` of `ArcRmlUITestRuntime`, context `MockList`) in a keyed and an unkeyed `data-for`. Run `ArcRml.MockList 1000` first, then `ArcRml.MockList.Edit update 1000` (or `add`, `remove`, `shuffle`) for each change. `ArcRml.MockList.Animate <N> [update|shuffle|churn]` edits N rows every frame (churn removes N rows and inserts N new ones); `ArcRml.MockList.Animate 0` stops it. |
| Animations | `/ArcRmlUITest/UI/Rml/Test/Perf/PerfAnimations.rml` | 200 running keyframe animations (transform, opacity, colour, background, border colour) on the left; 200 transitions on the right start when the pointer enters or leaves the right grid. |
| Decoration | `/ArcRmlUITest/UI/Rml/Test/Perf/PerfDecoration.rml` | 60 cards over a gradient: 4 box-shadows each (two inset), `backdrop-filter: blur`, radial and repeating gradients, nested `border-radius` with `overflow: hidden`, a `filter` per card (drop-shadow, blur, colour filters) or a `mask-image`; the cards and stripes animate. |
| Text | `/ArcRmlUITest/UI/Rml/Test/Perf/PerfText.rml` | 120 justified paragraphs in 3 families and 8 sizes with `font-variant-numeric`, `font-variant-ligatures`, `font-variant-caps`, `font-feature-settings`, `letter-spacing` and `text-wrap: pretty`; the right column's width animates, so its text re-breaks every frame. |
| Scroll | `/ArcRmlUITest/UI/Rml/Test/Perf/PerfScroll.rml` | 2000 rows with icons in one smooth-scrolling container (left 40, top 40, 700 px wide). Scroll with `ArcRml.SimulateKey MouseWheelAxis -1 @300,400`, repeated. |
| 3D transforms | `/ArcRmlUITest/UI/Rml/Test/Perf/Perf3DTransforms.rml` | 300 perspective scenes with `preserve-3d` cards, three rotating animations and one static 3D transform. |
| Tables | `/ArcRmlUITest/UI/Rml/Test/Perf/PerfTables.rml` | 40 tables of 20 rows by 6 cells, `border-collapse: collapse`, striped rows. |

Reading the data:

- `ArcRml.PerfLog 1` for timings (2 adds per-element lists and inflates times),
  `ArcRml.PerfLogInterval <s>` for the summary period.
- `ArcRml.PerfHitchMs <ms>` (0 off) logs `ArcRmlUI perf: hitch ...` lines for
  any UI-thread pump, view frame, render-thread submit or replay over the
  threshold, with the stage breakdown and counts of that frame.
- Insights: run with `-trace=cpu,frame` (or `Trace.Start cpu`); every scope
  is named `ArcRmlUI.*`.
