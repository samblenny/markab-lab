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
  for a in [4,14,21,35,82,87,135,163,176,183,216,218,232,250]:
    for b in [1,4,5,6,14,18,19,20,23,27,28,30]:
      # print(".", end='')
      # sys.stdout.flush()
      for c in [38,46,59,61,99,104,114,122,130,160,205,213,219]:
        hash_fn = lambda w: poly_hash(a, b, c, w)
        hashes = [hash_fn(w) for w in WORDS]
        for bin_count in [94,95,96,97,98,99]:
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


# worst:  3  80_%tile: 2  bins: 94  poly(183,20,130)
# worst:  3  80_%tile: 2  bins: 95  poly(218,23, 46)
# worst:  3  80_%tile: 2  bins: 96  poly(87,14, 99)
# worst:  3  80_%tile: 2  bins: 97  poly(14,28,122)
# worst:  3  80_%tile: 2  bins: 97  poly(21, 4,114)
# worst:  3  80_%tile: 2  bins: 97  poly(163, 5,104)
# worst:  3  80_%tile: 2  bins: 98  poly(135, 1, 38)
# worst:  3  80_%tile: 2  bins: 98  poly(232,27,160)
# worst:  3  80_%tile: 2  bins: 99  poly( 4,19,219)
# worst:  3  80_%tile: 2  bins: 99  poly(35,30,213)
# worst:  3  80_%tile: 2  bins: 99  poly(176,23, 59)
# worst:  3  80_%tile: 2  bins: 99  poly(216, 6,205)
# worst:  3  80_%tile: 2  bins: 99  poly(250,18, 61)
# worst:  4  80_%tile: 2  bins: 95  poly(82, 1,213)
# worst:  4  80_%tile: 2  bins: 95  poly(218,23,114)
# worst:  4  80_%tile: 2  bins: 95  poly(218,27,213)
# worst:  4  80_%tile: 2  bins: 95  poly(218,30,122)
# worst:  4  80_%tile: 2  bins: 95  poly(232,28,122)
# worst:  4  80_%tile: 2  bins: 96  poly(21, 1,122)
# worst:  4  80_%tile: 2  bins: 96  poly(21, 5,219)
#
# worst:  3  80_%tile: 2  bins: 94  poly(183,20,130)
#   0 ***
#   1 **
#   2 **
#   3 **
#   4 **
#   5
#   6 *
#   7 **
#   8
#   9 **
#  10 **
#  11 *
#  12 **
#  13 **
#  14 **
#  15 *
#  16
#  17 ***
#  18 **
#  19 ***
#  20 ***
#  21 *
#  22 **
#  23 *
#  24 **
#  25
#  26 ***
#  27 *
#  28 **
#  29
#  30 **
#  31 ***
#  32 *
#  33 *
#  34 **
#  35 *
#  36 *
#  37 **
#  38 **
#  39
#  40 *
#  41 *
#  42 *
#  43 **
#  44 *
#  45 **
#  46 *
#  47 **
#  48
#  49 *
#  50 *
#  51 ***
#  52 *
#  53 **
#  54 ***
#  55 **
#  56 **
#  57 ***
#  58 ***
#  59 *
#  60 **
#  61 **
#  62 **
#  63 **
#  64 **
#  65 **
#  66
#  67 ***
#  68 **
#  69 *
#  70 ***
#  71 **
#  72 **
#  73 ***
#  74 ***
#  75 ***
#  76 **
#  77 *
#  78 **
#  79 **
#  80 *
#  81 ***
#  82 ***
#  83 **
#  84 *
#  85 *
#  86 *
#  87 **
#  88 *
#  89 *
#  90
#  91 **
#  92 **
#  93 ***
