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
LIMIT = 3

# These values came from an exhaustive search by poly_hash.c, which takes
# a few minutes to run. These are from the highest ranking results.
A = [82, 87, 108, 109, 129, 135, 165, 186, 203, 232, 254]
B = [1, 2, 4, 11, 14, 18, 19, 22, 27]
C = [1, 38, 57, 62, 76, 99, 149, 151, 160, 205]
BINS = [95, 96, 97, 98]

def load_words():
  """Load list of words from kernel symbols file"""
  global WORDS
  words = []
  with open(SYM_IN, 'r') as f:
    lines = f.read().strip().split("\n")
    words = [L.split()[1].encode('utf8') for L in lines]
  WORDS = words

def histogram(hashes, bin_count):
  """Generate histogram from applying hash_fn to each word of words"""
  bins = [0] * bin_count
  for h in hashes:
    key = h % bin_count
    count = bins[key]
    bins[key] = count + 1
  return bins

def poly_hash(a, b, c, word):
  """Polynomial hash algorithm"""
  k = c
  for byte_ in word:
    k = ((k * a) ^ byte_) & 0xffffffff
  return k ^ (k >> b)

def gen_stats():
  """Generate stats for combinations of polynomial hash parameters"""
  stats = []
  limit = 4
  for a in A:
    for b in B:
      for c in C:
        hash_fn = lambda w: poly_hash(a, b, c, w)
        hashes = [hash_fn(w) for w in WORDS]
        for bin_count in BINS:
          # calculate histogram
          bins = histogram(hashes, bin_count)
          # calculate median (actually a little over to get better spread)
          bins = sorted(bins)
          median = bins[(len(bins) * 80) // 100]
          # record stats
          worst_case = max(bins)
          stats.append((median, worst_case, bin_count, a, b, c))
  print()
  return stats

def top_n(n, stats):
  stats = sorted(stats)
  return stats[:n]

def stats_to_str(s):
    (median, worst, bins, a, b, c) = s
    name = f"poly({a:>2},{b:>2},{c:>3})"
    return f"worst: {worst:>2}  80_%tile: {median}  bins: {bins}  {name}"

def summarize(stats):
  stats = [s for s in stats if s[1] <= 4]  # filter out worst=4+
  return "\n".join([stats_to_str(s) for s in stats])

def print_histogram(s):
  print(stats_to_str(s))
  (median, worst, bin_count, a, b, c) = s
  hash_fn = lambda w: poly_hash(a, b, c, w)
  hashes = [hash_fn(w) for w in WORDS]
  bins = histogram(hashes, bin_count)
  for (k, v) in enumerate(bins):
    print(f" {k:>2} {'*' * v}")
  print()

def go():
  """Print stats for the top polynomial hash parameters"""
  load_words()
  stats = top_n(20, gen_stats())
  print(summarize(stats))
  print()
  #Print histogram for the best polynomial hash parameters
  print_histogram(stats[0])


go()
# cProfile.run("go()", sort='cumulative')


# worst:  3  80_%tile: 2  bins: 95  poly(109,14, 62)
# worst:  3  80_%tile: 2  bins: 96  poly(87,14, 99)
# worst:  3  80_%tile: 2  bins: 96  poly(254, 4, 76)
# worst:  3  80_%tile: 2  bins: 97  poly(82,11,205)
# worst:  3  80_%tile: 2  bins: 97  poly(165, 2, 57)
# worst:  3  80_%tile: 2  bins: 98  poly(108,22,  1)
# worst:  3  80_%tile: 2  bins: 98  poly(129,18,160)
# worst:  3  80_%tile: 2  bins: 98  poly(135, 1, 38)
# worst:  3  80_%tile: 2  bins: 98  poly(186,22,151)
# worst:  3  80_%tile: 2  bins: 98  poly(203,19,149)
# worst:  3  80_%tile: 2  bins: 98  poly(232,27,160)
# worst:  4  80_%tile: 2  bins: 96  poly(87,19, 62)
# worst:  4  80_%tile: 2  bins: 96  poly(108,19,151)
# worst:  4  80_%tile: 2  bins: 96  poly(135, 4, 38)
# worst:  4  80_%tile: 2  bins: 96  poly(165, 1, 57)
# worst:  4  80_%tile: 2  bins: 96  poly(165, 4, 99)
# worst:  4  80_%tile: 2  bins: 96  poly(203,11,205)
# worst:  4  80_%tile: 2  bins: 97  poly(82,11,151)
# worst:  4  80_%tile: 2  bins: 97  poly(108, 1,160)
# worst:  4  80_%tile: 2  bins: 97  poly(109, 2,160)
#
# worst:  3  80_%tile: 2  bins: 95  poly(109,14, 62)
#   0 **
#   1 **
#   2 *
#   3 ***
#   4 **
#   5 **
#   6 *
#   7 
#   8 *
#   9 **
#  10 ***
#  11 **
#  12 *
#  13 ***
#  14 *
#  15 **
#  16 ***
#  17 *
#  18 ***
#  19 *
#  20 
#  21 **
#  22 **
#  23 **
#  24 **
#  25 *
#  26 ***
#  27 *
#  28 **
#  29 **
#  30 **
#  31 **
#  32 **
#  33 
#  34 *
#  35 
#  36 
#  37 ***
#  38 **
#  39 **
#  40 
#  41 **
#  42 **
#  43 **
#  44 ***
#  45 **
#  46 **
#  47 
#  48 **
#  49 **
#  50 ***
#  51 ***
#  52 
#  53 ***
#  54 
#  55 ***
#  56 *
#  57 **
#  58 **
#  59 *
#  60 *
#  61 
#  62 **
#  63 *
#  64 *
#  65 **
#  66 **
#  67 ***
#  68 **
#  69 *
#  70 **
#  71 **
#  72 *
#  73 **
#  74 
#  75 **
#  76 *
#  77 **
#  78 ***
#  79 *
#  80 *
#  81 **
#  82 **
#  83 *
#  84 ***
#  85 
#  86 ***
#  87 **
#  88 ***
#  89 **
#  90 **
#  91 ***
#  92 
#  93 **
#  94 **
