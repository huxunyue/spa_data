/*
Copyright (C) 2024, Sorbonne Universite, LIP6
This file is part of the Blind-Folded work, under the GPL v3.0 license
See https://www.gnu.org/licenses/gpl-3.0.en.html for license information
SPDX-License-Identifier: GPL-3.0-only
Author(s): Xunyue HU
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


#define LEN_BASE 7  /* base is 7 limbs */
#define OFFSET_SHORT 36
#define OFFSET_MEDIUM 30
#define OFFSET_LARGE 16

const char * trace_file = NULL;
const char * pattern_file_0 = NULL;
const char * pattern_file_1 = NULL;
char pattern_files = '\0';




#define min(x, y) ((x) < (y) ? (x) : (y))


int64_t fsize(const char * filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    fprintf(stderr, "*** Error: cannot determine size of %s: %s\n", filename, strerror(errno));
    return -1;
}


int parsing_single_pattern_start(int lst[4096][17], float * pattern, int64_t pattern_len, float * trace, int64_t nb_samples) {
    int c = 0;
    int n_limb = 0;
    for (int i = 0; i < (nb_samples - pattern_len); i += 1) {
        double diff = 0;
        for (int j = 0; j < pattern_len; j += 1) {
            diff += fabs(pattern[j] - trace[i + j]);
        }

        if (diff / pattern_len < 0.0095) {
            assert(c < 4096);

            lst[c][0] = i + OFFSET_LARGE;
            //if (c > 0 && (lst[c][16] - lst[c - 1][16]) > 250000) {
            //    printf("# [%d:%d]\n", lst[c - 1][16], lst[c][16]);
            //}
            for (int j = 1; j < 16; j += 1) {
                n_limb = LEN_BASE * (1 + (j - 1) * 2);
                n_limb = min(n_limb, 64);
                lst[c][j] = lst[c][j - 1] + (89 + 13 * n_limb);
            }
            lst[c][16] = lst[c][15] + 916;
            //printf("# ROI %d: %d\n", c, lst[c][0]);
            //printf("# POIS: ");
            //for (int u = 0; u < 17; u += 1) {
            //    printf("%d, ", lst[c][u]);
            //}
            //printf("\n");
            c += 1;
            i += 30000;
        }
    }
    return c;
}


int parsing_double_patterns_start(int lst[4096][17], float * pattern_1, float * pattern_0, int64_t pattern_len, float * trace, int64_t nb_samples) {
    int c = 0;
    int n_limb = 0;
    for (int i = 0; i < (nb_samples - pattern_len); i += 1) {
        double diff0 = 0;
        double diff1 = 0;
        for (int j = 0; j < pattern_len; j += 1) {
            diff0 += fabs(pattern_0[j] - trace[i + j]);
            diff1 += fabs(pattern_1[j] - trace[i + j]);
        }

        if (diff0 / pattern_len < 0.0068 || diff1 / pattern_len < 0.0155) {  //0.012550 
            assert(c < 4096);

            lst[c][0] = i + OFFSET_MEDIUM;
            //if (c > 0 && (lst[c][16] - lst[c - 1][16]) > 250000) {
            //    printf("# [%d:%d]\n", lst[c - 1][16], lst[c][16]);
            //}
            for (int j = 1; j < 16; j += 1) {
                n_limb = LEN_BASE * (1 + (j - 1) * 2);
                n_limb = min(n_limb, 64);
                lst[c][j] = lst[c][j - 1] + (89 + 13 * n_limb);
            }
            lst[c][16] = lst[c][15] + 916;
            c += 1;
            i += 30000;
        }
    }
    return c;
}


int parsing_double_patterns_end(int lst[4096][17], float * pattern_1, float * pattern_0, int64_t pattern_len, float * trace, int64_t nb_samples) {
    int c = 0;
    int n_limb = 0;
    int offset;
    for (int i = 0; i < (nb_samples - pattern_len); i += 1) {
        double diff0 = 0;
        double diff1 = 0;
        for (int j = 0; j < pattern_len; j += 1) {
            diff0 += fabs(pattern_0[j] - trace[i + j]);
            diff1 += fabs(pattern_1[j] - trace[i + j]);
        }

        if (diff0 / pattern_len < 0.0095 || diff1 / pattern_len < 0.0085) {
            assert(c < 4096);

            lst[c][16] = i + OFFSET_SHORT;
            //if (c > 0 && (lst[c][16] - lst[c - 1][16]) > 250000) {
            //    printf("# [%d:%d]\n", lst[c - 1][16], lst[c][16]);
            //}
            lst[c][15] = lst[c][16] - 916;
            for (int j = 14; j >= 0; j--) {
                n_limb = LEN_BASE * (1 + j * 2);
                if (n_limb < 64) {
                    lst[c][j] = lst[c][j + 1] - (89 + 13 * n_limb);
                }
                else {
                    lst[c][j] = lst[c][j + 1] - 921;   // 921 = 89 + 13 * 64  // max_n_limb = 64
                }
            }
            c += 1;
            i += 30000;
        }
    }
    return c;
}


