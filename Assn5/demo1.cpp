#include <iostream>
#include "memlab.h"

using namespace std;

void func(Object x, Object y, type t) {
    gc_initialize();
    Object newArr = createArr(t, 50000);
    gc_run(true);
    return;
}

int main() {
    createMem(50000 * 10);
    Object x1 = createVar(integer);
    Object y1 = createVar(integer);
    Object x2 = createVar(medium_integer);
    Object y2 = createVar(medium_integer);
    Object x3 = createVar(character);
    Object y3 = createVar(character);
    Object x4 = createVar(boolean);
    Object y4 = createVar(boolean);

    func(x1, y1, x1.objType);
    func(x2, y2, x2.objType);
    func(x3, y3, x3.objType);
    func(x4, y4, x4.objType);

    return 0;
}