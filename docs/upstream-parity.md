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
| `objects.hasOwn` / `pick` / `omit` | `gutc_json_path_has` / `gutc_json_pick_paths` / `gutc_json_omit_paths` (+ direct-key borrowed value APIs) | `src/object_path.c`, `src/collections.c` | `tests/test_object_path.c`, `objects.tsv` | **Partial:** native JSON nested objects/array indexes/quoted bracket paths; no JS prototype, symbol keys, property descriptors or unsupported Lodash paths |\n| `objects.isPlainObject` | `gutc_json_is_plain_object` | `src/object_path.c` | `tests/test_object_path.c`, `objects.tsv` | Native JSON DOM object detection; does not model custom JavaScript prototypes |
| `strings.isEmail/capitalize/camelCase/kebabCase/snakeCase` | `gutc_is_email_ascii` / `gutc_string_transform_ascii` | `src/string_case.c` | `tests/test_string_case.c`, `strings.tsv` | Partial: real standard ASCII email and Lodash case segmentation; Unicode/deburr and quoted addresses explicitly unsupported |
| `urls.isUrl/getQueryParam/setQueryParam` | `gutc_url_is_http_url` / `gutc_url_get_query_param` / `gutc_url_set_query_param` | `src/url_query.c` | `tests/test_url_query.c` | Partial: HTTP(S) and strict valid UTF-8; not general WHATWG/IDNA, malformed percent rejects rather than replacement |
| `json.parseJson/tryParseJson` | `gutc_json_validate` (syntax only) | `src/json.c` | `tests/test_json.c`, `testdata/upstream-golden/json.tsv` | **Partial:** validation, no parsed AST or exception semantics |
| `json.stringifyJson` | `gutc_json_quote_string` (strings only) | `src/json.c` | `tests/test_json.c` | **Partial:** no object/array serializer |
| `json.isJsonValue` | `gutc_json_dom_is_json_value` | `src/json_dom.c` | `tests/test_json_dom.c` | Native structured owned DOM, finite-value enforcement |
| `auth.identityRef/parseIdentityRef/parseLegacyApplicationId` | `gutc_identity_ref_parse` / `gutc_legacy_application_id` | `src/identity.c` | `tests/test_identity.c` | Borrowed tagged identity views and conflict-rejecting aliases; legacy requires explicit map entries |
| `auth.validateCapabilityDescriptor` | `gutc_validate_capability_descriptor_v1` | `src/auth_contracts.c` | `tests/test_auth_contracts.c`, `auth.tsv` | Native exact-key, schema version, scopes, params and privacy checks; not authorization |
| `auth.validateScopedInvocation` | `gutc_validate_scoped_invocation_v1` | `src/auth_contracts.c` | `tests/test_auth_contracts.c`, `auth.tsv` | Native typed schema validation and admission guards; not principal authentication |
| `auth.consumeInvocationRetry` | `gutc_invocation_budget_retry` | `src/identity.c` | `tests/test_identity.c` | Pure retry budget, not full invocation envelope |\n| `auth.validateInvocationAgainstDescriptor` | `gutc_match_invocation_descriptor_v1` | `src/auth_contracts.c` | `tests/test_auth_contracts.c`, `auth.tsv` | Native pure descriptor/revision/workspace matching; target must reauthorize |
| `auth OAuth client registration/redirect` | `gutc_validate_oauth_registration_v1` / `gutc_validate_oauth_redirect_v1` | `src/oauth_contracts.c` | `tests/test_oauth_contracts.c`, `oauth.tsv` | Pure client schema and canonical ASCII HTTP(S)/loopback redirect validation; not ownership verification, IDNA, or general WHATWG |\n| `auth Luth/session transport` | — | — | — | Not implemented; transport host-specific |
| `models`, `semantic-input`, `engineering`, `api` published exports | — | — | — | Requires full export audit; runtime model adapters are host-specific |

Additional native primitives with no exact GutTS exported counterpart: `gutc_utf8_validate`, `gutc_percent_encode`, `gutc_percent_decode`, `gutc_array_chunk_count`. Percent encoding matches JavaScript encodeURIComponent on **valid UTF-8** for unreserved bytes but binary inputs are an intentional C extension. Percent decoding is byte-oriented; malformed percent triplets fail, while arbitrary decoded binary remains permitted.

## Memory / failure contract

