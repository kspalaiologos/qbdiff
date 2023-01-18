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

#include "libqbdiff.h"

#include <lzma.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "blake2b.h"
#include "libqbdiff_private.h"
#include "libsais.h"
#include "libsais64.h"

#define QBDIFF_MAGIC_BIG "QBDB1"
#define QBDIFF_MAGIC_FULL "QBDF1"

// LZMA wrappers with a sane API.

static int compress(const uint8_t * src, int64_t src_size, uint8_t ** dest, int64_t * dest_written) {
    lzma_options_lzma opt;
    if(lzma_lzma_preset(&opt, 9 | LZMA_PRESET_EXTREME))
        return QBERR_LZMAERR;

    lzma_filter filters[] = {
        { LZMA_FILTER_LZMA2, &opt },
        { LZMA_VLI_UNKNOWN, NULL }
    };

    *dest = malloc(src_size);
    if(!*dest) {
        *dest_written = 0;
        return QBERR_NOMEM;
    }

    lzma_ret ret;

    ret = lzma_stream_buffer_encode(filters, LZMA_CHECK_CRC64, NULL,
                                    src, src_size, *dest, dest_written, src_size - 1);
    if (ret != LZMA_OK) {
        printf("LZMA err: %d, tried to compress %d bytes from buffer %p to %p.\n", ret, src_size, src, *dest);
        free(*dest);
        *dest = NULL;
        return QBERR_LZMAERR;
    }

    return QBERR_OK;
}

static int decompress(const uint8_t * src, int64_t src_size, uint8_t ** dest, int64_t dest_size) {
    *dest = malloc(dest_size + 1);
    if(!*dest)
        return QBERR_NOMEM;

    size_t mem_limit = UINT64_MAX;
    lzma_ret ret;
    size_t in_pos, out_pos;
    ret = lzma_stream_buffer_decode(&mem_limit, 0, NULL, src, &in_pos, src_size, *dest, &out_pos, dest_size);
    if (ret != LZMA_OK || in_pos != src_size || out_pos != dest_size) {
        free(*dest);
        *dest = NULL;
        return QBERR_LZMAERR;
    }

    return QBERR_OK;
}

// BLAKE2b checksum wrapper.
static void blake2b_cksum(const uint8_t * data, int64_t size, uint8_t cksum[64]) {
    blake2b_state state;
    blake2b_init(&state, 64);
    blake2b_update(&state, data, size);
    memset(cksum, 0, 64);
    blake2b_final(&state, cksum, 64);
}

static int64_t matchlen(const uint8_t * RESTRICT old, int64_t old_size, const uint8_t * RESTRICT new,
                        int64_t new_size) {
    int64_t i, end = min(old_size, new_size);
    for (i = 0; i < end; i++)
        if (old[i] != new[i]) break;
    return i;
}

static void search32(const int32_t * RESTRICT I, const uint8_t * RESTRICT old, int64_t old_size,
                     const uint8_t * RESTRICT new, int64_t new_size, int64_t st, int64_t en, int64_t * old_pos,
                     int64_t * max_len) {
    int64_t x, y;

    /* Initialize max_len for the binary search */
    if (st == 0 && en == old_size) {
        *max_len = matchlen(old, old_size, new, new_size);
        *old_pos = I[st];
    }

    /* The binary search terminates here when "en" and "st" are adjacent
     * indices in the suffix-sorted array. */
    if (en - st < 2) {
        x = matchlen(old + I[st], old_size - I[st], new, new_size);
        if (x > *max_len) {
            *max_len = x;
            *old_pos = I[st];
        }
        y = matchlen(old + I[en], old_size - I[en], new, new_size);
        if (y > *max_len) {
            *max_len = y;
            *old_pos = I[en];
        }

        return;
    }

    x = st + (en - st) / 2;

    int64_t length = min(old_size - I[x], new_size);
    const uint8_t * oldoffset = old + I[x];

    /* This match *could* be the longest one, so check for that here */
    int64_t tmp = matchlen(oldoffset, length, new, length);
    if (tmp > *max_len) {
        *max_len = tmp;
        *old_pos = I[x];
    }

    /* Determine how to continue the binary search */
    if (memcmp(oldoffset, new, length) < 0) {
        return search32(I, old, old_size, new, new_size, x, en, old_pos, max_len);
    } else {
        return search32(I, old, old_size, new, new_size, st, x, old_pos, max_len);
    }
}

