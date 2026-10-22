# The static hack-label map in `FUN_180062940`

- **Question** — `FUN_180062940` references ~250 `HACK_*` strings. What does it build, and
  can the 85 label → integer pairs be recovered statically?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `samples/absolllute.hackmega/_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.
- **Tool** — `pe_disasm.py`; a capstone stack-string simulator (scratch, not tracked);
  `recovered/functions/_init_table.tsv`, `_xrefs.tsv`, `_settings.tsv`.

## Answer

`FUN_180062940` is a **static initializer**, reached from the `_initterm` array (slot VA
`0x18063F0C8`, index 12). It builds an 85-entry array of `{std::string label; uint64_t value}`
on the stack and inserts every pair into the global FNV-1a-64 map at `0x180800D78`.

| Property | Value | Evidence |
| --- | --- | --- |
| Entry layout | string at `entry+0x00` (0x20 bytes); value at `entry+0x20` | `0x180062976`-`0x1800629C3` (first entry) |
| Array base | `rbp-0x60` | `0x18006297E`, `0x180064900` (`lea r8,[rbx+rbp]; add r8,-0x60`) |
| Stride / count | `0x28`; bound `0xD48` = 85 × 0x28 | `0x180064913`, `0x180064917` |
| Map object | `DAT_180800D78` (0x38-byte node allocated in the ctor) | ctor `0x180049080`, called at `0x1800648E9` |
| Insert | FNV-1a-64 hash of the label, chain walk, node `value` at `+0x30` | `0x180049390`, called at `0x18006490E`; value copy `0x180049904` (`mov rax,[rsi+0x20]; mov [rdi+0x30],rax`) |
| Destructor | `call 0x1803A5300` (atexit-shaped) with destructor thunk `0x1800427A0` → `0x1800426A0`, `rcx=&DAT_180800D78` | `0x180064993` |
| Labels | alphabetical, 85 distinct; 73 also exist as C strings in `.rdata`, 12 are built only as inline SSO bytes | see table |

The table is alphabetical by label, which is consistent with a static name→value lookup
registry. All 85 labels resolve; none are guesses.

## The table

`store VA` is the instruction that writes the integer into the stack entry.

| # | Label | Value | Hex | Store VA |
| ---: | --- | ---: | --- | --- |
| 0 | `HACK_0_PRACTICE_COMPLETE` | 296 | `0x128` | `0x1800629C3` |
| 1 | `HACK_ALLOW_LOW_VOLUME` | 451 | `0x1c3` | `0x180062A18` |
| 2 | `HACK_ALL_MODES_PLATFORMER` | 241 | `0x0f1` | `0x180062A6A` |
| 3 | `HACK_AUTO_DEAFEN` | 279 | `0x117` | `0x180062AB1` |
| 4 | `HACK_AUTO_MUSIC_SYNC` | 326 | `0x146` | `0x180062AFF` |
| 5 | `HACK_AUTO_PICKUP_COINS` | 320 | `0x140` | `0x180062B57` |
| 6 | `HACK_AUTO_SAFE_MODE` | 481 | `0x1e1` | `0x180062BBA` |
| 7 | `HACK_AUTO_SONG_DOWNLOAD` | 323 | `0x143` | `0x180062C24` |
| 8 | `HACK_CHECKPOINT_LIMIT_BYPASS` | 298 | `0x12a` | `0x180062C8B` |
| 9 | `HACK_CLASSIC_PARTICLES` | 390 | `0x186` | `0x180062CF5` |
| 10 | `HACK_COLLECT_COINS_IN_PRACTICE` | 297 | `0x129` | `0x180062D5C` |
| 11 | `HACK_CONFIRM_FULL_RESET` | 314 | `0x13a` | `0x180062DC6` |
| 12 | `HACK_CONFIRM_NORMAL` | 312 | `0x138` | `0x180062E29` |
| 13 | `HACK_CONFIRM_PRACTICE` | 311 | `0x137` | `0x180062E93` |
| 14 | `HACK_CONFIRM_RESET` | 313 | `0x139` | `0x180062EF5` |
| 15 | `HACK_COPY_HACK` | 43 | `0x02b` | `0x180062F43` |
| 16 | `HACK_CUSTOM_OBJECT_BYPASS` | 46 | `0x02e` | `0x180062FAA` |
| 17 | `HACK_DEFAULT_SONG_BYPASS` | 49 | `0x031` | `0x180063014` |
| 18 | `HACK_EDITOR_EXTENSION` | 50 | `0x032` | `0x18006307E` |
| 19 | `HACK_FAST_CHESTS` | 461 | `0x1cd` | `0x1800630DA` |
| 20 | `HACK_FORCE_PLATFORMER` | 242 | `0x0f2` | `0x180063144` |
| 21 | `HACK_FRAME_STEPPER` | 302 | `0x12e` | `0x1800631A6` |
| 22 | `HACK_FREE_SCROLL` | 52 | `0x034` | `0x180063202` |
| 23 | `HACK_HIDE_PAUSE_BUTTON` | 385 | `0x181` | `0x18006326C` |
| 24 | `HACK_HIDE_PAUSE_MENU` | 410 | `0x19a` | `0x1800632CF` |
| 25 | `HACK_HIDE_PLAYER` | 386 | `0x182` | `0x18006332B` |
| 26 | `HACK_HIDE_UI` | 53 | `0x035` | `0x180063372` |
| 27 | `HACK_INSTANT_COMPLETE` | 89 | `0x059` | `0x1800633D2` |
| 28 | `HACK_JUMPSCARE` | 291 | `0x123` | `0x180063420` |
| 29 | `HACK_LEVEL_EDIT` | 45 | `0x02d` | `0x18006346A` |
| 30 | `HACK_MAIN_LEVELS` | 39 | `0x027` | `0x1800634C6` |
| 31 | `HACK_MAIN_MENU_PLAY` | 457 | `0x1c9` | `0x180063529` |
| 32 | `HACK_MUSIC_UNLOCKER` | 36 | `0x024` | `0x18006358C` |
| 33 | `HACK_NOCLIP` | 72 | `0x048` | `0x1800635D3` |
| 34 | `HACK_NOCLIP_LIMITS` | 83 | `0x053` | `0x180063635` |
| 35 | `HACK_NO_CAMERA` | 387 | `0x183` | `0x180063683` |
| 36 | `HACK_NO_CAMERA_ZOOM` | 388 | `0x184` | `0x1800636E6` |
| 37 | `HACK_NO_C_MARK` | 44 | `0x02c` | `0x18006372A` |
| 38 | `HACK_NO_DEATH_EFFECT` | 399 | `0x18f` | `0x18006378D` |
| 39 | `HACK_NO_DO_NOT_FLIP` | 412 | `0x19c` | `0x1800637F0` |
| 40 | `HACK_NO_PARTICLES` | 402 | `0x192` | `0x18006384E` |
| 41 | `HACK_NO_PORTAL_LIGHTNING` | 416 | `0x1a0` | `0x1800638B8` |
| 42 | `HACK_NO_PULSE` | 413 | `0x19d` | `0x1800638FC` |
| 43 | `HACK_NO_RESPAWN_FLASH` | 398 | `0x18e` | `0x180063966` |
| 44 | `HACK_NO_SHADERS` | 389 | `0x185` | `0x1800639A6` |
| 45 | `HACK_NO_SHAKE` | 409 | `0x199` | `0x1800639EA` |
| 46 | `HACK_NO_TRAIL` | 395 | `0x18b` | `0x180063A2E` |
| 47 | `HACK_NO_TRANSITION` | 449 | `0x1c1` | `0x180063A90` |
| 48 | `HACK_NO_WAVE_PULSE` | 394 | `0x18a` | `0x180063AF2` |
| 49 | `HACK_NO_WAVE_TRAIL` | 397 | `0x18d` | `0x180063B54` |
| 50 | `HACK_NO_WAVE_TRAIL_BEHIND` | 414 | `0x19e` | `0x180063BBB` |
| 51 | `HACK_PITCH_SHIFTER` | 459 | `0x1cb` | `0x180063C1D` |
| 52 | `HACK_PLACE_OVER` | 51 | `0x033` | `0x180063C67` |
| 53 | `HACK_PRACTICE_BUG_FIX` | 317 | `0x13d` | `0x180063CD1` |
| 54 | `HACK_PRACTICE_MUSIC` | 88 | `0x058` | `0x180063D34` |
| 55 | `HACK_RANDOM_SEED` | 308 | `0x134` | `0x180063D90` |
| 56 | `HACK_REPLAY_LAST_CHECKPOINT` | 316 | `0x13c` | `0x180063DF7` |
| 57 | `HACK_RESPAWN_TIME` | 245 | `0x0f5` | `0x180063E55` |
| 58 | `HACK_SAFE_MODE` | 482 | `0x1e2` | `0x180063EA3` |
| 59 | `HACK_SAFE_MODE_POPUP` | 483 | `0x1e3` | `0x180063F06` |
| 60 | `HACK_SHOW_HITBOXES` | 192 | `0x0c0` | `0x180063F68` |
| 61 | `HACK_SHOW_HITBOXES_ON_DEATH` | 216 | `0x0d8` | `0x180063FCF` |
| 62 | `HACK_SHOW_HITBOXES_TRAIL` | 224 | `0x0e0` | `0x180064039` |
| 63 | `HACK_SHOW_LAYOUT` | 260 | `0x104` | `0x180064095` |
| 64 | `HACK_SHOW_TRIGGERS` | 286 | `0x11e` | `0x1800640F7` |
| 65 | `HACK_SLIDER_LIMIT` | 38 | `0x026` | `0x180064155` |
| 66 | `HACK_SMART_STARTPOS` | 235 | `0x0eb` | `0x1800641B8` |
| 67 | `HACK_SMOOTH_EDITOR_TRAIL` | 55 | `0x037` | `0x180064222` |
| 68 | `HACK_SOLID_WAVE_TRAIL` | 396 | `0x18c` | `0x18006428C` |
| 69 | `HACK_STARTPOS_RESET_CAMERA` | 426 | `0x1aa` | `0x1800642F3` |
| 70 | `HACK_STARTPOS_SWITCHER` | 130 | `0x082` | `0x18006435D` |
| 71 | `HACK_STOP_TRIGGERS_ON_DEATH` | 415 | `0x19f` | `0x1800643C4` |
| 72 | `HACK_TEXT_LENGTH` | 37 | `0x025` | `0x180064420` |
| 73 | `HACK_THREAD_PRIORITY` | 454 | `0x1c6` | `0x180064483` |
| 74 | `HACK_TOOLBOX_BUTTON_BYPASS` | 47 | `0x02f` | `0x1800644EA` |
| 75 | `HACK_TRAIL_CUTTING` | 392 | `0x188` | `0x18006454C` |
| 76 | `HACK_TREASURE_ROOM` | 35 | `0x023` | `0x1800645AE` |
| 77 | `HACK_TRIGGER_VALUE_BYPASS` | 54 | `0x036` | `0x180064615` |
| 78 | `HACK_UNLOCK_ICONS` | 32 | `0x020` | `0x180064673` |
| 79 | `HACK_UNLOCK_SHOPS` | 34 | `0x022` | `0x1800646D1` |
| 80 | `HACK_UNLOCK_VAULTS` | 33 | `0x021` | `0x180064733` |
| 81 | `HACK_VERIFY_HACK` | 48 | `0x030` | `0x18006478F` |
| 82 | `HACK_WAVE_PULSE_SIZE` | 407 | `0x197` | `0x1800647F2` |
| 83 | `HACK_WAVE_TRAIL_FIX` | 411 | `0x19b` | `0x180064855` |
| 84 | `HACK_WAVE_TRAIL_ON_DEATH` | 419 | `0x1a3` | `0x1800648BF` |

## Method

1. `pe_disasm.py 0x180062940 --size 0x2B00` established the function bounds and the entry
   layout. The first entry is unambiguous: `mov [rbp-0x60], rax` (heap string), length 0x18
   at `rbp-0x50`, capacity 0x1f at `rbp-0x48`, value `0x128` at `rbp-0x40`.
2. A capstone simulator walked the function and tracked:
   - `movabs` immediates per register (`rsi` is loaded with `"COMPLETE"` at `0x1800629B1`
     and with `"HACK_NO_"` at `0x180063661`; both are reused as SSO prefixes);
   - writes to `[rbp+B]` and `[rax+off]` inside each entry's build region;
   - `movups xmm0,[rip+…]` / `movups [rax],xmm0` pairs, which carry the 16-byte heap-string
     heads for labels longer than 15 bytes;
   - `mov qword [rbp+off], imm` at offsets where `(off + 0x60) % 0x28 == 0x20`, which is the
     value slot for the entry being built.
3. Each value slot closed the current entry; its label is the bytes written since the
   previous slot. Length came from the `[B+0x10]` store.
4. All 85 labels reconstructed with the correct length; no pool/prefix guessing was used for
   the final output. 12 labels are built only as SSO immediates and therefore do not exist
   as standalone C strings in `.rdata`:
   `HACK_COPY_HACK`, `HACK_HIDE_UI`, `HACK_JUMPSCARE`, `HACK_LEVEL_EDIT`,
   `HACK_NO_CAMERA`, `HACK_NO_C_MARK`, `HACK_NO_PULSE`, `HACK_NO_SHADERS`,
   `HACK_NO_SHAKE`, `HACK_NO_TRAIL`, `HACK_PLACE_OVER`, `HACK_SAFE_MODE`.
   `HACK_NO_C_MARK` is the literal the compiler builds (`"HACK_NO_"` + write at offset 6
   `"O_C_MARK"`); it is not a truncation artefact.

## Independent checks

- **Array bound.** `0xD48 / 0x28 = 85` exactly. The cleanup loop after the insert loop walks
  the same array backwards in `0x28` steps (`0x180064920`-`0x18006495E`).
- **Insertion stores the value.** The slow path at `0x180049820` copies `[entry+0x20]` into
  the node's `+0x30` (`0x180049904`), so the integers are the map's mapped values, not dead
  padding.
- **Static initializer.** `_init_table.tsv` lists target `0x180062940` at slot
  `0x18063F0C8` (index 12), so this runs during CRT init, not from gameplay code.
- **String xrefs.** `_xrefs.tsv` attributes 247 string references / 132 distinct strings to
  `FUN_180062940`; the extra strings belong to the later tables the same initializer builds.
- **No reader found.** A full `.text` scan for RIP-relative references to `0x180800D78`
  returns four sites, all on the construction/destruction path: the ctor (`0x1800648D4`),
  the insert loop (`0x1800648F0`), the destructor thunk (`0x1800427A0`), and one occurrence
  inside an unwind routine (`0x18006716B`). No `find`/lookup call against this map was found.

## What is the integer? Not settled

Three observations pull in different directions:

| Observation | Count | Implication |
| --- | ---: | --- |
| Value equals an id in `_settings.tsv` (accessor-read tags) | 14 / 85 | supports "value is a `hackpro::Tag`" |
| Value equals an enumerated `FUN_180120A40` UI row id | 10 / 18 | supports "value is a UI row id" |
| `HACK_RESPAWN_TIME` = 245, while setting tag 245 suppresses the New Best label | 1 | argues against a single tag space |

The 14 accessor hits are all `getConfigVar<bool>`:
`HACK_AUTO_SAFE_MODE` (481), `HACK_FRAME_STEPPER` (302), `HACK_NOCLIP` (72),
`HACK_NOCLIP_LIMITS` (83), `HACK_NO_TRANSITION` (449), `HACK_PITCH_SHIFTER` (459),
`HACK_RESPAWN_TIME` (245), `HACK_SAFE_MODE` (482), `HACK_SHOW_HITBOXES` (192),
`HACK_SHOW_HITBOXES_ON_DEATH` (216), `HACK_SHOW_HITBOXES_TRAIL` (224),
`HACK_SOLID_WAVE_TRAIL` (396), `HACK_STARTPOS_SWITCHER` (130),
`HACK_THREAD_PRIORITY` (454).

The UI row-id list is the 18 ids recorded in [definition-keys.md](definition-keys.md):
32, 394-399, 402, 411, 414, 422, 432-434, 436, 442, 445, 448. The 10 present here are 32,
394-399, 402, 411 and 414.

Note that `hackpro.json` (the observed install) stores hack enablement as **names in the
profile list**, `PROFILES/DEFAULT`, not as typed boolean keys. It contains no `"tag"`
fields. Whatever the integers mean, they are not needed to parse that file's typed buckets.

**UNVERIFIED:**
- whether the value is a `hackpro::Tag`, a UI row id, or a third id space. The `245`
  collision rules out assuming a single global tag space but does not rule out per-map tags.
- how the map is consumed. No read site exists in `_original.dll`, so the table may be
  written and reaped without a static reader (an API path that resolves the global at
  runtime, or dead data from an earlier UI).
- whether the 85 labels are exactly the set of hacks the release's UI can display. The
  observed profile lists 53 enabled hacks; 34 of those appear here and 19 do not.

## Why this matters for M3

This is the first statically enumerated link between 85 `HACK_*` names and numeric ids.
It does **not** close M3's per-feature mapping: a label and an integer are not an
implementing function, a default, or a set of interactions. The rows in
[feature-inventory.md](feature-inventory.md) stay `cluster`.

## Open questions

- Which id space the integers belong to, and which code reads `DAT_180800D78`.
- Why 85 labels were compiled in when the runtime registry is built from `hackpro.json`.
- Whether the table is related to the profile-enabled list (string membership) or to the
  UI's numeric row builders.
