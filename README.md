# qbdiff

Quick Binary Diff. An improved and modernised version of bsdiff. An overview:

- Faster than bsdiff, can take advantage of multiple CPU cores.
- Generally smaller patch files than bsdiff, supports only LZMA compression which performs excellently on sparse binary data.
- Requires less memory than bsdiff.
- Based on Colin Percival's algorithm used in bsdiff.
- Integrity checking using BLAKE2b.
- Can be used as a library.

## Benchmarks against bsdiff 4.3

```
bsdiff 1g: 229.83s +/- 5s
qbdiff 1g: 46s +/- 1s

bsdiff 1m: 87.5ms +/- 2ms
qbdiff 1m: 37ms +/- 1ms

bsdiff 10k: 1ms +/- 0.5ms
qbdiff 10k: 2.8ms +/- 1ms

bsdiff 10m: 1.1s +/- 0.05s
qbdiff 10m: 369ms +/- 1ms

bsdiff 100k: 11ms +/- 0.1ms
qbdiff 100k: 8ms +/- 2ms

bsdiff 100m: 16.3s +/- 100ms
qbdiff 100m: 4.5s +/- 20ms
```

qbdiff uses LZMA, which has considerably longer compression time which is responsible for great compression ratio (considerably better than bzip2 on binary data) and fast decompression speed, meaning that (compared to bsdiff's bzip2), patch creation is the bottleneck where it is supposed to be slower. However, when it comes to decompression, bzip2 is a lot slower taking usually two to six times more time than LZMA ([source](https://tukaani.org/lzma/benchmarks.html)).

Furthermore, the [bsdiff memory usage](https://www.daemonology.net/bsdiff/) is `max(17*n,9*n+m)+O(1)`, while qbdiff memory usage is usually bounded by `O(5*n+m)+O(1)` in most use cases - the improvement is clear and almost twofold-threefold.

