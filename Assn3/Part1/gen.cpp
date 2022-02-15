#include <iostream>

using namespace std;

const int mxn = 5;

void printMatrix(int rows, int cols) {
    srand(time(NULL));
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            cout << rand()%mxn << " ";
        }
        cout << '\n';
    }
}

int main() {
    int r1, c1;
    int c2;

    cerr << "Enter r1 and c1: ";
    cin >> r1 >> c1;

    cout << r1 << " " << c1 << '\n';

    cerr << "Enter c2: ";
    cin >> c2;
    cout << c1 << " " << c2 << '\n';

    printMatrix(r1, c1);
    printMatrix(c1, c2);

    cerr << "All done!\n";

    return 0;
}




