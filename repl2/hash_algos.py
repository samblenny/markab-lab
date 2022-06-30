#!/usr/bin/python3
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
# Experiment to compare string hashing algorithms for possible hashmap keying
#

SYM_IN = 'kernel.symbols'
BINS = 32
TOTAL = 0

def histogram(hash_fn, words):
  """Generate histogram from applying hash_fn to each word of words"""
  bins = [0] * BINS
  for w in words:
    k = hash_fn(w) & (BINS-1)
    bins[k] += 1
  return bins

def poly_hash(a, b, c, word):
  """Polynomial hash algorithm"""
  k = c
  for byte_ in word.encode('utf8'):
    k = (k * a) ^ byte_
  return k ^ (k >> b)

def m3_hash(seed, word):
  """Murmur3 hash algorithm (lazy version)"""
  bytes_ = word.encode('utf8')
  h = seed
  k = 0
  mask = 0xffffffff
  for byte_ in bytes_:
    k = byte_
    k = (k * 0xcc9e2d51) & mask
    k = ((k << 15) | (k >> (32-15))) & mask
    k = (k * 0x1b873593) & mask
    h ^= k
    h = ((h << 13) | (h >> (32-13))) & mask
    h = (h * 5) & mask
    h = (h + 0xe6546b64) & mask
  h ^= len(bytes_)
  h ^= h >> 16
  h = (h * 0x85ebca6b) & mask
  h ^= h >> 13
  h = (h * 0xc2b2ae35) & mask
  h ^= h >> 16
  return h

def print_histogram(name, bins):
  worst_case = max(bins)
  ideal = TOTAL / BINS
  efficiency = worst_case / ideal
  print(f"{name} (efficiency: {efficiency:.2f}, worst: {worst_case}):")
  for (k, v) in enumerate(bins):
    print(f" {k:>2} {'*' * v}")
  print()

def histo_stats(name, bins):
  worst_case = max(bins)
  ideal = TOTAL / BINS
  efficiency = worst_case / ideal
  return (efficiency, worst_case, name)

words = []
with open(SYM_IN, 'r') as f:
  lines = f.read().strip().split("\n")
  words = [L.split()[1] for L in lines]
TOTAL = len(words)

print("BINS =", BINS)
stats = []
for a in [2]:
  for b in range(16):
    for c in range(256):
      hash_fn = lambda w: poly_hash(a, b, c, w)
      stats.append(histo_stats((a,b,c), histogram(hash_fn, words)))
