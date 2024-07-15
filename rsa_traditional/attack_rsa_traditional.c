/*
Copyright (C) 2024, Sorbonne Universite, LIP6
This file is part of the Blind-Folded work, under the GPL v3.0 license
See https://www.gnu.org/licenses/gpl-3.0.en.html for license information
SPDX-License-Identifier: GPL-3.0-only
Author(s): Xunyue HU
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h> 
#include <string.h>
#include <assert.h>
#include <errno.h>

#define OFFSET_MASK_1 14
#define THRESHOLD_1 0.1


const char * trace_file = NULL;
const char * pattern_file_0 = NULL;
const char * pattern_file_1 = NULL;


#define min(x, y) ((x) < (y) ? (x) : (y))


float mean(float lst[], int size) {
    float average = 0;
    for (int i = 0; i < size; i++) {
        average = average + (lst[i] - average) / (i + 1);
    }
    return average;
}


int floatcomp(const void * elem0, const void * elem1) {
    float f0 = *(const float *) elem0;
    float f1 = *(const float *) elem1;
    if (f0 < f1) {
        return -1;
    }
    return f0 > f1;
}


float find_poi_threshold(int n, int lst[2048], float * trace) {
    // n is the number of poi
    float cons[n];
    float max_diff = 0;
    float tmp_threshold = 0;
    for (int j = 0; j < n; j += 1) {
        cons[j] = trace[lst[j]];
    }
    qsort(cons, n, sizeof(float), floatcomp);
    for (int j = 0; j < n - 1; j += 1) {
        if (cons[j + 1] - cons[j] > max_diff) {
            max_diff = cons[j + 1] - cons[j];
            tmp_threshold = max_diff / 2 + cons[j];
        }
    }
    return tmp_threshold;
} 


int64_t fsize(const char * filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    fprintf(stderr, "*** Error: cannot determine size of %s: %s\n", filename, strerror(errno));
    return -1;
}


int parsing(int lst[2048], float * pattern0, float * pattern1, int64_t pattern_len, float * trace, int64_t nb_samples) {

    int c = 0;
    double diff0max = 0;
    double diff1max = 0;
    int offset = OFFSET_MASK_1;

    for (int i = 0; i < nb_samples - pattern_len; i += 1) {
        double diff0 = 0;
        double diff1 = 0;
        for (int j = 0; j < pattern_len; j += 1) {
            diff0 += fabs(pattern0[j] - trace[i + j]);
            diff1 += fabs(pattern1[j] - trace[i + j]);
        }
        if (c < 3) {
            if ((diff0 / pattern_len < 0.01) || (diff1 / pattern_len < 0.01)) {
                lst[c] = i + offset;
                c += 1;
            }
        }
        else if ((diff0 / pattern_len < 0.0085) || (diff1 / pattern_len < 0.0087)) {
            assert(c < 2048);
            if (diff0 / pattern_len < 0.0085) {
                if (diff0 > diff0max) {
                    diff0max = diff0;
                }
            }
            else if (diff1 > diff1max) {
                diff1max = diff1;
            }
            lst[c] = i + offset;
            c += 1;
            if (c > 3) {
                i += 230000;
            }
        }
    }

    return c;
}


void usage(char * argv) {
    printf("Usage: %s <options>\n", argv);
    printf("Options:\n");
    printf("  [-t | --trace-file]    <trace_file.raw>: Raw file containing the trace file, with 32-bit floats (no default)\n");
    printf("  [-p | --pattern-files] <file_0.raw> <file_1.raw>: Raw files containing the patterns for mask values of 0 and 1 respectively, with 32-bit floats (no default)\n");
}


int main(int argc, char * argv[]) {
    int poi[2048] = {0};
    int key_found[2048] = {0};

    int fd;

    int idx = 1;
    while (idx < argc) {
        if (strcmp(argv[idx], "-t") == 0 || strcmp(argv[idx], "--trace-file") == 0) {
            idx += 1;
            if (idx < argc) {
                trace_file = argv[idx];
            }
        }
        else if (strcmp(argv[idx], "-p") == 0 || strcmp(argv[idx], "--pattern-files") == 0) {
            idx += 1;
            if (idx < argc) {
                pattern_file_0 = argv[idx];
                idx += 1;
                if (idx < argc) {
                    pattern_file_1 = argv[idx];
                }
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
    if (pattern_file_1 == NULL) {
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
    int64_t pattern_1_len = fsize(pattern_file_1) / 4;
    if (pattern_0_len != pattern_1_len) {
        fprintf(stderr, "*** Error: patterns 0 and 1 do not have the same size\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    float * pattern_0 = malloc(sizeof(float) * pattern_0_len);
    float * pattern_1 = malloc(sizeof(float) * pattern_1_len);

    if ((fd = open(pattern_file_0, O_RDONLY)) == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (read(fd, pattern_0, sizeof(float) * pattern_0_len) == -1) {
        perror("read");
        return EXIT_FAILURE;
    }

    if ((fd = open(pattern_file_1, O_RDONLY)) == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (read(fd, pattern_1, sizeof(float) * pattern_1_len) == -1) {
        perror("read");
        return EXIT_FAILURE;
    }


    int nb = parsing(poi, pattern_0, pattern_1, pattern_0_len, trace, nb_samples);
    //for (int i = 0; i < nb - 1; i += 1) {
    //    printf("%d, ", poi[i]);
    //}
    //printf("%d\n", poi[nb - 1]);

    key_found[0] = 1;
    int c = 1;
    float poi_threshold = find_poi_threshold(nb, poi, trace); 

    for (int i = 0; i < nb; i += 1) {
        if (trace[poi[i]] < poi_threshold) {
            key_found[c] = 1;
        }
        else {
            key_found[c] = 0;
        }
        c += 1;
    }

    printf("key found:       0b");
    for (int i = 0; i < nb + 1; i += 1) {
        printf("%d", key_found[i]);
    }
    printf("\n");

    return 0;
}

