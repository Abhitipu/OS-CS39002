#ifndef __MEMLAB_H
#define __MEMLAB_H

#include <iostream>
#include <cstdlib>
#include <climits>
#include <string>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <stack>
#include <map>
#include <bitset>
#include <cassert>

using namespace std;

const int mxn = 1e4;

typedef enum _type {
    boolean,
    character,
    medium_integer,
    integer
} type;

class Object {
private:
    friend std::ostream & operator<<(std::ostream &os, const Object& o);
public:
    string name, scope;
    type objType;
    int size;
    int totSize;
    Object(string _name, string _scope, type _objType, int _size, int _totSize);
};

class SymTableEntry {
public:
    Object refObj;
    int wordIndex, wordOffset;
    bool valid, marked;

    SymTableEntry(string _name, string _scope, type _objType, int _size, int _totSize, int _wordIndex, int _wordOffset, bool _valid, bool _marked);
    inline void unmark();
    inline void invalidate();
};

class entries {
public:
    // 1e9 / 4 words 256 mil. words
    // 32 * 1e6  --> bytes 32mb
    static bitset<256'000'000> validMem; // overhead 32MiB, True if word is in use, false if it is not in use
    SymTableEntry myEntries[mxn];
    int ctr;
    
    entries();
    int insert(SymTableEntry st);
    int search(string name, string scope);
};

class Stack {
public:
    int indices[1000];
    int top;
    Stack(); 
    void push(int index);
    int pop();
    int peek();
};

/*
    _ _ _ _
    _ _ _ _
    _ _ _ _
    _ _ _ _
*/
int createMem(size_t memSize);

Object createVar(type t, string name, string scope);

int assignVar(type t, type t2);

int createArr(type t, int len);

int freeElem(type* t);

#endif // __MEMLAB_H