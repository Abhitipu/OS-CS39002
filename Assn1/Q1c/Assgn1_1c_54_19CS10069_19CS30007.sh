mkdir -p nil
find $1 -mindepth 1 -type f -exec mv -t ./nil -f '{}' +
rm -rf $1
find nil -type f | awk -F "/" '{n=split($NF, a, "."); if(a[n] != $NF) ext[a[n]]++;} END{ for (e in ext) {system("mkdir -p "e";echo ./nil/*."e" | xargs mv -t ./"e)}}'