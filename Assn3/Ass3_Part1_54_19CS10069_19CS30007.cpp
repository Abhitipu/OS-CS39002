#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <cassert>

#define double int

using namespace std;

typedef struct _process_data {
    double *A;
    double *B;
    double *C;
    int veclen, i, j, r, c;
} ProcessData;

double getIndex(double *A, int i, int j, int r, int c);
void setIndex(double *A, int i, int j, int r, int c, double val);

void mult(void* arg) {
    // void * arg : ProcessData
    ProcessData* procData = (ProcessData *)arg;
    int idx= 0;
    double ans = 0.0;
    int i = procData->i, j = procData->j, veclen = procData->veclen, r = procData->r, c = procData->c;
    
    for(idx = 0; idx < (procData->veclen); ++idx)
    {
        // need rc
        ans += (getIndex(procData->A, i, idx, r, veclen) * getIndex(procData->B, idx, j, veclen, c));
    }
    
    setIndex(procData->C, i, j, r, c, ans);
    return ;
}

double getIndex(double *A, int i, int j, int r, int c) {
    assert(i < r && i >= 0 && j < c && j >= 0);
    return A[i*c + j];
}

void setIndex(double *A, int i, int j, int r, int c, double val) {
    assert(i < r && i >= 0 && j < c && j >= 0);
    A[i*c + j] = val;
    return;
}

int main(int argc, char *argv[]) {
    int r1, c1, r2, c2;
    
    cout << "Enter no of rows and columns in the first matrix: ";
    cin >> r1 >> c1;
    
    cout << "Enter no of rows and columns in the second matrix: ";
    cin >> r2 >> c2;
    
    assert(c1 = r2);
    /* the size (in bytes) of shared memory object */
    const int SIZE1 = r1 * c1 * sizeof(double);
    const int SIZE2 = r2 * c2 * sizeof(double);
    const int SIZE3 = r1 * c2 * sizeof(double);

    /* Name of the shared memory */
    const char* matrix1 = "A";
    const char* matrix2 = "B";
    const char* matrix3 = "C";

    /* shared memory file descriptor */
    int shm_fd1, shm_fd2, shm_fd3;

    /* pointer to shared memory obect */
    double *A, *B, *C;

    /* create the shared memory object */
    shm_fd1 = shm_open(matrix1, O_CREAT | O_RDWR, 0666);
    shm_fd2 = shm_open(matrix2, O_CREAT | O_RDWR, 0666);
    shm_fd3 = shm_open(matrix3, O_CREAT | O_RDWR, 0666);

    /* configure the size of the shared memory object */
    ftruncate(shm_fd1, SIZE1);
    ftruncate(shm_fd2, SIZE2);
    ftruncate(shm_fd3, SIZE3);

    /* memory map the shared memory object */
    A = (double *)mmap(0, r1 * c1 * sizeof(double), PROT_WRITE, MAP_SHARED, shm_fd1, 0);
    B = (double *)mmap(0, r2 * c2 * sizeof(double), PROT_WRITE, MAP_SHARED, shm_fd2, 0);
    C = (double *)mmap(0, r1 * c2 * sizeof(double), PROT_WRITE, MAP_SHARED, shm_fd3, 0);
    
    cout << "Enter values for first matrix\n";
    for(int i = 0; i < r1; i++) {
        for(int j = 0; j < c1; j++) {
            double val; cin >> val;
            // double val = rand()%2 + 1;
            setIndex(A, i, j, r1, c1, val);
        }
    }
    
    cout << "Enter values for second matrix\n";
    for(int i = 0; i < r2; i++) {
        for(int j = 0; j < c2; j++) {
            double val; cin >> val;
            // double val = rand()%2 + 1;
            setIndex(B, i, j, r2, c2, val);
        }
    }
    
    
    int fork_status = 1;

    for(int i = 0; i < r1 && fork_status !=0; i++) {
        for(int j = 0; j < c2 && fork_status != 0; j++) {
            ProcessData* procdata = new ProcessData();
            procdata->A = A, procdata->B = B, procdata->C = C;
            procdata->veclen = c1, procdata->i = i, procdata->j = j;
            procdata->r = r1, procdata->c = c2;
            
            fork_status = fork();
            if(fork_status == 0) {
                mult(procdata);
            }
        }
    }

    if(fork_status != 0) {
        // only for parent
        while(wait(NULL) > 0);

        cout << "Matrix A: \n";
        for(int i = 0; i < r1; i++) {
            for(int j = 0; j < c1; j++) {
                cout << getIndex(A, i, j, r1, c1) << " ";
            }
            cout << '\n';
        }
        cout << '\n';

        cout << "Matrix B: \n";
        for(int i = 0; i < r2; i++) {
            for(int j = 0; j < c2; j++) {
                cout << getIndex(B, i, j, r2, c2) << " ";
            }
            cout << '\n';
        }
        cout << '\n';

        cout << "Matrix C: \n";
        for(int i = 0; i < r1; i++) {
            for(int j = 0; j < c2; j++) {
                int check = getIndex(C, i, j, r1, c2);
                printf("%5d ", check);
                // int cur = 0;
                // for(int k = 0; k < c1; k++) {
                //     cur += getIndex(A, i, k, r1, c1) * getIndex(B, k, j, r2, c2);
                //     // cur += A[i][k]*B[k][j];
                // }
                // assert(cur == check);
            }
            cout << '\n';
        }

        // cout << "All tests passed!\n";
    }

    shm_unlink(matrix1);
    shm_unlink(matrix2);
    shm_unlink(matrix3);
    return 0;
}

/*
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
int main()
{
//     /* the size (in bytes) of shared memory object */
//     const int SIZE = 4096;
//     /* create the shared memory object */
//     shm fd = shm open(name, O CREAT | O RDRW, 0666);
//     /* strings written to shared memory */
//     const char *message 0 = "Hello";
//     const char *message 1 = "World!";
//     /* shared memory file descriptor */
//     int shm fd;
//     /* pointer to shared memory obect */
//     void *ptr;
    
//     /* configure the size of the shared memory object */
//     ftruncate(shm fd, SIZE);
//     /* memory map the shared memory object */
//     ptr = mmap(0, SIZE, PROT WRITE, MAP SHARED, shm fd, 0);
//     /* write to the shared memory object */
//     sprintf(ptr,"%s",message 0);
//     ptr += strlen(message 0);
//     sprintf(ptr,"%s",message 1);
//     ptr += strlen(message 1);
//     return 0;
// }
