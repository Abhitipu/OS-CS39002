#!/bin/bash
awk '{count[tolower($variable)]++}END{for(i in count)print i,count[i]}' variable=$2 $1|sort -k2 -rn>"1f_output_$2_column.freq"
