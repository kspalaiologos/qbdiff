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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libqbdiff_private.h"
#include "libsais.h"
#include "libsais64.h"

#define QBDIFF_MAGIC_BIG "QBDB1"
#define QBDIFF_MAGIC_FULL "QBDF1"

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
    int64_t cblen, dblen, eblen;
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

    if(cb == NULL) {
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.cb = cb;
    }

    db = malloc(new_size + new_size / 50 + 5);

    if(db == NULL) {
        free(cb);
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.db = db;
    }

    eb = malloc(new_size + new_size / 50 + 5);

    if(eb == NULL) {
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

    if(cb == NULL) {
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.cb = cb;
    }

    db = malloc(new_size + new_size / 50 + 5);

    if(db == NULL) {
        free(cb);
        result.error = QBERR_NOMEM;
        return result;
    } else {
        result.db = db;
    }

    eb = malloc(new_size + new_size / 50 + 5);

    if(eb == NULL) {
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

        if(I == NULL) {
            struct match_result ml = {0};
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

        if(I == NULL) {
            struct match_result ml = {0};
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
    if (old_size == 0) {
        // Handle the case where the old file is empty.
        if(fwrite(QBDIFF_MAGIC_FULL, 1, 5, diff_file) != 5)
            return QBERR_IOERR;
        if(fwrite(new, 1, new_size, diff_file) != new_size)
            return QBERR_IOERR;
        return QBERR_OK;
    }

    struct match_result ml = diff(old, new, old_size, new_size);

    if(ml.error != QBERR_OK)
        return ml.error;

    #define sfwrite(ptr, size, nmemb, stream) \
        if(fwrite(ptr, size, nmemb, stream) != nmemb) \
            goto io_err

    // TODO: Account for compression.
    if (ml.cblen + ml.dblen + ml.eblen > 2 * new_size) {
        sfwrite(QBDIFF_MAGIC_FULL, 1, 5, diff_file);
        sfwrite(new, 1, new_size, diff_file);
        return QBERR_OK;
    } else {
        sfwrite(QBDIFF_MAGIC_BIG, 1, 5, diff_file);
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
        sfwrite(ml.cb, 1, ml.cblen, diff_file);
        sfwrite(ml.db, 1, ml.dblen, diff_file);
        sfwrite(ml.eb, 1, ml.eblen, diff_file);
    }

    free(ml.cb);
    free(ml.db);
    free(ml.eb);
    return QBERR_OK;

    io_err:
        free(ml.cb);
        free(ml.db);
        free(ml.eb);
        return QBERR_IOERR;
}

int qbdiff_patch(const uint8_t * RESTRICT old, const uint8_t * RESTRICT patch, size_t old_len, size_t patch_len,
                 FILE * new_file) {
    // Check magic
    if (patch_len < 5)
        return QBERR_TRUNCPATCH;
    if (!memcmp(patch, QBDIFF_MAGIC_FULL, 5)) {
        // We can essentially relay diff_file to new_file.
        fwrite(patch + 5, 1, patch_len - 5, new_file);
        return 0;
    } else if (!memcmp(patch, QBDIFF_MAGIC_BIG, 5)) {
        if(patch_len < 45)
            return QBERR_TRUNCPATCH;
        int64_t cblen, dblen, eblen, new_size, old_size, i, ctrl[3];
        old_size = ri64(patch + 5);
        new_size = ri64(patch + 13);
        cblen = ri64(patch + 21);
        dblen = ri64(patch + 29);
        eblen = ri64(patch + 37);
        int64_t cb_off = 45;
        int64_t db_off = cb_off + cblen;
        int64_t eb_off = db_off + dblen;
        if(new_size < 0 || old_size < 0 || cblen < 0 || dblen < 0 || eblen < 0)
            return QBERR_TRUNCPATCH;
        if(eb_off + eblen != patch_len)
            return QBERR_TRUNCPATCH;
        int64_t old_pos = 0, new_pos = 0;
        uint8_t * new_data = malloc(new_size);
        if(new_data == NULL)
            return QBERR_NOMEM;
        memset(new_data, 0, new_size);
        while (new_pos < new_size) {
            for (i = 0; i <= 2; i++) {
                ctrl[i] = ri64(patch + cb_off);
                cb_off += 8;
            }

            if (ctrl[0] < 0 || ctrl[1] < 0) {
                free(new_data);
                return QBERR_BADPATCH;
            }

            /* Sanity-check */
            if (new_pos + ctrl[0] > new_size || ctrl[0] < 0 || new_pos + ctrl[0] < 0) {
                free(new_data);
                return QBERR_BADPATCH;
            }

            memcpy(new_data + new_pos, patch + db_off, ctrl[0]);
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
            if (new_pos + ctrl[1] > new_size || ctrl[1] < 0 || new_pos + ctrl[1] < 0) {
                free(new_data);
                return QBERR_BADPATCH;
            }
            if (old_pos + ctrl[2] > old_size || old_pos + ctrl[2] < 0) {
                free(new_data);
                return QBERR_BADPATCH;
            }

            /* Read extra string */
            memcpy(new_data + new_pos, patch + eb_off, ctrl[1]);
            eb_off += ctrl[1];

            /* Adjust pointers */
            new_pos += ctrl[1];
            old_pos += ctrl[2];
        }

        fwrite(new_data, 1, new_size, new_file);
        free(new_data);
    } else {
        return QBERR_BADPATCH;
    }

    return QBERR_OK;
}

const char * qbdiff_version(void) { return VERSION; }

const char * qbdiff_error(int code) {
    switch(code) {
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
        default:
            return "Unknown error";
    }
}