static void search64(const int64_t * RESTRICT I, const uint8_t * RESTRICT old, int64_t old_size,
                     const uint8_t * RESTRICT new, int64_t new_size, int64_t st, int64_t en, int64_t * old_pos,
                     int64_t * max_len) {
    int64_t x, y;

    /* Initialize max_len for the binary search */
    if (st == 0 && en == old_size) {
        *max_len = matchlen(old, old_size, new, new_size);
        *old_pos = I[st];
    }

    /* The binary search terminates here when "en" and "st" are adjacent
     * indices in the suffix-sorted array. */
    if (en - st < 2) {
        x = matchlen(old + I[st], old_size - I[st], new, new_size);
        if (x > *max_len) {
            *max_len = x;
            *old_pos = I[st];
        }
        y = matchlen(old + I[en], old_size - I[en], new, new_size);
        if (y > *max_len) {
            *max_len = y;
            *old_pos = I[en];
        }

        return;
    }

    x = st + (en - st) / 2;

    int64_t length = min(old_size - I[x], new_size);
    const uint8_t * oldoffset = old + I[x];

    /* This match *could* be the longest one, so check for that here */
    int64_t tmp = matchlen(oldoffset, length, new, length);
    if (tmp > *max_len) {
        *max_len = tmp;
        *old_pos = I[x];
    }

    /* Determine how to continue the binary search */
    if (memcmp(oldoffset, new, length) < 0) {
        return search64(I, old, old_size, new, new_size, x, en, old_pos, max_len);
    } else {
        return search64(I, old, old_size, new, new_size, st, x, old_pos, max_len);
    }
}

struct match_result {
    size_t cblen, dblen, eblen;
    uint8_t *cb, *db, *eb;
    int16_t error;
};

