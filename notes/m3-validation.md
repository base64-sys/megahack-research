# M3 validation matrix

- **Question** — which reconstructed M3 modules have been validated against the
  release, and how?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.

## Sampled clusters

| Cluster | Evidence note | Reconstructed artefact | Validation status |
| --- | --- | --- | --- |
| Input hooks | [input-hooks.md](input-hooks.md) | hook dispatch mechanism | UNVERIFIED — structural only |
| Render hooks | [render-hooks.md](render-hooks.md) | hook dispatch mechanism | UNVERIFIED — structural only |
| Gameplay rewrites | [gameplay-hooks.md](gameplay-hooks.md) | hook dispatch mechanism | UNVERIFIED — structural only |
| UI add-ons | [ui-addons.md](ui-addons.md) | nxUI menu layer | UNVERIFIED — structural only |
| Save-data features | [save-data-hooks.md](save-data-hooks.md) | persistence lifecycle | UNVERIFIED — structural only |

## What has been validated

- Module entry and early-load path: compiling C++ tests in
  `reconstruction/tests/test_reconstruction.cpp`.
- Show Hitboxes: gate `FUN_180203EE0` (tag 192/216), draw `FUN_180203F60`→`FUN_180204000`/`FUN_180204150`, per-object `FUN_180203070`. Two changelog rows promoted to `mapped` (Show Hitboxes, Show Hitboxes on Death). Three Map A tags (0xC1/0xD9/0xDA) are runtime-assigned and UNVERIFIED.
- Settings registry: compiling C++ tests covering all five value types, listeners,
  action interception, and JSON round-tripping.
- Hook dispatch mechanism: structural evidence from 338 registration sites and
  shared helpers; no runtime comparison yet.
- Persistence format: byte scan and host-method inventory, not runtime behaviour.

## What remains

- Runtime comparison of reconstructed hook callbacks against the release's
  observable behaviour.
- Cluster assignment exists but is only a lead for most rows. Counted from the status
  column on 2026-10-26: **105 of the 389** changelog rows in
  [feature-inventory.md](feature-inventory.md) are `mapped` and **284** are `cluster`;
  the authoritative per-row list with tag, registration and hook address is the
  Mapped-rows table there. Four of the `cluster` rows (tags 422, 442, 438, 424) carry
  mechanism evidence but stay `cluster` because their tag→row binding is unresolved.
- The keyword check found **185 of 389** rows whose linked note never mentioned the
  feature at the 2026-10-23 baseline (method below). That number is a dated snapshot: it
  predates every promotion since, the original tokeniser was not kept, and it has not
  been re-run, so it should not be quoted as the current gap.
- Per-feature settings-key and default mapping for the remaining 284 `cluster` rows
  remains open. The mapped rows have tags and code VAs, but their JSON keys and shipped
  defaults are not all confirmed.
- Alias/dead classification for features absent from the current build remains open.

### Cluster-assignment check (2026-10-23)

Each row's changelog wording was tokenised (words longer than three characters,
common verbs and connectives removed) and searched case-insensitively in the text of
the note the row links to. 204 rows share at least one token with their note; **185
share none**. A shared token is not proof of coverage either — words like "show" and
"level" appear in almost every note — so even the 204 are leads, not findings. The
check measures how thin the cluster assignment is; it does not certify any row as
mapped.

## Conclusion

The sampled clusters are documented as patterns, but they are not yet validated
against release behaviour. This criterion remains open.
