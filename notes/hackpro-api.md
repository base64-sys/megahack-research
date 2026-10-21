# The `hackpro` extension API (exported surface)

- **Question** — what public interface does Mega Hack actually offer other mods, and what
  does its export table give away about internal structure?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`
  (see [provenance.md](provenance.md)).

## Headline finding: the export table is not stripped

The binary exports **30 named symbols with intact MSVC mangling**. For a commercial,
obfuscation-conscious product this is unusual, and it is by a wide margin the biggest lever
available on this project: namespace, class and function names that would otherwise have to
be inferred from disassembly are simply present in the PE export directory.

This changes M1/M2 planning. The RTTI/vtable recovery criterion in the roadmap ("Mega Hack's
own RTTI/vtable set enumerated; the class hierarchy recovered") is partly pre-solved at the
namespace level — `hackpro`, `hackpro::extensions`, `mega::utils::safe_mode` are known
names, not guesses.

## Method

1. `tools/pe_exports.py absolllute.hackmega_original.dll` — reads
   `pefile.PE(...).DIRECTORY_ENTRY_EXPORT`, printing preferred-base VA and export name.
   30 symbols.
2. Demangling with `undname.exe` from VS 2022 BuildTools 14.44.35207
   (`VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\undname.exe`), the Microsoft reference
   undecorator. Used because no `llvm-undname`, `c++filt` or Python demangler was available
   on this machine.
3. Second angle on the addresses: `tools/pe_disasm.py` at two sampled exports to confirm the
   VAs land on real function prologues rather than arbitrary offsets.

Demangled signatures are reproduced below with `std::basic_string<char>` collapsed to
`std::string` and `__ptr64` noise stripped. That is a presentation choice only; the mangled
names are in [Evidence](#evidence--full-export-table).

## Evidence — full export table

### Entry point

| VA | Demangled |
| -- | --------- |
| `0x180072B60` | `geodeImplicitEntry` (C linkage, no mangling) |

### `hackpro::extensions` — config variables

Five value types, each with a getter, setter and change listener. The type is a template
parameter and each instantiation is separately exported.

| VA | Demangled |
| -- | --------- |
| `0x180073880` | `bool getConfigVar<bool>(std::string const&)` |
| `0x1800745C0` | `double getConfigVar<double>(std::string const&)` |
| `0x180073F20` | `__int64 getConfigVar<__int64>(std::string const&)` |
| `0x180074D10` | `std::string getConfigVar<std::string>(std::string const&)` |
| `0x180075430` | `std::vector<unsigned int> getConfigVar<std::vector<unsigned int>>(std::string const&)` |
| `0x180073800` | `void setConfigVar<bool>(std::string const&, bool const&)` |
| `0x180074540` | `void setConfigVar<double>(std::string const&, double const&)` |
| `0x180073EA0` | `void setConfigVar<__int64>(std::string const&, __int64 const&)` |
| `0x180074BD0` | `void setConfigVar<std::string>(std::string const&, std::string const&)` |
| `0x180075310` | `void setConfigVar<std::vector<unsigned int>>(std::string const&, std::vector<unsigned int> const&)` |
| `0x180073260` | `void registerListener<bool>(std::string const&, std::function<void(Tag, bool const&)>)` |
| `0x180073FA0` | `void registerListener<double>(std::string const&, std::function<void(Tag, double const&)>)` |
| `0x180073900` | `void registerListener<__int64>(std::string const&, std::function<void(Tag, __int64 const&)>)` |
| `0x180074630` | `void registerListener<std::string>(std::string const&, std::function<void(Tag, std::string const&)>)` |
| `0x180074D70` | `void registerListener<std::vector<unsigned int>>(std::string const&, std::function<void(Tag, std::vector<unsigned int> const&)>)` |

### `hackpro::extensions` — key/tag identity

| VA | Demangled |
| -- | --------- |
| `0x180072DF0` | `std::optional<hackpro::Tag> tagFor(std::string const&)` |
| `0x180072E20` | `std::string keyFor(hackpro::Tag)` |

### `hackpro::extensions` — enable state, listeners, UI

| VA | Demangled |
| -- | --------- |
| `0x180072FF0` | `bool isHackEnabled(std::string const&)` |
| `0x180073070` | `void setHackEnabled(std::string const&, bool)` |
| `0x1800730F0` | `void registerHackListener(std::string const&, std::function<void(Tag, bool const&)>)` |
| `0x180072E80` | `void registerActionListener(std::string const&, std::function<void(Tag)>)` |
| `0x180072E50` | `bool registerTab(std::istream<char>&, hackpro::extensions::Delegate*)` |
| `0x180075530` | `void showMessage(hackpro::MessageType, std::string const&, std::function<void()>)` |
| `0x1800754F0` | `void showError(std::string const&)` |
| `0x1800754C0` | `void addDefinition(std::string const&, std::string const&)` |

### `hackpro` — root namespace

| VA | Demangled |
| -- | --------- |
| `0x1800A3C40` | `std::string const& hackpro::define(std::string const&)` |

### `mega::utils::safe_mode`

| VA | Demangled |
| -- | --------- |
| `0x18019B350` | `bool mega::utils::safe_mode::areCheatsEnabled()` |
| `0x18019B450` | `void mega::utils::safe_mode::disableAllCheats()` |
| `0x18019B5A0` | `bool mega::utils::safe_mode::hasCheatedInAttempt()` |

### Second angle — addresses land on real code

`tools/pe_disasm.py absolllute.hackmega_original.dll 0x180072b60 --size 0x60`:

```
0000000180072b60  sub  rsp, 0x68
0000000180072b64  mov  rax, qword ptr [rip + 0x7844d5]
0000000180072b6b  xor  rax, rsp
0000000180072b6e  mov  qword ptr [rsp + 0x60], rax     ; __security_cookie
0000000180072b73  mov  rcx, qword ptr [rip + 0x78e396]
0000000180072b7a  test rcx, rcx
0000000180072b7d  je   0x180072bc6
0000000180072b7f  call qword ptr [rip + 0x5d0223]        ; IAT
0000000180072b85  test al, al
...
```

`tools/pe_disasm.py absolllute.hackmega_original.dll 0x18019b350 --size 0x40`:

```
000000018019b350  push rsi
000000018019b351  push rdi
000000018019b352  push rbx
000000018019b353  sub  rsp, 0x40
000000018019b357  call 0x18008a510
```

Both are genuine function entries: the first carries an MSVC `/GS` stack-cookie prologue and
the standard epilogue check, the second a normal three-register save. The export VAs are not
pointers into mid-function data.

## Evidence — spatial layout

| Region | VA span | Size |
| ------ | ------- | ---- |
| `geodeImplicitEntry` + `hackpro` + `hackpro::extensions` (26 named symbols) | `0x180072B60`–`0x180075530` | `0x29D0` (10,704 B) |
| `mega::utils::safe_mode` (3 symbols) | `0x18019B350`–`0x18019B5A0` | `0x250` (592 B) |
| `hackpro::define` (1 symbol) | `0x1800A3C40` | — |

30 named exports in total: 26 in the block above, 3 `safe_mode` functions, and
`hackpro::define`.

The whole extension API lives in one contiguous ~10 KB block, consistent with a single
translation unit. The three `safe_mode` functions sit within 0x250 bytes of each other,
consistent with one small source file.

UNVERIFIED: that the block is exactly one translation unit. Confirming needs the
`.pdata` unwind table to check for a single contiguous `RUNTIME_FUNCTION` region, or
`findAsciiReferences`/string evidence for a per-file marker.

## What the signatures imply

These are inferences from the signatures, not verified behaviour.

- **The config system is string-keyed and typed.** Five supported value types: `bool`,
  `double`, `int64`, `std::string`, `std::vector<unsigned int>`. Keys are
  `std::string`; values are always passed and returned by `const&`, including the `bool`
  and `double` cases.
- **`hackpro::Tag` is an opaque interned identifier** for a config key. `tagFor` maps
  `string -> optional<Tag>` — optional, so an unknown key has no tag. `keyFor` maps back.
  Listeners receive `Tag`, not the string, which suggests tags are cheap to compare and
  that lookups are interned.
- **Change notification is per-key and typed.** `registerListener<T>(key, fn)` binds to one
  key and one type; the callback gets `(Tag, T const&)`.
- **Enable state is a separate axis from config values.** `isHackEnabled` /
  `setHackEnabled` / `registerHackListener` mirror the config triple but are distinctly
  named and separately exported, and `setHackEnabled` takes `bool` **by value** while
  `setConfigVar<bool>` takes `bool const&`. So "is this hack on" is not just a config value
  — it is likely derived from several settings plus runtime context (safe mode, level type).
- **`registerTab(std::istream&, Delegate*)` is deserialisation-driven.** A tab is registered
  by reading a stream and handing it a `Delegate`. That implies a serialised tab-description
  format — worth recovering next, because it is the input side of the UI and therefore the
  format a third-party mod would need.
- **`hackpro::define(string) -> string const&` is an interning lookup**, returning a
  reference rather than a value, consistent with a string pool. `addDefinition(string
  const&, string const&)` is the writer. Together these look like the localisation /
  definition registry that the `STATUS/*/PREFIX` hook in
  `RE_NOTES_PERSISTENT_BEST_RUN.md` manipulates.
- **`showMessage(MessageType, string const&, function<void()>)` carries an action callback**,
  and `MessageType` is an enum — so dialogs can offer a follow-up action.

## Relationship to the `safe_mode` namespace and this project's ethics line

`mega::utils::safe_mode` is an **exported, shipped** safety feature:
`areCheatsEnabled()`, `disableAllCheats()`, `hasCheatedInAttempt()`. Mega Hack ships a
mechanism that disables cheat features and tracks whether the current attempt used them.

This is directly relevant to [AGENTS.md](../AGENTS.md), which forbids patches that improve
gameplay or help gain an advantage in ranked play. That rule is not merely a policy
imposed from outside: the product itself exports a ranked-play safety mechanism, and
`hasCheatedInAttempt()` implies cheats are attributed per attempt and presumably surfaced to
the leaderboard. Any reconstruction in this repo must preserve this behaviour rather than
work around it.

UNVERIFIED: that `safe_mode` is what gates ranked play specifically. Confirming needs
callers of `0x18019B350` and string references to ranked/safe-mode terminology via
`tools/pe_xrefs.py`. The *existence* of the three functions is verified by the export table.

## Conclusion

1. Mega Hack ships a real, deliberate public extension API under `hackpro` — a typed,
   string-keyed, change-notifying config system, plus UI hooks and a serialised tab
   registration path. This is the highest-value recoverable knowledge in the project and is
   reusable by any Geometry Dash mod author.
2. `mega::utils::safe_mode` is a shipped per-attempt cheat-safety mechanism and must be
   reproduced faithfully, not circumvented.
3. `geodeImplicitEntry` at `0x180072B60` is the mod lifecycle root and the correct starting
   point for M2.
4. Because symbols survive, M1's RTTI criterion is easier than planned and M2's naming is
   largely pre-solved. The roadmap should be re-weighted accordingly.

## Open questions

- (Resolved 2026-10-20.) `registerTab`'s `std::istream` wraps one of two compressed
  `CompressedBuffer` blobs (`DAT_1806FE004`/`DAT_1806FE2D9`), inflated by `FUN_1800BE770`.
  The uncompressed grammar is still unknown; see notes/tab-registration.md.
- (Partially resolved 2026-10-20.) `hackpro::MessageType` is passed as 0, 1 or 2 in the 15
  call sites of `showMessage` (9 of value 2, 5 of value 1, 1 of value 0). UNVERIFIED: the
  semantic name of each enumerator (info/warning/error or similar). The layout of
  `hackpro::Tag` (64-bit integer) and `hackpro::extensions::Delegate` remains unmapped.
  (Resolved 2026-10-20: `hackpro::Tag` is a 64-bit integer passed by value and used as a
  map key; no further fields are visible in this binary.)
- (Partially resolved 2026-10-20.) The 26 symbols in the `0x180072B60`–`0x180075530`
  block (25 `hackpro`/`hackpro::extensions` plus `geodeImplicitEntry`) occupy a single
  contiguous range (10,704 bytes) with no interleaving functions from
  other namespaces, which is consistent with one TU. UNVERIFIED: a compiler-level proof
  (e.g. a single .pdata range or identical prologue padding) rather than contiguity.
- (Resolved 2026-10-20.) `define` at `0x1800A3C40` is not a counterpart config API: it is
  the string-interning entry, forwarding to `FUN_18009B1C0(&DAT_180801308, param_1)` behind
  a thread-safe static initialiser that constructs the registry at `DAT_180800F58`. It is
  the mechanism behind UI widget name interning, not a settings accessor. UNVERIFIED: where
  the 160+ feature toggles' internal calls land; they are not in the export table and are not
  this function.
- (Resolved 2026-10-20.) Callers of the three `safe_mode` functions: `areCheatsEnabled`
  (`0x18019B350`) is called by `FUN_180090BC0`, `FUN_1801789C0` and the shared gate
  `FUN_18019B5F0`; `disableAllCheats` (`0x18019B450`) is called by `FUN_180090BC0` and
  `FUN_180027C30`. UNVERIFIED: which of these callers is the ranked-play gate, and what
  `FUN_180090BC0`/`FUN_1801789C0` represent. `FUN_18019B450` inside `FUN_180027C30`
  (a settings setter) suggests cheat-disable is triggered by a config write, not by
  gameplay.

## Tooling note

`undname.exe` from VS 2022 BuildTools is the only working MSVC demangler on this machine:

```powershell
undname.exe "??$getConfigVar@N@extensions@hackpro@@YANAEBV?$basic_string@..."
```

It prints a copyright banner and wraps the result in `Undecoration of :- "..." is :- "..."`,
so any wrapper around it should extract the quoted portion. This is a good candidate for
promotion to `scripts/` if a second investigation needs demangled exports — it is
reusable, it is not Absolllute's code, and it is currently absent from the shipped `.geode`
(see [provenance.md](provenance.md#open-questions)).