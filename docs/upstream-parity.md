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
| `urls.isUrl/getQueryParam/setQueryParam` | `gutc_url_is_http_url` / `gutc_url_get_query_param` / `gutc_url_set_query_param` | `src/url_query.c` | `tests/test_url_query.c` | Partial: HTTP(S) and strict valid UTF-8; not general WHATWG/IDNA, malformed percent rejects rather than replacement |
| `json.parseJson/tryParseJson` | `gutc_json_validate` (syntax only) | `src/json.c` | `tests/test_json.c`, `testdata/upstream-golden/json.tsv` | **Partial:** validation, no parsed AST or exception semantics |
| `json.stringifyJson` | `gutc_json_quote_string` (strings only) | `src/json.c` | `tests/test_json.c` | **Partial:** no object/array serializer |
| `json.isJsonValue` | `gutc_json_dom_is_json_value` | `src/json_dom.c` | `tests/test_json_dom.c` | Native structured owned DOM, finite-value enforcement |
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


## Pinned public export enumeration (mechanically transcribed)

Source: GutTS `3e2cbc0990ba29830ce91c5d1a1d552675d5334c`; `src/index.ts`, package subpath entrypoints and the specific barrels listed below. **187 individually identified published symbol/type names** are enumerated; 17 are described below as natively implemented, 18 as partial with specific divergences, and 152 are type-only, host-adapter or unimplemented. These counts are an *inventory*, not an acceptance rate: naming a struct is not equivalent to meeting the JS contract. Some symbols are reexported by multiple entrypoints but counted once here. The type-only rows need wire layouts or explicit native exceptions. New exports introduced after the pinned reference are not in scope.

### JSON: src/json/json-concepts.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `JsonPrimitive` | — | — | — | Type-only; native representation not implemented |
| `JsonValue` | — | — | — | Type-only; native representation not implemented |
| `JsonObject` | — | — | — | Type-only; native representation not implemented |
| `isJsonValue` | — | — | — | Not implemented |
| `parseJson` | `gutc_json_dom_parse` | src/json_dom.c | tests/test_json_dom.c | Partial: owned tree; lone surrogate escapes rejected |
| `tryParseJson` | `gutc_json_dom_parse` | src/json_dom.c | tests/test_json_dom.c | Partial: C status contract, not JS undefined |
| `stringifyJson` | `gutc_json_dom_stringify` | src/json_dom.c | tests/test_json_dom.c | Partial: full structural JSON; JS shortest decimal formatting differs |

### Strings: src/strings/string-concepts.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `isBlank` | `gut_is_blank_utf8` | src/gut_string_predicates.c | tests/test_string_predicates.c | Native UTF-8; golden checked |
| `isNonBlank` | `gut_is_nonblank_utf8` | src/gut_string_predicates.c | tests/test_string_predicates.c | Native UTF-8 |
| `isEmail` | — | — | — | Not implemented |
| `startsWithAny` | `gut_starts_with_any_utf8` | src/gut_string_predicates.c | tests/test_string_predicates.c; contracts.tsv | Native UTF-8; golden checked |
| `endsWithAny` | `gut_ends_with_any_utf8` | src/gut_string_predicates.c | tests/test_string_predicates.c; contracts.tsv | Native UTF-8; golden checked |
| `capitalize` | — | — | — | Not implemented |
| `camelCase` | — | — | — | Not implemented |
| `kebabCase` | — | — | — | Not implemented |
| `snakeCase` | — | — | — | Not implemented |
| `truncate` | `gut_truncate_utf8` | src/gut_truncate.c | tests/test_truncate.c | Partial: Lodash grapheme/UTF-16 segmentation |
| `truncateWords` | `gut_truncate_words_utf8_default` | src/gut_truncate_words.c | tests/test_truncate_words.c; contracts.tsv | Native UTF-8; golden checked |

