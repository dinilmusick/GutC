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

The install exports `GutC::gutc` from `find_package(GutC CONFIG REQUIRED)`.
Borrowed primitive/reference collections and ordered-map direct key selection are exposed via `<gutc/collections.h>`. JSON syntax validation and UTF-8 string quoting are exposed via `#include <gutc/json.h>`. The core byte-span API is allocation-free and reentrant (imported path normalization allocates temporary memory); see `include/gutc/gutc.h` for buffer ownership and status handling. Existing URL security and Unicode helper implementations from PRs #1, #3, and #4 are integrated. The library is **not yet fully parity-certified**; see `docs/upstream-parity.md` and the existing open GutC PRs.
