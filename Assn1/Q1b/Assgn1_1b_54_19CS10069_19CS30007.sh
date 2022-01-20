mkdir -p 1.b.files.out
for file in 1.b.files/*;do
	sort -n $file>"1.b.files.out/$(basename $file)"
done
cat 1.b.files.out/*|awk '{count[$0]++}END{for(i in count)print i,count[i]}'|sort -n>1.b.out.txt
