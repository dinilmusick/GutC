# GutTS → GutC upstream parity ledger

Pinned upstream: `dinilmusick/GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c`.

This file is an **in-progress** inventory, not a full parity certification. It is derived from the public barrel `src/index.ts` plus the implementations listed below. Every row requires behavioral comparison and a passing C fixture before being upgraded to full parity.

| Upstream export / behavior | Native C API | Implementation | Test / fixture | Result |
| --- | --- | --- | --- | --- |
| `identityIds.isIdentityKind` exact six kinds | `gutc_identity_kind_valid` | `src/gutc.c` | `tests/test_gutc.c` | Covered |
| `identityIds.parseIdentityId` 1–256 ASCII identifier | `gutc_identity_id_valid` | `src/gutc.c` | `testdata/upstream-golden/identity.tsv` | Covered (validation only; no tagged return) |
| `urls.joinUrl` trim outer slashes | `gutc_join_url` | `src/gutc.c` | `tests/test_gutc.c` | Covered for C strings |
| `strings.startsWithAny` | `gutc_string_starts_with_any` | `src/gutc.c` | `tests/test_gutc.c` | UTF-8 byte-safe; non-normalizing |
| `strings.endsWithAny` | `gutc_string_ends_with_any` | `src/gutc.c` | `tests/test_gutc.c` | UTF-8 byte-safe; non-normalizing |
| `strings.isBlank` / `isNonBlank` | `gutc_string_blank_ascii` / negation | `src/gutc.c` | `tests/test_gutc.c` | **Partial:** ASCII whitespace; JS Unicode trim outstanding |
| `numbers.clamp` | `gutc_number_clamp` | `src/gutc.c` | `tests/test_gutc.c` | Finite-only; nonfinite JS semantics outstanding |
| `numbers.inRange` | `gutc_number_in_range` | `src/gutc.c` | `tests/test_gutc.c` | Half-open interval; finite-only |
| `numbers.sum` | `gutc_number_sum` | `src/gutc.c` | `tests/test_gutc.c` | Finite-only / overflow detectable |
| `arrays.chunk` | `gutc_array_chunk_count` | `src/gutc.c` | `tests/test_gutc.c` | **Partial:** chunk count only |
| `auth.checkInvocationBudget` | `gutc_invocation_budget_check` | `src/gutc.c` | `tests/test_gutc.c` | **Partial:** pure budgets, not full envelope validation |
| `auth.advanceInvocationBudget` | `gutc_invocation_budget_advance` | `src/gutc.c` | `tests/test_gutc.c` | **Partial:** remainingCalls only |
| `urls.encodeSafePathSegments` | not on this branch; previous PR #1/#3 | — | — | Unmerged |
| `urls.normalizeStoragePath` | prior GutC PR #1 | — | — | Unmerged |
| `urls.resolveTrustedApiUrl` | prior GutC PR #3 | — | — | Unmerged |
| `strings.truncate` | prior GutC PR #4 | — | — | Unmerged; Lodash grapheme mismatch |
| `strings.truncateWords` | prior GutC PR #4 | — | — | Unmerged |
| `arrays.compact/unique/flatten/last` | — | — | — | Not implemented |
| `numbers.roundTo`, `numbers.isFiniteNumber` | — | — | — | Not implemented |
| `objects.hasOwn/pick/omit/isPlainObject` | — | — | — | Not implemented |
| `strings.isEmail/capitalize/camelCase/kebabCase/snakeCase` | — | — | — | Not implemented |
| `urls.isUrl/getQueryParam/setQueryParam` | — | — | — | Not implemented |
| `json.parseJson/tryParseJson/stringifyJson/isJsonValue` | — | — | — | Not implemented |
| `auth.identityRef/parseIdentityRef/parseLegacyApplicationId` | — | — | — | Not implemented |
| `auth.validateCapabilityDescriptor` | — | — | — | Not implemented |
| `auth.validateScopedInvocation` | — | — | — | Not implemented |
| `auth.consumeInvocationRetry/validateInvocationAgainstDescriptor` | — | — | — | Not implemented |
| `auth OAuth/client/session` structures and adapters | — | — | — | Not implemented; transport host-specific |
| `models`, `semantic-input`, `engineering`, `api` published exports | — | — | — | Requires full export audit; runtime model adapters are host-specific |

Additional native primitives with no exact GutTS exported counterpart: `gutc_utf8_validate`, `gutc_percent_encode`, `gutc_percent_decode`, `gutc_array_chunk_count`. Percent encoding matches JavaScript encodeURIComponent on **valid UTF-8** for unreserved bytes but binary inputs are an intentional C extension. Percent decoding is byte-oriented; malformed percent triplets fail, while arbitrary decoded binary remains permitted.

## Memory / failure contract

All APIs are allocation-free, reentrant, immutable-input functions; the library has no initializer or cleanup. Every provided output buffer is caller-owned. Sizing calls pass `output=NULL, capacity=0` and collect `needed`. When a textual buffer is present, capacity must accommodate `needed+1` bytes, including NUL. `GUTC_NO_SPACE` does not write partially. Invalid arguments and malformed UTF-8/percent inputs return status errors; callers must not read output parameters unless the call succeeds (except `needed` after a valid sizing or capacity check). Native C cannot validate arbitrary invalid pointers. All C string inputs must be readable, NUL-terminated, and under 1 MiB. Aliased input/output buffers are unsupported.

**Open gates:** full exported-symbol inventory, native object/JSON implementations, end-to-end auth schema validation, source-executed cross-language fixtures, Windows/macOS proof, sanitizer and independent exact-head release, merging all relevant PRs, and canonical main consumer certification.
