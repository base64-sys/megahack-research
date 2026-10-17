# Recovered source pipeline, and the "New Best" label

Two parts: the tooling that produces readable pseudocode for the whole image, and
the first feature mapped through it.

- **Question** — can the whole mod be read as source? And where does the New Best label
  get its content?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Correcting the brief first

The sample set does **not** contain a New Best label patch. A string diff of all three
experiment builds against the baseline shows they are all *persistence* variants:

| Build | SHA-256 | Strings added vs baseline |
| ----- | ------- | ------------------------- |
| `_original.dll` | `1635c339…` | — (baseline) |
| `_working_aslr.dll` | `3b448741…` | `best_runs.bin` |
| `_pre_persistence.dll` | `3b116767…` | `_test` |
| generated `.dll` | `24cebcac…` | `MHBRDB1`, `best_runs.bin`, `Import .dt`, `STATUS/BEST_RUN/DROPDOWN/PREFIX`, … |

The generated build is the sidecar-database + Death-Tracker-import patch described in
`RE_NOTES_PERSISTENT_BEST_RUN.md`, not a label change. `absolllute.hackmega.geode~\
absolllute.hackmega.dll` is byte-identical to the generated build, so it is not a separate
artifact either.

So the vibe-coded build is a *worked example of the patching technique*, which is still the
most useful thing in `samples/` — but the New Best behaviour had to be located from the
baseline, which is what the rest of this note does.

## The recovered source pipeline

Everything under `recovered/` is **gitignored**. It is Absolllute's pseudocode, produced by
a decompiler, and it is never committed. What is tracked is the *recipe*:

| File | Role |
| ---- | ---- |
| `scripts/decompile_all.py` | Drives Ghidra headless, writes the tree and a manifest |
| `scripts/ghidra_scripts/ExportAllFunctions.java` | Decompiles every function, one `.c` per function |

```powershell
python scripts/decompile_all.py --keep-project --reuse-project
```

Result on the baseline, Ghidra 12.1.2:

```
DONE total=19496 written=18822 failed=3 skipped=671
function files: 18825
total size    : 34.1 MB
elapsed       : 598 s
```

`skipped` is thunks and external thunks, which have no body worth writing. `failed` is 3
functions Ghidra could not decompile. Alongside the `.c` files:

| File | Size | Contents |
| ---- | ---- | -------- |
| `_index.tsv` | 996 KB | entry point, name, filename for every exported function |
| `_symbols.tsv` | 8.3 MB | every symbol with type, source and namespace |
| `_manifest.txt` / `.json` | small | image digest path, Ghidra version, elapsed, counts |

18,822 readable functions is the deliverable the project needed. Each file carries its VA,
RVA, signature, size and caller count in a header comment, so any claim can be traced back.

Note the Ghidra project must be kept (`--keep-project`) for `--reuse-project` to work; the
first run pays ~290 s of analysis and every later run is seconds.

## Ghidra 12 headless traps

Three, in order, because each cost a full run:

1. **A script outside Ghidra's script directories is not found.** `-postScript` with an
   absolute path gives `Failed to find source bundle containing script`. Pass
   `-scriptPath <dir>` as well. The directory *name* does not matter.
2. **A Java compile error is reported as a missing class.** `ExportAllFunctions not found by
   <hash> [1]` / `ClassNotFoundException` looked like a classloader or OSGi problem. It was a
   real compile error — an unreachable `return;` after a `throw`. Ghidra does not show the
   javac diagnostic.
3. **Verify scripts with javac before running Ghidra.** All 202 jars under the Ghidra install
   form the classpath:

   ```powershell
   $g = "C:\Users\Robert_\Downloads\ghidra_12.1.2_PUBLIC_20260605\ghidra_12.1.2_PUBLIC"
   $cp = (Get-ChildItem $g -Recurse -Filter *.jar | % FullName) -join ';'
   javac -nowarn -d out -cp $cp scripts\ghidra_scripts\ExportAllFunctions.java
   ```

