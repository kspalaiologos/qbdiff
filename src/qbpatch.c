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
                "Usage: qbpatch oldfile newfile deltafile\n\n"
                "Applies the binary patch DELTAFILE to OLDFILE to create file "
                "NEWFILE.\n",
                qbdiff_version());
        return 1;
    }

    struct file_mapping old_file, delta_file;
    old_file = map_file(argv[1]);
    delta_file = map_file(argv[3]);

    FILE * new_file = fopen(argv[2], "wb");
    if (!new_file) {
        fprintf(stderr, "Failed to open new file %s for writing: %s\n", argv[2], strerror(errno));
        return 1;
    }

    int ret = qbdiff_patch(old_file.data, delta_file.data, old_file.length, delta_file.length, new_file);
    if (ret != QBERR_OK) {
        fprintf(stderr, "Failed to patch (error %d: %s)\n", ret, qbdiff_error(ret));
        return 1;
    }

    close_out_file(new_file);

    unmap_file(old_file);
    unmap_file(delta_file);

    return 0;
}
