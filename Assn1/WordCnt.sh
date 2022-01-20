echo -n "" > counts.txt
for i in {a..g}; do
    wc -w ./Q1${i}/Assgn* >> counts.txt;
done 
