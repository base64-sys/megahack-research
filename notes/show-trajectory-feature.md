# Show Trajectory feature cluster

- **Question** — which code and settings implement Mega Hack's Show Trajectory?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

| Setting key | Observed value | Role |
| --- | --- | --- |
| `LEVEL/SHOW_TRAJECTORY/DROPDOWN/ITERATIONS` | 130 | Number of trajectory iterations |
| `LEVEL/SHOW_TRAJECTORY/DROPDOWN/THICKNESS` | 0.4 | Trajectory line thickness |

Changelog features include:

- "Added Show Trajectory iterations/thickness options"
- "Added CBS Tick Highlighting in Show Hitbox Trail"

## Evidence — implementation

- `ShowTrajectory` is a class with vtables at `0x180181C80`
  (`FUN_180181C80`) and `0x180182810` (`FUN_180182810`).
- `FUN_180182390` (`0x180182390`) compares against
  `s___AVShowTrajectory___1807f7f10`, so it identifies the class by RTTI.
- The constructor at `0x180181C80` allocates the object and installs vtables.

## Open questions

- UNVERIFIED: which settings tags correspond to `ITERATIONS` and `THICKNESS`.
- UNVERIFIED: which render hook draws the trajectory and whether it reuses
  `CCDrawNode` or a custom primitive.
- UNVERIFIED: whether "CBS Tick Highlighting" is a separate setting or a mode
  of Show Trajectory.
- UNVERIFIED: whether trajectory rendering is gated by Show Hitboxes.
