#include "memlab.h"
// Implementation file for our library
entries mySymbolTable;
pthread_t threadId; // AutomaticGC thread
Stack varStack;         // To track objects for Mark and Sweep algorithm

// memSeg is the whole memory
char* memSeg;
int totSize = 0;
const map<int, int> sizeInfo={
    {character,  1},
    {boolean,  1},
    {medium_integer,  3},
    {integer,  4},
};

// There will be a global stack? YES!


Object :: Object(type _objType=integer, int _size=-1, int _totSize=-1):objType(_objType),\
    size(_size), totSize(_totSize), symTabIdx(-1) { }

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
    wordOffset(_wordOffset), valid(_valid), marked(_marked){ 
        
    pthread_mutex_init(&lock, NULL);
}

SymTableEntry::~SymTableEntry() {
    pthread_mutex_destroy(&lock);
}

inline void SymTableEntry::unmark() {
    marked = false;
}

inline void SymTableEntry::invalidate() {
    valid = false;
}

// Hash table
entries::entries() {
    cerr<<"This should be printed only once\n";
    pthread_mutex_init(&(mySymbolTable.validMemlock), NULL);
    cerr<<"Initialized\n";
    validMem = 0;
    for(int i = 0; i < mxn; i++)  {
        listOfFreeIndices.push(i);
    }
    cerr << "Out of constructor\n";
}

// insert
int entries :: insert(SymTableEntry st) {
    if(listOfFreeIndices.isEmpty()) {
        cout<<"Symbol table full!\n";
        return -1;
    }
    int index = listOfFreeIndices.pop();
    myEntries[index] = st;
    
    return index;
}

Stack :: Stack():top(-1) {
    pthread_mutex_init(&lock, NULL);     
}

Stack :: ~Stack() {
    pthread_mutex_destroy(&lock);
}

bool Stack::isEmpty() {
    pthread_mutex_lock(&lock);
    if(top > 0 ){
        pthread_mutex_unlock(&lock);
        return false;
    }
    else{
        pthread_mutex_unlock(&lock);
        return true;
    }
}

void Stack::push(int index) {
    pthread_mutex_lock(&lock);

    if(top >= mxn)
        printf("Stack overflow!");
    else
        indices[++top] = index;
    
    pthread_mutex_unlock(&lock);
}

int Stack::pop() {
    pthread_mutex_lock(&lock);
    if(top < 0){
        printf("Stack Underflow!\n");
        pthread_mutex_unlock(&lock);
        return -1;
    } else {
        pthread_mutex_unlock(&lock);
        return indices[top--];
    }
}

int Stack::peek() {
    pthread_mutex_lock(&lock);
    if(top < 0) {
        pthread_mutex_unlock(&lock);
        printf("Stack Underflow!\n");
        return -1;
    }
    else {
        pthread_mutex_unlock(&lock);
        return indices[top];
    }
}


int mark() {
    int cur;
    int rem = 0;
    do {
        cur = varStack.pop();
        if(cur != START_SCOPE) {
            ++rem;
            pthread_mutex_lock(&mySymbolTable.myEntries[cur].lock);
            SymTableEntry& curEntry = mySymbolTable.myEntries[cur];
            curEntry.unmark();
            pthread_mutex_unlock(&mySymbolTable.myEntries[cur].lock);
        }
    } while(cur != START_SCOPE);

    return rem;
}

void sweep() {
    for(int i = 0; i< mxn; ++i)
    {
        pthread_mutex_lock(&mySymbolTable.myEntries[i].lock);
        SymTableEntry &curEntry = mySymbolTable.myEntries[i];
        if(!curEntry.marked)
        {
            freeElem(curEntry.refObj, true);
        }
        pthread_mutex_unlock(&mySymbolTable.myEntries[i].lock);
    }
    return;
}

void compact() {
    // 100% compactions
    // traverse through symbol table
    // get the reverse links for memory
    // do compaction
    // reassign in symbol table
    
    array<int,2> symTabIndices[mxn];

    int cur = 0;
    for(SymTableEntry &curEntry: mySymbolTable.myEntries) {
        pthread_mutex_lock(&(curEntry.lock));
        if(curEntry.valid) {
            symTabIndices[cur++] = {curEntry.wordIndex, curEntry.refObj.symTabIdx}; 
        }
        pthread_mutex_lock(&(curEntry.lock));
    }

    sort(symTabIndices, symTabIndices + cur, [](array<int,2> a, array<int,2> b){
        if(a[0]==b[0])
            return a[1]<b[1];
        return a[0] < b[0];
    });

    int leftptr = 0;
    
    for(int i =0;i< cur; i++)
    {
        int rightptr = symTabIndices[i][0];

        pthread_mutex_lock(&mySymbolTable.myEntries[symTabIndices[i][1]].lock);
        SymTableEntry &curEntry = mySymbolTable.myEntries[symTabIndices[i][1]];
        if(leftptr != curEntry.wordIndex)
        {
            char *memleft = memSeg + leftptr*4;
            char *memright = memSeg + rightptr*4;
            int sizeInBytes = ((curEntry.refObj.totSize+3)>>2)<<2; // Rounded up to nearest multiple of 4 
            memcpy(memleft, memright, sizeInBytes);
            curEntry.wordIndex = leftptr;
        }
        leftptr += (curEntry.refObj.totSize+3)>>2;
        pthread_mutex_unlock(&mySymbolTable.myEntries[symTabIndices[i][1]].lock);

    }
    return;
}

