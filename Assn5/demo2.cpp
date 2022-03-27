#include "memlab.h"

using namespace std;

int fib(int k) {
    gc_initialize();
    Object prod = createVar(integer);
    assignVar(prod, 1);
    Object secondLast = createVar(integer);
    assignVar(secondLast, 1);
    Object last = createVar(integer);
    assignVar(last, 1);

    Object temp = createArr(integer, k);
    
    assignArr(temp, 0, secondLast);
    assignArr(temp, 1, last);
    for(int i = 0; i < k - 2; i++) {
        int secondLastImg, lastImg, newFib, prodImg;
        getVar(secondLast, &secondLastImg);
        getVar(last, &lastImg);
        getVar(prod, &prodImg);
        
        newFib = secondLastImg + lastImg;
        
        assignVar(prod, prodImg*newFib);
        
        assignArr(temp, i + 2, newFib);
        assignVar(secondLast, newFib);
        assignVar(last, secondLastImg);
    }

    int ans;
    getVar(prod, &ans);
    cout<<temp;
    gc_run(true, true);
    return ans;
}

int main() {
    int k;
    cout << "Enter k to compute the product of first k fibonacci numbers: ";
    cin >> k;
    createMem(100);
    // Object o = createVar(integer);

    cout << "The product is: " << fib(k) << '\n';

    gc_run(true);
    return 0;
}