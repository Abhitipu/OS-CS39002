#include<bits/stdc++.h>

using namespace std;

int main() {
    int arr[] ={1,5,6,8,7};
    for(int a: arr)
        cout << a << " ";
    cout << '\n';
    cout << sizeof(arr) / sizeof(int) << '\n';
}