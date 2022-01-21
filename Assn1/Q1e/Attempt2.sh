_V=0

while getopts "v" OPTION
do
  case $OPTION in
    v) _V=1
       ;;
  esac
done

[[ $_V -eq 1 ]] && echo "Fetching html from example.com"
curl -s "https://www.example.com/">example.html
[[ $_V -eq 1 ]] && echo "Completed task 1"

[[ $_V -eq 1 ]] && echo "Obtaining ip address of the local system"
curl "http://ip.jsontest.com/"
[[ $_V -eq 1 ]] && echo "Completed task 2"

[[ $_V -eq 1 ]] && echo "Obtaining headers from headers.jsontest.com"
curl -s http://headers.jsontest.com/>temp_unique.json
for header in ${REQ_HEADERS//,/ };do
  [[ $_V -eq 1 ]] && echo -n $header" :"
  jq .\"$header\" temp_unique.json
done
[[ $_V -eq 1 ]] && echo "Response received"
[[ $_V -eq 1 ]] && jq . temp_unique.json
[[ $_V -eq 1 ]] && echo "Completed task 3"
rm temp_unique.json

[[ $_V -eq 1 ]] && echo "Checking validity of json files"
>valid.txt
>invalid.txt
for file in ./JSONData/*;do 
    verdict=$(curl -s --data-urlencode json@$file http://validate.jsontest.com|jq .validate)
    [[ "$verdict" = "true" ]] && echo $file>>valid.txt || echo $file>>invalid.txt;
    [[ $_V -eq 1 ]] && echo $verdict $file
done
[[ $_V -eq 1 ]] && echo "Completed task 4.. All done!"
