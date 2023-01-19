# qbdiff

Quick Binary Diff. An improved and modernised version of bsdiff. An overview:

- Faster than bsdiff, can take advantage of multiple CPU cores.
- Generally smaller patch files than bsdiff, supports only LZMA compression which performs excellently on sparse binary data.
- Requires less memory than bsdiff.
- Based on Colin Percival's algorithm used in bsdiff.
- Integrity checking using BLAKE2b.
- Can be used as a library.
