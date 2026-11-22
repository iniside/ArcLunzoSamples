# Images

Every way a document draws an image, each with a caption: an `<img>` with a PNG file, `rect` to draw one part of a
file, `@spritesheet` with `sprite`, `<svg>` with an SVG file, an inline `<svg>` drawn in the text colour, an
`asset:` texture, and two images bound to a view model. "Swap the bound images" switches both bound images; "Close"
closes the document.

- `<img src="Images/Picture.png" width="64" height="64"/>`: a path without `/` at the start is relative to the
  document. With both `width` and `height` the image has its size before the file has loaded.
- `rect="32 0 32 32"`: x, y, width and height in pixels of the file; only that part is drawn.
- `@spritesheet im-sprites` in `Images.rcss` names four parts of `Images/Sprites.png`; `<img sprite="im-circle"/>`
  draws one. `sprite` wins over `src`. Sprite sheets are an RmlUi feature with no browser counterpart.
- `<svg src="Images/Star.svg"/>`: an SVG file, sharp at any size.
- `<svg viewBox="0 0 24 24"><path fill="currentColor" .../></svg>`: an inline SVG; `currentColor` takes the
  element's CSS `color`, so one shape serves every colour.
- `src="asset:/ArcRmlUISamples/UI/Rml/Images/T_Sample_Picture.T_Sample_Picture"`: a `Texture2D` asset. It must not
  use virtual texture streaming; the document compile reports a missing or unsuitable texture.
- `<viewmodel ... class="/ArcRmlUISamples/UI/Rml/Images/BP_ImagesVM.BP_ImagesVM_C" create="true"/>`: a Blueprint
  subclass of the C++ view model. The C++ class declares the fields; the Blueprint's class defaults hold the image
  values (`First Icon`, `Second Icon`, `First Picture`, `Second Picture`), so no asset path is written in C++.
- `data-attr-src="Icon"`: an `FArcRmlImageRef` field binds as its `Path`, an SVG file path.
- `data-attr-src="Picture"`: a `TSoftObjectPtr<UTexture2D>` field binds as `asset:<path>`. The captions show
  both bound values.
- The image values are class defaults of the Blueprint view model `BP_ImagesVM`, so they ship: the cook follows an
  `FArcRmlImageRef` in Blueprint defaults and a texture they reference. Only a path held in a C++ or runtime string
  is not followed. The second picture is an engine texture.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open Images
```

`ArcRmlSamples.Close` closes it again.

## Files

- `Images.rml`, `Images.rcss`
- `Images/Picture.png`, `Images/Sprites.png`, `Images/Star.svg`, `Images/Heart.svg`
- `T_Sample_Picture` (the texture asset), `BP_ImagesVM` (the Blueprint view model with the image values)
- `Source/ArcRmlUISamples/Images/ArcRmlSamplesImages.h`, `.cpp` (the view model and the open function)

Guide page: Images.