static struct match_result match32(const int32_t * RESTRICT I, const uint8_t * RESTRICT old,
                                   const uint8_t * RESTRICT new, int64_t new_size, int64_t old_size) {
    int64_t new_pos = 0;
    int64_t old_pos = 0;
    int64_t match_len = 0;
    int64_t last_offset = 0;
    int64_t last_new_pos = 0;
    int64_t last_old_pos = 0;

    int64_t cblen = 0, dblen = 0, eblen = 0;

    uint8_t *cb, *db, *eb;

    struct match_result result;
    result.cb = NULL;
    result.db = NULL;
    result.eb = NULL;
    result.cblen = 0;
    result.dblen = 0;
    result.eblen = 0;
    result.error = QBERR_OK;

    cb = malloc(new_size + new_size / 50 + 5);

    if (cb == NULL) {
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.cb = cb;
    }

    db = malloc(new_size + new_size / 50 + 5);

    if (db == NULL) {
        free(cb);
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.db = db;
    }

    eb = malloc(new_size + new_size / 50 + 5);

    if (eb == NULL) {
        free(cb);
        free(db);
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.eb = eb;
    }

    while (new_pos < new_size) {
        int64_t old_score = 0;
        int64_t new_peek;
        for (new_peek = new_pos += match_len; new_pos < new_size; new_pos++) {
            search32(I, old, old_size, new + new_pos, new_size - new_pos, 0, old_size, &old_pos, &match_len);

            for (; new_peek < new_pos + match_len; new_peek++) {
                if ((new_peek + last_offset < old_size) && (old[new_peek + last_offset] == new[new_peek])) {
                    old_score++;
                }
            }

            if (((match_len == old_score) && (match_len != 0)) || (match_len > old_score + 8)) {
                break;
            }

            // Before beginning the next loop iteration, decrement
            // old_score if needed, since new_pos will be
            // incremented.
            if ((new_pos + last_offset < old_size) && (old[new_pos + last_offset] == new[new_pos])) {
                old_score--;
            }
        }

        if ((match_len != old_score) || (new_pos == new_size)) {
            int64_t bytes = 0, max = 0;
            int64_t len_fuzzyforward = 0;
            for (int64_t i = 0; (last_new_pos + i < new_pos) && (last_old_pos + i < old_size);) {
                if (old[last_old_pos + i] == new[last_new_pos + i]) {
                    bytes++;
                }
                i++;
                if (bytes * 2 - i > max * 2 - len_fuzzyforward) {
                    max = bytes;
                    len_fuzzyforward = i;
                }
            }

            int64_t len_fuzzybackward = 0;
            if (new_pos < new_size) {
                bytes = 0;
                max = 0;
                for (int64_t i = 1; (new_pos >= last_new_pos + i) && (old_pos >= i); i++) {
                    if (old[old_pos - i] == new[new_pos - i]) {
                        bytes++;
                    }
                    if (bytes * 2 - i > max * 2 - len_fuzzybackward) {
                        max = bytes;
                        len_fuzzybackward = i;
                    }
                }
            }

            // If there is an overlap between len_fuzzyforward and
            // len_fuzzybackward in the new file, that overlap must
            // be eliminated.
            if (last_new_pos + len_fuzzyforward > new_pos - len_fuzzybackward) {
                bytes = 0;
                max = 0;
                int64_t overlap = (last_new_pos + len_fuzzyforward) - (new_pos - len_fuzzybackward);
                int64_t len_fuzzyshift = 0;
                for (int64_t i = 0; i < overlap; i++) {
                    if (new[last_new_pos + len_fuzzyforward - overlap + i] ==
                        old[last_old_pos + len_fuzzyforward - overlap + i]) {
                        bytes++;
                    }
                    if (new[new_pos - len_fuzzybackward + i] == old[old_pos - len_fuzzybackward + i]) {
                        bytes--;
                    }
                    if (bytes > max) {
                        max = bytes;
                        len_fuzzyshift = i + 1;
                    }
                }

                len_fuzzyforward += len_fuzzyshift - overlap;
                len_fuzzybackward -= len_fuzzyshift;
            }

            for (int64_t i = 0; i < len_fuzzyforward; i++) {
                db[dblen + i] = new[last_new_pos + i] - old[last_old_pos + i];
            }

            for (int64_t i = 0; i < (new_pos - len_fuzzybackward) - (last_new_pos + len_fuzzyforward); i++) {
                eb[eblen + i] = new[last_new_pos + len_fuzzyforward + i];
            }

            dblen += len_fuzzyforward;
            eblen += (new_pos - len_fuzzybackward) - (last_new_pos + len_fuzzyforward);

            wi64(len_fuzzyforward, cb + cblen);
            cblen += 8;

            wi64((new_pos - len_fuzzybackward) - (last_new_pos + len_fuzzyforward), cb + cblen);
            cblen += 8;

            wi64((old_pos - len_fuzzybackward) - (last_old_pos + len_fuzzyforward), cb + cblen);
            cblen += 8;

            last_new_pos = new_pos - len_fuzzybackward;
            last_old_pos = old_pos - len_fuzzybackward;
            last_offset = old_pos - new_pos;
        }
    }

    result.cblen = cblen;
    result.dblen = dblen;
    result.eblen = eblen;

    return result;
}

