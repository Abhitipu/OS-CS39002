#include "memlab.h"

int main() {
    createMem(100);
    Object a = createVar(integer, "a", "main");
    cout<<a;
    Object b = createVar(integer, "a", "main");
    cout<<b;
    return 0;
}