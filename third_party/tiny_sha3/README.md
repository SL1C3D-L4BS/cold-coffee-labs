# tiny_sha3

Vendored copy of Markku-Juhani O. Saarinen's reference FIPS-202 Keccak /
SHA-3 / SHAKE implementation.

- **Upstream:** https://github.com/mjosaarinen/tiny_sha3
- **Licence:** MIT (see [LICENSE](LICENSE))
- **Consumer:** `engine/core/crypto/sha3.cpp` calls `sha3()` via `extern "C"`
  to produce `std::array<std::uint8_t, 32>` SHA3-256 digests exposed as
  `gw::crypto::sha3_256_digest`.

## Greywater patches

Only one deviation from pristine upstream: the Keccak-f endianness guard now
uses `defined(__BYTE_ORDER__)` to avoid the "undefined == 0" preprocessor
trap on Windows / `clang-cl`. See the comment block in `tiny_sha3.c`.

## Upgrade protocol

Any upstream merge goes through an ADR per
`docs/10_APPENDIX_ADRS_AND_REFERENCES.md §Third-party dependency policy`.
Record the upstream commit SHA in the commit body and reapply the
endianness-guard patch.