static struct match_result match64(const int64_t * RESTRICT I, const uint8_t * RESTRICT old,
                                   const uint8_t * RESTRICT new, int64_t new_size, int64_t old_size) {
    int64_t new_pos = 0;
    int64_t old_pos = 0;
    int64_t match_len = 0;
    int64_t last_offset = 0;
    int64_t last_new_pos = 0;
    int64_t last_old_pos = 0;

    int64_t cblen = 0, dblen = 0, eblen = 0;

    uint8_t *cb, *db, *eb;

    struct match_result result;
    result.cb = NULL;
    result.db = NULL;
    result.eb = NULL;
    result.cblen = 0;
    result.dblen = 0;
    result.eblen = 0;
    result.error = QBERR_OK;

    cb = malloc(new_size + new_size / 50 + 5);

    if (cb == NULL) {
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.cb = cb;
    }

    db = malloc(new_size + new_size / 50 + 5);

    if (db == NULL) {
        free(cb);
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.db = db;
    }

    eb = malloc(new_size + new_size / 50 + 5);

    if (eb == NULL) {
        free(cb);
        free(db);
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.eb = eb;
    }

    while (new_pos < new_size) {
        int64_t old_score = 0;
        int64_t new_peek;
        for (new_peek = new_pos += match_len; new_pos < new_size; new_pos++) {
            search64(I, old, old_size, new + new_pos, new_size - new_pos, 0, old_size, &old_pos, &match_len);

            for (; new_peek < new_pos + match_len; new_peek++) {
                if ((new_peek + last_offset < old_size) && (old[new_peek + last_offset] == new[new_peek])) {
                    old_score++;
                }
            }

            if (((match_len == old_score) && (match_len != 0)) || (match_len > old_score + 8)) {
                break;
            }

            // Before beginning the next loop iteration, decrement
            // old_score if needed, since new_pos will be
            // incremented.
            if ((new_pos + last_offset < old_size) && (old[new_pos + last_offset] == new[new_pos])) {
                old_score--;
            }
        }

        if ((match_len != old_score) || (new_pos == new_size)) {
            int64_t bytes = 0, max = 0;
            int64_t len_fuzzyforward = 0;
            for (int64_t i = 0; (last_new_pos + i < new_pos) && (last_old_pos + i < old_size);) {
                if (old[last_old_pos + i] == new[last_new_pos + i]) {
                    bytes++;
                }
                i++;
                if (bytes * 2 - i > max * 2 - len_fuzzyforward) {
                    max = bytes;
                    len_fuzzyforward = i;
                }
            }

            int64_t len_fuzzybackward = 0;
            if (new_pos < new_size) {
                bytes = 0;
                max = 0;
                for (int64_t i = 1; (new_pos >= last_new_pos + i) && (old_pos >= i); i++) {
                    if (old[old_pos - i] == new[new_pos - i]) {
                        bytes++;
                    }
                    if (bytes * 2 - i > max * 2 - len_fuzzybackward) {
                        max = bytes;
                        len_fuzzybackward = i;
                    }
                }
            }

            // If there is an overlap between len_fuzzyforward and
            // len_fuzzybackward in the new file, that overlap must
            // be eliminated.
            if (last_new_pos + len_fuzzyforward > new_pos - len_fuzzybackward) {
                bytes = 0;
                max = 0;
                int64_t overlap = (last_new_pos + len_fuzzyforward) - (new_pos - len_fuzzybackward);
                int64_t len_fuzzyshift = 0;
                for (int64_t i = 0; i < overlap; i++) {
                    if (new[last_new_pos + len_fuzzyforward - overlap + i] ==
                        old[last_old_pos + len_fuzzyforward - overlap + i]) {
                        bytes++;
                    }
                    if (new[new_pos - len_fuzzybackward + i] == old[old_pos - len_fuzzybackward + i]) {
                        bytes--;
                    }
                    if (bytes > max) {
                        max = bytes;
                        len_fuzzyshift = i + 1;
                    }
                }

                len_fuzzyforward += len_fuzzyshift - overlap;
                len_fuzzybackward -= len_fuzzyshift;
            }

            for (int64_t i = 0; i < len_fuzzyforward; i++) {
                db[dblen + i] = new[last_new_pos + i] - old[last_old_pos + i];
            }

            for (int64_t i = 0; i < (new_pos - len_fuzzybackward) - (last_new_pos + len_fuzzyforward); i++) {
                eb[eblen + i] = new[last_new_pos + len_fuzzyforward + i];
            }

            dblen += len_fuzzyforward;
            eblen += (new_pos - len_fuzzybackward) - (last_new_pos + len_fuzzyforward);

            wi64(len_fuzzyforward, cb + cblen);
            cblen += 8;

            wi64((new_pos - len_fuzzybackward) - (last_new_pos + len_fuzzyforward), cb + cblen);
            cblen += 8;

            wi64((old_pos - len_fuzzybackward) - (last_old_pos + len_fuzzyforward), cb + cblen);
            cblen += 8;

            last_new_pos = new_pos - len_fuzzybackward;
            last_old_pos = old_pos - len_fuzzybackward;
            last_offset = old_pos - new_pos;
        }
    }

    result.cblen = cblen;
    result.dblen = dblen;
    result.eblen = eblen;

    return result;
}

