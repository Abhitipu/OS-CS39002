#include "memlab.h"
// Implementation file for our library

// memSeg is the whole memory
void* memSeg;
int totSize = 0;
const map<int, int> sizeInfo={
    {character,  1},
    {boolean,  1},
    {medium_integer,  3},
    {integer,  4},
};

// There will be a global stack?

// Garbage Collection
// 1. gc_initialize
// 2. gc_run (check state and free)
// 3. Mark and sweep algorithm

// . _ _ _ _ _ 
// mark --> alive
// unmark --> dead
// symtable --> alive... scope
// 1bit of 1 word (32 bit) , 1 bit for 1 byte (8 bit)
// createVar() -- index
// stack -- index (in symtab)

// symtab --> _ _ _ _ -- 
// stack --> scope _ _ O(#variables) 

// append to scope
// pop from scope

// symtabEntry -->

// hash(varName + scope) --> index --> store
// stack --> 
// Segment  _________.________i_______.__________________
// 4. Need to check this out

/*
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
*/

Object :: Object(string _name="Temp", string _scope="global", type _objType=integer, int _size=-1, int _totSize=-1) {}

std::ostream & operator<<(std::ostream &os, const Object& p)
{
    
    os<<"\n+------------+--+\n| name       |"<<p.name <<"|\n+------------+--+\n| type       |"<<p.objType <<"|\n+------------+--+\n| scope      |"<<p.scope <<"|\n+------------+--+\n| size       |"<<p.size <<"|\n+------------+--+\n| total size |"<<p.totSize <<"|\n+------------+--+\n";
    return os;
}
SymTableEntry::SymTableEntry(string _name="Temp", string _scope="global", type _objType=integer, int _size=-1, int _totSize=-1, int _wordIndex=-1, int _wordOffset=-1, bool _valid= false, bool _marked = false):
     refObj(_name, _scope, _objType, _size, _totSize), wordIndex(_wordIndex), wordOffset(_wordOffset), valid(_valid), marked(_marked){ }

inline void SymTableEntry::unmark() {
    marked = false;
}

inline void SymTableEntry::invalidate() {
    valid = false;
}

// Hash table
/*
static long long entries:: compute_hash(string const& s) {
    const int p = 31;
    const int m = 1e9 + 9;
    long long hash_value = 0;
    long long p_pow = 1;
    for (char c : s) {
        hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
        
    }
    return hash_value;
}
*/
bitset<256'000'000> entries::validMem = 0;
entries::entries() {
    cerr<<"This should be printed only once\n";
    ctr = 0;
}

    // insert
int entries :: insert(SymTableEntry st) {
    /*
    long long hashVal = compute_hash(st.refObj.name + st.refObj.scope)%mxn;
    for(int k = 0; k < mxn; k++, hashVal++) {
        if(hashVal == mxn)
            hashVal = 0;
        // find the available length
        // how much length is required
        if(!myEntries[hashVal].valid) {
            myEntries[hashVal] = st;
            return hashVal;
        }
    }
    */

    if(ctr == mxn) {
        cout<<"Symbol table full!\n";
        return -1;
    }
    myEntries[ctr] = st;
    return ctr++;
}

// Search : assignVar / getVar
// Object a, b; --- index - entries
// a = Object()
// b = getVar(a)
// assignVar(b, a)

// stack --> ?
// symtab --> stack 
// -1 --> start
// indices >= 0
// -1 ...

int entries :: search(string name, string scope) {
    /*
    long long hashVal = compute_hash(name + scope)%mxn;
    for(int k = 0; k < mxn; k++, hashVal++) {
        if(hashVal == mxn)
            hashVal = 0;
        
        if(!myEntries[hashVal].valid)
            continue;

        if(myEntries[hashVal].refObj.name == name && myEntries[hashVal].refObj.scope == scope)
            return hashVal;
    }
    cout<<"Symbol not found!\n";
    return -1;
    */
   return -1;
}
entries mySymbolTable;


// entries --
// createVar -- push into stack

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

// Creates memory segment for memSize bytes
int createMem(size_t memSize) {
    memSeg = malloc(memSize);
    if(!memSeg) {
        printf("Malloc failed!!\n");
        return -1;
    }
    memset(memSeg, '\0', memSize);
    totSize = memSize;
    // const handled
    // sizeInfo[(int)character] = 1;
    // sizeInfo[(int)boolean] = 1;
    // sizeInfo[(int)medium_integer] = 3;
    // sizeInfo[(int)integer] = 4;

    // symbolTables.push()
    //  TODO: Spawn the garbage collector
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
        cout<<entries :: validMem[curIdx]<<" ? = true\n";
        cout<<"allocating word "<<curIdx<<'\n';
    }

    return idx;
}

size_t getSize(type t, int freq = 1) {
    return sizeInfo.at((int)t) * freq;
}

Object createVar(type t, string name, string scope) {
    int myIndex = getBestFit(getSize(t)); // 1 word
    if(myIndex == -1) {
        cout << "Couldn't allocate memory!!\n";
        return Object();
    }
    else {
        // allocate the memory
        SymTableEntry curEntry(name, scope, t, getSize(t), getSize(t), myIndex, 0, true, true);
        mySymbolTable.insert(curEntry);
        varStack.push(myIndex);
        return curEntry.refObj;
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
   return -1;
}

int createArr() {
    return -1;
}

int freeElem(type* t) {
    return -1;
}