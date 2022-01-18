REQ_HEADERS="User-Agent,Host";
curl -s "https://www.example.com/" > example.html
curl "http://ip.jsontest.com/"
echo $REQ_HEADERS | tr "," "\n" | awk '{system("curl -s http://headers.jsontest.com/ | grep "$0)}'
# echo $REQ_HEADERS | tr "," "\n" | awk '{system("curl -s http://headers.jsontest.com/ | jq ."$0)}'
# 1. find karenge, sahi hai
# 2. file ko direct curl mein bhej denge, thik
# 3. validate ko dekh lenge, 
# 4. valid.txt and invalid.txt mein concat
# curl -G -v "http://validate.jsontest.com" --data-urlencode 'json={"key":"value"}'
# Tera code paste karna? flags waala? --data.. waala
# Note that if you want to send a URL encoded string at a parameter, 
# WITHOUT using multipart form encoding, you should use curl --data-urlencode json@file http://validate.jsontest.com
# So kaise hoga? tu likh : P
# curl --data-urlencode json@JSONData/0.json http://validate.jsontest.com file name kaha se laye $([ "$b" == 5 ] && echo "$c" || echo "$d")
# -s laga... good good.. iske baad awk ? Ya kuch aur karsakte?... hmm good question.. good solution sahi hai! dikkat hai... true ko kaise detect karun? kar ke dekh.. ruk.. Abbey Load hogya
# pehle mein factor $1 nhi chalega.. Pata nhi re... ta bola allowed nhi hai
# thik hai baad me dekhte hai woh Haan
# find ./JSONData/* | awk '{system("curl -s --data-urlencode json@"$0" http://validate.jsontest.com  | jq .validate | "$0 ">>("$0==true")?valid.txt:invalid.txt") }' #| jq .validate | awk ''
# Abbey ye ( kahaan se aa rha
# find ./JSONData/* | awk '{system("curl -G -v )}' system kuch return karta hai?? lile woh true return kr detoh (pata nhi yar)
# find ./JSONData/* | awk '{system("curl -s --data-urlencode json@"$0" http://validate.jsontest.com  | jq .validate | echo "$0 ) }' #| jq .validate | awk ''
# sahi toh lagrha : / 
# find ./JSONData/* | awk '{"echo "$0 | getline var; print var }' var="aryan" dekhta (samajh nhi aya : (
# Samajh aaya ? : ( nahi
# saala itne quotation marks mein hi prob hai haa, part by part print karte hai
# jq se tru false bhi return hota
# https://stackoverflow.com/questions/1960895/assigning-system-commands-output-to-variable ye dekh 2way awk io

# kuch kharab solution dhundte hain fir optimize karenge
# Lets say true aur false ko ek aur file mein store karliya : Par fir wahi filename ka load hai

# find ./JSONData/* | awk '{system("curl -s --data-urlencode json@"$0" http://validate.jsontest.com  | jq .validate > temp.txt")}'
# Ye true false toh return karta wahi if else aur filename milata toh sab hojata

# find ./JSONData/* | awk 'BEGIN {cmd = "echo $0" } {cmd |& getline var; close(cmd); print var }' var="aryan" 
# step by step print karke chekc krna, echo $0 dekh le
# You can trick read into accepting from a pipe like this:

# echo "hello world" | { read test; echo test=$test; }
# bc sab undo ho jaa rha null kaha se aaya *.json?
# bkl yesss lol find kya hai ? :D function hai bhai
# [[ $b = 5 ]] && a="$c" || a="$d" ek baar echo valid invalid kar .. wahi usmein file name aa rha
# [ "$verdict" = "true" ] && echo $file >> valid.txt; || echo $file >> invalid.txt;
echo -n "" > valid.txt
echo -n "" > invalid.txt
verdict="LOL"
for file in ./JSONData/*; do 
    verdict=$(curl -s --data-urlencode json@$file http://validate.jsontest.com  | jq .validate)
    # Madarchod saala? Paaji kya kr rahe ho, khtm hogaya na?
    # Lol mai realize nhi kiya tha 
# cat krna hai ya echo? echo se ho rha? haa file name  chahiye WHAT!!!!!!and 
# Bc kya yaar.. itte time se mai ulta kaam kar rha tha
# write the names of the valid and invalid JSON files in

# two new text files - valid.txt and invalid.txt. The file should be in ascending order in the
# output files.

    # Bhai mujhe laga content chahiye
    # echo '$file >>' ("$verdict" == "true")? 'valid.txt' : 'invalid.txt';
    # invalid khali hai? cat karle
    # Nice
    [[ "$verdict" = "true" ]] && echo $file >> valid.txt || echo $file >> invalid.txt;
# yeh ^ bhi try krna

done
#sort valid.txt invalid.txt Ye chahiye nhi... automatically sorted hi rehta hai directory mein
# Bhai agar kuch nahi hoga toh for loop lage ke iterate kar lete hai, awk ko use nahi karte ok
