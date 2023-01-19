#!/bin/bash

# Test diffing with qbdiff.
hyperfine --show-output -N --warmup 10 'qbdiff o10k.bin o10k-100.bin o10k-100.patch' 'qbdiff o10k.bin o10k-1k.bin o10k-1k.patch' | tee bench-diff-10k.txt
hyperfine --show-output -N --warmup 10 'qbdiff o100k.bin o100k-100.bin o100k-100.patch' 'qbdiff o100k.bin o100k-1k.bin o100k-1k.patch' 'qbdiff o100k.bin o100k-10k.bin o100k-10k.patch' | tee bench-diff-100k.txt
hyperfine --show-output -N --warmup 10 'qbdiff o1m.bin o1m-100.bin o1m-100.patch' 'qbdiff o1m.bin o1m-1k.bin o1m-1k.patch' 'qbdiff o1m.bin o1m-10k.bin o1m-10k.patch' 'qbdiff o1m.bin o1m-100k.bin o1m-100k.patch' | tee bench-diff-1m.txt
hyperfine --show-output -N 'qbdiff o10m.bin o10m-100.bin o10m-100.patch' 'qbdiff o10m.bin o10m-1k.bin o10m-1k.patch' 'qbdiff o10m.bin o10m-10k.bin o10m-10k.patch' 'qbdiff o10m.bin o10m-100k.bin o10m-100k.patch' 'qbdiff o10m.bin o10m-1m.bin o10m-1m.patch' | tee bench-diff-10m.txt
hyperfine --show-output -N 'qbdiff o100m.bin o100m-100.bin o100m-100.patch' 'qbdiff o100m.bin o100m-1k.bin o100m-1k.patch' 'qbdiff o100m.bin o100m-10k.bin o100m-10k.patch' 'qbdiff o100m.bin o100m-100k.bin o100m-100k.patch' 'qbdiff o100m.bin o100m-1m.bin o100m-1m.patch' 'qbdiff o100m.bin o100m-10m.bin o100m-10m.patch' | tee bench-diff-100m.txt
hyperfine --show-output -N 'qbdiff o1g.bin o1g-100.bin o1g-100.patch' 'qbdiff o1g.bin o1g-1k.bin o1g-1k.patch' 'qbdiff o1g.bin o1g-10k.bin o1g-10k.patch' 'qbdiff o1g.bin o1g-100k.bin o1g-100k.patch' 'qbdiff o1g.bin o1g-1m.bin o1g-1m.patch' 'qbdiff o1g.bin o1g-10m.bin o1g-10m.patch' 'qbdiff o1g.bin o1g-100m.bin o1g-100m.patch' | tee bench-diff-1g.txt

# Test patching with qbdiff.
hyperfine --show-output -N --warmup 10 'qbpatch o10k.bin o10k-100.bin.new o10k-100.patch' 'qbpatch o10k.bin o10k-1k.bin.new o10k-1k.patch' | tee bench-patch-10k.txt
hyperfine --show-output -N --warmup 10 'qbpatch o100k.bin o100k-100.bin.new o100k-100.patch' 'qbpatch o100k.bin o100k-1k.bin.new o100k-1k.patch' 'qbpatch o100k.bin o100k-10k.bin.new o100k-10k.patch' | tee bench-patch-100k.txt
hyperfine --show-output -N --warmup 10 'qbpatch o1m.bin o1m-100.bin.new o1m-100.patch' 'qbpatch o1m.bin o1m-1k.bin.new o1m-1k.patch' 'qbpatch o1m.bin o1m-10k.bin.new o1m-10k.patch' 'qbpatch o1m.bin o1m-100k.bin.new o1m-100k.patch' | tee bench-patch-1m.txt
hyperfine --show-output -N 'qbpatch o10m.bin o10m-100.bin.new o10m-100.patch' 'qbpatch o10m.bin o10m-1k.bin.new o10m-1k.patch' 'qbpatch o10m.bin o10m-10k.bin.new o10m-10k.patch' 'qbpatch o10m.bin o10m-100k.bin.new o10m-100k.patch' 'qbpatch o10m.bin o10m-1m.bin.new o10m-1m.patch' | tee bench-patch-10m.txt
hyperfine --show-output -N 'qbpatch o100m.bin o100m-100.bin.new o100m-100.patch' 'qbpatch o100m.bin o100m-1k.bin.new o100m-1k.patch' 'qbpatch o100m.bin o100m-10k.bin.new o100m-10k.patch' 'qbpatch o100m.bin o100m-100k.bin.new o100m-100k.patch' 'qbpatch o100m.bin o100m-1m.bin.new o100m-1m.patch' 'qbpatch o100m.bin o100m-10m.bin.new o100m-10m.patch' | tee bench-patch-100m.txt
hyperfine --show-output -N 'qbpatch o1g.bin o1g-100.bin.new o1g-100.patch' 'qbpatch o1g.bin o1g-1k.bin.new o1g-1k.patch' 'qbpatch o1g.bin o1g-10k.bin.new o1g-10k.patch' 'qbpatch o1g.bin o1g-100k.bin.new o1g-100k.patch' 'qbpatch o1g.bin o1g-1m.bin.new o1g-1m.patch' 'qbpatch o1g.bin o1g-10m.bin.new o1g-10m.patch' 'qbpatch o1g.bin o1g-100m.bin.new o1g-100m.patch' | tee bench-patch-1g.txt

# Log sizes of qbdiff patches.
ls -l *.patch | tee bench-patch-sizes.txt

