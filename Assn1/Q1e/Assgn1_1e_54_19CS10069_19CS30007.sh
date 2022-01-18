#!/bin/bash

# REQ_HEADERS="\"User-Agent\",\"Host\"";
REQ_HEADERS="User-Agent,Host";
RESPONSE=$(curl -s  "http://headers.jsontest.com/")
# response=$(curl "http://headers.jsontest.com/")
echo $RESPONSE | jq .Host
echo $REQ_HEADERS | tr "," "\n" | awk '{system("echo $RESPONSE | jq ."$0" ")}'
# response=$(curl "http://headers.jsontest.com/")
# echo $response;
# REQ_SPLIT=(${REQ_HEADERS//,/ })
# echo ${REQ_SPLIT[@]}
# curl -s  "http://headers.jsontest.com/" | jq ".[\"${REQ_SPLIT[@]}\"]"
# echo ${REQ_HEADERS[1]
# curl -s  "http://headers.jsontest.com/" | jq ".[$REQ_HEADERS]"
# echo ${REQ_SPLIT[1]};
# curl -s  "http://headers.jsontest.com/" | jq ".[\"Host\",\"User-Agent\"]"
# curl -v -H "X-Custom-Header: Aryan" "http://headers.jsontest.com/";
# grep User-Agent < response
# curl -v -H "X-Custom-Header: Aryan" "http://headers.jsontest.com/" ;
# curl -I -H "X-Custom-Header: Aryan" "http://headers.jsontest.com/" ;

# 2>&1 | echo;
# echo $response | python3 -c "import sys, json; print(sys.stdin))"
# curl http://headers.jsontest.com/ > example2.json
