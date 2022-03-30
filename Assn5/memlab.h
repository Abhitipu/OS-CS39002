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

/*
 * An enum representing all the available types
 */
typedef enum _type {
    boolean,
    character,
    medium_integer,
    integer
} type;

/*
 * The generic variable in our program.
 *
 * It has the following attributes:
 * 1. objType: Indicates the type of the object.
 * 2. size: Indicates the size of an individual element for an array or total size for a single variable.
 * 3. totSize: Total size occupied by the variable in memory (after padding).
 * 4. symTabIdx: The position of the object in the symbol table.
 *
 * It has the following methods:
 * 1. ostream& operator<<: Used for printing the object.
 * 2. Constructor: Used for creating an instance of class Object.
 */
class Object {
private:
    friend std::ostream & operator<<(std::ostream &os, const Object& o);
public:
    type objType;
    int size, totSize, symTabIdx;
    Object(type _objType, int _size, int _totSize);
};

/*
 * One single entry in our symbol table.
 *
 * It has the following attributes
 * 1. lock: Used for implementing locking behaviour.
 * 2. wordIndex: Index of the variable in the memory created using createMem.
 * 3. wordOffSet: Offset of the variable in the memory creates using createMem.
 * 4. valid: Indicates whether it points to a valid memory location.
 * 5. marked: Indicates whether an entry in the sym table is still alive.
 * 
 * It has the following methods:
 * 1. Constructor: Used to create an entry in the symtable.
 * 2. Destructor: Used to destroy an entry in the symtable.
 * 3. unmark: Used to mark a variable as dead (done by garbage collector)
 * 4. invalidate: Used indicate that memory is invalidated.
 */
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

/*
 * An implementation of stack.
 *
 * It has the following attributes:
 * 1. indices[mxn]: An array that mimics the stack.
 * 2. top: Variable that points to the top of the stack.
 * 3. lock: Ensures locking behaviour.
 *
 * It has the following methods:
 * 1. Constructor: Used to create an instance of the stack.
 * 2. Destructor: Used to destroy an instance of the stack.
 * 3. push: Pushes an element into the stack.
 * 4. pop: Pops an element from the stack.
 * 5. peek: Returns the top element of the stack without popping.
 */
class Stack {
public:
    int mxn;
    int *indices;
    int top;
    pthread_mutex_t lock;
    Stack(int _mxn = ::mxn); 
    ~Stack();
    void push(int index);
    int pop();
    int peek();
    bool isEmpty();
};

const int maxWords = 1 << 28;  // approx 268e6
// 1gb --> 1 << 30 bytes
// 1gb --> 1 << 28 words
// 1gb --> 1 << 23 ints (each has 1 << 5)

/*
 * An implementation of bitset to indicate which word is valid.
 *
 * The attributes are:
 * ptr: Pointer to the current head of the physical memory.
 * sizeAvl: Size available if we allocate memory starting from ptr
 * totSizeAvl: Total size available in the memory.
 * mem[maxWords >> 5]: The actual bitset indicating if a word is valid.
 *
 * The methods are:
 * 1. Constructor/destructor: The ususal meaning
 * 2. getIndex: returns the index in the mem array for a given word.
 * 3. getOffset: returns the offset in the mem array for a given index.
 * 4. isSet: returns if a word in memory is valid.
 * 5. set: Sets the corresponding bit in the memory thus validating a word in memory.
 * 6. reset: Resets the corresponding bit in the memory thus invalidating the word in memory.
 */
class _validMem {
public:
    // ptr to a free location (int the last free block)
    int ptr;
    // size (in words) available in free mem starting from ptr
    int sizeAvl;
    // 1GB mem -> 256 Mil words -> for each word we need 1 bit, 1 int has 32 bits, thus we need 256Mil/32 = 8 Mil int  
    int totSizeAvl; // max word
    int sizeOfmem;
    uint32_t *mem;
    _validMem(int _sizeOfMem = (maxWords >> 5));
    int getIndex(int x);
    int getOffset(int x);
    int isSet(int x);
    void set(int x);
    void reset(int x);
};

/*
 * The symbol table data structure
 *
 * The attributes are:
 * 1. validMem: A bitset that stores if a word in memory is valid or not
 * 2. myEntries[mxn]: An array of symbol Table elements.
 * 3. listOfFreeIndices: A stack containing the free indices in the symbol table.
 * 4. validMemlock: Implementing locking behavious while locking.
 *
 * The methods are:
 * 1. Constructor/destructor: Default behaviour.
 * 2. insert: Inserts an entry into the symbol table.
 */
class entries {
public:
    int mxn;
    _validMem validMem; // overhead 32MiB, True if word is in use, false if it is not in use
    SymTableEntry *myEntries;
    Stack listOfFreeIndices;
    pthread_mutex_t validMemlock; // for valid Memory
    entries(int _maxWords = maxWords, int _mxn = ::mxn);
    int insert(SymTableEntry st);
};

/*
 * Helper function to create the memory segment of memSize bytes
 */
int createMem(size_t memSize);

// for variables
/*
 * Helper function to create a variable of a given type
 */
Object createVar(type t);
/*
 * Overloaded helper functions to assign one variable into another.
 */
int assignVar(Object dest, Object src);
int assignVar(Object, int);
int assignVar(Object dest, Object src, int srcIdx);
/*
 * Helper function to get the value of a particular variable.
 */
void getVar(Object, void*);

// for arrays
/*
 * Helper function to create an array of a given type and length
 */
Object createArr(type t, int length);

/*
 * Overloaded helper functions to assign one array element into another.
 */
int assignArr(Object dest, int srcIdx, int x);
int assignArr(Object dest, int destIdx, Object src);
int assignArr(Object dest, int destIdx, Object src, int srcIdx);
/*
 * Helper function to get the value of a particular array element
 */
void getArr(Object, int, void*);

/*
 * Helper function to free the elements in memory
 */
int freeElem(Object toDel, bool locked = false);

/*
 * Helper function to return the size of a variable (may be an array)
 */
size_t getSize(type t, int freq = 1);

/*
 * Helper function to initialize the garbage collector to indicate the beginning of a scope.
 */
void gc_initialize(); // Write this line at the start of every new functions
/*
 * Helper function to run the garbage collector
 */
void gc_run(bool scopeEnd = false, bool toCompact = false);
#endif // __MEMLAB_H
