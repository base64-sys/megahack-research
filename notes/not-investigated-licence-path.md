# Not investigated: the licence validation path

- **Status** — deliberately not pursued. Recorded so the next person does not spend an hour
  re-discovering the wall.
- **Date** — 2026-10-17.

## What prompted it

`notes/rtti-classes.md` listed `mega::utils::EncryptedBuffer` as unexamined and worth a look
because of the persistence work. A first pass over the string index showed:

| Evidence | Where |
| -------- | ----- |
| `ENCRYPTED PRIVATE KEY` | string index |
| `AES-128-ECB`, `AES-128-CBC`, ... `id-aes256-wrap` | full OpenSSL cipher name table |
| `OPENSSL_sk_insert`, `openssl-version`, `OPENSSL_MODULES` | OpenSSL statically linked |
| `/api/mega_hack/v9/login`, `/api/mega_hack/v8/logout_all` | from [sso-decoder.md](sso-decoder.md) |
| `Error validating license` | string index |
| `Failed to write license` | string index |
| `Serial Number:` | string index |
| `license`, `LICENSE`, `activate` | string index |

That combination is a licence and entitlement system: a login endpoint, a locally cached and
validated licence file, a serial number, and statically linked OpenSSL for the crypto.

## Why this stops here

`AGENTS.md` is explicit:

> Do not reverse, bypass or work around licence checks, DRM, or entitlement checks.

This is the licence check. Mapping the validation flow, the key handling, the serial format or
the request/response shape would be precisely the work the rules exclude, and unlike the rest
of this reconstruction it has no interoperability or preservation justification — nothing here
is needed to read the mod's behaviour, its hooks or its file formats.

So: no disassembly of the validation routine, no schema for the licence file, no analysis of
the key material or the request format. The strings above are recorded only to explain the
decision, not as a map to work from.

## What is still fair game around it

The boundary is about the entitlement check specifically, not the whole neighbourhood:

- `mega::utils::CompressedBuffer` is compression, not licensing. If the persistence work ever
  needs it, it is in scope.
- Save/load formats for level data, progress and the `best_runs.bin` sidecar are in scope, and
  are covered by M1 and M4.
- The login endpoint's *existence* is already recorded, and stays a static observation. Nothing
  in this project should ever be pointed at a live service.

## If this ever needs revisiting

Only if the user is the licence holder and asks for interoperability with their own
entitlement system, and even then the ask should be scoped to what they own. It is not a
backlog item, and it is not a "blocked" item — it is out of scope by policy.

## Note on the earlier `tabcore` finding

Nothing in `tabcore` was found to be licence-related. It is a UI namespace and stays fully in
scope.