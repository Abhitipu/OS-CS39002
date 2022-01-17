find $1 -type f | awk -F '/' '{n=split($NF, parts, "."); ext=(parts[n]==$NF)?"nil":parts[n]; system("mkdir -p " ext); system("cp " $0 " " ext);}';
rm -r $1;
