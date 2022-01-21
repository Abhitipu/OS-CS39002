mkdir -p files_mod
for filename in $1/*;do
    awk '{print NR" "$s}' $filename|tr -s ' ' ','>"files_mod/$(basename $filename)"
done
