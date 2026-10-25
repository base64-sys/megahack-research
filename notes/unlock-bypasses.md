# Unlock and shop bypass toggles

- **Question** — which code implements the icon/colour/music unlock bypasses and the
  vault/shop/treasure-room entry bypasses, and why do they have no `Added …` row?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.
- **Tools** — `scripts/tag_callback_map.py`, `scripts/callback_hooks.py`,
  `scripts/lazy_stubs.py`.

## Answer

Five static `HACK_*` labels form one family of unlock bypasses. They are registered in
`FUN_1800AFAE0` like every other toggle, but **none of them has an `Added …` changelog row**:
the changelog names them only in fix bullets ("Fixed Music Unlocker Bypass", "Fixed colours
not being unlocked", "Vault/Shop bypasses condensed into individual mods"). They are
therefore outside the 389-feature universe, and this note does not change the counts.

| Tag | Static label | Registration | Callback |
| ---: | --- | --- | --- |
| `0x020` | `HACK_UNLOCK_ICONS` | `0x1800AFF01` | `0x18015DC00` |
| `0x021` | `HACK_UNLOCK_VAULTS` | `0x1800AFFEF` | `0x18015E220` |
| `0x022` | `HACK_UNLOCK_SHOPS` | `0x1800AFF78` | `0x18015DFA0` |
| `0x023` | `HACK_TREASURE_ROOM` | `0x1800AFE8A` | `0x18015D960` |
| `0x024` | `HACK_MUSIC_UNLOCKER` | `0x1800AFD25` | `0x18015D130` |

All five callbacks were located by the nearest-preceding-`lea rcx` rule and every target below
was resolved by **mangled symbol**, not by RVA immediate — one of the 120 tags whose hooks are
all name-resolved (see [host-abi.md](host-abi.md#recovering-the-rva-from-a-symbol-only-hook)).

## Unlock Icons: two predicates forced true

```
0x18015DCD0  mov  al, 1
0x18015DCD2  ret                 ; GameManager::isIconUnlocked, RVA 0x17C500
0x18015DCE0  mov  al, 1
0x18015DCE2  ret                 ; GameManager::isColorUnlocked, RVA 0x17C8E0
```

Both detours discard their arguments and return `true`. This is the whole feature: the game's
own "is this icon/colour unlocked" queries can no longer say no, so every icon and colour is
selectable without touching the save. It also matches the changelog fix bullet "Fixed colours
not being unlocked" — the same toggle covers colours, which the name does not advertise.

Independent corroboration: both targets also have lazy-binding stubs whose immediates are the
same RVAs — `0x180148B82` → `0x17C500` and `0x180148932` → `0x17C8E0`, read with
`scripts/lazy_stubs.py`.

## Music Unlocker: one item ID pair forced true

```
0x18015D1A0  mov  eax, edx
0x18015D1A2  xor  eax, 0xC        ; arg2 == 12 ?
0x18015D1A5  mov  r9d, r8d
0x18015D1A8  xor  r9d, 0x10       ; arg3 == 16 ?
0x18015D1AC  or   r9d, eax
0x18015D1AF  jne  0x18014BBB0     ; otherwise: original GameStatsManager::isItemUnlocked
0x18015D1B5  mov  al, 1
0x18015D1B7  ret
```

`GameStatsManager::isItemUnlocked` (RVA `0x1E5820`) is hooked, but the bypass is not blanket:
it answers `true` only for the argument pair `(12, 16)` and forwards everything else to the
original. Corroborated by the stub at `0x18014BC22` → `0x1E5820`.

**UNVERIFIED:** what item type `12` and item id `16` denote (candidate: a song/music unlock
entry), and why the bypass is expressed as one specific pair rather than a range.

## Vault, shop and treasure room: the handlers are replaced

These hooks sit on the *menu* methods that open the layers, not on an unlock predicate, and
the detours do not call the original method — they **replace** it:

| Tag | Hook | Detour | What the detour does |
| ---: | --- | --- | --- |
| `0x021` | `CreatorLayer::onSecretVault` RVA `0x9E590` | `0x18015E2F0` | `call 0x1801AB820` (scene) → `movss xmm0,0.5f` → `call 0x180532EA0` → `call 0x180531990` (object handed to replaceScene) → `jmp 0x180532EB0` (replaceScene) |
| `0x021` | `OptionsLayer::onSecretVault` RVA `0x372CD0` | `0x18015E330` | same shape, scene builder `0x1801ACF70` |
| `0x022` | `SecretRewardsLayer::onShop` RVA `0x3D66D0` | `0x18015E010` | `call [rdx+0x38]` (virtual index getter) → `jmp 0x1801AD0F0` with `rcx=this`, `edx=index` |
| `0x023` | `CreatorLayer::onTreasureRoom` RVA `0x9EDA0` | `0x18015D9D0` | `xor ecx,ecx; call 0x1801FE3F0` (scene) → 0.5f → `0x180532EA0` → `0x180531990` → `jmp 0x180532EB0` |

Three facts are measured here rather than inferred:

1. **The transition duration is the same 0.5f constant everywhere.** Every `movss xmm0,[rip+…]`
   in the three vault/treasure detours resolves to `0x180534410`, whose bytes `00 00 00 3f` are
   `0.5f`. (Shop's detour has no transition of its own.)
2. **The factory is `CCTransitionFade::create`.** This is independently established from the
   No Transition work, where the detour on that same RVA `0x180532EA0` zeroes `xmm0` and jumps
   into the real body with the scene in `rdx`. The two findings corroborate each other: these
   unlock handlers build an *animated* fade that "No Transition" can suppress.
3. **The shop detour forwards to a different internal function**, `0x1801AD0F0` (real prologue
   `push rsi; push rdi; push rbx; sub rsp,0x20`), not to the hooked method at RVA `0x3D66D0`.
   It supplies `this` and the button's virtual index, which is the shape of "open this shop
   directly" rather than "run the handler that decides whether the shop is open".

**UNVERIFIED, and the reason this stays a lead:** whether GD's original handler contained the
unlock gate. Confirming it needs Geometry Dash's own binary, which is not part of this baseline.
What the bytes prove is that Mega Hack substitutes its own handler and drives the transition
itself, so nothing in the *replaced* path can gate on unlock state; what they do not prove is
that the replaced path was where the gate lived. The scene constructors are host functions
reached through the lazy binder, so their identity rests on the friendly names. The JSON keys
for all five tags remain UNVERIFIED.

## Method

1. `python3 scripts/tag_callback_map.py …` → 125 registrations; the five tags above were
   selected by their static labels from `notes/static-hack-label-map.md`.
2. `callback_hooks.scan` over each callback → hooks resolved by mangled symbol, then joined to
   `recovered/functions/_hook_sites.tsv` to recover the RVA (a cross-reference, not an
   independent observation) and to `lazy_stubs.stubs()` for an independent stub immediate.
3. Detour bodies disassembled with capstone from the instruction bytes.

## Open questions

- UNVERIFIED: the JSON keys for tags 32-36 (`0x020`-`0x024`).
- UNVERIFIED: whether Geometry Dash's original `onSecretVault`/`onShop`/`onTreasureRoom`
  contained the unlock gate the toggle removes; the detours replace those handlers, so the
  replaced path cannot gate, but the gate's original location is not observable here.
- UNVERIFIED: the identity of the scene constructors `0x1801AB820`, `0x1801ACF70`,
  `0x1801FE3F0` and the shop opener `0x1801AD0F0`; their names come from the friendly names
  of the hooks, not from the calls themselves.
- UNVERIFIED: what `(12, 16)` means to `GameStatsManager::isItemUnlocked`.
- The changelog twice describes these as "bypasses" condensed into individual mods, so there
  may be sibling tags for RobTop's vault (the "Keymaster Vault Room Bypass" row) that are not
  in this group — not searched this pass.
