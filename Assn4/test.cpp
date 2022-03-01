#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

using namespace std;

int main() {
    double start = time(NULL);
    sleep(2);
    cout << time(NULL) - start << '\n';
    return 0;
}