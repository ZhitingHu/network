import codecs
import sys
from sets import Set

path = 'data/web-Google/'
data_filename = path + 'web-Google.txt'
output_filename = data_filename + '_reindex' 
map_filename = path + 'node_index_map.txt'
num_header_lines_to_skip = 4

data_fin = open(data_filename, 'r')
fout = open(output_filename, 'w')
nimap_fout = open(map_filename, 'w')

# skip header lines
for i in range(0, num_header_lines_to_skip):
    data_fin.readline();

num_edges = 0
new_idx = 0
idx_map = {}
for line in data_fin:
    line = line.strip()
    parts = line.split('\t')
    from_idx = int(parts[0])
    to_idx = int(parts[1])
    if from_idx not in idx_map:
        idx_map[from_idx] = new_idx
        new_idx += 1
    if to_idx not in idx_map:
        idx_map[to_idx] = new_idx
        new_idx += 1
    num_edges += 1
print("#Vertices {0}".format(len(idx_map)))
print("#Edges {0}".format(num_edges))

# output index map
for idx_prev, idx_new in idx_map.iteritems():
    nimap_fout.write("{0}\t{1}\n".format(idx_prev, idx_new));
nimap_fout.flush();
nimap_fout.close();

fout.write("{0}\n".format(len(idx_map)))

data_fin.seek(0)
# skip header lines
for i in range(0, num_header_lines_to_skip):
    data_fin.readline();
for line in data_fin:
    line = line.strip()
    parts = line.split('\t')
    from_idx = int(parts[0])
    to_idx = int(parts[1])
    fout.write("{0}\t{1}\n".format(idx_map[from_idx], idx_map[to_idx]))
fout.flush()
fout.close()
data_fin.close()
