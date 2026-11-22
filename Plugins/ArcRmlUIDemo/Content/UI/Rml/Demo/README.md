# ArcRmlUI demo game UI

## Port notes

The demo screens are a port of a Claude Design game UI prototype. The component kit is described in
[`Components/README.md`](Components/README.md).

They are the plugin's worked example of keyboard and gamepad navigation (the ArcRmlUI plugin's
"Keyboard and gamepad navigation" contract): RmlUi owns focus and the view model keeps no copy of it. A
call carries its context from the calling element (the screen model's own function with the item index or field
id as an argument, for a key a `data-event-uiaction` on the element with `callIf` and a matching `stopPropagation`),
the prompt group is chosen by `body[data-focus-active]`, and the prototype's `AREAS` table, `goArea` and focus memory
are not ported. Each screen document has its own view model (a Blueprint child of a C++ class that holds the demo
data); Credits is a Blueprint view model with no C++ class.

## Screen deviations

Where a screen differs from the prototype, and why.

- **Shared `GameUI.rcss`.** The page rules every prototype screen shares (`html`, `body`,
  scrollbars, the `gameui-in` enter animation, `.gu-layer`, `.gu-screen`) live in one sheet that
  every screen document links, because each RML document links its own sheets and one copy keeps
  them equal.
- **Mono family name.** The variable JetBrains Mono the demo ships is declared as `'JetBrains Mono Variable'`
  (`Fonts/Fonts.rcss`, `--ui-font-mono` in `tokens.rcss`), not `'JetBrains Mono'`: font faces are global to the
  process and the first face loaded for a family and weight wins, so with a project that declares its own
  `'JetBrains Mono'` (in this repo the test plugin's static faces) every second load would warn and one side would
  draw with the other's file.
- **`display: contents` area wrappers (`.gu-area`).** A focus area is an element with
  `data-focus-area`. Where the prototype has no element around an area's items, a wrapper with
  `display: contents` carries the attribute without adding a box, so the layout stays the
  prototype's.
- **Value rows step on their own keys.** Settings rows, the quantity `Stepper`s, the Character
  attribute rows and the loading tip step on `ev.key == 'ArrowLeft'` / `'ArrowRight'` in their
  `keydown` and stop the key, so the arrow does not also move focus.
- **Nested `dropdown` area.** An open Settings dropdown's options are their own contained focus area
  (`Components/Dropdown.rml`), entered from the row by `data-focus-enter`, so Up / Down move between
  options and Left / Right do nothing, and Back inside them or focus leaving their row (a press outside it) calls `CloseDropdown(field)` (the component's own `data-event-back` and `data-event-blur` with `!ev.related_in_owner`; the SettingRow import carries `data-focus-owner`, so a press on the row's label or box closes through the row's click and its Reset through `FieldReset`); the prototype handles the highlight in its key handler.
- **Settings row click runs the default action.** Enter, Space and pad A on a focused row arrive
  as a click on the row, so the row's click handler runs Confirm (toggle, open, edit, rebind) for
  every row type. In the prototype a row click only focuses, and only dropdown, text and keybind
  rows act (`GameUI.dc.html:1132`); here a mouse click on a toggle, checkbox, stepper, slider or
  radio row also changes it.
- **Text fields behave as in Chrome.** A focused field takes typing at once and each `input` writes
  the model (`TextInput`); there is no edit mode, no Done / Cancel and no revert. Up / Down leave a
  single-line field by spatial navigation, Esc keeps the text and the focus, pad B leaves the field.
  The prototype ignores arrows while typing (`GameUI.dc.html:695-699`) and starts typing on Confirm
  or on focus of the field. Left / Right move the caret: the input stops their `keydown`, so they
  never leave the area.
- **Keybinds take Space, 0-9, A-Z and Backspace only**, from `ev.key` of the field's `keydown`
  (`KeyBound`). Other keys (arrows, function keys, modifiers, Enter) bind nothing. The prototype
  binds any key (`GameUI.dc.html:701`). While listening the field consumes its keys: arrows do not
  move focus, Enter and Space do not restart the rebind, and the screen's own key actions (R reset,
  F apply, Q / E tabs) do not also run for the key that was bound. Esc cancels (`KeybindCancel`),
  and so does a Back that reaches the row.
- **Loading screen focus.** The screen root takes the first focus (`autofocus`), and it is where
  `anyinput` lands with only the document focused; the `tip` area is entered by the first
  Left / Right, which therefore does not change the tip yet.
- **Loading tip arrows at 100 %.** Left / Right continue when loading has finished, through the press's
  `anyinput` as every other key does; the tip handler does nothing then.
- **Any key on Splash and Loading.** Both declare `any-input`: every key and pad button continues through
  the `anyinput` handler on the screen root (except the dev bar action, F10, which toggles the dev bar),
  a left mouse release on the root continues through `mouseup` (not `click`, which RmlUi also fires for
  Enter and pad A), and the `body` back handler only stops Back so the host does not close the screen.
  Each press continues once.
- **Vendor transaction areas.** The prototype's `tx` area holds the quantity and the deal button,
  Left / Right change the quantity on the first and leave to `stock` / `bag` from the second
  (`GameUI.dc.html:803`). Here `tx` (on the Panel import, passed to the Panel root) holds only the
  stepper, whose own Left / Right step the quantity while it is focused, and the deal button sits
  in a nested `deal` area in the Panel footer, so Left / Right from it leave by geometry. Enter on
  the stepper does nothing; the prototype moves focus to the button. A `data-focus-enter` on the
  stepper root would also move focus on a mouse click on its arrows.
- **Vendor backpack cells.** The backpack grid draws 40 cells as the prototype does; only the filled
  cells (at least one, `Vendor.BagStops`) are focus stops, matching the area's item count
  (`GameUI.dc.html:366`).
- **Match results show the victory data only.** `GameUI.dc.html` never passes the screen's `state`,
  so the screen always shows its victory default; the view model fills that data. The defeat look
  (`.defeat` band and banner rules) is ported but no state of the demo selects it.
- **Pause subtitle, HUD texts and results data come from the view model.** The prototype screens
  hold them as defaults in `renderVals()`; here they are view model fields, so no demo data sits in
  the markup.
- **Vendor stock slot width.** Each stock cell is a centred column; the `dc-import` host wrapper
  is its flex item and would shrink-wrap the fluid `ItemSlot`, so the import carries
  `style="width: 100%"` (passed to the host) to give the slot the cell width the prototype's
  slot takes directly (`Vendor.dc.html:10`).
- **Stepper values in Vendor and Crafting show `1…`.** The known fork gap "ellipsis item too
  narrow in a centred column flex" also hits the quantity steppers;
  the Stepper sheet is kept verbatim.
- **Cross-area moves come from the plugin.** An arrow with no candidate inside the area leaves
  it by geometry (the ArcRmlUI plugin's "Spatial navigation, areas and leaving"): every focus stop
  is `tab-index: auto; nav: auto`. Where the prototype's `AREAS` neighbour table and the layout
  agree the move is the same. Where they differ geometry decides: Right from the top row of the
  SaveLoad grid lands on the `sort` dropdown (up and to the right) and only the bottom row reaches
  the `actions` buttons, where the table sends every Right edge to `actions`; Right from the top
  rows of the Vendor stock skips the transaction column (its stepper lies lower) and lands in the
  backpack, while the rows level with the stepper reach `tx`; Down from the Lobby
  `header` or `filters` row may land on any area below it; an area the table makes a dead end in
  one direction is left when another area lies that way. In-document popups carry
  `data-focus-contain` (the SaveLoad and Vendor confirms, the open Settings dropdown), so
  arrows never leave them. No screen needs `navnotarget`: nothing in the prototype acts at an
  edge other than moving to a neighbour.
- **Back.** Every screen's `body` calls `ui.Back` and stops the event (Splash and Loading only stop
  it); the demo subsystem passes it to the open dialog or the top screen's model. A child area
  (`data-focus-parent`) returns focus to its parent at the element it was entered from before the
  event reaches `body`; a top-level area, or a child whose parent is gone or has nothing to focus
  (SaveLoad `actions` with no saves, Lobby children while the server list is empty or searching,
  Crafting children with no recipes), lets Back reach `body`, so a Lobby search is cancelled from
  any area. A child area handles Back itself only while a view model state owns it and its parent
  still has rows: a running craft job (Crafting). Settings keybind areas cancel a listening key the
  same way. The prototype resolves Back from its mirrored
  focus (`GameUI.dc.html` `back`).
- **Return to the opener is selection.** A reopened screen focuses like a cold open, on the
  element whose `autofocus` the view model binds from its selection: the menu item that opened a
  child screen (Main, Pause), the selected save slot, server, recipe, stock or backpack item, the
  selected Settings row, Continue on Results. The prototype restores the remembered focus of
  every area (`S.mem`). Pause: Save Game keeps focus while saving, because a disabled-looking
  item stays a focus stop.
- **Disabled items stay focus stops.** As in the prototype, whose focus walks every item of an
  area, disabled or locked items are reached by the arrows and show their state; their calls
  are guarded (`callIf(!disabled, ...)`) or ignored by the view model: Settings locked rows, the
  SaveLoad Delete and Load buttons of an empty or corrupted slot, the Vendor Buy button when the
  gold is short, the Craft button when ingredients are missing or a job runs, recipes without
  ingredients, Pause Save Game while saving (WAI-ARIA APG: disabled menu items stay focusable).
  A mouse click on a disabled-looking recipe row also selects it, because Enter and pad A arrive
  as the same click; the prototype gives a disabled row no mouse handler.
- **Selection follows activation.** The Settings detail panel and the Vendor transaction panel
  show the row or item last activated (Enter, a click, Left / Right or a context key on it),
  not the focused one: the view model has no focus, the panels lie outside the focused item's
  subtree, and RCSS has no `:has()` to show a panel from the focused row. In the prototype both follow focus (`GameUI.dc.html:570-574`). Enter on a stock
  or backpack item therefore selects it and opens the confirm, as the prototype's Confirm does.
- **Prompts per area.** Each screen's RML holds one prompt group per focus area (`gu-prompts`
  with `data-area`) and the groups that apply to a screen state (`data-if` on a model field such as
  typing, a running job or an open confirm); `GameUI.rcss` shows the group whose
  `data-area` matches `body[data-focus-active]`, the default group while no area is active, and
  the single group of a screen without areas. Key labels are `ArcKeys.<IA name>`, so a rebound key shows.
  A screen-wide action key is `data-hotkey` on its `InputPrompt` with a
  `data-event-click` that calls the model, so the key runs exactly while its prompt is visible and the
  prompt is clickable; a disabled prompt's click is guarded in the call (`callIf`). The prototype builds the prompts from the focused
  item, so a label that depends on the item becomes the area's label: Settings rows show
  "Change" and "← → Change" for every row and Reset is enabled when any row of the tab differs
  from its default; SaveLoad actions, Lobby rows, Crafting filters and Results show "Select";
  on a Vendor stock or backpack item Buy max and Compare show as available and act on that item
  (the item's call passes its list and index and selects it), so Compare on an item that has no slot does
  nothing.
- **Focus into confirms, the dropdown and the friend menu.** The trigger carries `data-focus-enter`,
  and the core enters the area once the trigger's call is answered; closing returns focus to the
  trigger, the area's recorded origin. The demo has no `data-focus-request`. Everything else that
  returns focus is selection plus a bound `autofocus` (a reopened view focuses as a cold open).
- **Focus after a change of the list.** When a Settings or Vendor tab switch, a Lobby filter, the
  Crafting can-craft toggle or a finished craft job removes the focused element, the host
  recovers focus to the nearest focus stop. The prototype sets the focus itself (row 0, the first
  stock or backpack item, the first server, the Craft button).
- **Craft button during a job.** Starting a job keeps focus on the Craft button, which stays a
  focus stop while `loading`; Confirm does nothing there, Left reaches Cancel and Back cancels the
  job. The prototype moves focus to Cancel (`GameUI.dc.html:1056`).
- **Loading tip arrows keep focus.** Left / Right in the `tip` area change the tip and leave focus
  on the arrow it is on; the prototype focuses the arrow of the direction.
- **Dev bar status.** The status line shows the screen stack only; the prototype also shows the
  mirrored focus.

## HUD

`Hud.rml` replaces the earlier demo HUD and the removed `HudPort/` RML. It binds `ui`, `hud` (`BP_ArcRmlDemoHudVM`, `OpenVendor`, `OpenCrafting`, `EndMatch`, `bMultiplayer`) and the four Blueprint view models `hudstate`,
`hudunits`, `hudworld` and `hudbars` (`BP_HudPort*VM`, whose `S_HudPort*` row structs are in `Content/HudPort/ViewModels/`). The body is transparent, so the
game map shows behind the HUD, and the body takes the five theme classes of the dev bar.

- **Layout.** Nine docks (`data-hud-dock`: tl, tc, tr, ml, c, mr, bl, bc, br) in a grid, padded by the safe zone
  (`--hud-safe`, 32 px; 0 or 64 px from `SafeZone`); the three centre docks are capped at 40% or 60% of the width inside the safe zone. The HUD is laid
  out inside the content rect, so at 2560x1080 it is anchored inside the centred 16:9 column that the core's
  `MaxUIAspectRatio` default gives (`docs/reference/arc-rml-ui/guide/display-and-players.md`), not at the screen edges.
- **F9 prompt.** The HUD debug prompt sits in the prompt bar at all times; the mockup shows it only in its dev mode.
- **F9 menu.** `HudDebug` holds an Actions group (Vendor, Crafting, and End match while `bMultiplayer`, on the `hud` model), a Global group (theme, input scheme on `ui`) and one `HudDebugItems` list per HudPort view
  model, read from its `Debug` array. Vendor, Crafting and End match open here because the owner moved the old HUD's actions into this menu (2026-11-22);
  their hotkeys (`IA_ArcDemo_OpenVendor`, `OpenCrafting`, `EndMatch`) are `data-hotkey` on the buttons, so they work only while the
  menu is open, and the buttons show no key labels.
- **Focus.** The debug controls are focus stops even when disabled (`focusable="{{ true }}"`; the caller guards the call).
- **`SliderInput`** exists on all four HudPort view models; it calls the row's `Fn` and does nothing on State, Bars and World.
- **Dead and wheel.** The death overlay is its own toggle (`SetDead`); Kill player only sets health to 0. The weapon wheel is a
  toggle (`SetWheelOpen`), not a hold.
- **Nothing runs over time.** A Blueprint view model has no tick: the time scale buttons only set the clock label; camera motion
  shifts every nameplate's `X` by 6 once; Hold to interact sets the prompt's `Hold` to 0.6 or -1; Press slot starts the slot's
  cooldown at 100 / 5 s and Quick use at 100 / 10 s, and neither counts down (Quick use spends a charge whenever `Count` is above 0).
- **Add buff / Add debuff** pick the entry of `BuffPool` / `DebuffPool` at the total buff count modulo the pool length.
- **Kill player and Respawn** are always enabled; the prototype disables Kill player while dead and Respawn while alive.
- **Reload** moves the missing rounds from `Reserve` to `Clip` in one call, with no reload time.
- **Complete quest** (and the last Advance objective) shows the quest banner but adds no XP line to the loot feed.
- **Hold LT / Hold RT** are click toggles (`PadLt` / `PadRt` switch `PadSet` between the pad set and `none`), not held buttons.
- **Fixed labels.** Debug control labels are fixed text ("Show / hide"); the prototype's labels that follow state (`Show` / `Hide`) are not ported.
- **`WheelHighlight`** lives in `BP_HudPortBarsVM` beside `Weapons`, not in the HUD state.

## Game Menu hub (demo-game-ui step 8)

- **Frame and six tab documents.** `Hub.rml` is the frame (backdrop, `GameMenuHeader`, the `arc-embed` box,
  the Dev bar) and declares the UI model as `hubui` and the hub model as `hub`; `Hub/Inventory`, `Character`,
  `TalentTree`, `QuestLog`, `Friends` and `Achievements` are its tabs, each the prototype screen's `embedded`
  layout with the UI model `ui`, its own model (`inventory`, `character`, `talents`, `quests`, `social`, `achievements`), its own prompt bar (`.ht-footer`, shown per area by the `data-focus-active` its `body` mirrors from the frame) and its own sheets (`HubTab.rcss` makes the tab canvas transparent, as the
  prototype's embedded root is). The prototype draws the six screens under `sc-if` in one tree
  (`GameUI.dc.html:11`); here the view model switches the embedded document.
- **One tab mechanism for the demo.** Main tabs (hub, Settings, Vendor) are a `data-hotkey-cycle`
  container `IA_ArcDemo_PrevTab IA_ArcDemo_NextTab` (Q / E, pad LB / RB); the hub's sub-tab bars (talent
  trees, quest categories, social tabs, achievement filter chips) use `IA_ArcDemo_PrevSubTab
  IA_ArcDemo_NextSubTab` (Z / C, pad LT / RT) the same way. The view model no longer
  cycles tabs. A tab item is a cycle item but lies outside every focus area, so the arrows never reach it, as
  the prototype has no tab-bar area; a mouse click on a tab leaves focus on it, and the next arrow enters an
  area. A key or mouse switch keeps the focus where it was outside the bar; a new hub tab takes focus at its
  primary area.
- **Tab keys pass through the context-key handlers.** Every `data-event-uiaction` in the hub, Settings and
  Vendor calls the view model and stops the event only for actions that are not tab or sub-tab actions, so
  the cycles still run. In-document confirms (talent reset, quest abandon, the Vendor confirm), the open
  Settings dropdown, the open friend menu and an Inventory move stop the tab actions too, so Q / E and Z / C
  do nothing there (prototype `tab()` and `SubTabPrev`, `GameUI.dc.html:771`, `:820`). A mouse click on a
  tab is not blocked, as in the prototype's pointer path (`GameUI.dc.html:1119`).
- **Back.** Each tab's root element (`ht-root`, the element that declares the tab's model) calls the tab
  model's `Back`, which cancels a move, a confirm or the friend menu first and otherwise leaves the hub.
  Child areas return to their parent at the element they were entered from; Inventory's child areas call
  `Back` during a move, and the open friend menu (`FriendRow`) calls it to close itself. The frame `body`
  calls `hubui.Back` for a Back outside the tab.
