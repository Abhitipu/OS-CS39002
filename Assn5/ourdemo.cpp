#include "memlab.h"

void fun(Object a)
{
    gcInitialize();
    Object c = createVar(medium_integer);
    gcRun(true, true);
}

int main() {
    createMem(1<<28);
    cout<<"Size of MySymbolTable "<<(sizeof(entries) >> 20 )<< " MB\n";
    cout<<"Size of varStack "<<(sizeof(Stack) >> 20 )<< " MB\n";
    cout << "Testing createVar and assignVar\n";
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
    cout<<"Error handling \n";
    cout << "Trying incorrect assignment\n";
    assignVar(b, a);
    cout << b;
    cout << "Trying correct assignment\n";
    assignVar(a, b);
    cout<<a;
    
    cout << "Testing createArr and assignArr\n";

    cout << "Getting size: " << getSize(integer) << '\n';
    Object str = createArr(character, 5);
    assignArr(str, 0, 'H');
    assignArr(str, 1, 'e');
    assignArr(str, 2, 'l');
    assignArr(str, 3, 'l');
    assignArr(str, 4, 'o');
    cout<<str;
    char ch;
    getArr(str, 2, &ch);
    cout<<"str[2] = "<<ch<<"\n";
    getArr(str, 3, &ch);
    cout<<"str[3] = "<<ch<<"\n";
    getArr(str, 4, &ch);
    cout<<"str[4] = "<<ch<<"\n";
    Object str2 = createArr(character, 5);
    assignArr(str2, 0, 'W');
    assignArr(str2, 1, 'o');
    assignArr(str2, 2, 'r');
    assignArr(str2, 3, 'l');
    assignArr(str2, 4, 'd');
    cout<<str2;
    
    fun(str);
    assignArr(str2, 2, str, 0);
    cout<<str2;
    cout<<"Last\n";
    graph_data();
   
    gcRun(true, false);
    return 0;
}