// Writes in b the the value of s in binary, returns the binary size
int print_bin(int b[5], int s) {
    int i = 1;
    b[4] = 1;

    while (s != 0) {
        i += 1;
        b[5 - i] = s & 1;
        s >>= 1;
    }

    return i;
}


// Translate the secret found in binary and returns the number of bits found
int recover_key(int key_found[2048], int key[1024]) {
    key_found[0] = 1;
    int c = 1;
    for (int i = 0; i < 512; i += 1) {
        c += key[i * 2 + 1];
        int b[5] = {0};
        int len = print_bin(b, key[i * 2]);
        for (int j = 0; j < len; j += 1) {
            key_found[c - 1 - j] = b[4 - j];
        }
    }
    return c;
}


int cmp(const void * e1, const void * e2) {
    if ((*(float *) e1 - *(float *) e2) > 0) {
        return 1;
    }
    return 0;
}


float find_poi_threshold(int n, int lst[2048], float * trace, int64_t nb_samples) {
    // n is number of poi
    float cons[n];
    float max_diff = 0;
    float tmp_threshold = 0;
    for (int j = 0; j < n; j += 1) {
        cons[j] = trace[lst[j]];
    }
    qsort(cons, n, sizeof(float), cmp);
    for (int j = 0; j < n - 1; j += 1) {
        if ((cons[j + 1] - cons[j]) > max_diff) {
            max_diff = cons[j + 1] - cons[j];
            tmp_threshold = (max_diff / 2) + cons[j];
        }
    }
    return tmp_threshold;
}


void usage(char * argv) {
    printf("Usage: %s <options>\n", argv);
    printf("Options:\n");
    printf("  [-t | --trace-file]    <trace_file.raw>: Raw file containing the trace file, with 32-bit floats (no default)\n");
    printf("  [-sp | --short-pattern-files] <file_0.raw> <file_1.raw>: Raw files containing the short patterns for mask values of 0 and 1 respectively, with 32-bit floats (no default)\n");
    printf("  [-mp | --medium-pattern-files] <file_0.raw> <file_1.raw>: Raw files containing the medium patterns for mask values of 0 and 1 respectively, with 32-bit floats (no default)\n");
    printf("  [-lp | --large-pattern-file]  <file.raw> : Raw file containing the pattern for the entire ROI, with 32-bit floats (no default)\n");

}