The core `gutc_` byte/span APIs are allocation-free and reentrant; `gutc_normalize_storage_path` allocates temporary per-segment buffers and frees them before returning. Imported utilities are reentrant; the library has no initializer or cleanup. Every provided output buffer is caller-owned. Sizing calls pass `output=NULL, capacity=0` and collect `needed`. When a textual buffer is present, capacity must accommodate `needed+1` bytes, including NUL. `GUTC_NO_SPACE` does not write partially. Invalid arguments and malformed UTF-8/percent inputs return status errors; callers must not read output parameters unless the call succeeds (except `needed` after a valid sizing or capacity check). Native C cannot validate arbitrary invalid pointers. All C string inputs must be readable, NUL-terminated, and under 1 MiB. Aliased input/output buffers are unsupported.

**Open gates:** complete exported-symbol inventory, native object/JSON implementations, full authentication schema validation, actual source-executed cross-language golden fixtures, WHATWG URL normalization and Lodash grapheme segmentation. Existing PR #2 defines an incompatible `gutc_join_url` and duplicate percent codecs, so it is intentionally not imported; its branch is unchanged. Imported legacy APIs have distinct error and buffer sizing contracts (some include NUL in `required`); refer to individual headers.


## Pinned public export enumeration (mechanically transcribed)

Source: GutTS `3e2cbc0990ba29830ce91c5d1a1d552675d5334c`; `src/index.ts`, package subpath entrypoints and the specific barrels listed below. **195 individually identified published symbol/type or entrypoint obligations** are enumerated; 17 are described below as natively implemented, 20 as partial with specific divergences, and 158 are type-only, host-specific exceptions or unimplemented. These counts are an *inventory*, not an acceptance rate: naming a struct is not equivalent to meeting the JS contract. Some symbols are reexported by multiple entrypoints but counted once here. The type-only rows need wire layouts or explicit native exceptions. New exports introduced after the pinned reference are not in scope.

### JSON: src/json/json-concepts.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `JsonPrimitive` | `gutc_type_json_primitive; gutc_type_is_json_primitive` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `JsonValue` | `gutc_type_json_value; gutc_json_dom_is_json_value` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `JsonObject` | `gutc_type_json_object; gutc_type_is_json_object` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `isJsonValue` | `gutc_json_dom_is_json_value` | src/json_dom.c | tests/test_json_dom.c; api-dispatch.tsv | Pure native JSON DOM value predicate; JS undefined and cyclic objects represented explicitly by host-specific constraints |
| `parseJson` | `gutc_json_dom_parse` | src/json_dom.c | tests/test_json_dom.c | Partial: owned tree; lone surrogate escapes rejected |
| `tryParseJson` | `gutc_json_dom_parse` | src/json_dom.c | tests/test_json_dom.c | Partial: C status contract, not JS undefined |
| `stringifyJson` | `gutc_json_dom_stringify` + `gutc_js_number_format` | src/json_dom.c; src/js_number_format.c | tests/test_json_dom.c; tests/test_js_number_format.c; json-numbers.tsv | Partial: 351 source-executed ECMAScript binary64 shortest-roundtrip and exponent fixtures; libc numeric locale and exhaustive float boundary proof still pending |

