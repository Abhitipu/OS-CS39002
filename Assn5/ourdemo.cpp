#include "memlab.h"

int main() {
    createMem(100);
    Object a = createVar(integer);
    cout<<a;
    Object b = createVar(integer);
    cout<<b;

    cout << "Getting size: " << getSize(integer) << '\n';
    return 0;
}