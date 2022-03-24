#include <iostream>
#include "memlab.h"

using namespace std;

void func(Object x, Object y) {
    init("func");
    Object newArr = CreateArr(type, 50000);
    Object i = createVar(integer);
    gc_run();
}

int main() {
    Object1 x1, y1;
    Object2 x2, y2;
    Object3 x3, y3;
    Object4 x4, y4;

    func(x1, y1);
    func(x2, y2);
    func(x3, y3);
    func(x4, y4);

    return 0;
}