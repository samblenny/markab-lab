// Copyright (c) 2022 Sam Blenny
// SPDX-License-Identifier: MIT
//
// Calculate good polynomial hash parameters and bin size for kernel symbols
//
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SYM_IN "kernel.symbols"
#define MAX_WORDS 300
#define MAX_BINS MAX_WORDS
#define MAX_HASHES MAX_BINS
#define C_PER_WORD 32

// These control the search limits for the hash parameters and bin size
#define A_LO 1
#define A_HI 255
#define B_LO 1
#define B_HI 31
#define C_LO 0
#define C_HI 255
#define BIN_LO 94
#define BIN_HI 99

#define PERCENTILE 80

// This holds the stats for every combination of parameters and bin size
#define STATS_LEN ((A_HI-A_LO+1)*(B_HI-B_LO+1)*(C_HI-C_LO+1)*(BIN_HI-BIN_LO+1))

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

typedef struct {
    u8 worst;  // worst-case collisions (highest count out of all the bins)
    u8 med;    // median number of collisions
    u8 bins;   // number of bins
    u8 a;      // poly-hash coefficient a
    u8 b;      // poly-hash coefficient b
    u8 c;      // poly-hash coefficient c
} param_stats;

u8 WORDS[MAX_WORDS*C_PER_WORD];
int WORD_COUNT = 0;

u16 BINS[MAX_BINS];
int BIN_COUNT = 0;

u32 HASHES[MAX_HASHES];
int HASH_COUNT = 0;

param_stats STATS[STATS_LEN];

void reset_words() {
    for(int i=0; i<MAX_WORDS*C_PER_WORD; i++) {
        WORDS[i] = 0;
    }
}

void print_words() {
    for(int i=0; i<WORD_COUNT; i++) {
        int offset = i * C_PER_WORD;
        int len = WORDS[offset];
        printf("%2d/", len);
        for(int j=1; j<=len; j++) {
            char x = WORDS[offset+j];
            printf("%c", x);
        }
        printf("\n");
    }
}

void load_words() {
    reset_words();
    FILE *f = fopen(SYM_IN, "r");      // open the kernel symbol table file
    char c;
    WORD_COUNT = 0;
    if(f) {
        for(int i=0; i<MAX_WORDS; i++) {
            int offset = i * C_PER_WORD;
            WORDS[offset] = 0;         // first byte is length
            int field2 = 0;
            for(int j=1; j<C_PER_WORD; j++) {
                c = fgetc(f);
                if(c == EOF) {         // end of file -> stop loading
                    fclose(f);         //   (this is the normal exit path)
                    return;
                }
                if(c == ' ') {         // space after addr -> start copying
                    field2 = 1;
                    WORD_COUNT += 1;
                    continue;
                }
                if(field2 < 1) {       // still in address field -> skip it
                    continue;
                }
                if(c == '\n') {        // end of line -> next line
                    break;
                }
                WORDS[offset+field2] = c;  // normal character -> copy it
                field2 += 1;
                WORDS[offset] += 1;
            }
        }
        fclose(f);  // This should not happen unless there are too many words
    }
}

void reset_bins(int count) {
    BIN_COUNT = count < MAX_BINS ? count : MAX_BINS;
    for(int i=0; i<MAX_BINS; i++) {
        BINS[i] = 0;
    }
}

void reset_hashes(int count) {
    HASH_COUNT = count < MAX_HASHES ? count : MAX_HASHES;
    for(int i=0; i<MAX_HASHES; i++) {
        HASHES[i] = 0;
    }
}

u32 poly_hash(u32 a, u32 b, u32 c, int word) {
    int offset = word * C_PER_WORD;
    int len = WORDS[offset];
    u32 k = c;
    for(int i=1; i<=len && i<=C_PER_WORD; i++) {
        k = (k * a) ^ WORDS[offset+i];
    }
    return k ^ (k >> b);
}

void calc_hashes(u32 a, u32 b, u32 c) {
    reset_hashes(WORD_COUNT);
    for(int i=0; i<WORD_COUNT; i++) {
        HASHES[i] = poly_hash(a, b, c, i);
    }
}

