REQ_HEADERS="User-Agent,Host";
REQ_SPLIT=(${REQ_HEADERS//,/ });

echo $REQ_HEADERS | tr "," "\n" | awk '{system("curl -s http://headers.jsontest.com/ | grep "$0)}'
# echo $REQ_HEADERS | tr "," "\n" | awk '{system("curl -s http://headers.jsontest.com/ | jq ."$0)}'