### Strings: src/strings/string-concepts.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `isBlank` | `gut_is_blank_utf8` | src/gut_string_predicates.c | tests/test_string_predicates.c | Native UTF-8; golden checked |
| `isNonBlank` | `gut_is_nonblank_utf8` | src/gut_string_predicates.c | tests/test_string_predicates.c | Native UTF-8 |
| `isEmail` | `gutc_is_email_ascii` | src/string_case.c | tests/test_string_case.c; strings.tsv | Partial ASCII subset; explicit Unicode/validator.js extensions unsupported |
| `startsWithAny` | `gut_starts_with_any_utf8` | src/gut_string_predicates.c | tests/test_string_predicates.c; contracts.tsv | Native UTF-8; golden checked |
| `endsWithAny` | `gut_ends_with_any_utf8` | src/gut_string_predicates.c | tests/test_string_predicates.c; contracts.tsv | Native UTF-8; golden checked |
| `capitalize` | `gutc_string_transform_ascii` | src/string_case.c | tests/test_string_case.c; strings.tsv | Partial ASCII subset; explicit Unicode/validator.js extensions unsupported |
| `camelCase` | `gutc_string_transform_ascii` | src/string_case.c | tests/test_string_case.c; strings.tsv | Partial ASCII subset; explicit Unicode/validator.js extensions unsupported |
| `kebabCase` | `gutc_string_transform_ascii` | src/string_case.c | tests/test_string_case.c; strings.tsv | Partial ASCII subset; explicit Unicode/validator.js extensions unsupported |
| `snakeCase` | `gutc_string_transform_ascii` | src/string_case.c | tests/test_string_case.c; strings.tsv | Partial ASCII subset; explicit Unicode/validator.js extensions unsupported |
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
| `hasOwn` | `gutc_json_path_has` | src/object_path.c | tests/test_object_path.c; objects.tsv | Partial: JSON deep-path support; no JS prototype or unusual syntax |
| `pick` | `gutc_json_pick_paths` | src/object_path.c | tests/test_object_path.c; objects.tsv | Partial: JSON deep-path support; no JS prototype or unusual syntax |
| `omit` | `gutc_json_omit_paths` | src/object_path.c | tests/test_object_path.c; objects.tsv | Partial: JSON deep-path support; no JS prototype or unusual syntax |
| `isPlainObject` | `gutc_json_is_plain_object` | src/object_path.c | tests/test_object_path.c; objects.tsv | Native JSON DOM object kind; no prototype chain |

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
| `ApiDefinition` | `gutc_type_api_definition` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `ApiExample` | `gutc_type_api_example` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `ApiParameter` | `gutc_type_api_parameter` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `ApiSemantics` | `gutc_type_api_semantics` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `JsonSchema` | `gutc_type_json_schema` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `defineApi` | `gutc_define_api` | src/api_definition.c | tests/test_api_definition.c; api-definition.tsv | Native owned JSON metadata definition and input schema, not API dispatch or model execution |
| `guttsApiDefinitions` | `gutc_api_registry_list` | src/api_registry.c | tests/test_api_registry.c; api-catalog.json | Full 38 pinned definitions metadata; FOD catalog computed; no model execution |
| `guttsApiById` | `gutc_api_registry_lookup` | src/api_registry.c | tests/test_api_registry.c; api-catalog.json | Full 38 pinned definitions metadata; FOD catalog computed; no model execution |
| `guttsFodNodeDefinitions` | `gutc_api_registry_fod_nodes` | src/api_registry.c | tests/test_api_registry.c; api-catalog.json | Full 38 pinned definitions metadata; FOD catalog computed; no model execution |
| `guttsApiToFodNode` | `gutc_api_to_fod_node` | src/fod_node.c | tests/test_fod_node.c; fod.tsv | Native metadata-only, owned JSON DOM; no dispatch or host model runtime |
| `executeGuttsApi` | `gutc_execute_api` | src/api_dispatch.c; src/api_dispatch_text.c | tests/test_api_dispatch.c; api-dispatch.tsv | Partial: 33 portable utility IDs on JSON DOM; 5 model IDs require actual host callback; Unicode/WHATWG/JS edge exceptions explicitly return unsupported |

### Engineering: src/engineering/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `Identifier` | `gutc_eng_identifier` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `ProviderKind` | `gutc_eng_provider_kind` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `RepositoryRef` | `gutc_eng_repository_ref` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `WorkspaceRef` | `gutc_eng_workspace_ref` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `ExecutionRequest` | `gutc_eng_execution_request` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `ExecutionResult` | `gutc_eng_execution_result` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `VerificationCheck` | `gutc_eng_verification_check` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `VerificationReport` | `gutc_eng_verification_report` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `ArtifactRef` | `gutc_eng_artifact_ref` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `EngineeringCapability` | `gutc_eng_capability` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `CapabilityGrant` | `gutc_eng_capability_grant` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `EngineeringOperationContext` | `gutc_eng_operation_context` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |
| `ProvenanceRecord` | `gutc_eng_provenance_record` | include/gutc/engineering_types.h; src/engineering_types.c | tests/test_engineering_types.c; engineering-kinds.tsv | Source-pinned caller-borrowed C11 structural view, optional fields and exact literal enums; no execution or authorization |

### Auth types: src/auth/luthAuthTypes.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `DeploymentEnvironment` | `gutc_luth_environment` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `LuthDeploymentRecord` | `gutc_luth_deployment_record` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `LuthAuthClientConfig` | `gutc_luth_auth_client_config` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `LuthAuthTokenStore` | `gutc_luth_auth_token_store` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `RegisterApplicationResult` | `gutc_luth_registration_result` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `LuthUser` | `gutc_luth_user` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `LuthSession` | `gutc_luth_session` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `LuthSessionResult` | `gutc_luth_session_result` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed native fields supplement source-verified application-session validator |
| `EmailSignInCredentials` | `gutc_luth_email_sign_in` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `EmailSignUpData` | `gutc_luth_email_sign_up` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `GoogleSignInOptions` | `gutc_luth_google_sign_in` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `LuthAuthError` | `gutc_luth_auth_error` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed native fields supplement source-verified application-session validator |

