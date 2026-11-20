# GutC

Portable C11 foundation for selected GutTS contracts, pinned to `3e2cbc0990ba29830ce91c5d1a1d552675d5334c`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /tmp/gutc
cmake -S tests/consumer -B consumer-build -DCMAKE_PREFIX_PATH=/tmp/gutc
cmake --build consumer-build
```

The pinned cross-language corpus in `testdata/upstream-golden/contracts.tsv` is independently executed by a Node build of the exact GutTS commit and a CTest runner. The install exports `GutC::gutc` from `find_package(GutC CONFIG REQUIRED)`.
Portable OAuth client and redirect contract shape validators are exposed via `<gutc/oauth_contracts.h>` (not proof of ownership). Pure versioned capability and scoped invocation shape/budget validators are exposed from `<gutc/auth_contracts.h>`, with exact reason strings. These never replace target-side authorization. HTTP(S) URL search parameter get/set and form encoding are available via `<gutc/url_query.h>` with explicit non-WHATWG host exceptions. Heap-owned structured JSON parsing and serialization are available via `<gutc/json_dom.h>`, with bounded node/depth budgets and `gutc_json_dom_free()`. Borrowed primitive/reference collections and ordered-map direct key selection are exposed via `<gutc/collections.h>`. JSON syntax validation and UTF-8 string quoting are exposed via `#include <gutc/json.h>`. The core byte-span API is allocation-free and reentrant (imported path normalization allocates temporary memory); see `include/gutc/gutc.h` for buffer ownership and status handling. Safe storage-relative URI encoding and borrowed identity/legacy mapping are also available via `<gutc/safe_path.h>` and `<gutc/identity.h>`. Existing URL security and Unicode helper implementations from PRs #1, #3, and #4 are integrated. The library is **not yet fully parity-certified**; see `docs/upstream-parity.md` and the existing open GutC PRs.
