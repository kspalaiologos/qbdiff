/*
   Based on the public domain BLAKE2b C code.
   Copyright (C) 2022 Kamila Szewczyk

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _BLAKE2B_H
#define _BLAKE2B_H

#include <stddef.h>
#include <stdint.h>

enum {
    BLAKE2B_BLOCKBYTES = 128,
    BLAKE2B_OUTBYTES = 64,
    BLAKE2B_KEYBYTES = 64,
    BLAKE2B_SALTBYTES = 16,
    BLAKE2B_PERSONALBYTES = 16
};

typedef struct {
    uint64_t h[8];
    uint64_t t[2];
    uint64_t f[2];
    uint8_t buf[BLAKE2B_BLOCKBYTES];
    size_t buflen;
    size_t outlen;
    uint8_t last_node;
} blake2b_state;

int blake2b_init(blake2b_state * S, size_t outlen);
int blake2b_update(blake2b_state * S, const void * in, size_t inlen);
int blake2b_final(blake2b_state * S, void * out, size_t outlen);

#endif
