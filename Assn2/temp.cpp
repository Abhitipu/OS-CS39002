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

int fib(int n) {
    if(n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    cout << "Hello world!\n";
    // mp["cd"] = cdfunc;
    // mp["help"] = helpfunc;

    // mp["help"]("checker");
    cout << fib(45) << '\n';
    return 0;
}