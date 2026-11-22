# ArcRmlUIPackagingTest

Content-only plugin (mount `/ArcRmlUIPackagingTest/`) that checks what an ArcRmlUI cook ships with Package Everything off
(`bPackageEverything=False` in `ArcRmlUIHost/Config/DefaultGame.ini`). None of its paths is in Paths To Package: every
file below reaches the cook only through the reference scan and the frame / `open:` edges. It is an ArcRmlUI content root
with no setup, because it is a project plugin with `CanContainContent` mounted at `/ArcRmlUIPackagingTest/`
(`ArcRmlPaths::MakeContentRoot`, `Interfaces/ArcRmlPaths.cpp` in `ArcRmlUI`). `DT_Icons` uses the demo's row struct
`FArcRmlDemoTabRow`, so the plugin depends on `ArcRmlUIDemo`.

Generated assets (tracked): `Content/Data/BP_SeedList` (Blueprint class of `UPrimaryDataAsset` with a `Documents` array of
`FArcRmlDocumentRef`), `Content/Data/DA_Seed`, `Content/Data/DT_Icons` and `Content/Data/BP_PinSeed` (Actor Blueprint:
`Event BeginPlay` → `Open Document`, Target from `Get ArcRmlViewportSubsystem` fed by `Get Player Controller`, the
`Document` pin split with `Document_Path` = `/ArcRmlUIPackagingTest/UI/Rml/Pinned.rml`), made by an editor Python
script that is not part of the host.

## Cases

| # | Source | Reaches | Ships |
| --- | --- | --- | --- |
| 1 | `DA_Seed` `Documents[0]` | `Seeded.rml` → `Seeded.rcss` → `img/seeded.svg` (decorator) | yes |
| 2 | `Seeded.rml` `data-action="open:/ArcRmlUIPackagingTest/UI/Rml/Opened.rml"` | `Opened.rml` | yes |
| 3 | `DA_Seed` `Documents[1]` | `Tab.rml`, whose `<meta name="frame">` names `Frame.rml` | yes |
| 4 | `BP_PinSeed` Event Graph, Open Document with the split `Document_Path` pin | `Pinned.rml` | yes |
| 5 | `DT_Icons` row `Table` `Icon` (`FArcRmlImageRef`) | `img/table.svg` | yes |
| 6 | `DA_Seed` `Documents[2]` | `Dynamic.rml`, with a `data-attr-data-action` (cook Warning; its runtime target is not followed) | yes |
| 7 | nothing | `Unreferenced.rml`, `img/unreferenced.svg` | no |
| 8 | scanned assets nothing references | `BP_SeedList`, `DA_Seed`, `DT_Icons`, `BP_PinSeed` as cooked packages | no |

## Before packaging (owner, once)

1. Nothing to generate: the `Content/Data/` assets are tracked, so a clone has them.
2. Package `ArcRmlUIHost` for Windows as usual (Package Everything stays off).

## Expected `Saved/Cooked/Windows/ArcRmlUIHost/Metadata/ArcRmlUI/PackagedFiles.txt`

Present (one line each, order not checked):

```text
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Seeded.rmlc
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Seeded.rcssc
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/img/seeded.svg
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Opened.rmlc
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Tab.rmlc
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Frame.rmlc
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/img/table.svg
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Dynamic.rmlc
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Pinned.rmlc
```

Absent (case 7, and files the cook never packages):

```text
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Unreferenced.rmlc
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/img/unreferenced.svg
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Preview/Seeded.json
ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/Content/UI/Rml/Preview/Dynamic.json
```

No other line starting with `ArcRmlUIHost/Plugins/ArcRmlUIPackagingTest/`.

## Expected `Saved/Cooked/Windows/ArcRmlUIHost/Metadata/ReferencedSet.txt` (case 8)

The cook's list of cooked packages (lower case) has no line starting with `/arcrmluipackagingtest/`: the reference scan
loads `BP_SeedList`, `DA_Seed`, `DT_Icons` and `BP_PinSeed` at cook start, and nothing references them, so none of
them is cooked.

## Expected cook log lines

```text
LogArcRmlUI: Display: ArcRmlUI cook: reference scan: <n> assets under <n> package paths, <n> scanned, <n> references in <s> s
LogArcRmlUI: Display: ArcRmlUI cook: seed '/ArcRmlUIPackagingTest/UI/Rml/Seeded.rml' from '/ArcRmlUIPackagingTest/Data/DA_Seed' property Documents[0]
LogArcRmlUI: Display: ArcRmlUI cook: seed '/ArcRmlUIPackagingTest/UI/Rml/Tab.rml' from '/ArcRmlUIPackagingTest/Data/DA_Seed' property Documents[1]
LogArcRmlUI: Display: ArcRmlUI cook: seed '/ArcRmlUIPackagingTest/UI/Rml/Dynamic.rml' from '/ArcRmlUIPackagingTest/Data/DA_Seed' property Documents[2]
LogArcRmlUI: Display: ArcRmlUI cook: seed '/ArcRmlUIPackagingTest/UI/Rml/img/table.svg' from '/ArcRmlUIPackagingTest/Data/DT_Icons' property Table.Icon
LogArcRmlUI: Display: ArcRmlUI cook: seed '/ArcRmlUIPackagingTest/UI/Rml/Pinned.rml' from '/ArcRmlUIPackagingTest/Data/BP_PinSeed' pin EventGraph: OpenDocument: Document_Path
LogArcRmlUI: Warning: ArcRmlUI cook: '/ArcRmlUIPackagingTest/UI/Rml/Dynamic.rml' builds a data-action from data (data-attr-data-action, data-attrif-data-action or {{ }}); the documents it opens that way are not followed, list them in Project Settings > ArcRmlUI > Paths To Package
LogArcRmlUI: Display: ArcRmlUI cook: Windows: <n> roots, <n> files written, <n> files removed, <n> asset packages added
```

No `LogArcRmlUI: Error` line that names `/ArcRmlUIPackagingTest/` or `ArcRmlUIPackagingTest`.
