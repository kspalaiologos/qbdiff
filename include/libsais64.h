/*--

This file is a part of libsais, a library for linear time suffix array,
longest common prefix array and burrows wheeler transform construction.

   Copyright (c) 2021-2022 Ilya Grebnov <ilya.grebnov@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

Please see the file 3rdparty/libsais-LICENSE for full copyright information.

--*/

#ifndef LIBSAIS64_H
#define LIBSAIS64_H 1

#include <stdint.h>

/**
 * Constructs the suffix array of a given string.
 * @param T [0..n-1] The input string.
 * @param SA [0..n-1+fs] The output array of suffixes.
 * @param n The length of the given string.
 * @param fs The extra space available at the end of SA array (0 should be enough for most cases).
 * @param freq [0..255] The output symbol frequency table (can be NULL).
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
int64_t libsais64(const uint8_t * T, int64_t * SA, int64_t n, int64_t fs, int64_t * freq);

#if defined(_OPENMP)
/**
 * Constructs the suffix array of a given string in parallel using OpenMP.
 * @param T [0..n-1] The input string.
 * @param SA [0..n-1+fs] The output array of suffixes.
 * @param n The length of the given string.
 * @param fs The extra space available at the end of SA array (0 should be enough for most cases).
 * @param freq [0..255] The output symbol frequency table (can be NULL).
 * @param threads The number of OpenMP threads to use (can be 0 for OpenMP default).
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
int64_t libsais64_omp(const uint8_t * T, int64_t * SA, int64_t n, int64_t fs, int64_t * freq, int64_t threads);
#endif

#endif
