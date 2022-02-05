#include <iostream>
#include <unistd.h>

using namespace std;

int fib(int n) {
    return (n <= 1) ? n : fib(n - 1) + fib(n -2);
}

int main() {
    cout << "Started prog 2 yay!" << endl;
    int check = fib(43); 
    sleep(2);   
    cout << "Ended prog 2! " << check << endl;
    return 0;
}
