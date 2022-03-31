#include <iostream>
#include <random>
#include "memlab.h"

using namespace std;

void func(Object x, Object y, type t) {
    gcInitialize();
    Object newArr = createArr(t, 50000);
    Object dest = createVar(t);
    for(int i = 0; i < 50000; i++) {
        switch(t) {
            case integer: {
                int x = rand();
                assignVar(dest, x);
                break;
            }
            case medium_integer: {
                int x = rand()%(1 << 24);
                assignVar(dest, x);
                break;
            }
            case character: {
                char c = rand()%(1 << 8);
                assignVar(dest, c);
                break;
            }
            case boolean: {
                bool b = rand()%2;
                assignVar(dest, b);
                break;
            }
            default: {
                cout << "Error type encountered!!!!";
                exit(-1);
            }
        }
        assignArr(newArr, i, dest);
    }
    graph_data();
    gcRun(true, true);
    return;
}

int main() {
    // 200'000 * bytes Array + 50'000Word --> 
    // 400'000 bytes
    // 1MB
    // 50000 * 20 * 8
    // 50,000
    createMem(1 << 28);
    srand(time(NULL));

    Object x1 = createVar(integer);
    Object y1 = createVar(integer);
    Object x2 = createVar(medium_integer);
    Object y2 = createVar(medium_integer);
    Object x3 = createVar(character);
    Object y3 = createVar(character);
    Object x4 = createVar(boolean);
    Object y4 = createVar(boolean);

    func(x4, y4, x4.objType);
    cout<<"\n\nEnd of func1 \n";
    graph_data();
    func(x3, y3, x3.objType);
    cout<<"\n\nEnd of func2 \n";
    graph_data();
    func(x4, y4, x4.objType);
    cout<<"\n\nEnd of func3 \n";
    graph_data();
    func(x2, y2, x2.objType);
    cout<<"\n\nEnd of func4 \n";
    graph_data();
    func(x3, y3, x3.objType);
    cout<<"\n\nEnd of func5 \n";
    graph_data();
    func(x2, y2, x2.objType);
    cout<<"\n\nEnd of func6 \n";
    graph_data();
    func(x1, y1, x1.objType);
    cout<<"\n\nEnd of func7 \n";
    graph_data();
    func(x1, y1, x1.objType);
    cout<<"\n\nEnd of func8 \n";
    graph_data();

    func(x1, y1, x1.objType);
    cout<<"\n\nEnd of func9 \n";
    graph_data();
    func(x2, y2, x2.objType);
    cout<<"\n\nEnd of func10 \n";
    graph_data();
    gcRun(true);
    return 0;
}
