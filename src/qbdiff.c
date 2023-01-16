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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "libqbdiff.h"
#include "libqbdiff_private.h"

int main(int argc, char * argv[]) {
    if (argc < 4) {
        fprintf(stderr,
                "qbdiff %s - Quick Binary Diff\n"
                "Usage: qbdiff oldfile newfile deltafile\n\n"
                "Creates a binary patch DELTAFILE from OLDFILE to NEWFILE.\n",
                qbdiff_version());
        return 1;
    }

    struct file_mapping old_file, new_file;
    old_file = map_file(argv[1]);
    new_file = map_file(argv[2]);

    FILE * delta_file = fopen(argv[3], "wb");
    if (!delta_file) {
        fprintf(stderr, "Failed to open delta file %s for writing: %s\n", argv[3], strerror(errno));
        return 1;
    }

    if(old_file.length > INT32_MAX - 1) {
        fprintf(stderr, "NYI: Old file over 2 gigabytes, refusing to diff.\n");
        return 1;
    }

    if(new_file.length > INT32_MAX - 1) {
        fprintf(stderr, "NYI: New file over 2 gigabytes, refusing to diff.\n");
        return 1;
    }

    int ret = qbdiff_compute(old_file.data, new_file.data, old_file.length, new_file.length, delta_file);
    if (ret) {
        fprintf(stderr, "Failed to create delta (error %d: %s)\n", ret, qbdiff_error(ret));
        return 1;
    }

    fclose(delta_file);
    unmap_file(old_file);
    unmap_file(new_file);

    return 0;
}
