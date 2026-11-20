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
JSON syntax validation and UTF-8 string quoting are exposed via `#include <gutc/json.h>`. All utility functions are allocation-free and reentrant; see `include/gutc/gutc.h` for buffer ownership and status handling. The library is **not yet fully parity-certified**; see `docs/upstream-parity.md` and the existing open GutC PRs.
