# © 2024-2025 Hiroyuki Sakai

import numpy
import sys

with open(sys.argv[1], 'r') as f:
  lines = f.readlines()
  lastlineindices = lines[-1].split()[:-1]
  numdimensions = len(lastlineindices)
  outputsize = [int(i) + 1 for i in lastlineindices]
  numvalues = numpy.prod(numpy.array(outputsize))
  output = numpy.empty([numvalues], dtype=object)

  for i in range(0, numvalues):
    output[i] = lines[i].split()[numdimensions]

  reshapedoutput = output.reshape(outputsize)

it = numpy.nditer(reshapedoutput, flags=['multi_index', 'refs_ok'])
lastindex = (None,) * (numdimensions-1);

sys.stdout.write('{\n')
while not it.finished:
  # for i in range(0, numdimensions-1):
    # if lastindex[i] is not None and it.multi_index[i] != lastindex[i]:
    #   sys.stdout.write('}')
  commaset = False
  for i in range(0, numdimensions-1):
    if lastindex[i] is not None and it.multi_index[i] != lastindex[i] and not commaset:
      sys.stdout.write(',\n')
      commaset = True
  # for i in range(0, numdimensions-1):
    # if it.multi_index[i] != lastindex[i]:
    #   sys.stdout.write('{')

  if it[0] == "nan":
    sys.stdout.write("0.f")
  else:
    # it[0] doesn't work flawlessly in some cases (spurious terminating bytes)
    #sys.stdout.write(it[0])
    sys.stdout.write(output[numpy.ravel_multi_index(it.multi_index, outputsize)])

  if '.' in str(it[0]):
    sys.stdout.write('f')
  if numdimensions > 0 and it.multi_index[numdimensions-1] != outputsize[numdimensions-1] - 1:
    sys.stdout.write(', ')

  lastindex = it.multi_index[0:numdimensions-1]
  it.iternext()

# for i in range(0, numdimensions-1):
#   sys.stdout.write('}')
sys.stdout.write('\n}\n')