void calc_histogram(u32 bin_count) {
    reset_bins(bin_count);
    for(int i=0; i<HASH_COUNT; i++) {
        int k = HASHES[i] % bin_count;
        BINS[k] += 1;
    }
}

int compare_bins(const void *a_, const void *b_) {
    u16 a = *(const u16 *) a_;
    u16 b = *(const u16 *) b_;
    if(a == b) {
        return 0;
    } else if(a < b) {
        return -1;
    }
    return 1;
}

void calc_stats(u32 a, u32 b, u32 c, param_stats *ps) {
    // Find the highest number of items (hash collisions) in a bin
    u8 worst = 0;
    for(int i=0; i<BIN_COUNT; i++) {
        int count = BINS[i];
        if(count > worst) {
            worst = count;
        }
    }
    // Calculate median collision frequency (actually this is a bit above the
    // median to get more of a spread, since the medians are mostly 1)
    qsort(BINS, BIN_COUNT, sizeof(u16), compare_bins);
    u8 over_median = BINS[(BIN_COUNT * PERCENTILE) / 100];
    // Update the stats struct
    ps->worst = worst;
    ps->med = over_median;
    ps->bins = BIN_COUNT;
    ps->a = a;
    ps->b = b;
    ps->c = c;
}

int compare_ps(const void *a_, const void *b_) {
    param_stats a = *(const param_stats *) a_;
    param_stats b = *(const param_stats *) b_;
    if(a.med == b.med) {
        if(a.worst == b.worst) {
            if(a.bins == b.bins) {
                if(a.a == b.a) {
                    if(a.b == b.b) {
                        if(a.c == b.c) {
                            return 0;
                        } else if(a.c < b.c) {
                            return -1;
                        }
                        return 1;
                    } else if(a.b < b.b) {
                        return -1;
                    }
                    return 1;
                } else if(a.a < b.a) {
                    return -1;
                }
                return 1;
            } else if(a.bins < b.bins) {
                return -1;
            }
            return 1;
        } else if(a.worst < b.worst) {
            return -1;
        }
        return 1;
    } else if(a.med < b.med) {
        return -1;
    }
    return 1;
}

void summarize_stats() {
    qsort(STATS, STATS_LEN, sizeof(param_stats), compare_ps);
    int top_n = 20;
    int limit = STATS_LEN < top_n ? STATS_LEN : top_n;
    param_stats p;
    for(int i=0; i<limit; i++) {
        p = STATS[i];
        printf("worst: %2d  ", p.worst);
        printf("%d_%%tile: %2d  bins: %3d  ", PERCENTILE, p.med, p.bins);
        printf("poly(%2d,%2d,%3d)\n", p.a, p.b, p.c);
    }
}

int main() {
    load_words();
    printf("words: %d\n", WORD_COUNT);
    // Calculate stats for all combinations of hash parameters and bin sizes
    int s = 0;
    for(u32 a=A_LO; a<=A_HI; a++) {
        for(u32 b=B_LO; b<=B_HI; b++) {
            printf(".");
            fflush(stdout);
            for(u32 c=C_LO; c<=C_HI; c++) {
                calc_hashes(a, b, c);
                for(u32 bin_count=BIN_LO; bin_count<=BIN_HI; bin_count++) {
                    calc_histogram(bin_count);
                    calc_stats(a, b, c, &STATS[s]);
                    s++;
                }
            }
        }
    }
    printf("\n");
    // Find best combinations ranking by low collisions then low bin size
    summarize_stats();
    printf("\n");
    return 0;
}


