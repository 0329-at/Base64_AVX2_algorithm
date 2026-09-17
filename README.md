# base64-avx2

A header-only Base64 codec accelerated with AVX2 intrinsics.

- **Header-only** — drop `src/Base64_algorithm.hpp` into your project and go. Zero integration cost.
- **Fast** — ~3.7 GiB/s encode, ~5.0 GiB/s decode on a single core (Ryzen 9 7950X, 1 MiB random data, hot cache).
- **Zero dependencies** — only the C++20 standard library.
- **Two alphabets** — standard Base64 (`+` `/`) and URL-safe Base64 (`-` `_`), selectable at runtime.
- **Simple API** — one `Base64` class; `encode` never fails, `decode` throws on invalid input.
- **Well-tested** — 100+ GoogleTest cases, including cross-validation against a scalar reference and mutation tests.

---

## Table of Contents

- [Quick Start](#quick-start)
- [API](#api)
- [URL-safe Mode](#url-safe-mode)
- [Error Handling](#error-handling)
- [Building and Testing](#building-and-testing)
- [Design Notes](#design-notes)
- [Requirements](#requirements)
- [FAQ](#faq)
- [License](#license)

---

## Quick Start

Copy `src/Base64_algorithm.hpp` into your project, then:

```cpp
#include "Base64_algorithm.hpp"
#include <iostream>

int main() {
    base64_avx2::Base64 b64;

    // Encode arbitrary bytes
    std::string encoded = b64.encode("hello, world!");
    std::cout << encoded << "\n";        // aGVsbG8sIHdvcmxkIQ==

    // Decode (throws on invalid input)
    std::string decoded = b64.decode(encoded);
    std::cout << decoded << "\n";        // hello, world!
}
```

Compile with AVX2 enabled:

```bash
g++ -std=c++20 -O3 -mavx2 demo.cpp -o demo
```

MSVC: `/std:c++20 /O2 /arch:AVX2`

---

## API

All functionality lives in `base64_avx2::Base64`. The class is stateless except for the current mode and has no hidden buffer — you get a plain `std::string` back.

### `Base64`

```cpp
base64_avx2::Base64 b64;                             // standard mode (default)
base64_avx2::Base64 b64{base64_avx2::Mode::UrlSafe}; // URL-safe mode
```

Switch the alphabet at runtime:

```cpp
b64.set(base64_avx2::Mode::UrlSafe);
```

### `encode`

```cpp
std::string encode(std::string_view input);
```

Encodes arbitrary bytes. **Never fails** — any byte sequence is a valid input. Returns a `std::string`.

### `decode`

```cpp
std::string decode(std::string_view input);
```

Decodes Base64. **Validates internally**. On invalid input, throws `base64_avx2::DecodeError`. Empty input returns an empty string (valid).

### `validate`

```cpp
bool validate(std::string_view input) noexcept;
```

Returns `true` if `input` is well-formed Base64. **Does not throw.** Use when you want to branch on validity without exceptions:

```cpp
if (b64.validate(input)) {
    std::string data = b64.decode(input);   // will not throw
}
```

---

## URL-safe Mode

URL-safe Base64 (RFC 4648 §5) replaces `+` and `/` with `-` and `_`. It is commonly used in JWTs, URL parameters, and filenames.

```cpp
using base64_avx2::Base64;
using base64_avx2::Mode;

// Construct with URL-safe
Base64 b64{Mode::UrlSafe};

// Or switch at runtime
Base64 b64;
b64.set(Mode::UrlSafe);

// Same API as standard mode
std::string encoded = b64.encode(data);
std::string decoded = b64.decode(encoded);
```

| Mode | Characters 62 & 63 | Typical use |
|---|---|---|
| `Mode::Standard` (default) | `+` `/` | General purpose |
| `Mode::UrlSafe` | `-` `_` | URLs, JWTs, filenames |

**The two modes are not interchangeable.** Decoding standard-mode data with URL-safe mode (or vice versa) will throw `DecodeError` if the input contains the mode-specific characters.

---

## Error Handling

`decode` throws `base64_avx2::DecodeError`, which derives from `std::invalid_argument`.

```cpp
try {
    std::string data = b64.decode(input);
    // use data
} catch (const base64_avx2::DecodeError& e) {
    std::cerr << e.what() << "\n";       // base64: invalid character '$' at position 4
    std::cerr << e.position() << "\n";   // 4
}
```

### Failure cases

| Input | `what()` |
|---|---|
| `"AAAA$AAA"` | `base64: invalid character '$' at position 4` |
| `"Zg="` | `base64: length not a multiple of 4 (got 3)` |
| `"A=AA"` | `base64: invalid padding at position 1` |

### Compatibility

`DecodeError` inherits from `std::invalid_argument`, so existing code that catches the base class still works:

```cpp
try {
    auto data = b64.decode(input);
} catch (const std::invalid_argument& e) {
    // works
}
```

### Non-throwing alternative

If you do not want exceptions:

```cpp
if (!b64.validate(input)) {
    std::cerr << "invalid base64\n";
    return;
}
std::string data = b64.decode(input);   // will not throw
```

---

## Building and Testing

The included test suite is built with xmake.

```bash
xmake f -m release -y
xmake build
xmake run
```

### Test Coverage

| Test file | Coverage |
|---|---|
| `test_known_vectors` | RFC 4648 standard vectors |
| `test_validate` | Validator semantics (length, characters, padding) |
| `test_roundtrip` | Round-trip for all lengths 0..1024 |
| `test_encode_vs_ref` | Byte-for-byte comparison against reference |
| `test_byte_values` | Every byte value 0..255 |
| `test_padding_positions` | Padding at SIMD block boundaries |
| `test_plus_slash` | `+` / `/` special characters |
| `test_invalid_chars` | Exception behavior and error positions |
| `test_decode_length` | Decode output length assertions |
| `test_cross_validate` | 2000 random cross-validation cases |
| `test_mutation` | Random mutation attacks |
| `test_urlsafe` | URL-safe alphabet behavior |
| `test_api_consistency` | `validate` / `decode` agreement |

## Design Notes

### Why one class instead of free functions

All functionality lives in `base64_avx2::Base64`. The mode is held as a member, so both alphabets share the same code path — the SIMD loop is compiled once, and the alphabet only affects a handful of constants at the entry of `encode` and `decode`.

### Why `decode` throws instead of returning a status

Silent failure (returning empty + setting an error flag) is easy to forget to check. An exception forces the caller to handle the failure path. `validate` is provided as a non-throwing escape hatch when exceptions are inconvenient.

### The `'='` trick

Padding bytes are **never special-cased** in the SIMD loop. `'='` maps to whatever the arithmetic produces — when packed into the 24-bit output, it only pollutes bytes outside the final output length. The output buffer is sized to `out_len + 8` and trimmed with `resize()`, so the garbage is never observed.

### AVX2 lane layout

An AVX2 register holds 8 × 32-bit lanes. Each lane holds one Base64 group:

- **Encode**: 3 input bytes per lane → 4 output characters per lane
- **Decode**: 4 input characters per lane → 3 output bytes per lane

During encoding, `permutevar8x32` is used once to duplicate a boundary dword so both 128-bit lanes have self-contained data (since `shuffle_epi8` cannot cross 128-bit lanes). Character mapping uses arithmetic rather than lookup tables — `shuffle_epi8` is a scarce resource and is reserved for operations that truly need it.

### URL-safe alphabet constants

The character-to-6-bit mapping uses a correction constant for the last two characters (`+`/`-` and `/`/`_`). These constants are computed from the actual character values at the start of `decode`, so both alphabets share the same SIMD loop. The encode-side correction is computed analogously.

---

## Requirements

- **C++ standard**: C++20
- **Compiler**:
  - GCC 11+
  - Clang 14+
  - MSVC 19.30+ (VS 2022)
- **CPU**: x86-64 with AVX2 support
  - Intel Haswell (2013, 4th Gen Core) or newer
  - AMD Excavator (2015) or newer
- **Build tool** (for tests only): xmake 2.8+

> ⚠️ **Note**: This library uses AVX2 **unconditionally**. Running on a CPU without AVX2 will trigger `SIGILL` (illegal instruction). No runtime detection is performed.
>
> **Unsupported platforms**: Apple Silicon (M1/M2/M3), all ARM, Intel CPUs older than 2013, AMD CPUs older than 2015.

---

## FAQ

**Q: Is an empty string valid Base64?**

Yes. An empty string corresponds to 0 bytes of data. `validate("") == true`, `decode("") == ""`, `encode("") == ""`.

**Q: What happens if I decode invalid input?**

`decode` throws `base64_avx2::DecodeError` (derived from `std::invalid_argument`). Use `validate` first if you do not want exceptions.

**Q: Why not use lookup tables for character mapping?**

The 6-bit to ASCII mapping uses arithmetic (4 comparisons + 4 conditional add/subtract), saving roughly 5 instructions over table lookups. `shuffle_epi8` is a scarce resource (only one execution port), and it is reserved for operations that truly need it.

**Q: Can I use this on a non-AVX2 CPU?**

Yes, but the code needs modification: add runtime detection (`__builtin_cpu_supports("avx2")`) and a scalar fallback path. The current version does not do this, because the vast majority of x86-64 users already have AVX2.

**Q: Is a `Base64` instance thread-safe?**

`encode` and `decode` do not mutate any member state, so a `const` instance can be shared across threads. Calling `set()` concurrently with other operations is not safe — use a separate instance per thread, or set the mode once before spawning threads.

**Q: How does this compare to aklomp/base64?**

aklomp's library is more mature and slightly faster (it uses dual-block parallelism and more aggressive hand optimization), but it is not header-only, so integration cost is higher. This library is more lightweight — single file, fast enough.

**Q: Why not return `std::vector<uint8_t>` for decoded data?**

`std::string` is the de facto standard "byte buffer" in C++ and pairs naturally with `std::string_view`. Also, `std::string`'s SSO (small string optimization) is friendlier for short data than `std::vector`.

---

## License

MIT License

Copyright (c) 2026 0329-at

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
