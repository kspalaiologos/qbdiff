#!/bin/bash

head -c 10000 /dev/urandom > o10k.bin
head -c 100000 /dev/urandom > o100k.bin
head -c 1000000 /dev/urandom > o1m.bin
head -c 10000000 /dev/urandom > o10m.bin
head -c 100000000 /dev/urandom > o100m.bin
head -c 1000000000 /dev/urandom > o1g.bin

cp o10k.bin o10k-100.bin && ./modbytes o10k-100.bin 100
cp o10k.bin o10k-1k.bin && ./modbytes o10k-1k.bin 1000

cp o100k.bin o100k-100.bin && ./modbytes o100k-100.bin 100
cp o100k.bin o100k-1k.bin && ./modbytes o100k-1k.bin 1000
cp o100k.bin o100k-10k.bin && ./modbytes o100k-10k.bin 10000

cp o1m.bin o1m-100.bin && ./modbytes o1m-100.bin 100
cp o1m.bin o1m-1k.bin && ./modbytes o1m-1k.bin 1000
cp o1m.bin o1m-10k.bin && ./modbytes o1m-10k.bin 10000
cp o1m.bin o1m-100k.bin && ./modbytes o1m-100k.bin 100000

cp o10m.bin o10m-100.bin && ./modbytes o10m-100.bin 100
cp o10m.bin o10m-1k.bin && ./modbytes o10m-1k.bin 1000
cp o10m.bin o10m-10k.bin && ./modbytes o10m-10k.bin 10000
cp o10m.bin o10m-100k.bin && ./modbytes o10m-100k.bin 100000
cp o10m.bin o10m-1m.bin && ./modbytes o10m-1m.bin 1000000

cp o100m.bin o100m-100.bin && ./modbytes o100m-100.bin 100
cp o100m.bin o100m-1k.bin && ./modbytes o100m-1k.bin 1000
cp o100m.bin o100m-10k.bin && ./modbytes o100m-10k.bin 10000
cp o100m.bin o100m-100k.bin && ./modbytes o100m-100k.bin 100000
cp o100m.bin o100m-1m.bin && ./modbytes o100m-1m.bin 1000000
cp o100m.bin o100m-10m.bin && ./modbytes o100m-10m.bin 10000000

cp o1g.bin o1g-100.bin && ./modbytes o1g-100.bin 100
cp o1g.bin o1g-1k.bin && ./modbytes o1g-1k.bin 1000
cp o1g.bin o1g-10k.bin && ./modbytes o1g-10k.bin 10000
cp o1g.bin o1g-100k.bin && ./modbytes o1g-100k.bin 100000
cp o1g.bin o1g-1m.bin && ./modbytes o1g-1m.bin 1000000
cp o1g.bin o1g-10m.bin && ./modbytes o1g-10m.bin 10000000
cp o1g.bin o1g-100m.bin && ./modbytes o1g-100m.bin 100000000
