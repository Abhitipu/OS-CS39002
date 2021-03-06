#include "memlab.h"
// Implementation file for our library
entries *mySymbolTable;
pthread_t threadId; // AutomaticGC thread
Stack *varStack;         // To track objects for Mark and Sweep algorithm
pthread_mutex_t compactLock;

// int symTabIndices[mxn][2]; declared below

void copyWordWise(char *firstWord, char *secondWord, int offset, int nBytes, int data);
int copyWordWiseGet(char *firstWord, char *secondWord, int offset, int nBytes);

void printHformat(int nbytes)
{
    if(nbytes < 1<<10)
    {
        cout<<nbytes<<"B";
    }
    else if(nbytes < 1<<20)
    {
        cout<<(nbytes>>10)<<"KB";
    }
    else if(nbytes < 1<<30)
    {
        cout<<(nbytes>>20)<<"MB";
    }
    else
    {
        cout<<(nbytes>>30)<<"GB";
    }
}

// memSeg is the whole memory
char* memSeg;
int totSize = 0;
const int sizeInfo[] = {1, 1, 3, 4};

/*
 * The constructor of the object class
 */
Object :: Object(type _objType=integer, int _size=-1, int _totSize=-1):objType(_objType),\
    size(_size), totSize(_totSize), symTabIdx(-1) { }

