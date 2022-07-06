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

# These values came from an exhaustive search by poly_hash.c, which takes
# a few minutes to run. These are from the highest ranking results.
A = [2, 15, 24, 78, 79, 85, 119, 121, 125, 229]
B = [4, 6, 8, 9, 10, 14, 18, 22, 27]
C = [38, 55, 64, 79, 81, 85, 95, 171, 206, 218]
BINS = [64]

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
  for a in A:
    for b in B:
      for c in C:
        hash_fn = lambda w: poly_hash(a, b, c, w)
        hashes = [hash_fn(w) for w in WORDS]
        for bin_count in BINS:
          # calculate histogram
          bins = histogram(hashes, bin_count)
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
          stats.append((worst_case, over_median, bin_count, a, b, c))
  print()
  return stats

def top_n(n, stats):
  stats = sorted(stats)
  return stats[:n]

def stats_to_str(s):
    (worst, median, bins, a, b, c) = s
    name = f"poly({a:>3},{b:>2},{c:>3})"
    return f"worst: {worst:>2}  over_median: {median}  bins: {bins}  {name}"

def summarize(stats):
  return "\n".join([stats_to_str(s) for s in stats])

def print_histogram(s):
  print(stats_to_str(s))
  (worst, median, bin_count, a, b, c) = s
  hash_fn = lambda w: poly_hash(a, b, c, w)
  hashes = [hash_fn(w) for w in WORDS]
  bins = histogram(hashes, bin_count)
  for (k, v) in enumerate(bins):
    print(f" {k:>2} {'*' * v}")
  print()

def go():
  """Print stats for the top polynomial hash parameters"""
  load_words()
  stats = top_n(11, gen_stats())
  print(summarize(stats))
  print()
  #Print histogram for the best polynomial hash parameters
  print_histogram(stats[0])


go()
# cProfile.run("go()", sort='cumulative')


# worst:  4  over_median: 10  bins: 64  poly(  2, 4, 55)
# worst:  4  over_median: 10  bins: 64  poly(119, 4,218)
# worst:  4  over_median: 11  bins: 64  poly( 15, 6, 95)
# worst:  4  over_median: 11  bins: 64  poly( 79,18, 81)
# worst:  4  over_median: 12  bins: 64  poly( 78,22, 79)
# worst:  4  over_median: 12  bins: 64  poly( 85, 9,206)
# worst:  4  over_median: 12  bins: 64  poly(125,14, 64)
# worst:  4  over_median: 12  bins: 64  poly(229,27, 85)
# worst:  4  over_median: 13  bins: 64  poly(121,10, 38)
# worst:  4  over_median: 14  bins: 64  poly( 24, 8,171)
# worst:  5  over_median: 13  bins: 64  poly( 15,10, 95)
#
# worst:  4  over_median: 10  bins: 64  poly(  2, 4, 55)
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
