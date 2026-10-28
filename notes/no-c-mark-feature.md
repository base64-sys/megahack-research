# No (C) Mark feature

- **Question** — which code implements the "No (C) Mark" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_C_MARK` (tag 44, `0x02C`) is registered as a hook in the large initializer
`FUN_1800afae0` (`0x1800afae0`) at `0x1800b049a`:

```
0x1800b049a  mov  edx, 0x2C           ; tag = HACK_NO_C_MARK
0x1800b049f  mov  rcx, rax            ; resolver
0x1800b04a2  mov  r8, rsi             ; context structure
0x1800b04a5  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180162d20` (`0x180162d20`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `ShareLevelLayer::onShare`
  (RVA `0x490270`) via `FUN_180162E10`, using the string
  `"ShareLevelLayer::onShare"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x02C | 44 | (registered via FUN_18008f910) | `HACK_NO_C_MARK` | Master toggle (No (C) Mark) |

## Method

1. Searched for byte pattern `ba 2c 00 00 00` (mov edx, 0x2C) in `_original.dll`
   using a Python byte scan. Found 2 occurrences; only one (`0x1800b049a`) in the
   Mega Hack code range.
2. Disassembled `FUN_1800afae0` around `0x1800b049a` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180162d20` (the callback) with REA/Ghidra 12.1.2 — hooks
   `ShareLevelLayer::onShare` (RVA `0x490270`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x02C.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes share events).
- UNVERIFIED: how ShareLevelLayer::onShare relates to the (C) mark on levels.
## Evidence - tag 48 HACK_VERIFY_HACK spoofs one counter around onShare (2026-10-28)

The second label that hooks `ShareLevelLayer::onShare` is tag 48 (`0x030`,
`HACK_VERIFY_HACK`, label map entry 81, store VA `0x18006478F`). It is registered in the same
initializer at `0x1800B06F2` (`ba 30 00 00 00`) with callback `FUN_180164F00`, a 110-byte body
whose only action is to install or remove one hook on host RVA `0x490270` (`FUN_180162E10`,
order `0xFFFFFFE0`, final argument 1). `notes/hook-registrations.md` line 78 already carried the
registration row; what was missing is what the detour does.

**The detour** `FUN_180164F70` (RVA `0x164F70`, file `0x164370`, 81 bytes, verified with
`tools/pe_disasm.py absolllute.hackmega_original.dll 0x180164F70 --size 84`):

```
0x180164f75  mov  rax, [rcx + 0x278]      ; inner object
0x180164f7c  mov  r8d, [rax + 0x2dc]      ; current value
0x180164f83  cmp  r8d, [rax + 0x2e0]      ; compare with the second value
0x180164f8a  jne  0x180164fb7             ; differ -> tail-jump to the original
0x180164f8c  inc  r8d
0x180164f8f  mov  [rax + 0x2dc], r8d      ; bump the current value
0x180164f99  call 0x1801FECD0             ; original ShareLevelLayer::onShare
0x180164fa5  mov  ecx, [rax + 0x2e0]
0x180164fab  mov  [rax + 0x2dc], ecx      ; restore it
```

`0x180164FB7` is `add rsp,0x20; pop rsi; jmp 0x1801FECD0`, so when the two integers already
differ the detour is a pure pass-through. The hook therefore makes one comparison inside the
share path see `current == max + 1` for the duration of the call and then restores the original
value.

**Conclusion.** Tag 48 is a share-path spoof, not a render or gameplay toggle, and it shares its
hook target with tag 44 (No (C) Mark). No `Added ...` changelog bullet names it, so it is a
shipped label outside the 389-row universe; it now has a row in
[feature-inventory.md](feature-inventory.md) under "Toggles outside the 389".

UNVERIFIED: what the two integers at `[inner+0x2DC]`/`[+0x2E0]` mean (a revision-versus-
current pair is the shape, but nothing in the image names them), and whether the equality
test is a duplicate-upload guard. Closing this needs GD 2.2081 `ShareLevelLayer` layout or a
runtime observation of a share with the toggle on and off.
