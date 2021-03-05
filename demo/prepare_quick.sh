# add empty lines where needed, and remove the first 2 columns

python3 add_null.py $1
nullf="$1_null" # empty lines added, but line format is still that of the original data
sed 's/	/ /g' ca-GrQc.txt_adj_null > temp # change all tabs to spaces
cut -d " " -f 3-  temp > $2
rm temp
rm $nullf