static struct match_result diff(const uint8_t * old, const uint8_t * new, const size_t old_size,
                                const size_t new_size) {
    if (old_size < INT32_MAX - 8) {
        int32_t * I = malloc((old_size + 1) * sizeof(int32_t));

        if (I == NULL) {
            struct match_result ml = { 0 };
            ml.error = QBERR_NOMEM;
            return ml;
        }

#if defined(_OPENMP)
        libsais_omp(old, I, old_size, 1, NULL, 0);
#else
        libsais(old, I, old_size, 1, NULL);
#endif

        struct match_result ml = match32(I, old, new, new_size, old_size);

        free(I);

        return ml;
    } else {
        int64_t * I = malloc((old_size + 1) * sizeof(int64_t));

        if (I == NULL) {
            struct match_result ml = { 0 };
            ml.error = QBERR_NOMEM;
            return ml;
        }

#if defined(_OPENMP)
        libsais64_omp(old, I, old_size, 1, NULL, 0);
#else
        libsais64(old, I, old_size, 1, NULL);
#endif

        struct match_result ml = match64(I, old, new, new_size, old_size);

        free(I);

        return ml;
    }
}

int qbdiff_compute(const uint8_t * RESTRICT old, const uint8_t * RESTRICT new, size_t old_size, size_t new_size,
                   FILE * diff_file) {
    int err_code = 0;
    uint8_t cksum[64];
    blake2b_cksum(new, new_size, cksum);

    if (old_size == 0) {
        // Handle the case where the old file is empty.
        if (fwrite(QBDIFF_MAGIC_FULL, 1, 5, diff_file) != 5) return QBERR_IOERR;
        if (fwrite(cksum, 1, 64, diff_file) != 64) return QBERR_IOERR;
        
        uint8_t * compressed;
        size_t compressed_len;
        int result = compress(new, new_size, &compressed, &compressed_len);
        if (result != QBERR_OK) return result;

        uint8_t buf[8];
        wi64(new_size, buf);
        if (fwrite(buf, 1, 8, diff_file) != 8) {
            free(compressed);
            return QBERR_IOERR;
        }

        if (fwrite(compressed, 1, compressed_len, diff_file) != compressed_len) {
            free(compressed);
            return QBERR_IOERR;
        }

        free(compressed);
        return QBERR_OK;
    }

    struct match_result ml = diff(old, new, old_size, new_size);

    if (ml.error != QBERR_OK) return ml.error;

    uint8_t *newcb = NULL, *newdb = NULL, *neweb = NULL;

    size_t orig_cb_len, orig_db_len, orig_eb_len;
    orig_cb_len = ml.cblen;
    orig_db_len = ml.dblen;
    orig_eb_len = ml.eblen;

    err_code = compress(ml.cb, ml.cblen, &newcb, &ml.cblen);
    if (err_code != QBERR_OK) goto err;

    err_code = compress(ml.db, ml.dblen, &newdb, &ml.dblen);
    if (err_code != QBERR_OK) goto err;

    err_code = compress(ml.eb, ml.eblen, &neweb, &ml.eblen);
    if (err_code != QBERR_OK) goto err;

#define sfwrite(ptr, size, nmemb, stream) \
    if (fwrite(ptr, size, nmemb, stream) != nmemb) { err_code = QBERR_IOERR; goto err; }

    // TODO: Account for compression.
    if (ml.cblen + ml.dblen + ml.eblen > 2 * new_size) {
        err_code = QBERR_IOERR;
        if (fwrite(QBDIFF_MAGIC_FULL, 1, 5, diff_file) != 5) goto err;
        if (fwrite(cksum, 1, 64, diff_file) != 64) goto err;
        
        uint8_t * compressed;
        size_t compressed_len;
        err_code = compress(new, new_size, &compressed, &compressed_len);
        if (err_code != QBERR_OK) goto err;

        err_code = QBERR_IOERR;
        uint8_t buf[8];
        wi64(new_size, buf);
        if (fwrite(buf, 1, 8, diff_file) != 8) goto err;
        if (fwrite(compressed, 1, compressed_len, diff_file) != compressed_len) goto err;
        free(compressed);
    } else {
        sfwrite(QBDIFF_MAGIC_BIG, 1, 5, diff_file);
        sfwrite(cksum, 1, 64, diff_file);
        uint8_t buf[8];
        wi64(old_size, buf);
        sfwrite(buf, 1, 8, diff_file);
        wi64(new_size, buf);
        sfwrite(buf, 1, 8, diff_file);
        wi64(ml.cblen, buf);
        sfwrite(buf, 1, 8, diff_file);
        wi64(ml.dblen, buf);
        sfwrite(buf, 1, 8, diff_file);
        wi64(ml.eblen, buf);
        sfwrite(buf, 1, 8, diff_file);
        wi64(orig_cb_len, buf);
        sfwrite(buf, 1, 8, diff_file);
        wi64(orig_db_len, buf);
        sfwrite(buf, 1, 8, diff_file);
        wi64(orig_eb_len, buf);
        sfwrite(buf, 1, 8, diff_file);
        sfwrite(ml.cb, 1, ml.cblen, diff_file);
        sfwrite(ml.db, 1, ml.dblen, diff_file);
        sfwrite(ml.eb, 1, ml.eblen, diff_file);
    }

    free(newcb);
    free(newdb);
    free(neweb);
    free(ml.cb);
    free(ml.db);
    free(ml.eb);
    return QBERR_OK;

err:
    free(newcb);
    free(newdb);
    free(neweb);
    free(ml.cb);
    free(ml.db);
    free(ml.eb);
    return err_code;
}