/*
// These are the top 20 sorted by 80th percentile bin size
worst:  3  80_%tile:  2  bins:  94  poly(183, 20, 130)
worst:  3  80_%tile:  2  bins:  95  poly(218, 23,  46)
worst:  3  80_%tile:  2  bins:  96  poly( 87, 14,  99)
worst:  3  80_%tile:  2  bins:  97  poly( 14, 28, 122)
worst:  3  80_%tile:  2  bins:  97  poly( 21,  4, 114)
worst:  3  80_%tile:  2  bins:  97  poly( 82, 11, 205)
worst:  3  80_%tile:  2  bins:  97  poly(163,  5, 104)
worst:  3  80_%tile:  2  bins:  98  poly(135,  1,  38)
worst:  3  80_%tile:  2  bins:  98  poly(232, 27, 160)
worst:  3  80_%tile:  2  bins:  99  poly(  4, 19, 219)
worst:  3  80_%tile:  2  bins:  99  poly( 35, 30, 213)
worst:  3  80_%tile:  2  bins:  99  poly(176, 23,  59)
worst:  3  80_%tile:  2  bins:  99  poly(216,  6, 205)
worst:  3  80_%tile:  2  bins:  99  poly(250, 18,  61)
worst:  4  80_%tile:  2  bins:  94  poly(  2,  3, 162)
worst:  4  80_%tile:  2  bins:  94  poly(  3,  1,  22)
worst:  4  80_%tile:  2  bins:  94  poly(  3,  1, 194)
worst:  4  80_%tile:  2  bins:  94  poly(  3,  3,  47)
worst:  4  80_%tile:  2  bins:  94  poly(  3,  5, 132)
worst:  4  80_%tile:  2  bins:  94  poly(  3,  6,  17)


// These were sorted using med=((BIN_COUNT * 6) / 9)
worst:  7  med:  1  bins: 101  poly(223, 1,101)
worst:  9  med:  1  bins: 101  poly(15, 7,193)
worst:  3  med:  2  bins:  89  poly(27,13, 41)
worst:  3  med:  2  bins:  89  poly(172,15, 73)
worst:  3  med:  2  bins:  91  poly(244, 1, 52)
worst:  3  med:  2  bins:  92  poly( 9,14,231)
worst:  3  med:  2  bins:  92  poly(171, 7, 94)
worst:  3  med:  2  bins:  92  poly(237,19,183)
worst:  3  med:  2  bins:  93  poly(26, 2,117)
worst:  3  med:  2  bins:  93  poly(86,14,138)
worst:  3  med:  2  bins:  93  poly(130,30, 89)
worst:  3  med:  2  bins:  93  poly(232,31,194)
worst:  3  med:  2  bins:  94  poly(31, 7,163)
worst:  3  med:  2  bins:  94  poly(82,17, 17)
worst:  3  med:  2  bins:  94  poly(107,20,178)
worst:  3  med:  2  bins:  94  poly(144,20,247)
worst:  3  med:  2  bins:  94  poly(183,20,130)
worst:  3  med:  2  bins:  94  poly(208,29,220)
worst:  3  med:  2  bins:  94  poly(216,25,180)
worst:  3  med:  2  bins:  94  poly(217,14,183)
worst:  3  med:  2  bins:  95  poly( 2,16,208)
worst:  3  med:  2  bins:  95  poly( 2,17,208)
worst:  3  med:  2  bins:  95  poly( 2,19,208)
worst:  3  med:  2  bins:  95  poly( 2,20,208)
worst:  3  med:  2  bins:  95  poly( 2,21,208)
worst:  3  med:  2  bins:  95  poly( 2,22,208)
worst:  3  med:  2  bins:  95  poly( 2,23,208)
worst:  3  med:  2  bins:  95  poly( 2,24,208)
worst:  3  med:  2  bins:  95  poly( 2,25,208)
worst:  3  med:  2  bins:  95  poly( 2,26,208)
worst:  3  med:  2  bins:  95  poly( 2,27,208)
worst:  3  med:  2  bins:  95  poly( 2,28,208)
worst:  3  med:  2  bins:  95  poly( 2,29,208)
worst:  3  med:  2  bins:  95  poly( 2,30,208)
worst:  3  med:  2  bins:  95  poly( 2,31,208)
worst:  3  med:  2  bins:  95  poly(13,14, 65)
worst:  3  med:  2  bins:  95  poly(17,16,220)
worst:  3  med:  2  bins:  95  poly(36, 9, 95)
worst:  3  med:  2  bins:  95  poly(109,14, 62)
worst:  3  med:  2  bins:  95  poly(138, 5,225)
*/
