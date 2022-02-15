#include <iostream>
#include <cassert>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cassert>

#include <pthread.h>

// TODO : REMOVE THISSSSSSSS
#define double int

using namespace std;

pthread_mutexattr_t mattr; 
pthread_mutex_t *mutex_lock;

typedef struct _process_data {
    double **A;
    double **B;
    double **C;
    int veclen, i, j, r, c;
} ProcessData;

void mult(void* arg, int* cur, int* best) {
    // void * arg : ProcessData
    
    if(pthread_mutex_lock(mutex_lock)) {
        cout << "Mutex lock error!\n";
        exit(-1);
    }

    (*cur)++;
    if(int(*cur) > int(*best))
        *best = *cur;

    if(pthread_mutex_unlock(mutex_lock)) {
        cout << "Mutex unlock error\n";
        exit(-1);
    }

    ProcessData* procData = (ProcessData *)arg;
    int idx= 0;
    double ans = 0.0;
    int i = procData->i, j = procData->j, veclen = procData->veclen, r = procData->r, c = procData->c;
    
    for(idx = 0; idx < (procData->veclen); ++idx) {
        ans += (procData->A[i][idx])*(procData->B[idx][j]);
    }
    procData->C[i][j] = ans;
    return ;
}

int main(int argc, char *argv[]) {

    pthread_mutexattr_init(&mattr);
    int mlockId = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), IPC_CREAT|0666);
    mutex_lock = (pthread_mutex_t*)shmat(mlockId, NULL, 0);
    
    if(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)!=0){
    	cerr<<"Lock sharing Unsuccessful\n";
    	exit(1);
    }
    if(pthread_mutex_init(mutex_lock, &mattr)!=0){
    	cerr<<"Error : mutex_lock init() failed\n";
    	exit(1);
    }

    int r1, c1, r2, c2;
    
    // cout << "Enter no of rows and columns in the first matrix: ";
    cin >> r1 >> c1;
    
    // cout << "Enter no of rows and columns in the second matrix: ";
    cin >> r2 >> c2;
    
    try {
        assert(c1 == r2);
    } catch (exception& e) {
        cout << "Exception caught! " << e.what() << '\n';
        exit(-1);
    }



    // Matrix Data
    size_t SIZE1 = (r1 * c1) * sizeof(double);
    int shmid1 = shmget(IPC_PRIVATE, SIZE1, IPC_CREAT | 0666);
    double* sharedMem1 = (double*)shmat(shmid1, NULL, 0);

    // Array of Pointers
    size_t PSIZE1 = (r1) * sizeof(double*);
    int pshmid1 = shmget(IPC_PRIVATE, PSIZE1, IPC_CREAT | 0666);
    double** A = (double**)shmat(pshmid1, NULL, 0);

    size_t SIZE2 = (r2 * c2) * sizeof(double);
    int shmid2 = shmget(IPC_PRIVATE, SIZE2, IPC_CREAT | 0666);
    double* sharedMem2 = (double*)shmat(shmid2, NULL, 0);

    size_t PSIZE2 = (r2) * sizeof(double*);
    int pshmid2 = shmget(IPC_PRIVATE, PSIZE2, IPC_CREAT | 0666);
    double** B = (double**)shmat(pshmid2, NULL, 0);

    size_t SIZE3 = (r1 * c2) * sizeof(double);
    int shmid3 = shmget(IPC_PRIVATE, SIZE3, IPC_CREAT | 0666);
    double* sharedMem3 = (double*)shmat(shmid3, NULL, 0);

    size_t PSIZE3 = (r1) * sizeof(double*);
    int pshmid3 = shmget(IPC_PRIVATE, PSIZE3, IPC_CREAT | 0666);
    double** C = (double**)shmat(pshmid3, NULL, 0);

    for(int i = 0; i < r1; i++)
        A[i] = sharedMem1 + i * c1;

    for(int i = 0; i < r2; i++)
        B[i] = sharedMem2 + i * c2;

    for(int i = 0; i < r1; i++)
        C[i] = sharedMem3 + i * c2;

    cout << "Enter values for first matrix\n";
    for(int i = 0; i < r1; i++) {
        for(int j = 0; j < c1; j++) {
            cin >> A[i][j];
        }
    }
    
    cout << "Enter values for second matrix\n";
    for(int i = 0; i < r2; i++) {
        for(int j = 0; j < c2; j++) {
            cin >> B[i][j];
        }
    }
    

    int shmCurId = shmget(IPC_PRIVATE, sizeof(int), 0666|IPC_CREAT);
    int *cur = (int*) shmat(shmCurId, NULL, 0);

    int shmBestId = shmget(IPC_PRIVATE, sizeof(int), 0666|IPC_CREAT);
    int *best = (int*) shmat(shmBestId, NULL, 0);

    *cur = *best = 1;
    int fork_status = 1;

    for(int i = 0; i < r1 && fork_status !=0; i++) {
        for(int j = 0; j < c2 && fork_status != 0; j++) {
            ProcessData* procdata = new ProcessData();
            procdata->A = A, procdata->B = B, procdata->C = C;
            procdata->veclen = c1, procdata->i = i, procdata->j = j;
            procdata->r = r1, procdata->c = c2;
            
            fork_status = fork();
            if(fork_status < 0) {
                cout << "At " << i * c2 + j + 1 << '\n';
                cout<<"Errno "<< errno <<"\n";
                cout<<"EAGAIN "<<EAGAIN<<", ENOMEM "<<ENOMEM<<"\n"; 
                perror("Error in fork\n");
                
                fflush(stdout);
                exit(-1);
            }
            else if(fork_status == 0) {
                mult(procdata, cur, best);
            }
        }
    }

    if(fork_status != 0) {
        // only for parent
        while(wait(NULL) > 0);

        cout << "Matrix A: \n";
        for(int i = 0; i < r1; i++) {
            for(int j = 0; j < c1; j++) {
                printf("%4d ", A[i][j]);
            }
            cout << '\n';
        }
        cout << '\n';

        cout << "Matrix B: \n";
        for(int i = 0; i < r2; i++) {
            for(int j = 0; j < c2; j++) {
                printf("%4d ", B[i][j]);
            }
            cout << '\n';
        }
        cout << '\n';

        cout << "Matrix C: \n";
        for(int i = 0; i < r1; i++) {
            for(int j = 0; j < c2; j++) {
                printf("%4d ", C[i][j]);
                // int cur = 0;
                // for(int k = 0; k < c1; k++) {
                //     cur += getIndex(A, i, k, r1, c1) * getIndex(B, k, j, r2, c2);
                //     // cur += A[i][k]*B[k][j];
                // }
                // assert(cur == check);
            }
            cout << '\n';
        }

        // assert(*cur == 1);
        cout << "Currently running: " << *cur << " processes\n";

        cout << "A maximum of " << *best << " processes ran concurrently." << endl;

        pthread_mutex_destroy(mutex_lock);
        pthread_mutexattr_destroy(&mattr);
        // cout << "All tests passed!\n";
    } else {
        if(pthread_mutex_lock(mutex_lock)) {
            cout << "Error in mutex lock!\n";
            exit(-1);
        }
        (*cur)--;
        if(pthread_mutex_unlock(mutex_lock)) {
            cout << "Error in mutex unlock!\n";
            exit(-1);
        }
        exit(0);
    }

    shmdt((void *)sharedMem1);
    shmdt((void *)sharedMem2);
    shmdt((void *)sharedMem3);

    shmdt((void *)A);
    shmdt((void *)B);
    shmdt((void *)C);
    
    shmdt((void *)cur);
    shmdt((void *)best);

    shmdt((void *)mutex_lock);

    shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);
    shmctl(shmid3, IPC_RMID, NULL);
    
    shmctl(pshmid1, IPC_RMID, NULL);
    shmctl(pshmid2, IPC_RMID, NULL);
    shmctl(pshmid3, IPC_RMID, NULL);
    
    shmctl(shmCurId, IPC_RMID, NULL);
    shmctl(shmBestId, IPC_RMID, NULL);

    shmctl(mlockId, IPC_RMID, NULL);

    return 0;
}
