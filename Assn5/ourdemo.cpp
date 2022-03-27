#include "memlab.h"

void fun(Object a)
{
    gc_initialize();
    freeElem(a);
    Object c = createVar(medium_integer);
    gc_run(true, true);
}

int main() {
    createMem(100);
    Object a = createVar(integer);
    assignVar(a, 53);
    cout<<a;
    Object a2 = createVar(medium_integer);
    assignVar(a2, 53);
    cout<<a2;
    Object b = createVar(character);
    assignVar(b, 'a');
    cout<<b;
    Object bdup = createVar(character);
    assignVar(bdup, 'd');
    cout<<bdup;
    Object c = createVar(boolean);
    assignVar(c, false);
    cout<<c;
    assignVar(a, c);
    cout<<"Assigning bdup to b\n";
    assignVar(b, bdup);
    cout<<b;
    cout << "Getting size: " << getSize(integer) << '\n';
    Object str = createArr(character, 5);
    assignArr(str, 0, 'H');
    assignArr(str, 1, 'e');
    assignArr(str, 2, 'l');
    assignArr(str, 3, 'l');
    assignArr(str, 4, 'o');
    cout<<str;

    Object str2 = createArr(character, 5);
    assignArr(str2, 0, 'W');
    assignArr(str2, 1, 'o');
    assignArr(str2, 2, 'r');
    assignArr(str2, 3, 'l');
    assignArr(str2, 4, 'd');
    cout<<str2;
    fun(str);
    cout<<str2;
    str = createArr(character, 5);
    gc_run(true);
    return 0;
}
