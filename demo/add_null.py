# The quasi-clique input format should be one vertex per line
# Each line only needs the vertex IDs in the adjacency list
# Since datasets may have vertex IDs that are not continuous, we need to add empty lines for missing vertices

import sys

fname = sys.argv[1]
fwn = fname + "_null" # new file with empty lines, to create
fw = open(fwn, "w")
file1 = open(fname, 'r') 
Lines = file1.readlines()
count = 0
for line in Lines:
    cur = int(line.split()[0]) # get first item in the line, which is ID
    diff = cur - count # if cur < count, missing (cur - count) lines, need to add them
    for i in range(diff):
        fw.write("\n")
        count += 1
    fw.write(line)
    count += 1
file1.close()
fw.close()
