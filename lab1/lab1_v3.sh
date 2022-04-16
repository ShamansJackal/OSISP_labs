#!/bin/bash

IFS=$'\n'
ERR="/tmp/errors.log"

exec 3>&2 2>$ERR 

dir=$(find $1 -type f)

for file in $dir; do
    if grep -x "$2" -H $file; then
        stat -c "%n %s" $file;
    fi
done
exec 2>&3 3>&-
sed "s/.[a-zA-Z]*:/`basename $0`:/" < $ERR 1>&2