stats = sorted(stats)
(median_e, _, _) = stats[len(stats)//2]
for (efficiency, worst_case, args) in stats[:10]:
  (a, b, c) = args
  name = f"poly({a:>2},{b:>2},{c:>3})"
  print(f"{name}  efficiency: {efficiency:.3f}  worst: {worst_case}")
print()

def print_poly(a, b, c, words):
  hash_fn = lambda w: poly_hash(a, b, c, w)
  print_histogram(f"poly({a},{b},{c})", histogram(hash_fn, words))

(a, b, c) = stats[0][2]
print_poly(a, b, c, words)

stats = []
for seed in range(999):
  hash_fn = lambda w: m3_hash(seed, w)
  stats.append(histo_stats(seed, histogram(hash_fn, words)))
stats = sorted(stats)
(median_e, _, _) = stats[len(stats)//2]
for (efficiency, worst_case, seed) in stats[:10]:
  if efficiency < median_e:
    print(f"m3({seed})  efficiency: {efficiency:.3f}  worst: {worst_case}")
print()

def print_m3(seed, words):
  hash_fn = lambda w: m3_hash(seed, w)
  print_histogram(f"m3({seed})", histogram(hash_fn, words))

print_m3(stats[0][2], words)


# BINS = 16
# poly( 2, 5, 88)  efficiency: 1.178  worst: 12
# poly( 2, 6, 88)  efficiency: 1.178  worst: 12
# poly( 2, 7, 57)  efficiency: 1.178  worst: 12
# poly( 2, 9,235)  efficiency: 1.178  worst: 12
# poly( 2, 2,  8)  efficiency: 1.276  worst: 13
# poly( 2, 2, 23)  efficiency: 1.276  worst: 13
# poly( 2, 2, 40)  efficiency: 1.276  worst: 13
# poly( 2, 2, 55)  efficiency: 1.276  worst: 13
# poly( 2, 2, 72)  efficiency: 1.276  worst: 13
# poly( 2, 2, 87)  efficiency: 1.276  worst: 13
#
# poly(2,5,88) (efficiency: 1.18, worst: 12):
#   0 ************
#   1 **********
#   2 *******
#   3 *********
#   4 **********
#   5 ***********
#   6 ***********
#   7 **********
#   8 **********
#   9 ***********
#  10 ********
#  11 **********
#  12 **********
#  13 ***********
#  14 ***********
#  15 ************
#
# m3(859)  efficiency: 1.178  worst: 12
# m3(39)  efficiency: 1.276  worst: 13
# m3(147)  efficiency: 1.276  worst: 13
# m3(149)  efficiency: 1.276  worst: 13
# m3(177)  efficiency: 1.276  worst: 13
# m3(227)  efficiency: 1.276  worst: 13
# m3(308)  efficiency: 1.276  worst: 13
# m3(314)  efficiency: 1.276  worst: 13
# m3(322)  efficiency: 1.276  worst: 13
# m3(334)  efficiency: 1.276  worst: 13
#
# m3(859) (efficiency: 1.18, worst: 12):
#   0 **********
#   1 ***********
#   2 ***********
#   3 *********
#   4 **********
#   5 **********
#   6 *********
#   7 ***********
#   8 ********
#   9 ************
#  10 ************
#  11 **********
#  12 **********
#  13 ***********
#  14 *********
#  15 **********

#================================================================

# BINS = 32
# poly( 2, 4,  7)  efficiency: 1.374  worst: 7
# poly( 2, 1, 27)  efficiency: 1.571  worst: 8
# poly( 2, 1, 59)  efficiency: 1.571  worst: 8
# poly( 2, 1, 91)  efficiency: 1.571  worst: 8
# poly( 2, 1,123)  efficiency: 1.571  worst: 8
# poly( 2, 1,155)  efficiency: 1.571  worst: 8
# poly( 2, 1,187)  efficiency: 1.571  worst: 8
# poly( 2, 1,219)  efficiency: 1.571  worst: 8
# poly( 2, 1,251)  efficiency: 1.571  worst: 8
# poly( 2, 2, 17)  efficiency: 1.571  worst: 8
#
# poly(2,4,7) (efficiency: 1.37, worst: 7):
#   0 *******
#   1 *****
#   2 ******
#   3 ******
#   4 ***
#   5 ******
#   6 *******
#   7 ******
#   8 *****
#   9 ******
#  10 *******
#  11 ****
#  12 ******
#  13 **
#  14 ******
#  15 **
#  16 ****
#  17 *****
#  18 **
#  19 ******
#  20 *******
#  21 *****
#  22 *****
#  23 **
#  24 ******
#  25 *******
#  26 *****
#  27 ******
#  28 ******
#  29 ****
#  30 ****
#  31 *****
#
# m3(511)  efficiency: 1.374  worst: 7
# m3(16)  efficiency: 1.571  worst: 8
# m3(17)  efficiency: 1.571  worst: 8
# m3(22)  efficiency: 1.571  worst: 8
# m3(61)  efficiency: 1.571  worst: 8
# m3(68)  efficiency: 1.571  worst: 8
# m3(115)  efficiency: 1.571  worst: 8
# m3(146)  efficiency: 1.571  worst: 8
# m3(148)  efficiency: 1.571  worst: 8
# m3(149)  efficiency: 1.571  worst: 8
#
# m3(511) (efficiency: 1.37, worst: 7):
#   0 *
#   1 ******
#   2 ******
#   3 ******
#   4 ******
#   5 *
#   6 ****
#   7 *******
#   8 ***
#   9 *******
#  10 *****
#  11 ******
#  12 *****
#  13 ******
#  14 ****
#  15 *****
#  16 ***
#  17 *******
#  18 ****
#  19 ******
#  20 *******
#  21 ******
#  22 *****
#  23 *****
#  24 ******
#  25 ****
#  26 ******
#  27 ******
#  28 *****
#  29 *******
#  30 ****
#  31 ****

#================================================================

# BINS = 64
# poly( 2, 4, 55)  efficiency: 1.571  worst: 4
# poly( 2, 1, 33)  efficiency: 1.963  worst: 5
# poly( 2, 1, 97)  efficiency: 1.963  worst: 5
# poly( 2, 1,161)  efficiency: 1.963  worst: 5
# poly( 2, 1,225)  efficiency: 1.963  worst: 5
# poly( 2, 2,  2)  efficiency: 1.963  worst: 5
# ...
# m3(146)  efficiency: 1.963  worst: 5
# m3(154)  efficiency: 1.963  worst: 5
# m3(166)  efficiency: 1.963  worst: 5
# m3(17)  efficiency: 1.963  worst: 5
# m3(173)  efficiency: 1.963  worst: 5
# m3(209)  efficiency: 1.963  worst: 5
