# Edge list (1 line per edge, undirected)
# =>
# Adjacency list (id adjacency_list_length \t neighbor1 neighbor2 ...)

# Vertices are sorted by IDs (Line 32)

import sys
import csv

if len(sys.argv) != 2:
    print("please give the file name")
    sys.exit()
else:
    store = {}
    fn = sys.argv[1]
    with open(fn) as f:
        f_csv = csv.reader(f, delimiter='\t')
        for row in f_csv:
            vid = int(row[0])
            adj = int(row[1])
            if(vid == adj):
                continue
            if vid not in store:
                store[vid] = set()
            store[vid].add(adj)
            if adj not in store:
                store[adj] = set()
            store[adj].add(vid)

    out_fn = fn+'_adj'
    with open(out_fn, 'w') as f:
        for key in sorted(store.keys()):
            f.write(str(key)+' '+str(len(store[key]))+'\t')
            count = 0
            for v in sorted(store[key]):
                f.write(str(v))
                count+=1
                if count != len(store[key]):
                    f.write(' ')
            f.write('\n')