int qbdiff_patch(const uint8_t * RESTRICT old, const uint8_t * RESTRICT patch, size_t old_len, size_t patch_len,
                 FILE * new_file) {
    // Check magic
    if (patch_len < 70) return QBERR_TRUNCPATCH;
    if (!memcmp(patch, QBDIFF_MAGIC_FULL, 5)) {
        // We can essentially relay diff_file to new_file.
        uint8_t new_cksum[64];
        blake2b_cksum(patch + 77, patch_len - 77, new_cksum);
        if (memcmp(patch + 5, new_cksum, 64)) return QBERR_BADCKSUM;
        int64_t uncompressed_size = ri64(patch + 69);
        uint8_t * uncompressed;
        int result = decompress(patch + 77, patch_len - 77, &uncompressed, uncompressed_size);
        if(result != QBERR_OK) return result;
        if (fwrite(uncompressed, 1, uncompressed_size, new_file) != uncompressed_size) {
            free(uncompressed);
            return QBERR_IOERR;
        }
        free(uncompressed);
        return QBERR_OK;
    } else if (!memcmp(patch, QBDIFF_MAGIC_BIG, 5)) {
        if (patch_len < 45) return QBERR_TRUNCPATCH;
        int64_t cblen, dblen, eblen, orig_cblen, orig_dblen, orig_eblen, new_size, old_size, i, ctrl[3],
            errn = QBERR_OK;
        old_size = ri64(patch + 69);
        new_size = ri64(patch + 77);
        cblen = ri64(patch + 85);
        dblen = ri64(patch + 93);
        eblen = ri64(patch + 101);
        orig_cblen = ri64(patch + 109);
        orig_dblen = ri64(patch + 117);
        orig_eblen = ri64(patch + 125);
        if (old_size != old_len) return QBERR_BADPATCH;
        int64_t cb_off = 133;
        int64_t db_off = cb_off + cblen;
        int64_t eb_off = db_off + dblen;
        if (new_size < 0 || old_size < 0 || cblen < 0 || dblen < 0 || eblen < 0 || orig_cblen < 0 || orig_dblen < 0 ||
            orig_eblen < 0)
            return QBERR_TRUNCPATCH;
        if (eb_off + eblen != patch_len) return QBERR_TRUNCPATCH;
        int64_t old_pos = 0, new_pos = 0;
        uint8_t * new_data = malloc(new_size);
        if (new_data == NULL) return QBERR_NOMEM;

        uint8_t *cb = NULL, *db = NULL, *eb = NULL;

        errn = decompress(patch + cb_off, cblen, &cb, orig_cblen);
        if(errn != QBERR_OK) goto err;
        errn = decompress(patch + db_off, dblen, &db, orig_dblen);
        if(errn != QBERR_OK) goto err;
        errn = decompress(patch + eb_off, eblen, &eb, orig_eblen);
        if(errn != QBERR_OK) goto err;

        memset(new_data, 0, new_size);
        cb_off = 0;
        db_off = 0;
        eb_off = 0;
        while (new_pos < new_size) {
            for (i = 0; i <= 2; i++) {
                ctrl[i] = ri64(cb + cb_off);
                cb_off += 8;
            }

            if (ctrl[0] < 0 || ctrl[1] < 0 || new_pos + ctrl[0] > new_size || ctrl[0] < 0 || new_pos + ctrl[0] < 0) {
                errn = QBERR_BADPATCH;
                goto err;
            }

            memcpy(new_data + new_pos, db + db_off, ctrl[0]);
            db_off += ctrl[0];

            /* Add old data to diff string */
            for (i = 0; i < ctrl[0]; i++) {
                if ((old_pos + i >= 0) && (old_pos + i < old_size)) {
                    new_data[new_pos + i] += old[old_pos + i];
                }
            }

            /* Adjust pointers */
            new_pos += ctrl[0];
            old_pos += ctrl[0];

            /* Sanity-check */
            if (new_pos + ctrl[1] > new_size || ctrl[1] < 0 || new_pos + ctrl[1] < 0 || old_pos + ctrl[2] > old_size ||
                old_pos + ctrl[2] < 0) {
                errn = QBERR_BADPATCH;
                goto err;
            }

            /* Read extra string */
            memcpy(new_data + new_pos, eb + eb_off, ctrl[1]);
            eb_off += ctrl[1];

            /* Adjust pointers */
            new_pos += ctrl[1];
            old_pos += ctrl[2];
        }

        uint8_t new_cksum[64];
        blake2b_cksum(new_data, new_size, new_cksum);
        if (memcmp(patch + 5, new_cksum, 64)) {
            errn = QBERR_BADCKSUM;
            goto err;
        }

        if (fwrite(new_data, 1, new_size, new_file) != new_size) {
            errn = QBERR_IOERR;
            goto err;
        }

        free(cb);
        free(db);
        free(eb);
        free(new_data);
        return QBERR_OK;

    err:
        free(cb);
        free(db);
        free(eb);
        free(new_data);
        return errn;
    } else {
        return QBERR_BADPATCH;
    }
}

const char * qbdiff_version(void) { return VERSION; }

const char * qbdiff_error(int code) {
    switch (code) {
        case QBERR_OK:
            return "No error";
        case QBERR_NOMEM:
            return "Out of memory";
        case QBERR_IOERR:
            return "I/O error";
        case QBERR_BADPATCH:
            return "Bad patch";
        case QBERR_TRUNCPATCH:
            return "Truncated patch";
        case QBERR_BADCKSUM:
            return "Bad checksum";
        case QBERR_LZMAERR:
            return "LZMA error";
        default:
            return "Unknown error";
    }
}
