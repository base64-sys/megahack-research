# Geometry Dash local save and level keys (external reference)

- **Source** — GD Docs, "Client Level Resource" (`https://boomlings.dev/resources/client/level`) and
  "Client Level String Resource" (`https://boomlings.dev/resources/client/level-components/level-string`),
  read 2026-10-27. Community documentation, not a shipped artifact.
- **Use** — a *lead*, in the sense AGENTS.md allows: it says which key means what, and every claim
  taken from it still has to be confirmed by observation before it enters a conclusion.
- **Why it matters here** — the runtime-validation fixture is a generated level, and the fixture
  generator was writing a level dict without knowing what its keys mean.

## Container keys

| Key | Meaning | Type |
| --- | --- | --- |
| `kCEK` | encoder class; `4` is a level | integer |
| `k1` | level ID | integer |
| `k2` | level name (player data — never recorded in a trace) | string |
| `k3` | description, base64 | string |
| `k4` | the level string (objects) | string |
| `k5` | creator name (player data) | string |
| `k8` | official song ID | integer |
| `k13` | isEditable | bool |
| `k16` | level version | integer |
| `k18` | attempts | integer |
| `k19` / `k20` | normal / practice best percentage | integer |
| `k21` | **levelType: 1 Official, 2 Local, 3 Saved, 4 Online** | integer |
| `k23` | length enum | integer |
| `k36` | jumps | integer |
| `k42` | ID of the original level when this is a copy | integer |
| `k45` | custom song ID | integer |
| `k47` | hasBeenModified | bool |
| `k48` | **object count** | integer |
| `k50` | binary version | integer |
| `k80` / `k81` | seconds spent editing (current / copies) | integer |
| `k95` | verification time in physics steps (240/s) | integer |
| `k101` | saved editor camera positions | string |
| `k104` | song list | string |
| `kI1`–`kI3` | editor camera x, y, zoom | float |
| `kI6` | editor build tab pages | **dictionary** |
| `kI7` | editor layer | float |

## What this corrected in our own notes

1. **`<t />` is `true`, not null.** GD's plist dialect has no null; a boolean is `<t />` (true) and
   the absence of the key is the false/null case. `notes/gd-save-format.md` had read `k13` and
   `k47` as "null".
2. **`k21 = 2` means Local.** A level with no `k21` was listed by the game as an *online* level —
   its only button was "Get it". That is consistent with an unset type defaulting to online, and it
   is the one behaviour a run has actually shown.

## What this does **not** establish

The keys' *requiredness*. The docs describe what each key means when present; they do not say
which are mandatory for the client to accept and list a level, and the two observations we have are
contradictory in exactly that respect:

- a dict of `kCEK`/`k2`/`k4`/`k5` → the level is listed, classified as online;
- the same dict plus `k21 = 2` → **no levels listed at all**.

That contradiction is not resolved by documentation. It needs either the client's own decoder (static
analysis of `GeometryDash.exe`) or a level the game wrote itself for a *local* level (a level created
in the editor is a structural reference, not a fixture: the fixture stays generated).