- **Selection follows activation.** The Inventory tooltip shows the slot last acted on (Equip, Use, Move,
  Drop, Junk; at first the first slot), the moved item during a move; the talent detail panel shows the node
  last learned, unlearned or clicked (at first the prototype's `e2`); the quest detail panel shows the quest
  whose Details or context key ran. In the prototype these follow focus (`invF`, `tsel`, `qsel`,
  `GameUI.dc.html:540-541`, `:1212`); the view model has no copy of focus.
- **Move target.** During a move the focused slot takes the drop-target look through `.iv-grid.moving
  .dk-itemslot:focus-visible`; the prototype also draws the moved item's icon on the focused slot
  (`GameUI.dc.html:1213-1217`), which needs the focused index.
- **Prompts per area.** Item-dependent labels become area labels: the Inventory grid shows Equip, Use, Move,
  Drop, Split and Mark as junk together; Character footer,
  Social party actions and the talent detail buttons show "Select"; Quest prompts use the selected quest.
- **Empty views share their area.** The Inventory empty view (Show all) is a second element named `grid`,
  the Social "no friends match" view sits inside `list`, because the prototype's `empty` areas replace a list
  whose area would then have no stop for Back to return to.
- **Character attributes.** Each attribute row is its own area named `attrs` with `data-focus-parent="equip"`
  and the `AttributeRow`'s own Left / Right `keydown`, so Left / Right change that row and Up / Down
  leave by geometry. The plan named the rows' parent `attrs`; an outer `attrs` area has no stop of its own,
  so Back from a row could not land there, and the prototype's `attrs` parent is `equip`
  (`GameUI.dc.html:332`). The footer's parent `attrs` returns to the first row.
- **Neighbours by geometry.** The paper doll and the talent tree move by spatial navigation. The talent
  nodes carry `nav-<dir>: #id` only where the prototype's table (`GameUI.dc.html:187-196`) names a node and
  geometry would pick another (computed by `UArcRmlDemoTalentsViewModel::NavOverride` with the fork's score); where the table names
  nothing (a row end, the top node), geometry may move to a node diagonally or leave to `detail` / `reset`.
  On the paper doll Down from the bottom slots and Right from the right column leave by geometry.
- **Friend menu.** The open `ContextMenu` of a friend row is the area `menu` (contained). Enter on a row
  opens it and focus enters its first command (`data-focus-enter="menu"` on the row, held until the
  call that opens the menu is answered). Back and every menu command close it and focus returns to the
  row, the area's origin; the prototype sets the focus itself (`GameUI.dc.html:914`, `:964`). The empty
  party card enters `list` on click.
- **Toasts in the hub.** As in the prototype, only the Inventory tab draws the toast (`inv.toast`,
  `GameUI.dc.html:1222`); toasts of the other tabs (achievement details, pins, quest map, social commands)
  are set but not drawn until the HUD shows them after the hub closes.
- **Social empty title** reads the typed query; the prototype's text holds a demo query (`Friends.dc.html`).

## Behaviour of the per-screen view models

Where the per-screen models differ from the earlier single-model port or from the prototype.

- **Screens are documents on the plugin's stack.** A push opens the screen's document with an init that
  calls its model's `InitScreen(Param)` before the screen is shown; a pop closes the top document. The
  screen below learns it is shown again from the plugin's document-activated event (`ScreenActivated`;
  Pause shows the saving banner there). The HUD stays open under the menus, hidden and paused. Splash,
  Loading and the HUD stop Back on `body`, and Main's `body` Back leaves nothing (`LeaveScreen` never
  pops a root screen), so Back never closes them.
- **Timers start at Push.** The splash delay and the loading progress start in the model's `InitScreen`,
  when the screen is pushed; the lobby search, saves and crafting start from the player's action. Every
  demo document is built at `Start` (preload) and parked, so the timer does not wait for a first open.
- **Vendor Compare by slot.** Compare takes the slot of the item it was called on and compares it with the
  equipped item of that slot (`ring` with `ring1`, `trinket` with `trinket1`); an item with no slot does nothing.
- **Crafting search starts on the first character.** Each `input` event writes the query and refilters the
  recipe list (`WriteText`).
- **Settings reset needs a focused row.** R / pad X resets the row that has focus (`FieldReset` on the row's
  `data-event-uiaction`), so with focus off the rows nothing resets; the prompt is enabled when any row of the tab differs.
- **Social guards.** Invite and Message do nothing while a friend's option menu is open; Message does nothing for an
  offline friend; an invite does nothing for a friend already in the party or when the party is full.
- **Dev bar input.** The Keyboard / Gamepad buttons switch the real input type of the player
  (`UCommonInputSubsystem::SetCurrentInputType`), not only the prompt style.
