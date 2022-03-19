#ifndef __MEMLAB_H
#define __MEMLAB_H

#include <iostream>
#include <cstdlib>
#include <string>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <stack>
#include <map>

typedef enum _type {
    boolean,
    character,
    medium_integer,
    integer
} type;

using namespace std;

/*
    _ _ _ _
    _ _ _ _
    _ _ _ _
    _ _ _ _
*/

struct symbolTableEntry {
    string symbolName;
    type dataType;
    int offset;
    size_t len; // how long is the segment? (word)
};

struct symbolTable {
    string scope;
    int tableSize;
    vector<symbolTableEntry> entries;

    symbolTable(string _scope): scope(_scope), tableSize(0), entries(){

    }

    void addEntry(symbolTableEntry newEntry) {
        entries.push_back(newEntry);
        tableSize++;
    }
};

int createMem(size_t memSize);

int createVar(type t, char* scope);

int assignVar(type t, type t2);

int createArr(type t, int len);

int freeElem(type* t);

#endif // __MEMLAB_H