### Auth client: src/auth/luthAuthClient.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `resolveRuntimeAppUrl` | `gutc_resolve_runtime_app_url` | src/runtime_app_url.c | tests/test_runtime_app_url.c; runtime-app-url.tsv | Host-injected location.origin / process.env.APP_URL, exact ECMAScript String.trim UTF-8; no ambient globals |
| `resolveClientOrigin` | `gutc_luth_resolve_client_origin` | src/luth_origin.c | tests/test_luth_origin.c; luth-origin.tsv | Partial: canonical ASCII HTTP(S) normalization with host-supplied location, unsupported WHATWG schemes and IDNA rejected |
| `resolveClientEnvironment` | `gutc_luth_resolve_client_environment` | src/luth_origin.c | tests/test_luth_origin.c; luth-origin.tsv | Partial: canonical ASCII HTTP(S) normalization with host-supplied location, unsupported WHATWG schemes and IDNA rejected |
| `DEFAULT_LUTH_URL` | `gutc_luth_default_url` | src/luth_origin.c | tests/test_luth_origin.c; luth-origin.tsv | Partial: canonical ASCII HTTP(S) normalization with host-supplied location, unsupported WHATWG schemes and IDNA rejected |
| `hydrateLuthTokenFromUrl` | `gutc_luth_hydrate_callback_token` | src/luth_callback_token.c | tests/test_luth_callback_token.c; luth-callback-token.tsv | Host-injected token store/history effects and 33 canonical URLSearchParams cases; malformed percent/UTF-8 replacement and arbitrary WHATWG location canonicalization require host |
| `LuthAuthClient` | `gutc_luth_client_init/register/get_session/refresh_session/sign_in_email/sign_up_email/sign_in_google/sign_out` | src/luth_client.c | tests/test_luth_client.c; luth-client.tsv | Partial: real trusted host HTTP contract, bounded failover and token store; requires external transport/TLS, no browser sessionStorage or live Luth deployment certification |
| `createLuthAuthClient` | `gutc_luth_client_init/register/get_session/refresh_session/sign_in_email/sign_up_email/sign_in_google/sign_out` | src/luth_client.c | tests/test_luth_client.c; luth-client.tsv | Partial: real trusted host HTTP contract, bounded failover and token store; requires external transport/TLS, no browser sessionStorage or live Luth deployment certification |

### OAuth contracts: src/auth/oauthClientContract.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ULTRST_APP_CONTRACT_VERSION` | `gutc_app_contract_version` | src/contract_primitives.c | tests/test_contract_primitives.c; contract-primitives.tsv | Source-matched portable JSON DOM predicate, no target authentication or grant authorization |
| `OAuthPlatform` | `gutc_oauth_platform` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `OAuthClientRegistrationV1` | `gutc_auth_oauth_registration_v1` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `ContractCheck` | `gutc_auth_contract_check` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `contractRecord` | `gutc_contract_record` | src/contract_primitives.c | tests/test_contract_primitives.c; contract-primitives.tsv | Source-matched portable JSON DOM predicate, no target authentication or grant authorization |
| `contractId` | `gutc_contract_id` | src/contract_primitives.c | tests/test_contract_primitives.c; contract-primitives.tsv | Source-matched portable JSON DOM predicate, no target authentication or grant authorization |
| `contractAppId` | `gutc_contract_app_id` | src/contract_primitives.c | tests/test_contract_primitives.c; contract-primitives.tsv | Source-matched portable JSON DOM predicate, no target authentication or grant authorization |
| `contractExactKeys` | `gutc_contract_exact_keys` | src/contract_primitives.c | tests/test_contract_primitives.c; contract-primitives.tsv | Source-matched portable JSON DOM predicate, no target authentication or grant authorization |
| `validateOAuthRedirectUri` | `gutc_validate_oauth_redirect_v1` | src/oauth_contracts.c | tests/test_oauth_contracts.c; oauth.tsv | Partial: canonical ASCII http(s), no WHATWG/IDNA generality |
| `validateOAuthClientRegistration` | `gutc_validate_oauth_registration_v1` | src/oauth_contracts.c | tests/test_oauth_contracts.c; oauth.tsv | Pure schema/PKCE/callback shape; no ownership authorization |

