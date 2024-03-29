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
#define A_HI 16
#define B_LO 1
#define B_HI 15
#define C_LO 0
#define C_HI 65535
#define BIN_LO 5
#define BIN_HI 7

// The stats array holds stats for all combinations of parameters and bin size
#define A_SIZE (A_HI-A_LO+1)
#define B_SIZE (B_HI-B_LO+1)
#define C_SIZE (C_HI-C_LO+1)
#define STATS_LEN (A_SIZE*B_SIZE*C_SIZE)

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

typedef struct {
    u8 worst;     // worst-case collisions (highest count out of all the bins)
    u8 over_med;  // count of bins with over the median number of collisions
    u8 med;       // median collisions per bin
    u8 bin_bits;  // number of bin bits; number of bins = (1 << bin_bits)
    u16 a;        // poly-hash coefficient a
    u16 b;        // poly-hash coefficient b
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

// This works pretty well, but the mwc_hash() below distributes keys better.
u32 poly_hash(u32 a, u32 b, u32 c, int word) {
    int offset = word * C_PER_WORD;
    int len = WORDS[offset];
    u32 k = c;
    for(int i=1; i<=len && i<=C_PER_WORD; i++) {
        k = (k << a) ^ WORDS[offset+i];
    }
    return k ^ (k >> b);
}

// This hash function was inspired by George Marsaglia's 1994 email entitled
// "Yet another RNG" email describing a class of multiply-with-carry (MWC)
// random number generator (RNG) functions. This MWC hash does noticably better
// than my polynomial hash function at uniformly distributing hash keys. I'm
// not sure if this method of hashing strings with a 16-bit RNG has a specific
// name. But, the method is step the RNG for each byte of a string and xor the
// string byte into the RNG state during each iteration.
u32 mwc_hash(u32 a, u32 b, u32 c, int word) {
    int offset = word * C_PER_WORD;
    int len = WORDS[offset];
    u32 k = c;
    for(int i=1; i<=len && i<=C_PER_WORD; i++) {
        k = (((k&0xffff)<<a)+(k>>16)) ^ WORDS[offset+i];
    }
    return k ^ (k >> b);
}

void calc_hashes(hashes_t *hashes, u32 a, u32 b, u32 c) {
    reset_hashes(hashes, WORD_COUNT);
    for(int i=0; i<WORD_COUNT; i++) {
//        hashes->table[i] = poly_hash(a, b, c, i);
        hashes->table[i] = mwc_hash(a, b, c, i);
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
    ps->over_med = over_median;
    ps->med = median;
    ps->bin_bits = histo->bin_bits;
    ps->a = a;
    ps->b = b;
    ps->c = c;
}

int compare_ps(const void *a_, const void *b_) {
    stats_t a = *(const stats_t *) a_;
    stats_t b = *(const stats_t *) b_;
    if(a.worst == b.worst) {
        if(a.over_med == b.over_med) {
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
        } else if(a.over_med < b.over_med) {
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
        if((i>0) && (i%64==0)) {
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
    printf("max: %d  over_median: %d  ", stats.worst, stats.over_med);
    printf("median: %d  ", stats.med);
    printf("mwc(%d, %d, %d)\n", stats.a, stats.b, stats.c);
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
words: 169

Bin Count:  32
==============

max: 7  over_median: 3  median: 6  mwc(2, 7, 35776)
▆▅▆▆▆▄▅▇▆▄▆▅▅▄▄▂▆▆▅▆▇▆▅▃▇▅▅▆▆▆▆▃

max: 7  over_median: 3  median: 6  mwc(2, 7, 45374)
▅▅▇▄▆▆▂▇▅▄▄▆▃▅▆▆▄▅▅▆▆▆▇▅▄▆▅▆▅▆▆▆

max: 7  over_median: 3  median: 6  mwc(3, 14, 23537)
▅▆▆▇▆▆▆▅▃▅▇▃▆▆▅▆▇▅▅▆▆▆▂▄▆▄▆▄▆▄▆▄

max: 7  over_median: 3  median: 6  mwc(11, 9, 26735)
▆▆▇▅▇▅▅▆▅▆▃▆▆▅▆▅▆▆▆▂▄▄▆▅▅▆▂▅▇▆▆▄

max: 7  over_median: 3  median: 6  mwc(11, 14, 37403)
▆▄▇▅▅▆▆▆▆▆▇▅▆▃▇▄▁▆▅▅▅▅▄▆▆▆▃▆▆▆▄▆


Bin Count:  64
==============

max: 4  over_median: 13  median: 3  mwc(3, 5, 38656)
▂▃▂▁▂▁▄▃▂▂▂▂▄▃▄▃▃▃▃▄▂▁▃▂▁▃▃▂▃▂▃▃▄▁▃▃▃ ▄▃▃▄▄▂▄▃▄▃▂▁▃▃▂▃▂▁▄▂▂▃▄▃▄▁

max: 4  over_median: 14  median: 3  mwc(7, 8, 38335)
▄▃▂▂▃▂▁▂▄▄▁▄▄▃▃▄▄▄▃▂▃▃▃▄▃▂▁▂▂▃▃▂▁▂▁▄▂▃▃▃▁▁▂▄▂▁▂▁▄▄▄▃▃▃▃▃▃▃▁▂▂▃▃▂

max: 4  over_median: 14  median: 3  mwc(13, 12, 29359)
▁▃▂▁▂▂▁▂▄▂▃▂▃▃▃ ▃▄▂▁▃▃▃▃▄▂▄▁▄▃▃▁▃▁▃▃▃▂▂▄▄▃▄▁▃▄▄▃▂▄▁▂▂▄▃▃▂▃▄▃▃▃▁▄

max: 4  over_median: 16  median: 3  mwc(3, 11, 14675)
▃▄▄▃▄▃▁▃▂▄▂▃▂▄▂▄ ▃▂▃▁▂▂▃▂▄▃▄▄▂▂▄▃▄▂▃▃▃▂▃▄▄▄▂▃▁▄▁▃▂▄▃▃▂▃▁▃ ▁▃ ▃▂▁

max: 4  over_median: 16  median: 3  mwc(5, 8, 18791)
▂▃▁▃ ▁▁▃▃▃▃▄▄▄▂▁▄▄▂▂▃▃▃▃▃▁▃▃▂▂▄▃▃▂▃▄▂▃▂▃▃▄▂▂▂▄▃▂▂▂▄▄▄▄▁▄▁▂▁▄▂▁▄▂


Bin Count: 128
==============

max: 3  over_median: 43  median: 1  mwc(13, 10, 53357)
▁▃▂▃▁▁  ▁▃▂▃ ▂▁ ▁▁▁  ▃▁  ▂▃▂▁▁▁▁▂▁▂▂▁▁ ▁▃▁▁▁ ▃▂▁▃▃▁▂ ▁▃▁ ▁▁▁▁▁▂
▂▂▁▁▂▂▃▁▁▃ ▁▂▂▃▃▁▁▂▁▁▂▂▁ ▁ ▂▁▁▁▁▂▃▁▁▁▁▁▁▃▁▃  ▂▂ ▁ ▁▁▁▁▁▁▁▁▁▁▁▃ ▁

max: 3  over_median: 44  median: 1  mwc(1, 4, 41269)
▂▂▂▁▁▁▁▂▁ ▁ ▁ ▃ ▁▃ ▃ ▁▁▁▁▁▁▂▁▁ ▁▂▁▁   ▁▁▃▃▁▁▁▁▃▁▂▁▁▃   ▃▁▂ ▁▂▁▃▃
▃▁ ▁ ▃▂▁▂▁▁▁▂▁▁▂ ▁▁▁ ▁▁▂▁▁▁▂▁▂▂ ▃▁▂▂▃▁▃ ▁▁▁▃▁▁▃▃▂▁▁▂▂▃▂▁▁▃▁▁   ▂

max: 3  over_median: 44  median: 1  mwc(3, 14, 26350)
▁▁ ▃▁▃▂▁▁▁▁▁ ▁▁▃▁▁▂▁▁▃ ▁▁▂▁▁▁▂▁▁▃▁▁▁  ▂ ▂▃▂ ▁▃▁▂▃▁   ▁▃ ▂▁ ▁▃▁▁▃
▂▁▁▂▁▁▂▁▁▁▂▂▁ ▁▃▂▃▃▁▁ ▃▁▁▃▁  ▃▂▁▃▁▁ ▃▂▁▁▁▁ ▁▁▃ ▁▂▂▂▂▂▃  ▁ ▁  ▁▂▁

max: 3  over_median: 44  median: 1  mwc(5, 9, 22826)
▁▁▁ ▃ ▃▂▁▁ ▂▁▁▁▁▁ ▃▁▁   ▂▃▁▂▁▁▁ ▃ ▁▂▃▃▁▂▁▃ ▂▂▃▂▁▃ ▁▃▂▃▁▁▁▃▂▁ ▁▁▁
▁▃▁▁▁▁▁ ▁ ▃▃▁ ▁  ▃▁ ▂▁▁▁▁  ▃▁ ▁ ▂▂▂▂ ▁  ▁▃▃ ▁▁▃▁▁ ▂▁▁▁▁▂▃▂▃▁▁▂▃▁

max: 3  over_median: 44  median: 1  mwc(14, 12, 1381)
▁ ▂▁▂▂  ▁▃▂▂▃▁▂▁▃▃▁▃ ▁▁▁▁▁▁▂▁  ▁▁▁  ▂ ▁▂▃▃▁▁▁▁▁▂▁▃▁▁▂▃▃ ▃▂▂▃▁ ▂▃
▃▁▃▂▁▂▁▁ ▁ ▂▃ ▃  ▁▃▁▁▁▁▁▂▁▁▁▁▁▁▁▃ ▁▁▁▁▁▁▃ ▁▂▂   ▁▂ ▁▁▁▃▁▁▁▃ ▃

*/
