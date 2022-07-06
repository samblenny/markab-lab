#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Experiment to calculate good polynomial hash parameters for kernel symbols
#
import cProfile
import sys

SYM_IN = 'kernel.symbols'
WORDS = []
WORDS_LEN = 0

# These came from the top results of an exhaustive search by poly_hash.c
A = [1, 2]
B = [3, 4, 6, 9, 11, 13]
C = [5, 55, 68, 103, 111, 155, 181, 194, 197, 201]
BIN_BITS = [6]

def load_words():
  """Load list of words from kernel symbols file"""
  global WORDS
  words = []
  with open(SYM_IN, 'r') as f:
    lines = f.read().strip().split("\n")
    words = [L.split()[1].encode('utf8') for L in lines]
  WORDS = words

def histogram(hashes, bin_bits):
  """Generate histogram from applying hash_fn to each word of words"""
  bin_count = 1 << bin_bits
  mask = bin_count - 1
  bins = [0] * bin_count
  for h in hashes:
    key = h & mask
    count = bins[key]
    bins[key] = count + 1
  return bins

def poly_hash(a, b, c, word):
  """Polynomial hash algorithm"""
  k = c
  for byte_ in word:
    k = ((k << a) ^ byte_) & 0xffffffff
  return k ^ (k >> b)

def gen_stats():
  """Generate stats for combinations of polynomial hash parameters"""
  stats = []
  for a in A:
    for b in B:
      for c in C:
        hash_fn = lambda w: poly_hash(a, b, c, w)
        hashes = [hash_fn(w) for w in WORDS]
        for bin_bits in BIN_BITS:
          # calculate histogram
          bins = histogram(hashes, bin_bits)
          # calculate percentile (bin actually) where median ends as a measure
          # of the distribution's flatness. Higher number means flatter.
          bins = sorted(bins)
          median = bins[len(bins) >> 1]
          over_median = 0;
          for bin_ in range(len(bins) >> 1, len(bins)):
            if(bins[bin_] > median):
              over_median += 1
          # record stats
          worst_case = max(bins)
          stats.append((worst_case, over_median, bin_bits, a, b, c))
  print()
  return stats

def top_n(n, stats):
  stats = sorted(stats)
  return stats[:n]

def stats_to_str(s):
    (worst, median, bins, a, b, c) = s
    name = f"poly({a}, {b:>2}, {c:>3})"
    return f"worst: {worst}  over_med: {median}  bins: {1<<bins}  {name}"

def summarize(stats):
  return "\n".join([stats_to_str(s) for s in stats])

def print_histogram(s):
  print(stats_to_str(s))
  (worst, median, bin_bits, a, b, c) = s
  hash_fn = lambda w: poly_hash(a, b, c, w)
  hashes = [hash_fn(w) for w in WORDS]
  bins = histogram(hashes, bin_bits)
  for (k, v) in enumerate(bins):
    print(f" {k:>2} {'*' * v}")
  print()

def go():
  """Print stats for the top polynomial hash parameters"""
  load_words()
  stats = top_n(10, gen_stats())
  print(summarize(stats))
  print()
  #Print histogram for the best polynomial hash parameters
  print_histogram(stats[0])


go()
# cProfile.run("go()", sort='cumulative')


# worst: 4  over_med: 10  bins: 64  poly(1,  4,  55)
# worst: 5  over_med: 10  bins: 64  poly(2, 13,   5)
# worst: 5  over_med: 11  bins: 64  poly(1,  3, 201)
# worst: 5  over_med: 11  bins: 64  poly(1,  6, 181)
# worst: 5  over_med: 11  bins: 64  poly(2,  6,  68)
# worst: 5  over_med: 11  bins: 64  poly(2, 11, 103)
# worst: 5  over_med: 11  bins: 64  poly(2, 13, 194)
# worst: 5  over_med: 12  bins: 64  poly(2,  9, 111)
# worst: 5  over_med: 12  bins: 64  poly(2, 13, 155)
# worst: 5  over_med: 12  bins: 64  poly(2, 13, 197)

# worst: 4  over_med: 10  bins: 64  poly(1,  4,  55)
#   0 ***
#   1 **
#   2 **
#   3 
#   4 ****
#   5 **
#   6 *
#   7 ***
#   8 *
#   9 ****
#  10 *
#  11 **
#  12 ****
#  13 **
#  14 ****
#  15 ***
#  16 ***
#  17 ****
#  18 ***
#  19 **
#  20 **
#  21 ***
#  22 *
#  23 ***
#  24 ***
#  25 **
#  26 **
#  27 ***
#  28 **
#  29 ***
#  30 ***
#  31 ****
#  32 ****
#  33 ***
#  34 **
#  35 *
#  36 **
#  37 *
#  38 ***
#  39 *
#  40 ****
#  41 ***
#  42 **
#  43 **
#  44 ****
#  45 ***
#  46 **
#  47 **
#  48 ***
#  49 **
#  50 **
#  51 ***
#  52 **
#  53 **
#  54 ****
#  55 ***
#  56 *
#  57 **
#  58 ***
#  59 **
#  60 ***
#  61 ***
#  62 ***
#  63 ***
