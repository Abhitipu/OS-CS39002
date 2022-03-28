#ifndef __MEMLAB_H
#define __MEMLAB_H

#include <iostream>
#include <cstdlib>
#include <climits>
#include <string>
#include <string.h>
#include <cassert>
#include <array>
#include <iomanip>

#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

const int mxn = 1 << 20;
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
    pthread_mutex_t lock;
    int wordIndex, wordOffset;
    bool valid, marked;

    SymTableEntry(type _objType, int _size, int _totSize, int _wordIndex, int _wordOffset, bool _valid, bool _marked);
    ~SymTableEntry();
    inline void unmark();
    inline void invalidate();
};

class Stack {
public:
    int indices[mxn];
    int top;
    pthread_mutex_t lock;
    Stack(); 
    ~Stack();
    void push(int index);
    int pop();
    int peek();
    bool isEmpty();
};

const int maxWords = 1 << 28;  // approx 8e6
// 1gb --> 1 << 30 bytes
// 1gb --> 1 << 28 words
// 1gb --> 1 << 23 ints (each has 1 << 5)
class _validMem {
public:
    // ptr to a free location (int the last free block)
    int ptr;
    // size (in words) available in free mem starting from ptr
    int sizeAvl;
    // 1GB mem -> 256 Mil words -> for each word we need 1 bit, 1 int has 32 bits, thus we need 256Mil/32 = 8 Mil int  
    int totSizeAvl; // max word
    uint32_t mem[maxWords >> 5];
    _validMem();
    int getIndex(int x);
    int getOffset(int x);
    int isSet(int x);
    void set(int x);
    void reset(int x);
};
class entries {
public:
    // 1e9 / 4 words 256 mil. words
    // 32 * 1e6  --> bytes 32mb
    _validMem validMem; // overhead 32MiB, True if word is in use, false if it is not in use
    SymTableEntry myEntries[mxn];
    Stack listOfFreeIndices;
    pthread_mutex_t validMemlock; // for valid Memory
    entries();
    int insert(SymTableEntry st);
    int search(string name, string scope);
};

int createMem(size_t memSize);

// for variables
Object createVar(type t);
int assignVar(Object dest, Object src);
int assignVar(Object, int);
int assignVar(Object dest, Object src, int srcIdx);
void getVar(Object, void*);

// for arrays
Object createArr(type t, int length);
int assignArr(Object dest, int srcIdx, int x);
int assignArr(Object dest, int destIdx, Object src);
int assignArr(Object dest, int destIdx, Object src, int srcIdx);
void getArr(Object, int, void*);

int freeElem(Object toDel, bool locked = false);

size_t getSize(type t, int freq = 1);
void gc_initialize(); // Write this line at the start of every new functions
void gc_run(bool scopeEnd = false, bool toCompact = false);
#endif // __MEMLAB_H