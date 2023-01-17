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

#ifndef LIBQBDIFF_PRIVATE_H
#define LIBQBDIFF_PRIVATE_H

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifndef RESTRICT
    #define RESTRICT
#endif

#ifndef VERSION
    #define VERSION "0.0.0"
#endif

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// Endianness-agnostic read/write functions.
static inline void wi64(int64_t x, uint8_t * buf) {
    buf[0] = (x >> 56) & 0xff;
    buf[1] = (x >> 48) & 0xff;
    buf[2] = (x >> 40) & 0xff;
    buf[3] = (x >> 32) & 0xff;
    buf[4] = (x >> 24) & 0xff;
    buf[5] = (x >> 16) & 0xff;
    buf[6] = (x >> 8) & 0xff;
    buf[7] = x & 0xff;
}

static inline int64_t ri64(const uint8_t * buf) {
    return ((int64_t)buf[0] << 56) | ((int64_t)buf[1] << 48) | ((int64_t)buf[2] << 40) | ((int64_t)buf[3] << 32) |
           ((int64_t)buf[4] << 24) | ((int64_t)buf[5] << 16) | ((int64_t)buf[6] << 8) | (int64_t)buf[7];
}

// Open the binary output file.
#ifdef __linux__
#include <sys/stat.h>

static int is_dir(const char * path) {
    struct stat sb;
    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)) return 1;
    return 0;
}
#else
#include <windows.h>

static int is_dir(const char * path) {
    DWORD attr = GetFileAttributes(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return 0;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
#endif

static FILE * open_output(char * output) {
    FILE * output_des = NULL;

    if (is_dir(output)) {
        fprintf(stderr, "Error: output file `%s' is a directory.\n", output);
        exit(1);
    }

    output_des = fopen(output, "wb");
    if (output_des == NULL) {
        fprintf(stderr, "Error: failed to open output file `%s': %s\n", output, strerror(errno));
        exit(1);
    }

    return output_des;
}

#ifdef __linux__
    // Needed for fsync.
    #include <unistd.h>
#endif

// Sync the data to disk using fsync.
static void close_out_file(FILE * des) {
    if (des) {
        if (fflush(des)) {
            fprintf(stderr, "Error: Failed on fflush: %s\n", strerror(errno));
            exit(1);
        }

#ifdef __linux__
        int outfd = fileno(des);

        while (1) {
            int status = fsync(outfd);
            if (status == -1) {
                if (errno == EINVAL) break;
                if (errno == EINTR) continue;
                fprintf(stderr, "Error: Failed on fsync: %s\n", strerror(errno));
                exit(1);
            }
            break;
        }
#endif

        if (fclose(des)) {
            fprintf(stderr, "Error: Failed on fclose: %s\n", strerror(errno));
            exit(1);
        }
    }
}

// Supply a slightly cross-platform file mapping utility.
// Used only by the CLI utilities, so we can simplify error handling.
struct file_mapping {
    uint8_t * data;
    size_t length;

    void *h1, *h2;
};

#ifdef _WIN32

    #include <windows.h>

struct file_mapping map_file(char * path) {
    struct file_mapping mapping = { 0 };
    HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Could not open file '%s'.\n", path);
        exit(1);
    }

    HANDLE mapping_handle = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapping_handle == NULL) {
        fprintf(stderr, "Could not create file mapping for '%s'.\n", path);
        exit(1);
    }

    mapping.data = MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
    if (mapping.data == NULL) {
        fprintf(stderr, "Could not map file '%s'.\n", path);
        exit(1);
    }

    mapping.length = GetFileSize(file, NULL);
    return mapping;
}

void unmap_file(struct file_mapping mapping) {
    UnmapViewOfFile(mapping.data);
    CloseHandle(mapping.h2);
    CloseHandle(mapping.h1);
}

#else

    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>

struct file_mapping map_file(char * path) {
    struct file_mapping mapping = { 0 };
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Could not open file '%s'.\n", path);
        exit(1);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "Could not stat file '%s'.\n", path);
        exit(1);
    }

    mapping.data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapping.data == MAP_FAILED) {
        fprintf(stderr, "Could not map file '%s'.\n", path);
        exit(1);
    }

    mapping.length = st.st_size;
    mapping.h1 = (void *)(intptr_t)fd;
    mapping.h2 = NULL;
    return mapping;
}

void unmap_file(struct file_mapping mapping) {
    munmap(mapping.data, mapping.length);
    close((int)(intptr_t)mapping.h1);
}

#endif

#endif
