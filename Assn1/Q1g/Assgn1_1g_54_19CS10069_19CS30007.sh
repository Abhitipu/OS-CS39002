echo -n>$1
for i in {1..150};do
    for j in {1..10};do
        echo -n "$RANDOM,">>$1;
    done
    echo "">>$1;
done

awk -F ',' '{if($colnum~regex){ok="YES"}}END{print ok}' ok="NO" colnum=$2 regex=$3 $1