int main(int argc, char * argv[]) {
    clock_t begin, end;
    int poi[4096][17] = {0};
    int key[1024] = {0};
    int key_found[2048] = {0};
    int num_bit = 0;
    int lst[3000] = {0};
    float poi_threshold[17];

    int fd;

    int idx = 1;
    while (idx < argc) {
        if (strcmp(argv[idx], "-t") == 0 || strcmp(argv[idx], "--trace-file") == 0) {
            idx += 1;
            if (idx < argc) {
                trace_file = argv[idx];
            }
        }
        else if (strcmp(argv[idx], "-sp") == 0 || strcmp(argv[idx], "--short-pattern-files") == 0) {
            pattern_files = 's';
            idx += 1;
            if (idx < argc) {
                pattern_file_0 = argv[idx];
                idx += 1;
                if (idx < argc) {
                    pattern_file_1 = argv[idx];
                }
            }
        }
        else if (strcmp(argv[idx], "-mp") == 0 || strcmp(argv[idx], "--medium-pattern-files") == 0) {
            pattern_files = 'm';
            idx += 1;
            if (idx < argc) {
                pattern_file_0 = argv[idx];
                idx += 1;
                if (idx < argc) {
                    pattern_file_1 = argv[idx];
                }
            }
        }
        else if (strcmp(argv[idx], "-lp") == 0 || strcmp(argv[idx], "--large-pattern-file") == 0) {
            pattern_files = 'l';
            idx += 1;
            if (idx < argc) {
                pattern_file_0 = argv[idx];
            }
        }
        else {
            fprintf(stderr, "*** Error: unrecognized option: %s\n", argv[idx]);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
        idx += 1;
    }
    if (trace_file == NULL) {
        fprintf(stderr, "*** Error: no trace file given\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (pattern_file_0 == NULL) {
        fprintf(stderr, "*** Error: no pattern_0 file given\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (pattern_files != 'l' && pattern_file_1 == NULL) {
        fprintf(stderr, "*** Error: no pattern_1 file given\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }


    int64_t nb_samples = fsize(trace_file) / 4;
    float * trace = malloc(sizeof(float) * nb_samples);

    if ((fd = open(trace_file, O_RDONLY)) == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    uint64_t offset = 0;
    uint64_t step = 100000000;
    while (offset < nb_samples) {
        uint64_t nb_read_samples = min(step, nb_samples - offset);
        if (read(fd, &trace[offset], sizeof(float) * nb_read_samples) == -1) {
            perror("read");
            return EXIT_FAILURE;
        }
        offset += step;
    }


    int64_t pattern_0_len = fsize(pattern_file_0) / 4;
    float * pattern_0 = malloc(sizeof(float) * pattern_0_len);

    if ((fd = open(pattern_file_0, O_RDONLY)) == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (read(fd, pattern_0, sizeof(float) * pattern_0_len) == -1) {
        perror("read");
        return EXIT_FAILURE;
    }

    float * pattern_1 = NULL;

    if (pattern_files == 's' || pattern_files == 'm') {
        int64_t pattern_1_len = fsize(pattern_file_1) / 4;
        if (pattern_0_len != pattern_1_len) {
            usage(argv[0]);
            fprintf(stderr, "*** Error: patterns 0 and 1 do not have the same size\n");
            exit(EXIT_FAILURE);
        }

        pattern_1 = malloc(sizeof(float) * pattern_1_len);

        if ((fd = open(pattern_file_1, O_RDONLY)) == -1) {
            perror("open");
            return EXIT_FAILURE;
        }

        if (read(fd, pattern_1, sizeof(float) * pattern_1_len) == -1) {
            perror("read");
            return EXIT_FAILURE;
        }
    }


    begin = clock();

    int nb;
    if (pattern_files == 'l') {
        nb = parsing_single_pattern_start(poi, pattern_0, pattern_0_len, trace, nb_samples);
    }
    else if (pattern_files == 'm') {
        nb = parsing_double_patterns_start(poi, pattern_1, pattern_0, pattern_0_len, trace, nb_samples);
    }
    else {
        assert(pattern_files == 's');
        nb = parsing_double_patterns_end(poi, pattern_1, pattern_0, pattern_0_len, trace, nb_samples);
    }
    //for (int i = 0; i < 6; i += 1) {
    //    printf("# ROI %d: %d\n", i, poi[i][0]);
    //    printf("# POIS: ");
    //    for (int j = 0; j < 17; j += 1) {
    //        printf("%d, ", poi[i][j]);
    //    }
    //    printf("\n");
    //}

    for (int j = 0; j < 17; j += 1) {
        for (int i = 0; i < nb; i += 1) {
            lst[i] = poi[i][j];
        }
        poi_threshold[j] = find_poi_threshold(nb, lst, trace, nb_samples);
        //printf("# threshold %d: %f\n", j, poi_threshold[j]);
    }

    int c = 0;
    int index = 0;
    for (int i = 0; i < nb; i += 1) {
        if ((trace[poi[i][16]] > poi_threshold[16])) {
            for (int j = 0; j < 16; j += 1) {
                if (trace[poi[i][j]] < poi_threshold[j]) {
                    key[index] = j;
                    key[index + 1] = c;
                    index += 2;
                    c = 0;
                    break;
                }
            }
        }
        else {
            c += 1;
        }
    }

    num_bit = recover_key(key_found, key);

    end = clock();
    printf("Time: %lu s\n ", ((end - begin) / CLOCKS_PER_SEC));

    //printf("# secret found:    ");
    //for (int i = 0; i < 1023; i += 1) {
    //    printf("%x, ", key[i]);
    //}
    //printf("%x\n", key[1023]);


    printf("key found:       0b");
    for (int i = 0; i < num_bit; i += 1) {
        printf("%d", key_found[i]);
    }
    printf("\n");
    return 0;
}

