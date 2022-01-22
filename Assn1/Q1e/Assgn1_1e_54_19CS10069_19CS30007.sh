[[ $1 == "-v" ]]&&_V=1||_V=0

[[ $_V -eq 1 ]] && echo "Fetching html from example.com"
curl -s "https://www.example.com/">example.html
[[ $_V -eq 1 ]] && echo "Completed task 1"

[[ $_V -eq 1 ]] && echo "Obtaining ip address of the local system"
curl "http://ip.jsontest.com/"
[[ $_V -eq 1 ]] && echo "Completed task 2"

[[ $_V -eq 1 ]] && echo "Obtaining headers from headers.jsontest.com"
echo $REQ_HEADERS|tr "," "\n"|awk '{system("curl -s http://headers.jsontest.com/|grep "$0)}'
[[ $_V -eq 1 ]] && echo "Completed task 3"

[[ $_V -eq 1 ]] && echo "Checking validity of json files"
>valid.txt
>invalid.txt
for file in ./JSONData/*;do 
    verdict=$(curl -s --data-urlencode json@$file http://validate.jsontest.com|jq .validate)
    [[ "$verdict" = "true" ]] && echo $file>>valid.txt || echo $file>>invalid.txt;
done
[[ $_V -eq 1 ]] && echo "Completed task 4.. All done!"