# Test diffing with bsdiff.
hyperfine --show-output -N --warmup 10 'bsdiff o10k.bin o10k-100.bin o10k-100.patch' 'bsdiff o10k.bin o10k-1k.bin o10k-1k.patch' | tee bench-bsdiff-10k.txt
hyperfine --show-output -N --warmup 10 'bsdiff o100k.bin o100k-100.bin o100k-100.patch' 'bsdiff o100k.bin o100k-1k.bin o100k-1k.patch' 'bsdiff o100k.bin o100k-10k.bin o100k-10k.patch' | tee bench-bsdiff-100k.txt
hyperfine --show-output -N --warmup 10 'bsdiff o1m.bin o1m-100.bin o1m-100.patch' 'bsdiff o1m.bin o1m-1k.bin o1m-1k.patch' 'bsdiff o1m.bin o1m-10k.bin o1m-10k.patch' 'bsdiff o1m.bin o1m-100k.bin o1m-100k.patch' | tee bench-bsdiff-1m.txt
hyperfine --show-output -N 'bsdiff o10m.bin o10m-100.bin o10m-100.patch' 'bsdiff o10m.bin o10m-1k.bin o10m-1k.patch' 'bsdiff o10m.bin o10m-10k.bin o10m-10k.patch' 'bsdiff o10m.bin o10m-100k.bin o10m-100k.patch' 'bsdiff o10m.bin o10m-1m.bin o10m-1m.patch' | tee bench-bsdiff-10m.txt
hyperfine --show-output -N 'bsdiff o100m.bin o100m-100.bin o100m-100.patch' 'bsdiff o100m.bin o100m-1k.bin o100m-1k.patch' 'bsdiff o100m.bin o100m-10k.bin o100m-10k.patch' 'bsdiff o100m.bin o100m-100k.bin o100m-100k.patch' 'bsdiff o100m.bin o100m-1m.bin o100m-1m.patch' 'bsdiff o100m.bin o100m-10m.bin o100m-10m.patch' | tee bench-bsdiff-100m.txt
hyperfine --show-output -N 'bsdiff o1g.bin o1g-100.bin o1g-100.patch' 'bsdiff o1g.bin o1g-1k.bin o1g-1k.patch' 'bsdiff o1g.bin o1g-10k.bin o1g-10k.patch' 'bsdiff o1g.bin o1g-100k.bin o1g-100k.patch' 'bsdiff o1g.bin o1g-1m.bin o1g-1m.patch' 'bsdiff o1g.bin o1g-10m.bin o1g-10m.patch' 'bsdiff o1g.bin o1g-100m.bin o1g-100m.patch' | tee bench-bsdiff-1g.txt

# Test patching with bsdiff.
hyperfine --show-output -N --warmup 10 'bspatch o10k.bin o10k-100.bin.new o10k-100.patch' 'bspatch o10k.bin o10k-1k.bin.new o10k-1k.patch' | tee bench-bspatch-10k.txt
hyperfine --show-output -N --warmup 10 'bspatch o100k.bin o100k-100.bin.new o100k-100.patch' 'bspatch o100k.bin o100k-1k.bin.new o100k-1k.patch' 'bspatch o100k.bin o100k-10k.bin.new o100k-10k.patch' | tee bench-bspatch-100k.txt
hyperfine --show-output -N --warmup 10 'bspatch o1m.bin o1m-100.bin.new o1m-100.patch' 'bspatch o1m.bin o1m-1k.bin.new o1m-1k.patch' 'bspatch o1m.bin o1m-10k.bin.new o1m-10k.patch' 'bspatch o1m.bin o1m-100k.bin.new o1m-100k.patch' | tee bench-bspatch-1m.txt
hyperfine --show-output -N 'bspatch o10m.bin o10m-100.bin.new o10m-100.patch' 'bspatch o10m.bin o10m-1k.bin.new o10m-1k.patch' 'bspatch o10m.bin o10m-10k.bin.new o10m-10k.patch' 'bspatch o10m.bin o10m-100k.bin.new o10m-100k.patch' 'bspatch o10m.bin o10m-1m.bin.new o10m-1m.patch' | tee bench-bspatch-10m.txt
hyperfine --show-output -N 'bspatch o100m.bin o100m-100.bin.new o100m-100.patch' 'bspatch o100m.bin o100m-1k.bin.new o100m-1k.patch' 'bspatch o100m.bin o100m-10k.bin.new o100m-10k.patch' 'bspatch o100m.bin o100m-100k.bin.new o100m-100k.patch' 'bspatch o100m.bin o100m-1m.bin.new o100m-1m.patch' 'bspatch o100m.bin o100m-10m.bin.new o100m-10m.patch' | tee bench-bspatch-100m.txt
hyperfine --show-output -N 'bspatch o1g.bin o1g-100.bin.new o1g-100.patch' 'bspatch o1g.bin o1g-1k.bin.new o1g-1k.patch' 'bspatch o1g.bin o1g-10k.bin.new o1g-10k.patch' 'bspatch o1g.bin o1g-100k.bin.new o1g-100k.patch' 'bspatch o1g.bin o1g-1m.bin.new o1g-1m.patch' 'bspatch o1g.bin o1g-10m.bin.new o1g-10m.patch' 'bspatch o1g.bin o1g-100m.bin.new o1g-100m.patch' | tee bench-bspatch-1g.txt

# Log sizes of patches.
ls -l *.patch | tee bench-bspatch-sizes.txt