### Arrays: src/arrays/array-concepts.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `compact` | `gutc_array_compact` | src/collections.c | tests/test_collections.c | Native tagged values; JS reference constraints |
| `unique` | `gutc_array_unique` | src/collections.c | tests/test_collections.c; contracts.tsv | Native SameValueZero tagged values; golden checked |
| `chunk` | `gutc_array_chunks` | src/collections.c | tests/test_collections.c | Native zero-copy borrowed views |
| `flatten` | `gutc_array_flatten_once` | src/collections.c | tests/test_collections.c | Native one-level borrowed values |
| `last` | `gutc_array_last` | src/collections.c | tests/test_collections.c | Native optional output/found flag |

### Objects: src/objects/object-concepts.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `hasOwn` | `gutc_object_has_direct` | src/collections.c | tests/test_collections.c | Partial: nested property paths omitted |
| `pick` | `gutc_object_pick_direct` | src/collections.c | tests/test_collections.c | Partial: direct keys only |
| `omit` | `gutc_object_omit_direct` | src/collections.c | tests/test_collections.c | Partial: direct keys only |
| `isPlainObject` | — | — | — | Not implemented |

### Numbers: src/numbers/number-concepts.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `clamp` | `gutc_number_clamp` | src/gutc.c | tests/test_gutc.c; contracts.tsv | Partial: C finite-only; golden checked |
| `inRange` | `gutc_number_in_range` | src/gutc.c | tests/test_gutc.c; contracts.tsv | Partial: C finite-only; golden checked |
| `roundTo` | `gutc_number_round_to` | src/collections.c | tests/test_collections.c | Partial: difficult IEEE-754/Lodash ties |
| `isFiniteNumber` | `gutc_number_is_finite` | src/collections.c | tests/test_collections.c | Partial: C double, not JS unknown type |
| `sum` | `gutc_number_sum` | src/gutc.c | tests/test_gutc.c; contracts.tsv | Native finite-only; golden checked |

### URLs: src/urls/url-concepts.ts and path-safety.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `isUrl` | `gutc_url_is_http_url` | src/url_query.c | tests/test_url_query.c | Partial: HTTP(S) absolute only, not WHATWG URL constructor |
| `joinUrl` | `gutc_join_url` | src/gutc.c | tests/test_gutc.c; contracts.tsv | Native bounded strings; golden checked |
| `getQueryParam` | `gutc_url_get_query_param` | src/url_query.c | tests/test_url_query.c | Partial: HTTP(S) + UTF-8; strict malformed percent errors |
| `setQueryParam` | `gutc_url_set_query_param` | src/url_query.c | tests/test_url_query.c | Partial: URLSearchParams form encoding, but raw URL origin not WHATWG canonicalized |
| `normalizeStoragePath` | `gutc_normalize_storage_path` | src/gutc_path.c | tests/test_path.c | Native validation; legacy errors |
| `encodeSafePathSegments` | `gutc_encode_safe_path_segments` | src/safe_path.c | tests/test_safe_path.c; contracts.tsv | Native bounded UTF-8; golden checked |
| `resolveTrustedApiUrl` | `gutc_resolve_trusted_api_url` | src/trusted_api.c | tests/trusted_api_test.c | Partial: WHATWG URL normalization |

### API: src/api/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ApiDefinition` | — | — | — | Type-only; native representation not implemented |
| `ApiExample` | — | — | — | Type-only; native representation not implemented |
| `ApiParameter` | — | — | — | Type-only; native representation not implemented |
| `ApiSemantics` | — | — | — | Type-only; native representation not implemented |
| `JsonSchema` | — | — | — | Type-only; native representation not implemented |
| `defineApi` | — | — | — | Not implemented |
| `guttsApiDefinitions` | — | — | — | Not implemented |
| `guttsApiById` | — | — | — | Not implemented |
| `guttsFodNodeDefinitions` | — | — | — | Not implemented |
| `guttsApiToFodNode` | — | — | — | Not implemented |
| `executeGuttsApi` | — | — | — | Not implemented |

