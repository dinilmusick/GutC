# GutTS → GutC upstream parity ledger

Pinned upstream: `dinilmusick/GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c`.

This file is an **in-progress** inventory, not a full parity certification. `testdata/upstream-golden/contracts.tsv` is executed independently against **both** the pinned, compiled original GutTS source (`scripts/verify-gutts-golden.mjs`) and the C implementations (`tests/test_cross_language.c`), under the dedicated GitHub Actions golden workflow. It is derived from the public barrel `src/index.ts` plus the implementations listed below. Every row requires behavioral comparison and a passing C fixture before being upgraded to full parity.

| Upstream export / behavior | Native C API | Implementation | Test / fixture | Result |
| --- | --- | --- | --- | --- |
| `identityIds.isIdentityKind` exact six kinds | `gutc_identity_kind_valid` | `src/gutc.c` | `tests/test_gutc.c` | Covered |
| `identityIds.parseIdentityId` 1–256 ASCII identifier | `gutc_identity_id_valid` | `src/gutc.c` | `testdata/upstream-golden/identity.tsv` | Covered (validation only; no tagged return) |
| `urls.joinUrl` trim outer slashes | `gutc_join_url` | `src/gutc.c` | `tests/test_gutc.c` | Covered for C strings |
| `strings.startsWithAny` | `gutc_string_starts_with_any` | `src/gutc.c` | `tests/test_gutc.c` | UTF-8 byte-safe; non-normalizing |
| `strings.endsWithAny` | `gutc_string_ends_with_any` | `src/gutc.c` | `tests/test_gutc.c` | UTF-8 byte-safe; non-normalizing |
| `strings.isBlank` / `isNonBlank` | `gut_is_blank_utf8` / `gut_is_nonblank_utf8` | `src/gut_string_predicates.c` (PR #4) | `tests/test_string_predicates.c` | Unicode JS whitespace supported; malformed UTF-8 error |
| `numbers.clamp` | `gutc_number_clamp` | `src/gutc.c` | `tests/test_gutc.c` | Finite-only; nonfinite JS semantics outstanding |
| `numbers.inRange` | `gutc_number_in_range` | `src/gutc.c` | `tests/test_gutc.c` | Half-open interval; finite-only |
| `numbers.sum` | `gutc_number_sum` | `src/gutc.c` | `tests/test_gutc.c` | Finite-only / overflow detectable |
| `arrays.chunk` | `gutc_array_chunks` | `src/collections.c` | `tests/test_collections.c` | Borrowed slice views with count and no copying |
| `auth.checkInvocationBudget` | `gutc_invocation_budget_check` | `src/gutc.c` | `tests/test_gutc.c` | **Partial:** pure budgets, not full envelope validation |
| `auth.advanceInvocationBudget` | `gutc_invocation_budget_advance` | `src/gutc.c` | `tests/test_gutc.c` | **Partial:** remainingCalls only |
| `urls.encodeSafePathSegments` | `gutc_encode_safe_path_segments` | `src/safe_path.c` | `tests/test_safe_path.c` | Implemented for bounded UTF-8 with JS URI punctuation; nested encoding validated |
| `urls.normalizeStoragePath` | `gutc_normalize_storage_path` | `src/gutc_path.c` imported PR #1 | `tests/test_path.c` | Implemented, distinct error status |
| `urls.resolveTrustedApiUrl` | `gutc_resolve_trusted_api_url` | `src/trusted_api.c` imported PR #3 | `tests/trusted_api_test.c` | Partial WHATWG URL normalization |
| `strings.truncate` | `gut_truncate_utf8` | `src/gut_truncate.c` imported PR #4 | `tests/test_truncate.c` | Partial: Lodash grapheme segmentation differs from scalar count |
| `strings.truncateWords` | `gut_truncate_words_utf8` | `src/gut_truncate_words.c` imported PR #4 | `tests/test_truncate_words.c` | Implemented for UTF-8 |
| `arrays.compact` / `unique` / `flatten` / `last` | `gutc_array_compact` / `gutc_array_unique` / `gutc_array_flatten_once` / `gutc_array_last` | `src/collections.c` | `tests/test_collections.c` | Borrowed typed values; falsy filter, SameValueZero primitive and explicit reference identity; not JS arbitrary values |
| `numbers.roundTo` / `isFiniteNumber` | `gutc_number_round_to` / `gutc_number_is_finite` | `src/collections.c` | `tests/test_collections.c` | Finite-only; roundTo 0–15 digits and difficult Lodash ties may differ |
| `objects.hasOwn` / `pick` / `omit` | `gutc_object_has_direct` / `gutc_object_pick_direct` / `gutc_object_omit_direct` | `src/collections.c` | `tests/test_collections.c` | **Partial:** direct-key ordered maps only, nested paths not implemented |\n| `objects.isPlainObject` | — | — | — | Not implemented |
| `strings.isEmail/capitalize/camelCase/kebabCase/snakeCase` | — | — | — | Not implemented |
| `urls.isUrl/getQueryParam/setQueryParam` | — | — | — | Not implemented |
| `json.parseJson/tryParseJson` | `gutc_json_validate` (syntax only) | `src/json.c` | `tests/test_json.c`, `testdata/upstream-golden/json.tsv` | **Partial:** validation, no parsed AST or exception semantics |
| `json.stringifyJson` | `gutc_json_quote_string` (strings only) | `src/json.c` | `tests/test_json.c` | **Partial:** no object/array serializer |
| `json.isJsonValue` | — | — | — | Not implemented |
| `auth.identityRef/parseIdentityRef/parseLegacyApplicationId` | `gutc_identity_ref_parse` / `gutc_legacy_application_id` | `src/identity.c` | `tests/test_identity.c` | Borrowed tagged identity views and conflict-rejecting aliases; legacy requires explicit map entries |
| `auth.validateCapabilityDescriptor` | — | — | — | Not implemented |
| `auth.validateScopedInvocation` | — | — | — | Not implemented |
| `auth.consumeInvocationRetry` | `gutc_invocation_budget_retry` | `src/identity.c` | `tests/test_identity.c` | Pure retry budget, not full invocation envelope |\n| `auth.validateInvocationAgainstDescriptor` | — | — | — | Not implemented |
| `auth OAuth/client/session` structures and adapters | — | — | — | Not implemented; transport host-specific |
| `models`, `semantic-input`, `engineering`, `api` published exports | — | — | — | Requires full export audit; runtime model adapters are host-specific |

Additional native primitives with no exact GutTS exported counterpart: `gutc_utf8_validate`, `gutc_percent_encode`, `gutc_percent_decode`, `gutc_array_chunk_count`. Percent encoding matches JavaScript encodeURIComponent on **valid UTF-8** for unreserved bytes but binary inputs are an intentional C extension. Percent decoding is byte-oriented; malformed percent triplets fail, while arbitrary decoded binary remains permitted.

## Memory / failure contract

The core `gutc_` byte/span APIs are allocation-free and reentrant; `gutc_normalize_storage_path` allocates temporary per-segment buffers and frees them before returning. Imported utilities are reentrant; the library has no initializer or cleanup. Every provided output buffer is caller-owned. Sizing calls pass `output=NULL, capacity=0` and collect `needed`. When a textual buffer is present, capacity must accommodate `needed+1` bytes, including NUL. `GUTC_NO_SPACE` does not write partially. Invalid arguments and malformed UTF-8/percent inputs return status errors; callers must not read output parameters unless the call succeeds (except `needed` after a valid sizing or capacity check). Native C cannot validate arbitrary invalid pointers. All C string inputs must be readable, NUL-terminated, and under 1 MiB. Aliased input/output buffers are unsupported.

**Open gates:** complete exported-symbol inventory, native object/JSON implementations, full authentication schema validation, actual source-executed cross-language golden fixtures, WHATWG URL normalization and Lodash grapheme segmentation. Existing PR #2 defines an incompatible `gutc_join_url` and duplicate percent codecs, so it is intentionally not imported; its branch is unchanged. Imported legacy APIs have distinct error and buffer sizing contracts (some include NUL in `required`); refer to individual headers.
