// Copyright (c) 2022 Sam Blenny
// SPDX-License-Identifier: MIT
//
// Calculate good polynomial hash parameters and bin size for kernel symbols
//
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __GNUC__
  #ifndef __clang__
    #include <omp.h>
  #endif
#endif

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
#define BIN_LO 64
#define BIN_HI 64

// The stats array holds stats for all combinations of parameters and bin size
#define A_SIZE (A_HI-A_LO+1)
#define B_SIZE (B_HI-B_LO+1)
#define C_SIZE (C_HI-C_LO+1)
#define BIN_SIZE (BIN_HI-BIN_LO+1)
#define STATS_LEN (A_SIZE*B_SIZE*C_SIZE*BIN_SIZE)

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
} stats_t;

typedef struct {
    u32 table[MAX_HASHES];
    int count;
} hashes_t;

typedef struct {
    u16 bins[MAX_BINS];
    int count;
} histogram_t;

stats_t STATS[STATS_LEN];

u8 WORDS[MAX_WORDS*C_PER_WORD];
int WORD_COUNT = 0;

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

void reset_bins(histogram_t *histo, int count) {
    histo->count = count < MAX_BINS ? count : MAX_BINS;
    for(int i=0; i<MAX_BINS; i++) {
        histo->bins[i] = 0;
    }
}

void reset_hashes(hashes_t *hashes, int count) {
    hashes->count = count < MAX_HASHES ? count : MAX_HASHES;
    for(int i=0; i<MAX_HASHES; i++) {
        hashes->table[i] = 0;
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

void calc_hashes(hashes_t *hashes, u32 a, u32 b, u32 c) {
    reset_hashes(hashes, WORD_COUNT);
    for(int i=0; i<WORD_COUNT; i++) {
        hashes->table[i] = poly_hash(a, b, c, i);
    }
}

void calc_histogram(hashes_t *hashes, histogram_t *histo, u32 bin_count) {
    reset_bins(histo, bin_count);
    for(int i=0; i<(hashes->count); i++) {
        int k = hashes->table[i] % bin_count;
        histo->bins[k] += 1;
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

void calc_stats(histogram_t *histo, u32 a, u32 b, u32 c, stats_t *ps) {
    // Find the highest number of items (hash collisions) in a bin
    u8 worst = 0;
    for(int i=0; i<(histo->count); i++) {
        int count = histo->bins[i];
        if(count > worst) {
            worst = count;
        }
    }
    // Count the number of bins with a frequency greater than the median
    // frequency equal to the median frequency. This is a convenient proxy for
    // smoothness and flatness of the distribution. Smaller is better.
    qsort(histo->bins, histo->count, sizeof(u16), compare_bins);
    u8 median = histo->bins[histo->count >> 1];
    int over_median = 0;
    for(int i=(histo->count >> 1); i<(histo->count); i++) {
        if(histo->bins[i] > median) {
            over_median++;
        }
    }
    // Update the stats struct
    ps->worst = worst;
    ps->med = over_median;
    ps->bins = histo->count;
    ps->a = a;
    ps->b = b;
    ps->c = c;
}

int compare_ps(const void *a_, const void *b_) {
    stats_t a = *(const stats_t *) a_;
    stats_t b = *(const stats_t *) b_;
    if(a.worst == b.worst) {
        if(a.med == b.med) {
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
        } else if(a.med < b.med) {
            return -1;
        }
        return 1;
    } else if(a.worst < b.worst) {
        return -1;
    }
    return 1;
}

void summarize_stats() {
    qsort(STATS, STATS_LEN, sizeof(stats_t), compare_ps);
    int top_n = 10;
    int limit = STATS_LEN < top_n ? STATS_LEN : top_n;
    stats_t p;
    for(int i=0; i<limit; i++) {
        p = STATS[i];
        printf("worst: %2d  ", p.worst);
        printf("over_med: %2d  bins: %3d  ", p.med, p.bins);
        printf("poly(%2d,%2d,%3d)\n", p.a, p.b, p.c);
    }
}

void analyze_hash_params(u32 a, u32 b, u32 c) {
    hashes_t hashes;
    histogram_t histo;
    int base = (a-A_LO) * (B_SIZE * C_SIZE * BIN_SIZE);
    base += (b-B_LO) * (C_SIZE * BIN_SIZE);
    base += (c-C_LO) * BIN_SIZE;
    calc_hashes(&hashes, a, b, c);
    for(u32 bin_count=BIN_LO; bin_count<=BIN_HI; bin_count++) {
        calc_histogram(&hashes, &histo, bin_count);
        calc_stats(&histo, a, b, c, &STATS[base+(bin_count-BIN_LO)]);
    }
}

int main() {
    load_words();
    printf("words: %d\n", WORD_COUNT);
    // Calculate stats for all combinations of hash parameters and bin sizes
#ifdef __GNUC__
  #ifndef __clang__
    #pragma omp parallel for schedule(dynamic)
  #endif
#endif
    for(u32 a=A_LO; a<=A_HI; a++) {
        printf(".");
        fflush(stdout);
        for(u32 b=B_LO; b<=B_HI; b++) {
            for(u32 c=C_LO; c<=C_HI; c++) {
                analyze_hash_params(a, b, c);
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
// Top 10 sorted by worst-case bin frequency then count of bins with frequency
// higher than the median frequency. The count of bins over median measures how
// smooth and flat the distribution of hash keys is. Lower is better.
worst:  4  over_med: 10  bins:  64  poly( 2, 4, 55)
worst:  4  over_med: 10  bins:  64  poly(119, 4,218)
worst:  4  over_med: 11  bins:  64  poly(15, 6, 95)
worst:  4  over_med: 11  bins:  64  poly(79,18, 81)
worst:  4  over_med: 12  bins:  64  poly(78,22, 79)
worst:  4  over_med: 12  bins:  64  poly(85, 9,206)
worst:  4  over_med: 12  bins:  64  poly(125,14, 64)
worst:  4  over_med: 12  bins:  64  poly(229,27, 85)
worst:  4  over_med: 13  bins:  64  poly(121,10, 38)
worst:  4  over_med: 14  bins:  64  poly(24, 8,171)

// Python code to extract lists of a, b, c, and bins parameters:
# bins a b c
params = """
64 2 4 55
64 119 4 218
64 15 6 95
64 79 18 81
64 78 22 79
64 85 9 206
64 125 14 64
64 229 27 85
64 121 10 38
64 24 8 171
"""
lines = [L.split(" ") for L in params.strip().split("\n")]
(A, B, C, BINS) = ([], [], [], [])
for (bins, a, b, c) in lines:
  A += [int(a)]
  B += [int(b)]
  C += [int(c)]
  BINS += [int(bins)]
print(f"A = {sorted(set(A))}\nB = {sorted(set(B))}")
print(f"C = {sorted(set(C))}\nBINS = {sorted(set(BINS))}")
# A = [2, 15, 24, 78, 79, 85, 119, 121, 125, 229]
# B = [4, 6, 8, 9, 10, 14, 18, 22, 27]
# C = [38, 55, 64, 79, 81, 85, 95, 171, 206, 218]
# BINS = [64]
*/