### Engineering: src/engineering/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `Identifier` | — | — | — | Type-only; native representation not implemented |
| `ProviderKind` | — | — | — | Type-only; native representation not implemented |
| `RepositoryRef` | — | — | — | Type-only; native representation not implemented |
| `WorkspaceRef` | — | — | — | Type-only; native representation not implemented |
| `ExecutionRequest` | — | — | — | Type-only; native representation not implemented |
| `ExecutionResult` | — | — | — | Type-only; native representation not implemented |
| `VerificationCheck` | — | — | — | Type-only; native representation not implemented |
| `VerificationReport` | — | — | — | Type-only; native representation not implemented |
| `ArtifactRef` | — | — | — | Type-only; native representation not implemented |
| `EngineeringCapability` | — | — | — | Type-only; native representation not implemented |
| `CapabilityGrant` | — | — | — | Type-only; native representation not implemented |
| `EngineeringOperationContext` | — | — | — | Type-only; native representation not implemented |
| `ProvenanceRecord` | — | — | — | Type-only; native representation not implemented |

### Auth types: src/auth/luthAuthTypes.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `DeploymentEnvironment` | — | — | — | Type-only; native representation not implemented |
| `LuthDeploymentRecord` | — | — | — | Type-only; native representation not implemented |
| `LuthAuthClientConfig` | — | — | — | Type-only; native representation not implemented |
| `LuthAuthTokenStore` | — | — | — | Type-only; native representation not implemented |
| `RegisterApplicationResult` | — | — | — | Type-only; native representation not implemented |
| `LuthUser` | — | — | — | Type-only; native representation not implemented |
| `LuthSession` | — | — | — | Type-only; native representation not implemented |
| `LuthSessionResult` | — | — | — | Type-only; native representation not implemented |
| `EmailSignInCredentials` | — | — | — | Type-only; native representation not implemented |
| `EmailSignUpData` | — | — | — | Type-only; native representation not implemented |
| `GoogleSignInOptions` | — | — | — | Type-only; native representation not implemented |
| `LuthAuthError` | — | — | — | Type-only; native representation not implemented |

### Auth client: src/auth/luthAuthClient.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `resolveRuntimeAppUrl` | — | — | — | Host-dependent; native adapter not implemented |
| `resolveClientOrigin` | — | — | — | Host-dependent; native adapter not implemented |
| `resolveClientEnvironment` | — | — | — | Host-dependent; native adapter not implemented |
| `DEFAULT_LUTH_URL` | — | — | — | Host-dependent; native adapter not implemented |
| `hydrateLuthTokenFromUrl` | — | — | — | Host-dependent; native adapter not implemented |
| `LuthAuthClient` | — | — | — | Host-dependent; native adapter not implemented |
| `createLuthAuthClient` | — | — | — | Host-dependent; native adapter not implemented |

### OAuth contracts: src/auth/oauthClientContract.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ULTRST_APP_CONTRACT_VERSION` | — | — | — | Type-only; native representation not implemented |
| `OAuthPlatform` | — | — | — | Type-only; native representation not implemented |
| `OAuthClientRegistrationV1` | — | — | — | Type-only; native representation not implemented |
| `ContractCheck` | — | — | — | Type-only; native representation not implemented |
| `contractRecord` | — | — | — | Not implemented |
| `contractId` | — | — | — | Not implemented |
| `contractAppId` | — | — | — | Not implemented |
| `contractExactKeys` | — | — | — | Not implemented |
| `validateOAuthRedirectUri` | — | — | — | Not implemented |
| `validateOAuthClientRegistration` | — | — | — | Not implemented |

### Capability contracts: src/auth/capabilityContract.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `CapabilityParameterKind` | — | — | — | Type-only; native representation not implemented |
| `CapabilityParameterV1` | — | — | — | Type-only; native representation not implemented |
| `CapabilityBaseV1` | — | — | — | Type-only; native representation not implemented |
| `StaticCapabilityV1` | — | — | — | Type-only; native representation not implemented |
| `WorkflowPublicationV1` | — | — | — | Type-only; native representation not implemented |
| `CapabilityDescriptorV1` | — | — | — | Type-only; native representation not implemented |
| `validateCapabilityDescriptor` | — | — | — | Not implemented |

