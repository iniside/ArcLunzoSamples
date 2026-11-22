# Virtual list

A list of 10 000 rows in a box that scrolls. Only the rows near the visible part exist as elements; the readout above
the box shows how many entries the view model holds.

- `data-for="n : Rows" data-virtual` on the row element. `Rows` is a `TArray<int32>` the view model fills with
  1 to 10 000 in its constructor, and `{{ n }}` is the entry.
- The row's parent, `.vl-list`, is the scroll container: it has a fixed `height` and `overflow-y: auto`. A virtual
  `data-for` whose parent does not scroll vertically creates no rows and logs an error.
- Every row takes the size of the first row, here `height: 36px` set in `VirtualList.rcss`. Rows of different heights,
  horizontal lists and other children inside the scroll container are not supported.
- The elements that exist are the visible rows plus about one box height before and after. A row that leaves that window
  is removed, and with it its focus, hover and animations. The scroll bar still has the length of all 10 000 rows.
- The rows are buttons, so Up and Down (or the D-pad) move focus row by row and the box scrolls to keep the focused row
  in view. The first arrow after the open only lets focus appear.
- `{{ Rows.size }}` is the entry count.

## Run it

Play in the map `L_ArcRmlSamples`, then type in the console:

```
ArcRmlSamples.Open VirtualList
```

`ArcRmlSamples.Close` closes it again.

## Files

- `VirtualList.rml`, `VirtualList.rcss`
- `Source/ArcRmlUISamples/VirtualList/ArcRmlSamplesVirtualList.h`, `.cpp` (the view model and the open function)

Guide page: Data binding.