std::ostream & operator<<(std::ostream &os, const Object& p)
{
    os  <<"+------------+----------+\n"\
        <<"| type       |"<<setw(10)<<p.objType <<"|\n"\
        <<"+------------+----------+\n"\
        <<"| size       |"<<setw(10)<<p.size <<"|\n"\
        <<"+------------+----------+\n"\
        <<"| total size |"<<setw(10)<<p.totSize <<"|\n"\
        <<"+------------+----------+\n"\
        <<"| SymtabIndex|"<<setw(10)<<p.symTabIdx <<"|\n"\
        <<"+------------+----------+\n";
    int symTabIndex = p.symTabIdx;
    SymTableEntry& curEntry = mySymbolTable->myEntries[symTabIndex];
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
entries::entries(int _maxWords, int _mxn):mxn(_mxn), validMem((_maxWords+31)>>5), listOfFreeIndices(_mxn) {
    cerr<<"This should be printed only once\n";
    myEntries = new SymTableEntry[mxn];
    cout<<"[Mem Allocated Symbols Array] : ";
    printHformat(sizeof(SymTableEntry[mxn]));
    cout<<'\n';

    pthread_mutex_init(&(validMemlock), NULL);
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

Stack :: Stack(int _mxn):mxn(_mxn), top(-1) {
    indices = new int[mxn];
    cout<<"[Mem Allocated Stack] : ";
    printHformat(sizeof(int[mxn]));
    cout<<"\n";
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

_validMem :: _validMem(int _sizeOfMem):ptr(0), maxptr(0), sizeOfmem(_sizeOfMem){
    sizeAvl = min(totSize>>2, maxWords); // Minimum of requested memory, maxWords bound
    mem = new uint32_t[sizeOfmem];
    cout<<"Size of mem "<<sizeOfmem<<"\n";
    cout<<"[Mem Allocated ValidMem] : ";
    printHformat(sizeof(uint32_t[sizeOfmem]));
    cout<<"\n";
    memset(mem, 0, sizeof(uint32_t[sizeOfmem]));
}

int _validMem :: getIndex(int x) {
    int check = x >> 5;
    if(check >= sizeOfmem) {
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

/*
 * Here the garbage collector pops the elements(from stack) which are out of scope
 * and unmarks them on the symbol table.
 */
int mark() {
    cerr<<"Mark started\n";
    int cur;
    int rem = 0;
    do {
        cur = varStack->pop();
        cerr<<"Cur : "<<cur<<"\n";
        if(cur != START_SCOPE) {
            ++rem;
            pthread_mutex_lock(&mySymbolTable->myEntries[cur].lock);
            SymTableEntry& curEntry = mySymbolTable->myEntries[cur];
            cerr<<"sym: "<<curEntry.refObj.symTabIdx << '\n';
            curEntry.unmark();
            pthread_mutex_unlock(&mySymbolTable->myEntries[cur].lock);
            cerr<<"SymTabIndex "<<cur<<" Unmarked\n";
        }
    } while(cur != START_SCOPE);
    cerr<<"Mark ended\n";
    return rem;
}

/*
 * Here the garbage collector calls freeElem on all the dead symbol table entries.
 */
void sweep() {
    cerr<<"Sweep started\n";
    for(int i = 0; i< mySymbolTable->mxn; ++i)
    {
        pthread_mutex_lock(&mySymbolTable->myEntries[i].lock);
        SymTableEntry &curEntry = mySymbolTable->myEntries[i];
        if(!curEntry.marked && curEntry.valid)
        {
            cerr<<"Calling free elem on "<<i<<" index\n";
            freeElem(curEntry.refObj, true);
        }
        pthread_mutex_unlock(&mySymbolTable->myEntries[i].lock);
    }
    cerr<<"Sweep ended\n";
    return;
}

/*
 * Comparator function for sorting indices
 */
int comp(const void* p1, const void* p2) {
    int* arr1 = (int*)p1;
    int* arr2 = (int*)p2;
    int diff1 = arr1[0] - arr2[0];
    if (diff1) return diff1;
    return arr1[1] - arr2[1];
}

/*
 * Temporary storage for sorting
 */
int symTabIndices[mxn][2];

/*
 * Compacts the memory by filling up the holes via shifting the memory blocks
 */
void compact() {
    // 100% compactions
    // traverse through symbol table
    // get the reverse links for memory
    // do compaction
    // reassign in symbol table
    pthread_mutex_lock(&compactLock);
    cerr<<"Compacting\n";
    pthread_mutex_lock(&mySymbolTable->validMemlock);
    cout<<"Memavl before compaction : "<<mySymbolTable->validMem.sizeAvl<<"\n";
    pthread_mutex_unlock(&mySymbolTable->validMemlock);
    
    cerr << "Looking up the memory\n";
    int cur = 0;
    for(int i=0; i<mySymbolTable->mxn; ++i) {
        SymTableEntry &curEntry= mySymbolTable->myEntries[i];
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

        pthread_mutex_lock(&mySymbolTable->myEntries[symTabIndices[i][1]].lock);
        int rightptr = symTabIndices[i][0];
        SymTableEntry &curEntry = mySymbolTable->myEntries[symTabIndices[i][1]];
        if(leftptr != curEntry.wordIndex)
        {
            pthread_mutex_lock(&mySymbolTable->validMemlock);
            char *memleft = memSeg + leftptr*4;
            char *memright = memSeg + rightptr*4;
            int sizeInBytes = ((curEntry.refObj.totSize+3)>>2)<<2; // Rounded up to nearest multiple of 4 
            memcpy(memleft, memright, sizeInBytes);
            for(int j=0;j<sizeInBytes/4; ++j)
            {
                mySymbolTable->validMem.set(leftptr + j);
                mySymbolTable->validMem.reset(rightptr + j);
            }
            curEntry.wordIndex = leftptr;
            pthread_mutex_unlock(&mySymbolTable->validMemlock);
        }
        leftptr += (curEntry.refObj.totSize+3)>>2;
        pthread_mutex_unlock(&mySymbolTable->myEntries[symTabIndices[i][1]].lock);

    }
    pthread_mutex_lock(&mySymbolTable->validMemlock);
    mySymbolTable->validMem.ptr = leftptr;
    mySymbolTable->validMem.sizeAvl = mySymbolTable->validMem.totSizeAvl;
    cout<<"Final Memavl after compaction : "<<mySymbolTable->validMem.sizeAvl<<"\n";
    pthread_mutex_unlock(&mySymbolTable->validMemlock);
    pthread_mutex_unlock(&compactLock);
    
    cerr << "All done!\n";
    return;
}
// 
void gcRun(bool scopeEnd, bool toCompact) {
    // return;
    // Garbage Collection
    // 1. gcInitialize
    // 2. gcRun (check state and free)
    // 3. Mark and sweep algorithm
    timespec startTime, endTime;
    clock_gettime(CLOCK_REALTIME, &startTime);
    cerr << "Garbage collector at work!\n";
    // (marked and valid at the time of insertion)
    if(scopeEnd)
        mark();

    sweep(); // -- freeElem

    // compact --> reverse pointers --> symboltables --> 
    if(toCompact)
        compact();
        
    cerr << "Garbage collected!\n";
    clock_gettime(CLOCK_REALTIME, &endTime);
    cout<<"GC exec time: "<<((endTime.tv_sec- startTime.tv_sec)*((int)1e9) + (endTime.tv_nsec- startTime.tv_nsec))/(1000000.0)<<" milli seconds\n";
    return;
}


void gcInitialize() {
    varStack->push(START_SCOPE);
}

/*
 * Periodically calls gcRun
 */
void* gcRoutine(void* args) {
    // gcInitialize();
    int cnt=0;
    while(true) {
        sleep(10);
        // compaction is done once in 5 times
        gcRun(false, cnt==0);
        cnt = (cnt + 1)%5;
    }
    
    return NULL;
}

// Creates memory segment for memSize bytes
int createMem(size_t memSize) {
    if(memSize % 4 )
    {
        cout<<"Extending memSize for word alignment\n";
    }
    memSize= ((memSize+31)/32)*32;
    memSeg = (char*)malloc(memSize);

    if(!memSeg) {
        cout << "Malloc failed!!\n";
        return -1;
    }
    cout<<"Successfully allocated ";
    printHformat(memSize);
    cout<<"\n";
    cout << "Created memory segment\n";
    memset(memSeg, '\0', memSize);
    totSize = memSize;
    if((totSize>>2) > maxWords)
    {
        cout<<"We're reducing your mem space from "<<(totSize>>22)<<"MWords to "<<maxWords<<"MWords\n";
    }
    int AllowedMaxWords = min(totSize>>2, maxWords);
    cout<<"Max allowed words "<<AllowedMaxWords<<"\n";
    if((totSize>>2) > mxn)
    {
        cout<<"We're reducing your Symbol table capacity from "<<(totSize>>22)<<"MWords to "<<(mxn>>20)<<"MWords\n";
    } 
    int maxSymbols = min((totSize>>2), mxn);
    cout<<"Max symobols "<<maxSymbols<<"\n";
    cout<<"Allocating memory for mySymbolTable\n";
    mySymbolTable = new entries(AllowedMaxWords, maxSymbols);
    mySymbolTable->validMem.totSizeAvl = AllowedMaxWords;
    mySymbolTable->validMem.sizeAvl = AllowedMaxWords;
    
    cout<<"Allocating memory for varStack\n";
    varStack = new Stack(maxSymbols);
    gcInitialize();
    // How maxWords impact mxn and other maxWords

    // init Compact lock
    pthread_mutex_init(&compactLock, NULL);

    // Garbage collector
    pthread_create(&threadId, NULL, gcRoutine, NULL);
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
    pthread_mutex_lock(&mySymbolTable->validMemlock);
    for(i=0; i< totSize; i+=4) {
        // assert((i>>2) <(int) mySymbolTable->validMem.size());
        // cerr<< mySymbolTable->validMem.isSet(i>>2)<<" ";
        if(mySymbolTable->validMem.isSet(i>>2) ){
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
        pthread_mutex_unlock(&mySymbolTable->validMemlock);
        return -1;
    }
    cerr << "allocating word " << idx << ": " << idx + reqdSize - 1 << '\n';
    for(int done = 0, curIdx = idx; done < reqdSize; done++, curIdx++) {
        assert(!(mySymbolTable->validMem.isSet(curIdx)));
        mySymbolTable->validMem.set(curIdx);
        // cerr<<entries :: validMem[curIdx]<<" ? = true\n";
        // cerr<<"allocating word "<<curIdx<<'\n';
    }
    pthread_mutex_unlock(&mySymbolTable->validMemlock);
    return idx;
}
*/

/*
 * Gets the next fit in memory i.e. essentially the one pointed by ptr.
 * First a check is done on the total available memory, if not sufficient, we throw an error.
 * If it doesnt fit, a compaction is performed first and then the object is stored.
 */
int getNextFit(int reqdSize){
    // reqdSize -- bytes
    // start ptr -- free mem
    // length available

    cerr<<"reqd Size "<<reqdSize<<" bytes converted to ";
    reqdSize = (reqdSize + 3) / 4; // round up for word alignment
    cerr<<reqdSize<<" words (for word alignment)\n";
    pthread_mutex_lock(&compactLock);

    pthread_mutex_lock(&mySymbolTable->validMemlock);
    if(reqdSize > mySymbolTable->validMem.totSizeAvl) {
        cerr << "Insufficient memory!!\n";
        pthread_mutex_unlock(&mySymbolTable->validMemlock);
        exit(-1);
    } else if(reqdSize > mySymbolTable->validMem.sizeAvl) {
        cerr<<"Running compaction to accomodate "<<reqdSize<<" words\n";
        pthread_mutex_unlock(&mySymbolTable->validMemlock);
        gcRun(false, true);
    }
    else{
        pthread_mutex_unlock(&mySymbolTable->validMemlock);
    }
    pthread_mutex_lock(&mySymbolTable->validMemlock);
    int idx = mySymbolTable->validMem.ptr;
    cerr<<"Free Segment found "<<mySymbolTable->validMem.sizeAvl<<" at index "<<idx<<'\n';
    for(int done = 0, curIdx = idx; done < reqdSize; done++, curIdx++) {
        assert(!(mySymbolTable->validMem.isSet(curIdx)));
        mySymbolTable->validMem.set(curIdx);
    }
    mySymbolTable->validMem.sizeAvl -= reqdSize;
    mySymbolTable->validMem.totSizeAvl -= reqdSize;
    mySymbolTable->validMem.ptr += reqdSize;
    mySymbolTable->validMem.maxptr = max(mySymbolTable->validMem.maxptr, mySymbolTable->validMem.ptr);
    pthread_mutex_unlock(&mySymbolTable->validMemlock);
    
    pthread_mutex_unlock(&compactLock);
    return idx;
}

size_t getSize(type t, int freq) {
    return sizeInfo[(int)t] * freq;
}

Object createVar(type t) {
    int myIndex = getNextFit(getSize(t, 1)); // 1 word
    if(myIndex == -1) {
        cerr << "Couldn't allocate memory!!\n";
        return Object();
    }
    else {
        // allocate the memory
        SymTableEntry curEntry(t, getSize(t, 1), getSize(t, 1), myIndex, 0, true, true);
        int symTabIdx = mySymbolTable->insert(curEntry);
        curEntry.refObj.symTabIdx = symTabIdx;
        varStack->push(symTabIdx);
        cerr << "Created a variable\n" << curEntry.refObj;
        return curEntry.refObj;
    }
}

// 
int assignVar(Object o, int x) {
    // O -> location
    int symTabIdx = o.symTabIdx; // -- in symtable

    SymTableEntry& curEntry = mySymbolTable->myEntries[symTabIdx];
    pthread_mutex_lock(&curEntry.lock);
    char *firstword = memSeg + curEntry.wordIndex * 4 ;
    char *secondword = memSeg + curEntry.wordIndex * 4 + 4;
    cerr << "Assigned to\n" << o << '\n';
    copyWordWise(firstword, secondword, curEntry.wordOffset, getSize(o.objType, 1), x);
    pthread_mutex_unlock(&curEntry.lock);
    return 1;
}

void getVar(Object o, void *dest) {
    // O -> location
    int symTabIdx = o.symTabIdx; // -- in symtable

    SymTableEntry& curEntry = mySymbolTable->myEntries[symTabIdx];
    pthread_mutex_lock(&curEntry.lock);
    // char* memAddr = memSeg + curEntry.wordIndex * 4 + curEntry.wordOffset;
    char *firstword = memSeg + curEntry.wordIndex * 4;
    char *secondword = memSeg + curEntry.wordIndex * 4+ 4;
    int temp = copyWordWiseGet(firstword, secondword, curEntry.wordOffset, getSize(o.objType, 1));
    memcpy((char*)dest, &temp, getSize(o.objType, 1));
    pthread_mutex_unlock(&curEntry.lock);
    return ;
}


int assignVar(Object dest, Object src) {
    if(dest.objType >= src.objType) {
        int symTabIdx = src.symTabIdx;
        SymTableEntry &curEntry = mySymbolTable->myEntries[symTabIdx];
        pthread_mutex_lock(&curEntry.lock);
        char *firstword = memSeg + curEntry.wordIndex * 4;
        char *secondword = memSeg + curEntry.wordIndex * 4+ 4;
        int x = copyWordWiseGet(firstword, secondword, curEntry.wordOffset, getSize(src.objType, 1));
        // cerr << "Assigned " << src << "to " << dest << '\n';
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
    int myIndex = getNextFit(getSize(t, length)); // start index
    if(myIndex == -1) {
        cerr << "Couldn't allocate memory!!\n";
        return Object();
    }
    else {
        // allocate the memory
        SymTableEntry curEntry(t, getSize(t, 1), getSize(t, length), myIndex, 0, true, true);
        int symTabIdx = mySymbolTable->insert(curEntry);
        curEntry.refObj.symTabIdx = symTabIdx;
        cerr << "Created array\n" << curEntry.refObj << '\n';
        varStack->push(symTabIdx);
        return curEntry.refObj;
    }
}

void copyWordWise(char *firstWord, char *secondWord, int offset, int nBytes, int data)
{
    int lenToCopyFirstWord = min(4-offset, nBytes);
    cerr<<"[WORD ALIGN] Copying "<<lenToCopyFirstWord<<" bytes to offset "<<offset<<" of first word\n";
    // first copy to first word
    memcpy(firstWord+offset, (char *)&data, lenToCopyFirstWord);

    if(nBytes- lenToCopyFirstWord > 0)
    {
        cerr<<"[WORD ALIGN] Copying next "<<nBytes- lenToCopyFirstWord<<" bytes "<<offset<<" to the second word\n";
        memcpy(secondWord, ((char *)&data) + lenToCopyFirstWord, nBytes - lenToCopyFirstWord);
    }
    return;
}
int copyWordWiseGet(char *firstWord, char *secondWord, int offset, int nBytes)
{
    int data = 0;
    int lenToCopyFirstWord = min(4-offset, nBytes);
    cerr<<"[WORD ALIGN] Copying "<<lenToCopyFirstWord<<" bytes from offset "<<offset<<" of first word\n";
    // first copy to first word
    memcpy((char *)&data, firstWord+offset, lenToCopyFirstWord);

    if(nBytes- lenToCopyFirstWord > 0)
    {
        cerr<<"[WORD ALIGN] Copying next"<<nBytes- lenToCopyFirstWord<<" bytes "<<offset<<" from the second word\n";
        memcpy(((char *)&data) + lenToCopyFirstWord, secondWord, nBytes - lenToCopyFirstWord);
    }
    return data;
}
int assignArr(Object dest, int destIdx, int x) {
    if(destIdx < 0 || destIdx >= dest.totSize / dest.size) {
        cerr << "Out of array limits!\n";
        return -1;
    }
    int symTabIdx = dest.symTabIdx; // -- in symtable

    SymTableEntry& curEntry = mySymbolTable->myEntries[symTabIdx];
    pthread_mutex_lock(&curEntry.lock);
    // char* memAddr = memSeg + curEntry.wordIndex * 4 + getSize(dest.objType, destIdx);
    // memcpy(memAddr, (char*)(&x), getSize(dest.objType, 1));

    char *firstword = memSeg + curEntry.wordIndex * 4 + (getSize(dest.objType, destIdx)/4)*4;
    char *secondword = memSeg + curEntry.wordIndex * 4 + (getSize(dest.objType, destIdx)/4)*4 + 4;
    copyWordWise(firstword, secondword, getSize(dest.objType, destIdx)%4, getSize(dest.objType, 1), x);
    cerr << "Assigned to array at index " << destIdx << '\n';
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

        SymTableEntry& curEntry = mySymbolTable->myEntries[symTabIdx];
        pthread_mutex_lock(&curEntry.lock);
        char *firstword = memSeg + curEntry.wordIndex * 4 + (getSize(src.objType, srcIdx)/4)*4;
        char *secondword = memSeg + curEntry.wordIndex * 4 + (getSize(src.objType, srcIdx)/4)*4 + 4;
        int temp = copyWordWiseGet(firstword, secondword, getSize(src.objType, srcIdx)%4, getSize(src.objType, 1));
        cerr << "Assigning to array from " << srcIdx << '\n'; 
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

    SymTableEntry& curEntry = mySymbolTable->myEntries[symTabIdx];
    pthread_mutex_lock(&curEntry.lock);
    // char* memAddr = memSeg + curEntry.wordIndex * 4 + getSize(src.objType, srcIdx);
    // memcpy((char*)mem, memAddr, getSize(src.objType, 1));

    char *firstword = memSeg + curEntry.wordIndex * 4 + (getSize(src.objType, srcIdx)/4)*4;
    char *secondword = memSeg + curEntry.wordIndex * 4 + (getSize(src.objType, srcIdx)/4)*4 + 4;
    int temp = copyWordWiseGet(firstword, secondword, getSize(src.objType, srcIdx)%4, getSize(src.objType, 1));
    memcpy((char*)mem, &temp, getSize(src.objType, 1));
    cerr << "Getting value from array at index " << srcIdx << '\n';
    pthread_mutex_unlock(&curEntry.lock);

    return;
}

int freeElem(Object toDel, bool locked) {
    if(toDel.symTabIdx == -1)
        return -1;

    SymTableEntry &curEntry = mySymbolTable->myEntries[toDel.symTabIdx];
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
    pthread_mutex_lock(&mySymbolTable->validMemlock);
    cerr << "deallocating word " << curEntry.wordIndex << ": " << curEntry.wordIndex + tofree - 1 << endl;
    for(int done = 0, curIdx = curEntry.wordIndex; done < tofree; done++, curIdx++) {
        // cerr<<"Done "<<done<<'\n';
        assert(mySymbolTable->validMem.isSet(curIdx));
        mySymbolTable->validMem.reset(curIdx);
        // cerr<<entries :: validMem[curIdx]<<" ? = true\n";
        // cerr<<"deallocating word "<<curIdx<<'\n';
    }
    mySymbolTable->validMem.totSizeAvl += tofree;
    pthread_mutex_unlock(&mySymbolTable->validMemlock);
    if(!locked)
        pthread_mutex_unlock(&(curEntry.lock));
    mySymbolTable->listOfFreeIndices.push(toDel.symTabIdx);
    return 1;
}

void graph_data(){
    ofstream fout;
    fout.open("output.txt", ios::out |ios::app);
    
    cout<<"Total size: ";
    printHformat(totSize);
    cout<<"\n";
    cout<<"Total memory: ";
    pthread_mutex_lock(&mySymbolTable->validMemlock);
    printHformat(mySymbolTable->validMem.sizeOfmem*32*4);// each element of valid mem takes 32 bits, 1 bit represent 1 word, 1 word - 4 bytes
    cout<<"\n";
    cout<<"Avl mem: ";
    printHformat(mySymbolTable->validMem.totSizeAvl*4);
    cout<<'\n';
    cout<<"Used Mem: ";
    printHformat(mySymbolTable->validMem.sizeOfmem*32*4 - mySymbolTable->validMem.totSizeAvl*4);
    cout<<"\n";
    cout<<"ptr: ";
    printHformat(mySymbolTable->validMem.ptr*4);
    fout<<mySymbolTable->validMem.maxptr*4<<" "<<mySymbolTable->validMem.sizeOfmem*32*4 - mySymbolTable->validMem.totSizeAvl*4<<'\n';
    cout<<"\n";

    cout<<"Max ptr: "<<mySymbolTable->validMem.maxptr*4<<"\n";
    pthread_mutex_unlock(&mySymbolTable->validMemlock);

}