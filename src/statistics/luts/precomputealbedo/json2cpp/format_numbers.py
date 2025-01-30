# © 2024-2025 Hiroyuki Sakai

import sys

def round_to_n(x, n):
 if n < 1:
   raise ValueError("number of significant digits must be >= 1")
 # Use %e format to get the n most significant digits, as a  string.
 format = "%." + str(n-1) + "e"
 as_string = format % x
 return as_string

with open(sys.argv[1], 'r') as f:
  lines = f.readlines()

  for line in lines:
    values = line.split()
    for j in range(0, len(values)-1):
      print values[j],
    print round_to_n(float(values[len(values)-1]), 9); # use 9 significant digits for float
