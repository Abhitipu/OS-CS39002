#include "memlab.h"
// Implementation file for our library
entries mySymbolTable;
pthread_t threadId; // AutomaticGC thread
Stack varStack;         // To track objects for Mark and Sweep algorithm

// memSeg is the whole memory
char* memSeg;
int totSize = 0;
const int sizeInfo[] = {1, 1, 3, 4};

Object :: Object(type _objType=integer, int _size=-1, int _totSize=-1):objType(_objType),\
    size(_size), totSize(_totSize), symTabIdx(-1) { }

std::ostream & operator<<(std::ostream &os, const Object& p)
{
    os  <<"+------------+-----+\n"\
        <<"| type       |"<<setw(5)<<p.objType <<"|\n"\
        <<"+------------+-----+\n"\
        <<"| size       |"<<setw(5)<<p.size <<"|\n"\
        <<"+------------+-----+\n"\
        <<"| total size |"<<setw(5)<<p.totSize <<"|\n"\
        <<"+------------+-----+\n"\
        <<"| SymtabIndex|"<<setw(5)<<p.symTabIdx <<"|\n"\
        <<"+------------+-----+\n";
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
entries::entries():validMem() {
    cerr<<"This should be printed only once\n";
    pthread_mutex_init(&(mySymbolTable.validMemlock), NULL);
    cerr<<"Initialized\n";
    for(int i = 0; i < mxn; i++)  {
        listOfFreeIndices.push(i);
    }
    cerr << "Out of constructor\n";
}

// insert
int entries :: insert(SymTableEntry st) {
    if(listOfFreeIndices.isEmpty()) {
        cerr<<"Symbol table full!\n";
        return -1;
    }
    int index = listOfFreeIndices.pop();
    myEntries[index] = st;
    myEntries[index].refObj.symTabIdx = index;
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

_validMem :: _validMem():ptr(0){
    sizeAvl = min(totSize>>2, maxWords); // Minimum of requested memory, maxWords bound
    memset(mem, 0, sizeof(mem));
}

int _validMem :: getIndex(int x) {
    int check = x >> 5;
    if(check >= 8'000'000) {
        cerr << "Out of memory bounds (_valid Mem)" << '\n';
        exit(-1);
    }
    return check;
}

int _validMem :: getOffset(int x) {
    return ((x >> 5) << 5) ^ x;
}

int _validMem :: isSet(int x) {
    return (mem[getIndex(x)] >> getOffset(x))&1;
}

void _validMem :: set(int x) {
    mem[getIndex(x)] |= (1 << getOffset(x));
}

void _validMem :: reset(int x){
    if(isSet(x))
        mem[getIndex(x)] ^= (1 << getOffset(x));
}

int mark() {
    cerr<<"Mark started\n";
    int cur;
    int rem = 0;
    do {
        cur = varStack.pop();
        cerr<<"Cur : "<<cur<<"\n";
        if(cur != START_SCOPE) {
            ++rem;
            pthread_mutex_lock(&mySymbolTable.myEntries[cur].lock);
            SymTableEntry& curEntry = mySymbolTable.myEntries[cur];
            cerr<<"sym: "<<curEntry.refObj.symTabIdx << '\n';
            curEntry.unmark();
            pthread_mutex_unlock(&mySymbolTable.myEntries[cur].lock);
            cerr<<"SymTabIndex "<<cur<<" Unmarked\n";
        }
    } while(cur != START_SCOPE);
    cerr<<"Mark ended\n";
    return rem;
}

void sweep() {
    cerr<<"Sweep started\n";
    for(int i = 0; i< mxn; ++i)
    {
        pthread_mutex_lock(&mySymbolTable.myEntries[i].lock);
        SymTableEntry &curEntry = mySymbolTable.myEntries[i];
        if(!curEntry.marked && curEntry.valid)
        {
            cerr<<"Calling free elem on "<<i<<" index\n";
            freeElem(curEntry.refObj, true);
        }
        pthread_mutex_unlock(&mySymbolTable.myEntries[i].lock);
    }
    cerr<<"Sweep ended\n";
    return;
}

int comp(const void* p1, const void* p2) {
    int* arr1 = (int*)p1;
    int* arr2 = (int*)p2;
    int diff1 = arr1[0] - arr2[0];
    if (diff1) return diff1;
    return arr1[1] - arr2[1];
}

void compact() {
    // 100% compactions
    // traverse through symbol table
    // get the reverse links for memory
    // do compaction
    // reassign in symbol table
    cerr<<"Compacting\n";
    int symTabIndices[mxn][2];

    cerr << "Looking up the memory\n";
    int cur = 0;
    for(SymTableEntry &curEntry: mySymbolTable.myEntries) {
        pthread_mutex_lock(&(curEntry.lock));
        if(curEntry.valid) {
            symTabIndices[cur][0] = curEntry.wordIndex;
            symTabIndices[cur++][1] = curEntry.refObj.symTabIdx;
        }
        pthread_mutex_unlock(&(curEntry.lock));
    }
    
    cerr<<"Sorting started\n"; 
    // sort(symTabIndices, symTabIndices + cur);
    qsort(symTabIndices, cur, sizeof(symTabIndices[0]), comp);
    cerr<<"Sorting Ended\n";
    int leftptr = 0;
    
    cerr << "Two pointer magic\n";
    for(int i =0;i< cur; i++)
    {

        pthread_mutex_lock(&mySymbolTable.myEntries[symTabIndices[i][1]].lock);
        int rightptr = symTabIndices[i][0];
        SymTableEntry &curEntry = mySymbolTable.myEntries[symTabIndices[i][1]];
        if(leftptr != curEntry.wordIndex)
        {
            pthread_mutex_lock(&mySymbolTable.validMemlock);
            char *memleft = memSeg + leftptr*4;
            char *memright = memSeg + rightptr*4;
            int sizeInBytes = ((curEntry.refObj.totSize+3)>>2)<<2; // Rounded up to nearest multiple of 4 
            memcpy(memleft, memright, sizeInBytes);
            for(int j=0;j<sizeInBytes/4; ++j)
            {
                mySymbolTable.validMem.set(leftptr + j);
                mySymbolTable.validMem.reset(rightptr + j);
            }
            curEntry.wordIndex = leftptr;
            pthread_mutex_unlock(&mySymbolTable.validMemlock);
        }
        leftptr += (curEntry.refObj.totSize+3)>>2;
        pthread_mutex_unlock(&mySymbolTable.myEntries[symTabIndices[i][1]].lock);

    }
    pthread_mutex_lock(&mySymbolTable.validMemlock);
    mySymbolTable.validMem.ptr = leftptr;
    mySymbolTable.validMem.sizeAvl = mySymbolTable.validMem.totSizeAvl;
    pthread_mutex_unlock(&mySymbolTable.validMemlock);
    
    cerr << "All done!\n";
    return;
}

void gc_run(bool scopeEnd, bool toCompact) {

    // Garbage Collection
    // 1. gc_initialize
    // 2. gc_run (check state and free)
    // 3. Mark and sweep algorithm

    cerr << "Garbage collector at work!\n";
    // (marked and valid at the time of insertion)
    if(scopeEnd)
        mark();

    sweep(); // -- freeElem

    // compact --> reverse pointers --> symboltables --> 
    if(toCompact)
        compact();
        
    cerr << "Garbage collected!\n";
}


void gc_initialize() {
    varStack.push(START_SCOPE);
}

void* gc_routine(void* args) {
    // gc_initialize();
    int cnt=0;
    while(true) {
        sleep(2);
        // compaction is done once in 5 times
        gc_run(false, cnt==0);
        cnt = (cnt + 1)%5;
    }
    
    return NULL;
}

// Creates memory segment for memSize bytes
int createMem(size_t memSize) {
    memSeg = (char*)malloc(memSize);
    gc_initialize();

    if(!memSeg) {
        cout << "Malloc failed!!\n";
        return -1;
    }
    
    cerr << "Created memory segment\n";
    memset(memSeg, '\0', memSize);
    totSize = memSize;
    if((totSize>>2) > maxWords)
    {
        cout<<"We're reducing your mem space from "<<(totSize>>2)<<" to "<<maxWords<<"\n";
    }
    mySymbolTable.validMem.totSizeAvl = min(totSize>>2, maxWords);
    mySymbolTable.validMem.sizeAvl = min(totSize>>2, maxWords);

    // Garbage collector
    pthread_create(&threadId, NULL, gc_routine, NULL);
    return memSize;
}

/*
int getBestFit(int reqdSize){
    // reqdSize -- bytes
    cerr<<"reqd Size "<<reqdSize<<" bytes converted to ";
    reqdSize = (reqdSize + 3) / 4; // round up for word alignment
    cerr<<reqdSize<<" words (for word alignment)\n";
    int best = INT_MAX;
    int idx = -1;
    int cur = 0;
    int i;
    pthread_mutex_lock(&mySymbolTable.validMemlock);
    for(i=0; i< totSize; i+=4) {
        // assert((i>>2) <(int) mySymbolTable.validMem.size());
        // cerr<< mySymbolTable.validMem.isSet(i>>2)<<" ";
        if(mySymbolTable.validMem.isSet(i>>2) ){
            if(cur >= reqdSize)
                if(cur < best) {
                    best = cur, idx = (i>>2) - cur;
                }
            cur = 0;
        } else {
            // invalid --> entend
            cur++;
        }
    }
    cerr<<i<<'\n';
    if(cur!=0)
    {
        if(cur >= reqdSize)
            if(cur < best) {
                best = cur, idx = (i>>2) - cur;
            }
        cur = 0;
    }
    cerr<<"Best Segment found "<<best<<" at index "<<idx<<'\n';
    if(idx == -1) {
        pthread_mutex_unlock(&mySymbolTable.validMemlock);
        return -1;
    }
    cerr << "allocating word " << idx << ": " << idx + reqdSize - 1 << '\n';
    for(int done = 0, curIdx = idx; done < reqdSize; done++, curIdx++) {
        assert(!(mySymbolTable.validMem.isSet(curIdx)));
        mySymbolTable.validMem.set(curIdx);
        // cerr<<entries :: validMem[curIdx]<<" ? = true\n";
        // cerr<<"allocating word "<<curIdx<<'\n';
    }
    pthread_mutex_unlock(&mySymbolTable.validMemlock);
    return idx;
}
*/

int getFirstFit(int reqdSize){
    // reqdSize -- bytes
    // start ptr -- free mem
    // length available

    // _ _ 11_ 11111 ptr_ _ _ _ _ _ _ _ _
    // compact
    // 1111111ptr______________________
    cerr<<"reqd Size "<<reqdSize<<" bytes converted to ";
    reqdSize = (reqdSize + 3) / 4; // round up for word alignment
    cerr<<reqdSize<<" words (for word alignment)\n";

    pthread_mutex_lock(&mySymbolTable.validMemlock);
    if(reqdSize > mySymbolTable.validMem.totSizeAvl) {
        cout << "Insufficient memory!!\n";
        pthread_mutex_unlock(&mySymbolTable.validMemlock);
        exit(-1);
    } else if(reqdSize > mySymbolTable.validMem.sizeAvl) {
        cout<<"Running compaction to accomodate "<<reqdSize<<" words\n";
        pthread_mutex_unlock(&mySymbolTable.validMemlock);
        gc_run(false, true);
    }
    else{
        pthread_mutex_unlock(&mySymbolTable.validMemlock);
    }
    pthread_mutex_lock(&mySymbolTable.validMemlock);
    int idx = mySymbolTable.validMem.ptr;
    cerr<<"Free Segment found "<<mySymbolTable.validMem.sizeAvl<<" at index "<<idx<<'\n';
    for(int done = 0, curIdx = idx; done < reqdSize; done++, curIdx++) {
        assert(!(mySymbolTable.validMem.isSet(curIdx)));
        mySymbolTable.validMem.set(curIdx);
    }
    mySymbolTable.validMem.sizeAvl -= reqdSize;
    mySymbolTable.validMem.totSizeAvl -= reqdSize;
    mySymbolTable.validMem.ptr += reqdSize;
    pthread_mutex_unlock(&mySymbolTable.validMemlock);
    
    return idx;
}

size_t getSize(type t, int freq) {
    return sizeInfo[(int)t] * freq;
}

Object createVar(type t) {
    int myIndex = getFirstFit(getSize(t, 1)); // 1 word
    if(myIndex == -1) {
        cerr << "Couldn't allocate memory!!\n";
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

void getVar(Object o, void *dest) {
    // O -> location
    int symTabIdx = o.symTabIdx; // -- in symtable

    // TODO: mutex
    SymTableEntry& curEntry = mySymbolTable.myEntries[symTabIdx];
    pthread_mutex_lock(&curEntry.lock);
    char* memAddr = memSeg + curEntry.wordIndex * 4 + curEntry.wordOffset;
    memcpy(dest, memAddr, getSize(o.objType, 1));
    pthread_mutex_unlock(&curEntry.lock);
    return ;
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
        cerr << "Cannot assign objects of incompatible types\n";
        return -1;
    }
}

int assignVar(Object dest, Object src, int srcIdx) {
    return assignArr(dest, 0, src, srcIdx);
}

int assignArr(Object dest, int destIdx, Object src) {
    return assignArr(dest, destIdx, src, 0);
}


Object createArr(type t, int length) {    
    int myIndex = getFirstFit(getSize(t, length)); // start index
    if(myIndex == -1) {
        cerr << "Couldn't allocate memory!!\n";
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
        cerr << "Out of array limits!\n";
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
            cerr << "Out of array limits!\n";
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
        cerr << "Cannot assign objects of incompatible types\n";
        return -1;
    }
}

void getArr(Object src, int srcIdx, void* mem) {

    if(srcIdx < 0 || srcIdx >= src.totSize / src.size) {
        cerr << "Out of array limits!\n";
        return;
    }
    int symTabIdx = src.symTabIdx; // -- in symtable

    // TODO: mutex
    SymTableEntry& curEntry = mySymbolTable.myEntries[symTabIdx];
    // TODO: word alignment
    pthread_mutex_lock(&curEntry.lock);
    char* memAddr = memSeg + curEntry.wordIndex * 4 + getSize(src.objType, srcIdx);
    memcpy((char*)mem, memAddr, getSize(src.objType, 1));
    pthread_mutex_unlock(&curEntry.lock);

    return;
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

    cerr<<"Free elem called\n";
    
    curEntry.invalidate();

    // validMem --> flip
    int tofree = (toDel.totSize + 3) / 4; // round up for word alignment
    cerr<<"Going to free "<<tofree<<" word(s)\n";
    pthread_mutex_lock(&mySymbolTable.validMemlock);
    cerr << "deallocating word " << curEntry.wordIndex << ": " << curEntry.wordIndex + tofree - 1 << endl;
    for(int done = 0, curIdx = curEntry.wordIndex; done < tofree; done++, curIdx++) {
        // cerr<<"Done "<<done<<'\n';
        assert(mySymbolTable.validMem.isSet(curIdx));
        mySymbolTable.validMem.reset(curIdx);
        // cerr<<entries :: validMem[curIdx]<<" ? = true\n";
        // cerr<<"deallocating word "<<curIdx<<'\n';
    }
    mySymbolTable.validMem.totSizeAvl += tofree;
    pthread_mutex_unlock(&mySymbolTable.validMemlock);
    if(!locked)
        pthread_mutex_unlock(&(curEntry.lock));
    mySymbolTable.listOfFreeIndices.push(toDel.symTabIdx);
    return 1;
}