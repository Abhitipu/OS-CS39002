#include <iostream>
#include <unistd.h>

using namespace std;

int fib(int n) {
    return (n <= 1) ? n : fib(n - 1) + fib(n -2);
}

int main() {
    cout<<"Started yay!"<<endl;
    int check = fib(43); 
    sleep(2);   
    cout << endl;
    printf("Ended! %d\n", check);;
    return 0;
}