### Capability contracts: src/auth/capabilityContract.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `CapabilityParameterKind` | `gutc_auth_parameter_kind` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `CapabilityParameterV1` | `gutc_auth_capability_parameter_v1` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `CapabilityBaseV1` | `gutc_auth_capability_base_v1` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `StaticCapabilityV1` | `gutc_auth_static_capability_v1` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `WorkflowPublicationV1` | `gutc_auth_workflow_publication_v1` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `CapabilityDescriptorV1` | `gutc_auth_capability_descriptor_v1` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `validateCapabilityDescriptor` | `gutc_validate_capability_descriptor_v1` | src/auth_contracts.c | tests/test_auth_contracts.c; auth.tsv | Pure schema; not authorization |

### Invocation contracts: src/auth/invocationContract.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ScopedInvocationV1` | `gutc_invocation_budget` | include/gutc/gutc.h | tests/test_gutc.c | Partial: budget subset of schema |
| `validateScopedInvocation` | `gutc_validate_scoped_invocation_v1` | src/auth_contracts.c | tests/test_auth_contracts.c; auth.tsv | Pure schema; not authentication |
| `checkInvocationBudget` | `gutc_invocation_budget_check` | src/gutc.c | tests/test_gutc.c | Partial: budget subset, not full envelope |
| `advanceInvocationBudget` | `gutc_invocation_budget_advance` | src/gutc.c | tests/test_gutc.c | Partial: remaining calls only |
| `consumeInvocationRetry` | `gutc_invocation_budget_retry` | src/identity.c | tests/test_identity.c | Partial: remaining retries only |
| `validateInvocationAgainstDescriptor` | `gutc_match_invocation_descriptor_v1` | src/auth_contracts.c | tests/test_auth_contracts.c; auth.tsv | Pure version/workspace match; not authorization |

### Identity contracts: src/auth/identityIds.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `IDENTITY_KINDS` | `gutc_identity_kind_valid` | src/gutc.c | tests/test_gutc.c | Six published tag strings |
| `IdentityKind` | `gutc_auth_identity_kind` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `IdentityId` | `gutc_auth_identity_id` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `ApplicationId` | `gutc_auth_application_id` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `DeploymentId` | `gutc_auth_deployment_id` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `OAuthClientId` | `gutc_auth_oauth_client_id` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `UserId` | `gutc_auth_user_id` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `WorkspaceId` | `gutc_auth_workspace_id` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `UiSessionId` | `gutc_auth_ui_session_id` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `IdentityRef` | `gutc_auth_identity_ref` | include/gutc/auth_types.h; src/auth_types.c | tests/test_auth_types.c; auth-types.tsv | Typed caller-borrowed native structural fields; metadata alone never verifies a grant, identity, token or OAuth origin |
| `isIdentityKind` | `gutc_identity_kind_valid` | src/gutc.c | tests/test_gutc.c | Six published tags |
| `parseIdentityId` | `gutc_identity_id_valid` | src/gutc.c | identity.tsv; contracts.tsv | Validation only; golden checked |
| `requireIdentityId` | `gutc_identity_ref_parse` | src/identity.c | tests/test_identity.c | Partial: C status instead of JS throw |
| `identityRef` | `gutc_identity_ref_parse` | src/identity.c | tests/test_identity.c | Borrowed tagged ref |
| `parseIdentityRef` | `gutc_identity_ref_parse` | src/identity.c | tests/test_identity.c | Partial: direct typed spans, not arbitrary map |
| `parseLegacyApplicationId` | `gutc_legacy_application_id` | src/identity.c | tests/test_identity.c | Native explicit map; conflict rejection |

### Auth façade: src/auth/applicationAuthClient.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ApplicationAuthClientConfig` | `gutc_app_auth_host; gutc_application_auth_init` | src/application_auth.c | tests/test_application_auth.c; application-auth.tsv | Partial: Host-injected config preserves first-party fetch/navigation requirements |
| `ApplicationAuthClient` | `gutc_application_auth_client` | src/application_auth.c | tests/test_application_auth.c; application-auth.tsv | Partial: Real callback-driven GET session, POST logout, verification and navigation; host deployment not exercised |
| `createApplicationAuthClient` | `gutc_application_auth_init` | src/application_auth.c | tests/test_application_auth.c; application-auth.tsv | Partial: Caller-owned native client initialization, no ambient browser environment |

