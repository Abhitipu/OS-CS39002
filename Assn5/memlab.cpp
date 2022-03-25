#include "memlab.h"
// Implementation file for our library
entries mySymbolTable;
pthread_t threadId; // AutomaticGC thread

// memSeg is the whole memory
char* memSeg;
int totSize = 0;
const map<int, int> sizeInfo={
    {character,  1},
    {boolean,  1},
    {medium_integer,  3},
    {integer,  4},
};

// There will be a global stack?


Object :: Object(type _objType=integer, int _size=-1, int _totSize=-1):objType(_objType),\
    size(_size), totSize(_totSize) { }

std::ostream & operator<<(std::ostream &os, const Object& p)
{
    os<<"\n+------------+--+\n| type        |"\
    <<p.objType <<"|\n+------------+--+\n| size      |"\
    <<p.size <<"|\n+------------+--+\n| total size |"\
    <<p.totSize <<"|\n+------------+--+\n";
    int symTabIndex = p.symTabIdx;
    SymTableEntry& curEntry = mySymbolTable.myEntries[symTabIndex];
    char* memAddr = memSeg + curEntry.wordIndex * 4 + curEntry.wordOffset;
    os<<"Value : ";
    switch (p.objType)
    {
    case medium_integer:
    case integer:
        {
            int *x = (int *)memAddr;
            os<<*x;
            // if its an array
            if(p.totSize != p.size)
            {
                int totalElements = p.totSize / p.size;
                for(int i=1; i<totalElements; ++i)
                {   
                    x += 1;
                    os<<", "<<*x;
                }
            }
        }
        break;
    case boolean:
        {
            bool *b = (bool*)memAddr;
            os<<*b;
            // if its an array
            if(p.totSize != p.size)
            {
                int totalElements = p.totSize / p.size;
                for(int i=1; i<totalElements; ++i)
                {   
                    b += 1;
                    os<<", "<<*b;
                }
            }
        }
        break;
    case character:
        {
            char *ch = memAddr;
            os<<*ch;
            // if its an array
            if(p.totSize != p.size)
            {
                int totalElements = p.totSize / p.size;
                for(int i=1; i<totalElements; ++i)
                {   
                    ch += 1;
                    os<<", "<<*ch;
                }
            }
        }
        break;
    default:
        {
            os<<"Print still not implemented!";
        }
        break;
    }
    
    os<<'\n';    
    return os;
}

SymTableEntry::SymTableEntry(type _objType=integer, int _size=-1, int _totSize=-1, int _wordIndex=-1, \
    int _wordOffset=-1, bool _valid= false, bool _marked = false):
    refObj(_objType, _size, _totSize), wordIndex(_wordIndex), \
    wordOffset(_wordOffset), valid(_valid), marked(_marked){ }

inline void SymTableEntry::unmark() {
    marked = false;
}

inline void SymTableEntry::invalidate() {
    valid = false;
}

// Hash table
bitset<256'000'000> entries::validMem = 0;
entries::entries() {
    cerr<<"This should be printed only once\n";
    ctr = 0;
}

// insert
int entries :: insert(SymTableEntry st) {
    if(ctr == mxn) {
        cout<<"Symbol table full!\n";
        return -1;
    }
    myEntries[ctr] = st;
    return ctr++;
}

Stack :: Stack():top(-1) {
        
}

void Stack::push(int index) {
    if(top == 1000)
        printf("Stack overflow!");
    else
        indices[++top] = index;
}

int Stack::pop() {
    if(top < 0)
        printf("Stack Underflow!\n");
    else
        top--;
    return top;
}

int Stack::peek() {
    if(top < 0)
        printf("Stack Underflow!\n");
    else
        return indices[top];
    return -1;
}
Stack varStack;

void gc_run(bool scopeEnd = false) {

    // (marked and valid at the time of insertion)

    // stack se pop
    // symtable --> unmark
    // sweep --> symtable --> if unmarked and valid --> free up memory

    // compact --> reverse pointers --> symboltables --> 

    // _________       ________
    // cccc c___ cccc c___ ____
    // ____ ____ cccc c___ ____
    // I want 10 char array
    // c___ ____ c___ ____
    // next 5 char array
    // firstFit --> compaction
    // largest hole available - 15
    // for each entry in symbol table, find new first fit(), then copy
    // largest hole available - 25
    // 1. Mark
    // 2. Sweep
    // 3. Compact
}

void* gc_routine(void* args) {
    // Garbage Collection
    // 1. gc_initialize
    // 2. gc_run (check state and free)
    // 3. Mark and sweep algorithm
    
    return NULL;
}

// Creates memory segment for memSize bytes
int createMem(size_t memSize) {
    memSeg = (char*)malloc(memSize);
    if(!memSeg) {
        printf("Malloc failed!!\n");
        return -1;
    }
    memset(memSeg, '\0', memSize);
    totSize = memSize;

    //  TODO: Spawn the garbage collector
    pthread_create(&threadId, NULL, gc_routine, NULL);
    return memSize;
}

