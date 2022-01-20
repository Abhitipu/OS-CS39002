num=$1
for i in $(seq 2 $1);do
    while [ $((num%$i)) == 0 ];do
        echo -n $i" " 
        num=$((num/$i))
    done
done
echo