void gc_run(bool scopeEnd, bool toCompact) {

    // (marked and valid at the time of insertion)
    if(scopeEnd)
        mark();

    sweep(); // -- freeElem

    // compact --> reverse pointers --> symboltables --> 
    if(toCompact)
        compact();
}


void gc_initialize() {
    varStack.push(START_SCOPE);
}

void* gc_routine(void* args) {
    // gc_initialize();
    // int cnt=0;
    while(true) {
        /*
        sleep(2);
        gc_run(false, cnt==0);
        cnt = (cnt + 1)%5;
        */
    }
    // Garbage Collection
    // 1. gc_initialize
    // 2. gc_run (check state and free)
    // 3. Mark and sweep algorithm
    
    return NULL;
}

// Creates memory segment for memSize bytes
int createMem(size_t memSize) {
    memSeg = (char*)malloc(memSize);
    gc_initialize();
    // varStack.push(START_SCOPE);
    if(!memSeg) {
        printf("Malloc failed!!\n");
        return -1;
    }
    memset(memSeg, '\0', memSize);
    totSize = memSize;

    //  TODO: Spawn the garbage collector
    // pthread_create(&threadId, NULL, gc_routine, NULL);
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
    pthread_mutex_lock(&mySymbolTable.validMemlock);
    for(i=0; i< totSize; i+=4) {
        assert((i>>2) <(int) mySymbolTable.validMem.size());
        cout<< mySymbolTable.validMem[i>>2]<<" ";
        if(mySymbolTable.validMem[i>>2] ){
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
    if(idx == -1) {
        pthread_mutex_unlock(&mySymbolTable.validMemlock);
        return -1;
    }
    
    for(int done = 0, curIdx = idx; done < reqdSize; done++, curIdx++) {
        assert(!(mySymbolTable.validMem[curIdx]));
        mySymbolTable.validMem[curIdx]=true;
        // cout<<entries :: validMem[curIdx]<<" ? = true\n";
        cout<<"allocating word "<<curIdx<<'\n';
    }
    pthread_mutex_unlock(&mySymbolTable.validMemlock);
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
    pthread_mutex_lock(&curEntry.lock);
    char* memAddr = memSeg + curEntry.wordIndex * 4 + curEntry.wordOffset;
    memcpy(memAddr, (char*)(&x), getSize(o.objType, 1));
    pthread_mutex_unlock(&curEntry.lock);
    return 1;
}

int assignVar(Object dest, Object src) {
    if(dest.objType >= src.objType) {
        int symTabIdx = src.symTabIdx;
        SymTableEntry &curEntry = mySymbolTable.myEntries[symTabIdx];
        pthread_mutex_lock(&curEntry.lock);
        char* memAddr = memSeg + curEntry.wordIndex * 4 + curEntry.wordOffset;
        int x;
        memcpy((char*)(&x), memAddr, getSize(src.objType, 1));
        pthread_mutex_unlock(&curEntry.lock);
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
    pthread_mutex_lock(&curEntry.lock);
    char* memAddr = memSeg + curEntry.wordIndex * 4 + getSize(dest.objType, destIdx);
    memcpy(memAddr, (char*)(&x), getSize(dest.objType, 1));
    pthread_mutex_unlock(&curEntry.lock);
    
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
        pthread_mutex_lock(&curEntry.lock);
        char* memAddr = memSeg + curEntry.wordIndex * 4 + getSize(src.objType, srcIdx);
        int temp;
        memcpy((char*)(&temp), memAddr, getSize(src.objType, 1));
        pthread_mutex_unlock(&curEntry.lock);
        return assignArr(dest, destIdx, temp);
    } else {
        cout << "Cannot assign objects of incompatible types\n";
        return -1;
    }
}

int freeElem(Object toDel, bool locked) {
    if(toDel.symTabIdx == -1)
        return -1;

    SymTableEntry &curEntry = mySymbolTable.myEntries[toDel.symTabIdx];
    if(!locked)
        pthread_mutex_lock(&(curEntry.lock));
    // if already freed, skip
    if(!curEntry.valid) {
        if(!locked)
            pthread_mutex_unlock(&curEntry.lock);
        return -1;
    }
    
    curEntry.invalidate();

    // validMem --> flip
    int tofree = (toDel.totSize + 3) / 4; // round up for word alignment
    pthread_mutex_lock(&mySymbolTable.validMemlock);
    for(int done = 0, curIdx = curEntry.wordIndex; done < tofree; done++, curIdx++) {
        cout<<"Done "<<done<<'\n';
        assert(mySymbolTable.validMem[curIdx]);
        mySymbolTable.validMem[curIdx]=false;
        // cout<<entries :: validMem[curIdx]<<" ? = true\n";
        cout<<"deallocating word "<<curIdx<<'\n';
    }
    pthread_mutex_unlock(&mySymbolTable.validMemlock);
    if(!locked)
        pthread_mutex_unlock(&(curEntry.lock));
    mySymbolTable.listOfFreeIndices.push(toDel.symTabIdx);
    return 1;
}