import codecs
import sys
from sets import Set

# input
data_filename = 'data/wiki-Vote/wiki-Vote_reindex.txt'
test_data_filename = 'data/wiki-Vote/validation-edges.txt'
# ouput
out_train_filename = 'data/wiki-Vote/wiki-Vote_train.txt'

data_fin = open(data_filename, 'r')
test_data_fin = open(test_data_filename, 'r')

train_fout = open(out_train_filename, 'w')

# read test data & output
test_set = {}
for line in test_data_fin:
  parts = line.strip().split('\t')
  from_idx = int(parts[0])
  to_idx = int(parts[1])
  min_idx = min(from_idx, to_idx)
  max_idx = max(from_idx, to_idx)
  test_set[(min_idx, max_idx)] = 1
print("#Test links (pos and neg) {0}".format(len(test_set)))
test_data_fin.close()

# read whole data & output
# header lines
train_fout.write(data_fin.readline().strip() + '\n');
num_train_links = 0
for line in data_fin:
  line = line.strip()
  parts = line.split('\t')
  from_idx = int(parts[0])
  to_idx = int(parts[1])
  min_idx = min(from_idx, to_idx)
  max_idx = max(from_idx, to_idx)
  if (min_idx, max_idx) in test_set:
    continue
  train_fout.write("{0}\t{1}\n".format(min_idx, max_idx))
  num_train_links += 1
print("#Train links (pos) {0}".format(num_train_links))
data_fin.close()
train_fout.flush()
