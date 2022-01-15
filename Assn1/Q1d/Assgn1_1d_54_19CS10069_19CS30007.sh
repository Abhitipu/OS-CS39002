#!/bin/bash
for filename in $1/*; do
    echo "$(basename $filename) processing started"
    awk '{print NR  " " $s}' $filename |  tr -s ' ' ',' >"files_mod/$(basename $filename)"
    echo "$(basename $filename) processing done"
done