### Model types: src/models/model-types.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ModelMetadata` | `gutc_model_registry_lookup; gutc_model_library.metadata` | include/gutc/model_runtime.h; src/model_runtime.c | tests/test_model_runtime.c; model-residency.tsv | Partial: Typed native borrowed/owned model lifecycle representation; host must supply real inference runtime |
| `ModelLoadContext` | `gutc_model_load_context` | include/gutc/model_runtime.h; src/model_runtime.c | tests/test_model_runtime.c; model-residency.tsv | Partial: Typed native borrowed/owned model lifecycle representation; host must supply real inference runtime |
| `ModelRuntime` | `gutc_model_runtime_host` | include/gutc/model_runtime.h; src/model_runtime.c | tests/test_model_runtime.c; model-residency.tsv | Partial: Typed native borrowed/owned model lifecycle representation; host must supply real inference runtime |
| `ModelLease` | `gutc_model_lease` | include/gutc/model_runtime.h; src/model_runtime.c | tests/test_model_runtime.c; model-residency.tsv | Partial: Typed native borrowed/owned model lifecycle representation; host must supply real inference runtime |
| `ModelLoader` | `gutc_model_loader` | include/gutc/model_runtime.h; src/model_runtime.c | tests/test_model_runtime.c; model-residency.tsv | Partial: Typed native borrowed/owned model lifecycle representation; host must supply real inference runtime |
| `ModelUnloader` | `gutc_model_unloader` | include/gutc/model_runtime.h; src/model_runtime.c | tests/test_model_runtime.c; model-residency.tsv | Partial: Typed native borrowed/owned model lifecycle representation; host must supply real inference runtime |

### Model index: src/models/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `ModelResidency` | `gutc_model_residency_acquire; gutc_model_lease_release` | include/gutc/model_runtime.h; src/model_runtime.c | tests/test_model_runtime.c; model-residency.tsv | Partial: Synchronous nested leases, real host load/unload; JS async promise dedup requires host adapter |
| `ModelLibrary` | `gutc_model_library_init; gutc_model_library_invoke; gutc_model_library_destroy` | include/gutc/model_runtime.h; src/model_runtime.c | tests/test_model_runtime.c; model-residency.tsv | Partial: Source-matched load-operation-unload with actual host invocation, no inferred or generated model output |
| `guttsModelDefinitions` | `gutc_model_registry_list` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | All eight pinned model metadata definitions available; runtime loading/inference is host-dependent |
| `guttsModelById` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | All eight pinned model metadata definitions available; runtime loading/inference is host-dependent |
| `Needle3` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `needle3CompleteApi` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `needle3Metadata` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | Exact pinned metadata; no fake inference |
| `SmolLM2` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `smolLM2GenerateApi` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `smolLM2Metadata` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | Exact pinned metadata; no fake inference |
| `Whistle` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `whistleTranscribeApi` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `whistleMetadata` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | Exact pinned metadata; no fake inference |
| `EttinReranker` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `EttinReranker17M` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `EttinReranker32M` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `EttinReranker68M` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `EttinReranker150M` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `ettinRerankApi` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `ettinReranker17MMetadata` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | Exact pinned metadata; no fake inference |
| `ettinReranker32MMetadata` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | Exact pinned metadata; no fake inference |
| `ettinReranker68MMetadata` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | Exact pinned metadata; no fake inference |
| `ettinReranker150MMetadata` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | Exact pinned metadata; no fake inference |
| `MiniLM` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `miniLMSimilarityApi` | `gutc_model_client_init/call/destroy; gutc_model_family` | src/model_operations.c | tests/test_model_operations.c; model-operations.tsv | Partial: strict native typed host-bound load/invoke/unload; no model weights bundled, no browser/JS Promise behavior, external inference runtime required |
| `miniLML6Metadata` | `gutc_model_registry_lookup` | src/model_registry.c | tests/test_model_registry.c; model-catalog.json | Exact pinned metadata; no fake inference |

