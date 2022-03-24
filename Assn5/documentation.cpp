#include <iostream>
#include <string>

using namespace std;

typedef enum _type {
    boolean,
    character,
    medium_integer,
    integer
} type;

class ScopeHandler {
    string scope;
public:
    ScopeHandler(string _scope = "global") {}

    void appendScope(string newScope) {
        scope += "/";
        scope += newScope;
    }

    void pop() {
        int lastPos = scope.find_last_of('/');
        if(lastPos != scope.npos)
            scope = scope.substr(0, lastPos);
    }
};

ScopeHandler myScope;
class Object;

class SymTableEntry {
public:
    Object refObj;
    int wordIndex, wordOffset;
    bool valid, marked;

    SymTableEntry(string _name="Temp", string _scope="global", type _objType=integer, int _size=-1, int _totSize=-1, int _wordIndex=-1, int _wordOffset=-1, bool _valid= false, bool _marked = false):
    wordIndex(_wordIndex), wordOffset(_wordOffset), valid(_valid), marked(_marked), refObj(_name, _scope, _objType, _size, _totSize) { }

    inline void unmark() {
        marked = false;
    }

    inline void invalidate() {
        valid = false;
    }
};

SymTableEntry entries[1000];

// entries --
// createVar -- push into stack

class Stack {
public:
    int indices[1000];
    int top;

    Stack():top(-1) {
        
    }

    void push(int index) {
        if(top == 1000)
            printf("Stack overflow!");
        else
            indices[++top] = index;
    }

    int pop() {
        if(top < 0)
            printf("Stack Underflow!\n");
        else
            top--;
    }

    int peek() {
        if(top < 0)
            printf("Stack Underflow!\n");
        else
            return indices[top];
    }
};

Stack varStack;

class Object {
public:
    string name, scope;
    type objType;
    int size;
    int totSize;
    Object(string _name="Temp", string _scope="global", type _objType=integer, int _size=-1, int _totSize=-1) {}
};

int main() {
    return 0;
}