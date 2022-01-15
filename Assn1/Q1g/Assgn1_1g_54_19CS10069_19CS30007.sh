touch $1;
for i in {1..150}; do
    for j in {1..10}; do
        echo $i $j >> $1;
    done
done