### Semantic input: src/semantic-input/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `SemanticInputSchema` | `gutc_semantic_schema` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputContext` | `gutc_semantic_context` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputErrorCode` | `gutc_semantic_error_code` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputRisk` | `gutc_semantic_risk` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputProgress` | `gutc_semantic_progress` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputEvent` | `gutc_semantic_event` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputModelPolicy` | `gutc_semantic_model_policy` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputPolicy` | `gutc_semantic_policy` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputProvenanceStore` | `gutc_semantic_provenance_store` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputProvenanceRecord` | `gutc_semantic_provenance_record` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputProvenance` | `gutc_semantic_provenance` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputCandidate` | `gutc_semantic_candidate` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputEvidence` | `gutc_semantic_evidence` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputClarification` | `gutc_semantic_clarification` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputResolution` | `gutc_semantic_resolution` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `SemanticInputCacheKey` | `const char *key, size_t length` | src/semantic_cache.c | tests/test_semantic_cache.c; semantic-cache.tsv | Explicit UTF-8 byte span with embedded NUL; JS lone-surrogate keys unsupported |
| `SemanticInputCache` | `gutc_semantic_cache_init/set/get/clear/size/destroy` | src/semantic_cache.c | tests/test_semantic_cache.c; semantic-cache.tsv | Full LRU sequence behavior; C heap ownership and external synchronization instead of JS Map references |
| `SemanticInputRequest` | `gutc_semantic_request` | include/gutc/portable_types.h; src/portable_types.c | tests/test_portable_types.c; portable-type-kinds.tsv | Source-pinned borrowed C11 structural contract; enum discriminants verified where applicable, no live model inference |
| `resolveDeterministicSemanticInput` | `gutc_resolve_deterministic_semantic` | src/semantic_deterministic.c | tests/test_semantic_deterministic.c; semantic.tsv | Partial: deterministic enums, string format, booleans, numeric input; JS RegExp pattern unsupported, decimal locale subtleties |
| `rankSemanticInputCandidates` | `gutc_semantic_rank_with_host` / `gutc_semantic_rank_order` | src/semantic_rank.c | tests/test_semantic_rank.c; semantic-rank.tsv | Partial: full stable scored-candidate comparator; real host MiniLM+Ettin callbacks required; non-simple ASCII collation tie explicitly unsupported |
| `resolveSemanticInput` | `gutc_resolve_semantic_input` | src/semantic_resolver.c | tests/test_semantic_resolver.c; semantic-resolver.tsv | Partial: source-pinned deterministic input; real host MiniLM/Ettin/Needle completion, candidate ranking, ambiguity and errors; JS Promises, provenance/cache/events, locale/RegExp need host-specific support |

### Server adapters: src/server/auth.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `resolveServerRuntimeAppUrl` | `gutc_resolve_server_runtime_app_url` | src/runtime_app_url.c | tests/test_runtime_app_url.c; runtime-app-url.tsv | Host-injected location.origin / process.env.APP_URL, exact ECMAScript String.trim UTF-8; no ambient globals |

### Storage SDK reexports: src/storage/index.ts

| GutTS export | Native C API | Implementation | Test/fixture | Parity result |
| --- | --- | --- | --- | --- |
| `S3Client` | `gutc_s3_client_execute; gutc_s3_command` | src/s3_client.c | tests/test_s3_client.c; s3-commands.tsv | Host-specific exception: GutTS reexports external AWS JavaScript SDK; portable C has validated binary host-backed command ABI, real SigV4/IO/streaming supplied by host, no native JS class equivalence |
| `GetObjectCommand` | `gutc_s3_client_execute; gutc_s3_command` | src/s3_client.c | tests/test_s3_client.c; s3-commands.tsv | Host-specific exception: GutTS reexports external AWS JavaScript SDK; portable C has validated binary host-backed command ABI, real SigV4/IO/streaming supplied by host, no native JS class equivalence |
| `HeadObjectCommand` | `gutc_s3_client_execute; gutc_s3_command` | src/s3_client.c | tests/test_s3_client.c; s3-commands.tsv | Host-specific exception: GutTS reexports external AWS JavaScript SDK; portable C has validated binary host-backed command ABI, real SigV4/IO/streaming supplied by host, no native JS class equivalence |
| `ListObjectsV2Command` | `gutc_s3_client_execute; gutc_s3_command` | src/s3_client.c | tests/test_s3_client.c; s3-commands.tsv | Host-specific exception: GutTS reexports external AWS JavaScript SDK; portable C has validated binary host-backed command ABI, real SigV4/IO/streaming supplied by host, no native JS class equivalence |
| `PutObjectCommand` | `gutc_s3_client_execute; gutc_s3_command` | src/s3_client.c | tests/test_s3_client.c; s3-commands.tsv | Host-specific exception: GutTS reexports external AWS JavaScript SDK; portable C has validated binary host-backed command ABI, real SigV4/IO/streaming supplied by host, no native JS class equivalence |
| `CopyObjectCommand` | `gutc_s3_client_execute; gutc_s3_command` | src/s3_client.c | tests/test_s3_client.c; s3-commands.tsv | Host-specific exception: GutTS reexports external AWS JavaScript SDK; portable C has validated binary host-backed command ABI, real SigV4/IO/streaming supplied by host, no native JS class equivalence |
| `DeleteObjectCommand` | `gutc_s3_client_execute; gutc_s3_command` | src/s3_client.c | tests/test_s3_client.c; s3-commands.tsv | Host-specific exception: GutTS reexports external AWS JavaScript SDK; portable C has validated binary host-backed command ABI, real SigV4/IO/streaming supplied by host, no native JS class equivalence |
| `DeleteObjectsCommand` | `gutc_s3_client_execute; gutc_s3_command` | src/s3_client.c | tests/test_s3_client.c; s3-commands.tsv | Host-specific exception: GutTS reexports external AWS JavaScript SDK; portable C has validated binary host-backed command ABI, real SigV4/IO/streaming supplied by host, no native JS class equivalence |