int getBestFit(int reqdSize){
    // reqdSize -- bytes
    cout<<"reqd Size "<<reqdSize<<" bytes converted to ";
    reqdSize = (reqdSize + 3) / 4; // round up for word alignment
    cout<<reqdSize<<" words (for word alignment)\n";
    int best = INT_MAX;
    int idx = -1;
    int cur = 0;
    int i;
    for(i=0; i< totSize; i+=4) {
        assert((i>>2) <(int) entries :: validMem.size());
        cout<<entries :: validMem[i>>2]<<" ";
        if(entries :: validMem[i>>2]) {
            if(cur > reqdSize)
                if(cur < best) {
                    best = cur, idx = (i>>2) - cur;
                }
            cur = 0;
        } else {
            // invalid --> entend
            cur++;
        }
    }
    cout<<i<<'\n';
    if(cur!=0)
    {
        if(cur > reqdSize)
            if(cur < best) {
                best = cur, idx = (i>>2) - cur;
            }
        cur = 0;
    }
    cout<<"Best Segment found "<<best<<" at index "<<idx<<'\n';
    if(idx == -1)
        return -1;
    
    for(int done = 0, curIdx = idx; done < reqdSize; done++, curIdx++) {
        assert(!(entries :: validMem[curIdx]));
        entries :: validMem[curIdx]=true;
        // cout<<entries :: validMem[curIdx]<<" ? = true\n";
        cout<<"allocating word "<<curIdx<<'\n';
    }

    return idx;
}

size_t getSize(type t, int freq) {
    return sizeInfo.at((int)t) * freq;
}

Object createVar(type t) {
    int myIndex = getBestFit(getSize(t, 1)); // 1 word
    if(myIndex == -1) {
        cout << "Couldn't allocate memory!!\n";
        return Object();
    }
    else {
        // allocate the memory
        SymTableEntry curEntry(t, getSize(t, 1), getSize(t, 1), myIndex, 0, true, true);
        int symTabIdx = mySymbolTable.insert(curEntry);
        curEntry.refObj.symTabIdx = symTabIdx;
        varStack.push(symTabIdx);
        return curEntry.refObj;
    }
}

// 
int assignVar(Object o, int x) {
    // O -> location
    int symTabIdx = o.symTabIdx; // -- in symtable

    // TODO: mutex
    SymTableEntry& curEntry = mySymbolTable.myEntries[symTabIdx];
    char* memAddr = memSeg + curEntry.wordIndex * 4 + curEntry.wordOffset;
    memcpy(memAddr, (char*)(&x), getSize(o.objType, 1));
    
    return 1;
}

int assignVar(Object dest, Object src) {
    if(dest.objType >= src.objType) {
        int symTabIdx = src.symTabIdx;
        SymTableEntry &curEntry = mySymbolTable.myEntries[symTabIdx];
        char* memAddr = memSeg + curEntry.wordIndex * 4 + curEntry.wordOffset;
        int x;
        memcpy((char*)(&x), memAddr, getSize(src.objType, 1));
        return assignVar(dest, x);
    } else {
        cout << "Cannot assign objects of incompatible types\n";
        return -1;
    }
}

Object createArr(type t, int length) {    
    int myIndex = getBestFit(getSize(t, length)); // start index
    if(myIndex == -1) {
        cout << "Couldn't allocate memory!!\n";
        return Object();
    }
    else {
        // allocate the memory
        SymTableEntry curEntry(t, getSize(t, 1), getSize(t, length), myIndex, 0, true, true);
        int symTabIdx = mySymbolTable.insert(curEntry);
        curEntry.refObj.symTabIdx = symTabIdx;
        varStack.push(symTabIdx);
        return curEntry.refObj;
    }
}


int assignArr(Object dest, int destIdx, int x) {
    if(destIdx < 0 || destIdx >= dest.totSize / dest.size) {
        cout << "Out of array limits!\n";
        return -1;
    }
    int symTabIdx = dest.symTabIdx; // -- in symtable

    // TODO: mutex
    SymTableEntry& curEntry = mySymbolTable.myEntries[symTabIdx];
    // TODO: word alignment
    char* memAddr = memSeg + curEntry.wordIndex * 4 + getSize(dest.objType, destIdx);
    memcpy(memAddr, (char*)(&x), getSize(dest.objType, 1));
    
    return 1;
}

int assignArr(Object dest, int destIdx, Object src, int srcIdx) {

    if(dest.objType >= src.objType) {
        if(destIdx < 0 || srcIdx < 0 || destIdx >= dest.totSize / dest.size || srcIdx >= src.totSize / src.size) {
            cout << "Out of array limits!\n";
            return -1;
        }
        int symTabIdx = src.symTabIdx; // -- in symtable

        // TODO: mutex
        SymTableEntry& curEntry = mySymbolTable.myEntries[symTabIdx];
        // TODO: word alignment
        char* memAddr = memSeg + curEntry.wordIndex * 4 + getSize(src.objType, srcIdx);
        int temp;
        memcpy((char*)(&temp), memAddr, getSize(src.objType, 1));
        return assignArr(dest, destIdx, temp);
    } else {
        cout << "Cannot assign objects of incompatible types\n";
        return -1;
    }
}

int freeElem(Object toDel) {
    return -1;
}