### Invocation contracts: src/auth/invocationContract.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ScopedInvocationV1` | `gutc_invocation_budget` | include/gutc/gutc.h | tests/test_gutc.c | Partial: budget subset of schema |
| `validateScopedInvocation` | — | — | — | Not implemented |
| `checkInvocationBudget` | `gutc_invocation_budget_check` | src/gutc.c | tests/test_gutc.c | Partial: budget subset, not full envelope |
| `advanceInvocationBudget` | `gutc_invocation_budget_advance` | src/gutc.c | tests/test_gutc.c | Partial: remaining calls only |
| `consumeInvocationRetry` | `gutc_invocation_budget_retry` | src/identity.c | tests/test_identity.c | Partial: remaining retries only |
| `validateInvocationAgainstDescriptor` | — | — | — | Not implemented |

### Identity contracts: src/auth/identityIds.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `IDENTITY_KINDS` | `gutc_identity_kind_valid` | src/gutc.c | tests/test_gutc.c | Six published tag strings |
| `IdentityKind` | — | — | — | Type-only; native representation not implemented |
| `IdentityId` | — | — | — | Type-only; native representation not implemented |
| `ApplicationId` | — | — | — | Type-only; native representation not implemented |
| `DeploymentId` | — | — | — | Type-only; native representation not implemented |
| `OAuthClientId` | — | — | — | Type-only; native representation not implemented |
| `UserId` | — | — | — | Type-only; native representation not implemented |
| `WorkspaceId` | — | — | — | Type-only; native representation not implemented |
| `UiSessionId` | — | — | — | Type-only; native representation not implemented |
| `IdentityRef` | `gutc_identity_ref` | include/gutc/identity.h | tests/test_identity.c | Borrowed struct; no JSON serializer |
| `isIdentityKind` | `gutc_identity_kind_valid` | src/gutc.c | tests/test_gutc.c | Six published tags |
| `parseIdentityId` | `gutc_identity_id_valid` | src/gutc.c | identity.tsv; contracts.tsv | Validation only; golden checked |
| `requireIdentityId` | `gutc_identity_ref_parse` | src/identity.c | tests/test_identity.c | Partial: C status instead of JS throw |
| `identityRef` | `gutc_identity_ref_parse` | src/identity.c | tests/test_identity.c | Borrowed tagged ref |
| `parseIdentityRef` | `gutc_identity_ref_parse` | src/identity.c | tests/test_identity.c | Partial: direct typed spans, not arbitrary map |
| `parseLegacyApplicationId` | `gutc_legacy_application_id` | src/identity.c | tests/test_identity.c | Native explicit map; conflict rejection |

### Auth façade: src/auth/applicationAuthClient.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ApplicationAuthClientConfig` | — | — | — | Host-dependent; native adapter not implemented |
| `ApplicationAuthClient` | — | — | — | Host-dependent; native adapter not implemented |
| `createApplicationAuthClient` | — | — | — | Host-dependent; native adapter not implemented |

### Model types: src/models/model-types.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ModelSize` | — | — | — | Type-only; native representation not implemented |
| `ModelMetadata` | — | — | — | Type-only; native representation not implemented |
| `ModelLoadContext` | — | — | — | Type-only; native representation not implemented |
| `ModelRuntime` | — | — | — | Type-only; native representation not implemented |
| `ModelLease` | — | — | — | Type-only; native representation not implemented |
| `ModelLoader` | — | — | — | Type-only; native representation not implemented |
| `ModelUnloader` | — | — | — | Type-only; native representation not implemented |