### Published entrypoint and host-specific notes

`./browser` and `./worker` reexport portions of the common core; no additional symbols were counted. `./server`, `./server/auth`, `./server/models`, `./server/storage`, `./storage`, `./storage/s3` and `./storage/google-drive` expose deployment/SDK-bound adapters. AWS SDK command classes, Google APIs modules, model inference loaders and a live Luth HTTP client are not pure C utility contracts: implementing them requires explicit host/transport callbacks and compatible vendor runtime dependencies. Do not claim they are fully implemented just because their metadata types exist. `./gutts.manifest.json` publishes registry metadata, not a callable utility.

The `semantic-input` deterministic resolution path has native subset coverage and 35 pinned GutTS fixtures; pattern and model-backed paths remain unavailable; ranking/model inference paths require native model/runtime adapters. `api` metadata and FOD dispatch are portable structured contracts but remain unimplemented. Host exceptions do not justify omitting the portable descriptor/metadata parts.



## Additional pinned host and entrypoint inventory (2026-11-21)

The independently compared GutRS source-pinned inventory contains 195 individually identified entries. The earlier GutC enumeration had 187 entries but included `ModelSize`, which is defined within `src/models/model-types.ts` **without being re-exported** by the public `src/models/index.ts` or a published model-types subpath. The published-surface enumeration therefore removes that internal type and explicitly adds the following nine host/entrypoint contracts. The four `*Exports` rows represent *entrypoint-composition obligations*, not invented JS variables.

| GutTS export / entrypoint obligation | Native C API / exception | Native source | Executable test | Classification and missing behavior |
| --- | --- | --- | --- | --- |
| `GoogleApisModule` | none (Node-only googleapis module shape) | none | none | Platform-specific exception: TypeScript-only GoogleApisModule type/SDK boundary; no portable C JavaScript SDK module |
| `loadGoogleApis` | none (Node dynamic import) | none | none | Platform-specific exception: Node dynamic import does not exist in C11; integration requires explicit host Google Drive provider |
| `createGoogleDriveClient` | none (host must provide Drive operations) | none | none | Unimplemented native Google Drive SDK client; exact GutTS `googleapis` adapter cannot be represented as an implemented C host callback |
| `nodeCactusNeedle3Runtime` | gutc_model_client_init/call/destroy (host-injected) | src/model_operations.c | tests/test_model_operations.c | Partial: true host-native Needle3 provider must be wired, loaded and independently executed; C callback is not model inference |
| `nodeCactusWhistleRuntime` | gutc_model_client_init/call/destroy (host-injected) | src/model_operations.c | tests/test_model_operations.c | Partial: true host-native Whistle provider must be wired, loaded and independently executed; C callback is not transcription |
| `rootExports` | GutC::gutc | CMakeLists.txt | tests/installed-consumer | Platform-specific exception: source entrypoint composition row, not a named runtime symbol; C11 installed package/headers substitute JS root bundling |
| `browserExports` | none (no browser DOM/Web APIs in native C11) | none | none | Platform-specific exception: source entrypoint composition row; native C11 does not emulate browser APIs |
| `workerExports` | none (no Cloudflare JS Worker runtime in native C11) | none | none | Platform-specific exception: source entrypoint composition row; no Worker JS execution claimed |
| `serverExports` | GutC::gutc with explicit host adapters | CMakeLists.txt; src/luth_client.c; src/s3_client.c | tests/installed-consumer | Platform-specific exception: source entrypoint composition row, not a named runtime symbol; C11 host adapters are separate and require real providers |

The complete 195-row source-pinned matrix is in `docs/wave1-contract-matrix.json`. **Mapped ≠ behaviourally equivalent**: nonexecutable type declarations and callbacks remain partial or native-platform exceptions. The nine additional entries do not confer full runtime parity. Immutable reference `GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c`.
