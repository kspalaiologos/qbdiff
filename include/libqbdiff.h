/*
 * qbdiff - Quick Binary Diff
 * Copyright (C) 2023 Kamila Szewczyk
 *
 * Based on the algorithm developed by Colin Percival:
 * Copyright (C) 2003-2005 Colin Percival
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBQBDIFF_H
#define LIBQBDIFF_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define QBERR_OK 0
#define QBERR_NOMEM 1
#define QBERR_IOERR 2
#define QBERR_TRUNCPATCH 3
#define QBERR_BADPATCH 4
#define QBERR_BADCKSUM 5
#define QBERR_LZMAERR 6
#define QBERR_SAIS 7

int qbdiff_compute(const uint8_t * old, const uint8_t * new, size_t old_len, size_t new_len, FILE * diff_file);
int qbdiff_patch(const uint8_t * old, const uint8_t * patch, size_t old_len, size_t patch_len, FILE * new_file);
const char * qbdiff_version(void);
const char * qbdiff_error(int code);

#endif
