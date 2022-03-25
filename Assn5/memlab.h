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
#include <array>
#include <algorithm>
// Multithreads / multiprocesses
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

const int mxn = 1e4;
const int START_SCOPE = -1;

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
    type objType;
    int size, totSize, symTabIdx;
    Object(type _objType, int _size, int _totSize);
};

class SymTableEntry {
public:
    Object refObj;
    int wordIndex, wordOffset;
    bool valid, marked;

    SymTableEntry(type _objType, int _size, int _totSize, int _wordIndex, int _wordOffset, bool _valid, bool _marked);
    inline void unmark();
    inline void invalidate();
};

class Stack {
public:
    int indices[mxn];
    int top;
    Stack(); 
    void push(int index);
    int pop();
    int peek();
    bool isEmpty();
};

class entries {
public:
    // 1e9 / 4 words 256 mil. words
    // 32 * 1e6  --> bytes 32mb
    static bitset<256'000'000> validMem; // overhead 32MiB, True if word is in use, false if it is not in use
    SymTableEntry myEntries[mxn];
    Stack listOfFreeIndices;
    
    entries();
    int insert(SymTableEntry st);
    int search(string name, string scope);
};



/*
    _ _ _ _
    _ _ _ _
    _ _ _ _
    _ _ _ _
*/
int createMem(size_t memSize);

// for variables
Object createVar(type t);
int assignVar(Object dest, Object src);
int assignVar(Object, int);

// for arrays
Object createArr(type t, int length);
int assignArr(Object dest, int srcIdx, int x);
int assignArr(Object dest, int destIdx, Object src, int srcIdx);

int freeElem(Object toDel);

size_t getSize(type t, int freq = 1);
void startScope(); // Write this line at the start of every new functions
void gc_run(bool , bool) ;
#endif // __MEMLAB_H