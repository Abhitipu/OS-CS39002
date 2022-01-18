REQ_HEADERS="User-Agent,Host";
curl -s "https://www.example.com/" > example.html
curl "http://ip.jsontest.com/"
echo $REQ_HEADERS | tr "," "\n" | awk '{system("curl -s http://headers.jsontest.com/ | grep "$0)}'

echo -n "" > valid.txt
echo -n "" > invalid.txt
# verdict="LOL"
for file in ./JSONData/*; do 
    verdict=$(curl -s --data-urlencode json@$file http://validate.jsontest.com  | jq .validate)
    [[ "$verdict" = "true" ]] && echo $file >> valid.txt || echo $file >> invalid.txt;
done
