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
#define A_HI 8
#define B_LO 1
#define B_HI 8
#define C_LO 0
#define C_HI 999
#define BIN_LO 5
#define BIN_HI 7

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
    u8 worst;     // worst-case collisions (highest count out of all the bins)
    u8 med;       // count of bins with over the median number of collisions
    u8 bin_bits;  // number of bin bits; number of bins = (1 << bin_bits)
    u8 a;         // poly-hash coefficient a
    u8 b;         // poly-hash coefficient b
    u16 c;        // poly-hash coefficient c
} stats_t;

typedef struct {
    u32 table[MAX_HASHES];
    int count;
} hashes_t;

typedef struct {
    u16 bins[MAX_BINS];
    int count;
    int bin_bits;
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

void reset_bins(histogram_t *histo, int bin_bits) {
    histo->bin_bits = bin_bits;
    histo->count = 1 << bin_bits;
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
        k = (k << a) ^ WORDS[offset+i];
    }
    return k ^ (k >> b);
}

void calc_hashes(hashes_t *hashes, u32 a, u32 b, u32 c) {
    reset_hashes(hashes, WORD_COUNT);
    for(int i=0; i<WORD_COUNT; i++) {
        hashes->table[i] = poly_hash(a, b, c, i);
    }
}

void calc_histogram(hashes_t *hashes, histogram_t *histo, u32 bin_bits) {
    u32 bin_count = 1 << bin_bits;
    u32 mask = bin_count -1;
    reset_bins(histo, bin_bits);
    for(int i=0; i<(hashes->count); i++) {
        int k = hashes->table[i] & mask;
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
    ps->bin_bits = histo->bin_bits;
    ps->a = a;
    ps->b = b;
    ps->c = c;
}

int compare_ps(const void *a_, const void *b_) {
    stats_t a = *(const stats_t *) a_;
    stats_t b = *(const stats_t *) b_;
    if(a.worst == b.worst) {
        if(a.med == b.med) {
            if(a.bin_bits == b.bin_bits) {
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
            } else if(a.bin_bits < b.bin_bits) {
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

// Print the histogram as sparkline charts with up to 64 bins per line.
// This uses Unicode characters in the U+2580..U+259F "Block Elements" range.
void print_histogram(stats_t stats) {
    hashes_t hashes;
    histogram_t histo;
    calc_hashes(&hashes, stats.a, stats.b, stats.c);
    calc_histogram(&hashes, &histo, stats.bin_bits);
    for(int i=0; i<(histo.count); i++) {
        if(i==64) {
            printf("\n");  // Insert a line-break after 64 columns
        }
        switch(histo.bins[i]) {
        case 0:  printf(" "); break;
        case 1:  printf("▁"); break; // "lower one eighth block"
        case 2:  printf("▂"); break; // "lower one quarter block"
        case 3:  printf("▃"); break; // "lower three eighths block"
        case 4:  printf("▄"); break; // "lower half block"
        case 5:  printf("▅"); break; // "lower five eigths block"
        case 6:  printf("▆"); break; // "lower three quarters block"
        case 7:  printf("▇"); break; // "lower seven eigths block"
        default: printf("█"); break; // "full block"
        }
    }
    printf("\n");
}

void print_stats_summary(stats_t stats) {
    printf("max: %d  over_median: %d  ", stats.worst, stats.med);
    printf("poly(%d, %d, %d)\n", stats.a, stats.b, stats.c);
}

void summarize_stats() {
    int size = A_SIZE * B_SIZE * C_SIZE;
    qsort(STATS, size, sizeof(stats_t), compare_ps);
    int top_n = 5;
    int limit = size < top_n ? size : top_n;
    printf("Bin Count: %3d\n", 1 << STATS[0].bin_bits);
    printf("==============\n\n");
    for(int i=0; i<limit; i++) {
        print_stats_summary(STATS[i]);
        print_histogram(STATS[i]);
        printf("\n");
    }
}

void analyze_hash_params(u32 a, u32 b, u32 c, u32 bin_bits) {
    hashes_t hashes;
    histogram_t histo;
    int index = (a-A_LO) * (B_SIZE * C_SIZE);
    index += (b-B_LO) * (C_SIZE);
    index += (c-C_LO);
    calc_hashes(&hashes, a, b, c);
    calc_histogram(&hashes, &histo, bin_bits);
    calc_stats(&histo, a, b, c, &STATS[index]);
}

int main() {
    load_words();
    printf("words: %d\n", WORD_COUNT);
    // Calculate stats for all combinations of hash parameters and bin sizes
    for(int bin_bits=BIN_LO; bin_bits<=BIN_HI; bin_bits++) {
#ifdef __GNUC__
  #ifndef __clang__
    #pragma omp parallel for schedule(dynamic)
  #endif
#endif
        for(u32 a=A_LO; a<=A_HI; a++) {
            for(u32 b=B_LO; b<=B_HI; b++) {
                for(u32 c=C_LO; c<=C_HI; c++) {
                    analyze_hash_params(a, b, c, bin_bits);
                }
            }
        }
        printf("\n");
        summarize_stats();
    }
    return 0;
}


/*
words: 161

Bin Count:  32
==============

max: 7  over_median: 3  poly(2, 8, 167)
▆▄▇▆▄▂▅▆▅▅▆▇▆▆▄▆▃▆▆▆▅▆▇▃▄▆▃▆▅▅▃▂

max: 7  over_median: 12  poly(1, 6, 600)
▅▆▅▅▇▆▅▄▄▆▆▆▃▇▆▇▄▄▅▆▅▂▃▆▅▄▆▄▅▅▄▅

max: 7  over_median: 12  poly(1, 7, 671)
▆▇▁▄▄▆▅▄▄▁▇▇▅▃▆▅▅▅▄▇▆▆▅▅▄▅▆▇▄▇▅▅

max: 7  over_median: 12  poly(2, 7, 52)
▅▄▄▄▄▇▄▃▃▅▇▅▃▆▆▇▅▆▇▅▅▅▂▄▇▆▇▇▅▃▇▃

max: 7  over_median: 12  poly(2, 7, 564)
▇▅▄▄▅▇▄▃▃▆▇▅▃▆▇▇▃▅▇▅▄▅▂▄▇▅▇▇▅▃▆▃


Bin Count:  64
==============

max: 4  over_median: 10  poly(1, 4, 55)
▃▂▂ ▄▂▁▃▁▄▁▂▄▂▄▃▃▄▃▂▂▃▁▃▃▂▂▃▂▃▃▄▄▃▂▁▂▁▃▁▄▃▂▂▄▃▂▂▃▂▂▃▂▂▄▃▁▂▃▂▃▃▃▃

max: 4  over_median: 10  poly(1, 4, 567)
▃▂▂ ▄▂▁▃▁▄▁▂▄▂▄▃▃▄▃▂▂▃▁▃▃▂▂▃▂▃▃▄▄▃▂▁▂▁▃▁▄▃▂▂▄▃▂▂▃▂▂▃▂▂▄▃▁▂▃▂▃▃▃▃

max: 5  over_median: 9  poly(1, 7, 864)
▁▁▃▂▅▂▃▁▃▁▃▂▂▄▂▃▁▂▂▃▃▃ ▃▃▂▃▁▃▂▂▃▂▁▂▂▃▂▃▃▂▁▂▄▃▂▃▃▅▅▃▃▅▄▄▁▃▂▂▄▃▂▂▁

max: 5  over_median: 9  poly(2, 6, 324)
▂▂▁▃▁▁▁▁▃▂▃▂ ▂▂▁▃▄▁▃▃▂▃▃▁▂▃▃▂▁▁▂▂▂▃▃▁▂▅▃▄▅▄▃▂▃▃▃▃▃▃▁▄▄▃▂▃▃▃▄▃▃▃▄

max: 5  over_median: 10  poly(2, 6, 749)
▃▃▃▃▃▁▁▄▃▂▃▂▃▂▃▁▅▃▅▃▂▂▂▁▃▃▁▃▅▂▂▁▃▄▃▃▂▂▃▄▂▂▁▁▄▃▄▁▃▃▂▁▁▁▄▂▁▅▂▃▂ ▃▃


Bin Count: 128
==============

max: 3  over_median: 45  poly(2, 3, 209)
 ▃▁▂▁▁▂▁▁▂▂ ▃▂ ▂▂▁▁▁ ▁▁▂▂▂▂ ▁▃ ▂▂▁▁▂▁▃ ▃▃▃▁▁▂▁▁ ▁▂  ▂▃▃▁▂  ▂▁▁
▁▂▁▂▁▁▂▁ ▁▁▁ ▁▁▂▃▁▁▁ ▁   ▁▁▁▁▁▁▂ ▁▁ ▁▂▁▁▁▁▂▁▃▂▁▂▂▂▁▂▁▁▁ ▁▁▁ ▂▂▁▃

max: 3  over_median: 45  poly(2, 3, 465)
 ▃▁▂▁▁▂▁▁▂▂ ▃▂ ▂▂▁▁▁ ▁▁▂▂▂▂ ▁▃ ▂▂▁▁▂▁▃ ▃▃▃▁▁▂▁▁ ▁▂  ▂▃▃▁▂  ▂▁▁
▁▂▁▂▁▁▂▁ ▁▁▁ ▁▁▂▃▁▁▁ ▁   ▁▁▁▁▁▁▂ ▁▁ ▁▂▁▁▁▁▂▁▃▂▁▂▂▂▁▂▁▁▁ ▁▁▁ ▂▂▁▃

max: 3  over_median: 45  poly(2, 3, 721)
 ▃▁▂▁▁▂▁▁▂▂ ▃▂ ▂▂▁▁▁ ▁▁▂▂▂▂ ▁▃ ▂▂▁▁▂▁▃ ▃▃▃▁▁▂▁▁ ▁▂  ▂▃▃▁▂  ▂▁▁
▁▂▁▂▁▁▂▁ ▁▁▁ ▁▁▂▃▁▁▁ ▁   ▁▁▁▁▁▁▂ ▁▁ ▁▂▁▁▁▁▂▁▃▂▁▂▂▂▁▂▁▁▁ ▁▁▁ ▂▂▁▃

max: 3  over_median: 45  poly(2, 3, 977)
 ▃▁▂▁▁▂▁▁▂▂ ▃▂ ▂▂▁▁▁ ▁▁▂▂▂▂ ▁▃ ▂▂▁▁▂▁▃ ▃▃▃▁▁▂▁▁ ▁▂  ▂▃▃▁▂  ▂▁▁
▁▂▁▂▁▁▂▁ ▁▁▁ ▁▁▂▃▁▁▁ ▁   ▁▁▁▁▁▁▂ ▁▁ ▁▂▁▁▁▁▂▁▃▂▁▂▂▂▁▂▁▁▁ ▁▁▁ ▂▂▁▃

max: 3  over_median: 46  poly(2, 6, 588)
▁▁ ▃▁  ▁▂▃▁ ▁▂▂▁▂▁▁▂▁ ▂  ▂▂▂▃▁ ▂ ▁▃▂▁ ▁▁▁▃▂▁▁▃▁▂▁▁ ▃▂▁▁▂▁▃▃▁▃▃▁▃
▃ ▁   ▂ ▁▂ ▁   ▁▁▂  ▂▃▁▁ ▁▂▂▁  ▁▁ ▁▁ ▁▃▁▃▁▂▁ ▁  ▃▂▁▁▁▂▃▁▁▂▂ ▃  ▃

*/
