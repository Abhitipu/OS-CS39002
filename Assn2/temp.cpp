#include <iostream>
#include <functional>
#include <map>
#include <string>

using namespace std;

map<string, function<int(string)>> mp;

int cdfunc(string s) {
    cout << "Inside cd! " << s << '\n';
    return 1;
}

int helpfunc(string s) {
    cout << "Inside help! " << s << '\n';
    return 1;
}

int main() {
    cout << "Hello world!\n";
    mp["cd"] = cdfunc;
    mp["help"] = helpfunc;

    mp["help"]("checker");
    return 0;
}