#!/bin/bash

IFS=$'\n'
ERR="/tmp/errors.log"

exec 3>&2 2>$ERR 

dir1=$(find $1 -type f);
dir2=$(find $2 -type f);

n=0;

for file in $dir2; do
    ((n++))
done

exec 2>&3 3>&-
sed "s/.[a-zA-Z]*:/`basename $0`:/" < $ERR 1>&2

exec 3>&2 2>$ERR 
for file in $dir1; do
    ((n++))
    for subFile in $dir2; do
        if cmp -s $file $subFile; then
            echo "$file = $subFile";
        fi
    done 
done

exec 2>&3 3>&-
sed "s/.[a-zA-Z]*:/`basename $0`:/" < $ERR 1>&2

echo "$n";
rm $ERR;