Also: the Windows archive unpacks to a nested `ghidra_12.1.2_PUBLIC_20260605\ghidra_12.1.2_PUBLIC\`,
so `decompile_all.py` descends one level when locating the install.

## The "New Best" label: what it actually is

`PlayLayer::showNewBest` is a GD member name string at VA `0x1805759E9` with exactly one
code reference, at `0x180169E23`, inside the hook registrar at `0x180169DE0`. That
registrar installs **two** hooks on `PlayLayer`:

| Original member | Replacement | Enum arg |
| --------------- | ----------- | -------- |
| `PlayLayer::showNewBest` | `0x180169EB0` | `0xFFFFF428` |
| `PlayLayer::destroyPlayer` | `0x180169F20` | `0xFFFFFFE0` |

Both replacements share one global flag at VA `0x18080C378`: `showNewBest` sets it to 1,
`destroyPlayer` sets it to 0 and only acts if it was 1. So `destroyPlayer` is the
"a new best actually landed" moment, gated on `showNewBest` having fired.

Ghidra's output for `0x180169EB0` (`recovered/functions/180169eb0_FUN_180169eb0.c`):

```c
void FUN_180169eb0(undefined8 param_1)
{
  undefined8 uVar1;
  char cVar2;

  DAT_18080c378 = 1;
  uVar1 = FUN_18008a510();
  cVar2 = FUN_18008f420(uVar1,0xf5);
  if (cVar2 != '\0') {
    return;
  }
  uVar1 = FUN_180147ba0();
  cVar2 = FUN_180148610(uVar1,&DAT_180575a00);
  if (cVar2 == '\0') {
    FUN_18017f000(param_1,0x3ff0000000000000);
    return;
  }
  FUN_18017f000(param_1,0x3fe0000000000000);
  return;
}
```

Reading that against the disassembly and the `.rdata` dump:

| Detail | Value |
| ------ | ----- |
| `FUN_18008f420(this, 0xF5)` | `getConfigVar<bool>` for setting id **245**; if set, the whole path is suppressed |
| `DAT_180575A00` | the string **`0052`** — a numeric language-definition key, stored immediately after `PlayLayer::showNewBest\0` |
| key absent | tail-call with `0x3FF0000000000000` = **1.0** |
| key present | tail-call with `0x3FE0000000000000` = **0.5** |

The shared tail at `0x18017F000` then computes opacity:

```
xmm0 = maxsd(1.1920928955078125e-07, alpha)     ; 2^-23, i.e. FLT_EPSILON as a double
xmm0 = mulsd(xmm0, xmm7)                        ; scale, default 1.0
xmm6 = cvtsd2ss(xmm0, xmm0)                      ; final float opacity
```

where `xmm7` is replaced by `FUN_180003d80(this, 0x1F5)` when settings `0x1F6` (502) and
`0xF7` (247) are both on.

### What this means, and what it does not

**Verified:** Mega Hack hooks `PlayLayer::showNewBest` / `PlayLayer::destroyPlayer` purely
to compute an **opacity** for the New Best label. The label text is GD's; Mega Hack dims it.
Setting 245 suppresses it entirely. If the language dictionary defines key `0052`, the label
is drawn at half opacity — consistent with greying out a label whose translation is present
but not localised.

**Not here:** the session-vs-global distinction. This code chooses between `1.0` and `0.5`;
it never changes a string. The `STATUS/*/PREFIX` keys in the persistence patch are a
different subsystem (menu rows, not the in-game indicator).

UNVERIFIED: where the session-vs-global label text is chosen. The `STATUS/…` definition-key
scheme is the likely home, since that is how Mega Hack addresses labels — but the specific
key pair was not located. Next step: dump every definition key in `.rdata` and look for a
pair whose values differ between a session and an all-time best. `recovered/functions/
_symbols.tsv` plus a string inventory should find it.

## Conclusion

1. The whole mod is now readable: 18,822 decompiled functions, indexed, gitignored, and
   regenerable from two tracked files.
2. The New Best label is an opacity hook, not a text hook. That is the useful surprise — the
   session/global change is elsewhere.
3. The sample set's vibe-coded build is a persistence patch, useful as a technique example
   rather than as the feature.
4. Ghidra headless failures are misreported as class-loading errors; verify scripts with javac.

## Open questions

- Locate the session-vs-global definition keys. Start from a full `.rdata` string inventory
  grouped by the `STATUS/…` key shape.
- Identify settings 245, 247, 501 and 502 by name. Setting ids are passed straight into
  `FUN_18008f420`; a sweep for `mov edx, <id>` before that call would enumerate the whole
  registry.
- Decide what `FUN_180148610` means: key-present/definition-exists, or something narrower.
- Three functions failed to decompile: `0x180030600`, `0x1802FFDA0`, `0x180352640`.

## Failure modes recorded

- Two full decompile runs were wasted on a compile error reported as a classloading error,
  and one on a missing `-scriptPath`. Compile Ghidra scripts with javac first.
- A rip-relative target was computed from the instruction address instead of the next
  instruction, producing a wrong string. Always `target = insn.address + insn.size + disp`.
- Reading `Select-Object -Last` on a long-running command hid a run that was progressing
  normally and made it look hung. That is now an explicit rule in
  [AGENTS.md](../AGENTS.md#long-running-commands), and long commands go through
  `scripts\run_supervised.ps1`.