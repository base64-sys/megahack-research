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

## Vault, shop and treasure room: entry points redirected

These three hooks sit on the *menu* methods that open the layers, not on an unlock predicate:

| Tag | Hooks | Detour | Behaviour read so far |
| ---: | --- | --- | --- |
| `0x021` | `CreatorLayer::onSecretVault` (RVA `0x9E590`), `OptionsLayer::onSecretVault` (`0x372CD0`) | `0x18015E2F0`, `0x18015E330` | builds a scene (`0x1801AB820` / `0x1801ACF70`), calls the transition factory `0x180532EA0` |
| `0x022` | `SecretRewardsLayer::onShop` (RVA `0x3D66D0`) | `0x18015E010` | calls `[arg2+0x38]` (a virtual getter), then tail-calls with its value |
| `0x023` | `CreatorLayer::onTreasureRoom` (RVA `0x9EDA0`) | `0x18015D9D0` | builds a scene (`0x1801FE3F0`), calls `0x180532EA0`, then `0x180531990` |

The bodies are only partly read, and the layer constructors they call are host functions
reached through Mega Hack's own lazy binder. **UNVERIFIED:** which layer each detour opens
(the likely reading is "open the layer without the unlock check", but the constructor
arguments are not resolved here), whether they replace or supplement the original call, and
the JSON keys for all five tags.

## Method

1. `python3 scripts/tag_callback_map.py …` → 125 registrations; the five tags above were
   selected by their static labels from `notes/static-hack-label-map.md`.
2. `callback_hooks.scan` over each callback → hooks resolved by mangled symbol, then joined to
   `recovered/functions/_hook_sites.tsv` to recover the RVA (a cross-reference, not an
   independent observation) and to `lazy_stubs.stubs()` for an independent stub immediate.
3. Detour bodies disassembled with capstone from the instruction bytes.

## Open questions

- UNVERIFIED: the JSON keys for tags 32-36 (`0x020`-`0x024`).
- UNVERIFIED: whether the three layer-entry detours substitute a "bypassed" layer or merely
  change the transition, and what `0x180532EA0`'s duration argument is in each case.
- UNVERIFIED: what `(12, 16)` means to `GameStatsManager::isItemUnlocked`.
- The changelog twice describes these as "bypasses" condensed into individual mods, so there
  may be sibling tags for RobTop's vault (the "Keymaster Vault Room Bypass" row) that are not
  in this group — not searched this pass.