### Model index: src/models/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ModelResidency` | — | — | — | Host-dependent; native adapter not implemented |
| `ModelLibrary` | — | — | — | Host-dependent; native adapter not implemented |
| `guttsModelDefinitions` | — | — | — | Host-dependent; native adapter not implemented |
| `guttsModelById` | — | — | — | Host-dependent; native adapter not implemented |
| `Needle3` | — | — | — | Host-dependent; native adapter not implemented |
| `needle3CompleteApi` | — | — | — | Host-dependent; native adapter not implemented |
| `needle3Metadata` | — | — | — | Host-dependent; native adapter not implemented |
| `SmolLM2` | — | — | — | Host-dependent; native adapter not implemented |
| `smolLM2GenerateApi` | — | — | — | Host-dependent; native adapter not implemented |
| `smolLM2Metadata` | — | — | — | Host-dependent; native adapter not implemented |
| `Whistle` | — | — | — | Host-dependent; native adapter not implemented |
| `whistleTranscribeApi` | — | — | — | Host-dependent; native adapter not implemented |
| `whistleMetadata` | — | — | — | Host-dependent; native adapter not implemented |
| `EttinReranker` | — | — | — | Host-dependent; native adapter not implemented |
| `EttinReranker17M` | — | — | — | Host-dependent; native adapter not implemented |
| `EttinReranker32M` | — | — | — | Host-dependent; native adapter not implemented |
| `EttinReranker68M` | — | — | — | Host-dependent; native adapter not implemented |
| `EttinReranker150M` | — | — | — | Host-dependent; native adapter not implemented |
| `ettinRerankApi` | — | — | — | Host-dependent; native adapter not implemented |
| `ettinReranker17MMetadata` | — | — | — | Host-dependent; native adapter not implemented |
| `ettinReranker32MMetadata` | — | — | — | Host-dependent; native adapter not implemented |
| `ettinReranker68MMetadata` | — | — | — | Host-dependent; native adapter not implemented |
| `ettinReranker150MMetadata` | — | — | — | Host-dependent; native adapter not implemented |
| `MiniLM` | — | — | — | Host-dependent; native adapter not implemented |
| `miniLMSimilarityApi` | — | — | — | Host-dependent; native adapter not implemented |
| `miniLML6Metadata` | — | — | — | Host-dependent; native adapter not implemented |

### Semantic input: src/semantic-input/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `SemanticInputSchema` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputContext` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputErrorCode` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputRisk` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputProgress` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputEvent` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputModelPolicy` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputPolicy` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputProvenanceStore` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputProvenanceRecord` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputProvenance` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputCandidate` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputEvidence` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputClarification` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputResolution` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputCacheKey` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputCache` | — | — | — | Type-only; native representation not implemented |
| `SemanticInputRequest` | — | — | — | Type-only; native representation not implemented |
| `resolveDeterministicSemanticInput` | — | — | — | Not implemented |
| `rankSemanticInputCandidates` | — | — | — | Host-dependent; native adapter not implemented |
| `resolveSemanticInput` | — | — | — | Host-dependent; native adapter not implemented |

### Server adapters: src/server/auth.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `resolveServerRuntimeAppUrl` | — | — | — | Host-dependent; native adapter not implemented |

### Storage SDK reexports: src/storage/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `S3Client` | — | — | — | Host-dependent; native adapter not implemented |
| `GetObjectCommand` | — | — | — | Host-dependent; native adapter not implemented |
| `HeadObjectCommand` | — | — | — | Host-dependent; native adapter not implemented |
| `ListObjectsV2Command` | — | — | — | Host-dependent; native adapter not implemented |
| `PutObjectCommand` | — | — | — | Host-dependent; native adapter not implemented |
| `CopyObjectCommand` | — | — | — | Host-dependent; native adapter not implemented |
| `DeleteObjectCommand` | — | — | — | Host-dependent; native adapter not implemented |
| `DeleteObjectsCommand` | — | — | — | Host-dependent; native adapter not implemented |


### Published entrypoint and host-specific notes

`./browser` and `./worker` reexport portions of the common core; no additional symbols were counted. `./server`, `./server/auth`, `./server/models`, `./server/storage`, `./storage`, `./storage/s3` and `./storage/google-drive` expose deployment/SDK-bound adapters. AWS SDK command classes, Google APIs modules, model inference loaders and a live Luth HTTP client are not pure C utility contracts: implementing them requires explicit host/transport callbacks and compatible vendor runtime dependencies. Do not claim they are fully implemented just because their metadata types exist. `./gutts.manifest.json` publishes registry metadata, not a callable utility.

The `semantic-input` deterministic resolution path is potentially portable but is not yet implemented; ranking/model inference paths require native model/runtime adapters. `api` metadata and FOD dispatch are portable structured contracts but remain unimplemented. Host exceptions do not justify omitting the portable descriptor/metadata parts.

