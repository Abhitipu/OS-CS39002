#include "memlab.h"
// Implementation file for our library

// memSeg is the whole memory
void* memSeg;
int totSize;
stack<symbolTable> symbolTables;
map<int, int> sizeInfo;

// There will be a global stack?

// Garbage Collection
// 1. gc_initialize
// 2. gc_run (check state and free)
// 3. Mark and sweep algorithm
// 4. Need to check this out

int createMem(size_t memSize) {
    memSeg = malloc(memSize);
    if(!memSeg) {
        printf("Malloc failed!!\n");
        return -1;
    }
    memset(memSeg, '\0', memSize);
    totSize = memSize;
    sizeInfo[(int)character] = 1;
    sizeInfo[(int)boolean] = 1;
    sizeInfo[(int)medium_integer] = 3;
    sizeInfo[(int)integer] = 4;

    while(!symbolTables.empty())
        symbolTables.pop();

    // symbolTables.push()
    //  TODO: Spawn the garbage collector
    return memSize;
}

int getBestFit(int reqdSize){
    // for(int i=0; i< totSize/4; ++i)
    // {
    //     checkIfWordFree(i);
    // }
    /*
    
    int curSize = 0;
    int bestSize = INT_MAX;
    int bestPoint = -1;
    for(int l = 0, r = 0; l < totSize; r++) {
        memSeg[n] = *(memSeg + n)
        if(*((char*)memSeg + r) != '\0') {
            if(curSize >= reqdSize) {
                if(curSize < bestSize) {
                    bestSize = curSize;
                    bestPoint = l;
                }
            }
            
            l = r + 1;
        } else {
            curSize++;
        }
    }

    return bestPoint;
    */

}

size_t getSize(type t, int freq = 1) {
    return sizeInfo[(int)t] * freq;
}

int createVar(type t, char* scope) {
    // local address is generated --> internal counter
    // local addr --> suitable addr --> within assigned memory
    
    // local --> page table?
    // map<<string:var name,string:scope>, <address, datatype, end address>> --> symbolTableEntry
    // symbolTable --> physical // malloc 
    int myOffset = getBestFit(getSize(t)); // 1 word
    if(myOffset == -1) {
        cout << "Couldn't allocate memory!!\n";
        return -1;
    }
    else {
        // allocate the memory
    }
}

int assignVar(type t, type t2) {

    /*
        1. find local addr --> ??
        2. find logical addr --> ??

        3. access page table --> extra structure
        4. locate frame --> is this the memory?
        5. access / modify
    */
}

int createArr() {

}

int freeElem(type* t) {

}