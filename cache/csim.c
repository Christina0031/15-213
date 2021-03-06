#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cachelab.h"

typedef unsigned long long uint64_t;

typedef struct CacheLine {
    uint64_t tag;
    unsigned counter;
} cacheLine;

uint64_t getTag(uint64_t address, int b, int s) { return address >> (b + s); }

uint64_t getSet(uint64_t address, int b, int s) {
    return (address << (64 - s - b)) >> (64 - s);
}

int main(int argc, char *argv[]) {
    int opt;
    int s = -1, b = -1, lines = -1, verbose = 0;
    char *file = NULL;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                lines = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                file = optarg;
                break;
            case 'h':
                /* Fall Through */
            default:
                fprintf(stderr, "Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (s <= 0 || b <= 0 || lines <= 0) {
        printf("Invalid parameter");
        exit(EXIT_FAILURE);
    }
    int S = 1 << s;

    cacheLine **cache = (cacheLine **) malloc(S * sizeof(cacheLine *));
    if (cache == NULL) {
        printf("Malloc error");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < S; i++) {
        cache[i] = (cacheLine *) malloc(lines * sizeof(cacheLine));
    }

    int *cacheLineSize = (int *) malloc(S * sizeof(int));
    if (cacheLineSize == NULL) {
        printf("Malloc error");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < lines; j++) {
            cache[i][j].counter = 0;
            cache[i][j].tag = 0;
        }
        cacheLineSize[i] = 0;
    }

    FILE *pFile;
    if ((pFile = fopen(file, "r")) == NULL) {
        printf("Fopen error");
        exit(EXIT_FAILURE);
    }

    char identifier;
    uint64_t address;
    int size;

    int hits = 0, misses = 0, evictions = 0;
    while (fscanf(pFile, " %c %llx,%d", &identifier, &address, &size) != EOF) {
        if (identifier == 'I') {
            continue;
        }
        if (verbose) {
            printf("%c %llx,%d", identifier, address, size);
        }
        if (identifier == 'L' || identifier == 'S') {
            // can create a function, input: address, cache; output: void
            // use static int to count hit, miss, evict. Or use a array.
            uint64_t set = getSet(address, b, s);
            uint64_t tag = getTag(address, b, s);
            int lineSize = cacheLineSize[set];
            bool found = false;
            if (lineSize == lines) {
                for (int i = 0; i < lines; ++i) {
                    if (cache[set][i].tag == tag) {
                        hits++;
                        cache[set][i].counter = 0;
                        found = true;
                    } else {
                        cache[set][i].counter++;
                    }
                }
                if (found) {
                    printf(" hit\n");
                } else {
                    int max = 0;
                    for (int i = 0; i < lines; ++i) {
                        if (cache[set][i].counter > cache[set][max].counter) {
                            max = i;
                        }
                    }
                    evictions++;
                    misses++;
                    cacheLine line = {tag, 0};
                    cache[set][max] = line;
                    printf(" miss eviction\n");
                }
            } else if (lineSize < lines) {
                for (int i = 0; i < lineSize; ++i) {
                    if (cache[set][i].tag == tag) {
                        hits++;
                        cache[set][i].counter = 0;
                        found = true;
                    } else {
                        cache[set][i].counter++;
                    }
                }
                if (found) {
                    printf(" hit\n");
                } else {
                    misses++;
                    cacheLine line = {tag, 0};
                    cache[set][lineSize] = line;
                    cacheLineSize[set]++;
                    printf(" miss\n");
                }
            }
        } else if (identifier == 'M') {
            uint64_t set = getSet(address, b, s);
            uint64_t tag = getTag(address, b, s);
            int lineSize = cacheLineSize[set];
            bool found = false;
            if (lineSize == lines) {
                for (int i = 0; i < lines; ++i) {
                    if (cache[set][i].tag == tag) {
                        hits++;
                        cache[set][i].counter = 0;
                        found = true;
                    } else {
                        cache[set][i].counter++;
                    }
                }
                if (found) {
                    printf(" hit");
                } else {
                    int max = 0;
                    for (int i = 0; i < lines; ++i) {
                        if (cache[set][i].counter > cache[set][max].counter) {
                            max = i;
                        }
                    }
                    evictions++;
                    misses++;
                    cacheLine line = {tag, 0};
                    cache[set][max] = line;
                    printf(" miss eviction");
                }
            } else if (lineSize < lines) {
                for (int i = 0; i < lineSize; ++i) {
                    if (cache[set][i].tag == tag) {
                        hits++;
                        cache[set][i].counter = 0;
                        found = true;
                    } else {
                        cache[set][i].counter++;
                    }
                }
                if (found) {
                    printf(" hit");
                } else {
                    misses++;
                    cacheLine line = {tag, 0};
                    cache[set][lineSize] = line;
                    cacheLineSize[set]++;
                    printf(" miss");
                }
            }
            lineSize = cacheLineSize[set];
            for (int i = 0; i < lines && i < lineSize; ++i) {
                if (cache[set][i].tag == tag) {
                    hits++;
                    cache[set][i].counter = 0;
                } else {
                    cache[set][i].counter++;
                }
            }
            printf(" hit\n");
        }
    }

    if (fclose(pFile) != 0) {
        printf("Fclose error");
        exit(EXIT_FAILURE);
    }
    if (cache != NULL) {
        for (int i = 0; i < S; i++) {
            free(cache[i]);
        }
        free(cache);
    }
    if (cacheLineSize != NULL) {
        free(cacheLineSize);
    }
    printSummary(hits, misses, evictions);
    exit(EXIT_SUCCESS);
}
