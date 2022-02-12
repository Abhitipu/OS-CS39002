#include<stdio.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/wait.h>
#include<sys/types.h>
#include <bitset>
#include <iostream>
#include <set>

#define shmsize 100
#define shmmode (SHM_R|SHM_W)
#define shmkey (key_t)31

using namespace std;

const int SIZE = 8;
const int N = 1000;
const int resultantMatrixProducer = -10;
set<int> matrixId;

int getrandId() {
    int ans = -1;
    do {
        ans = rand()%100000 + 1;
    } while(matrixId.count(ans));
    matrixId.insert(ans);
    return ans;
}


struct job {
    int producerNumber;
    bitset<8> status;
    int matrix[N][N];   // -9 <= matrix[i][j] <= 9
    int matrixId;       // rand(1...1e5)
    int resultIdx;      // where the resultant matrix is stored

    job(int _producerNumber = -1, int _matrixId = -1, int seed = 0, bool zero = false) {
        producerNumber = _producerNumber;
        matrixId = _matrixId;
        srand(seed);
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++) {
                matrix[i][j] = (zero) ? 0 : (rand()%19 - 9);
            }
        }
        resultIdx = -1;
        // all ones , that means all of them needs to be completed       
        status.all();
        // status.flip();
    }
};

struct Q {
    job jobs[SIZE];
    int producerPtr, workerPtr; // tail, head
    int size; // job created
    int jobCreated;
    int totalJobs;

    Q(int _totalJobs) {
        producerPtr = workerPtr = 0;
        jobCreated = size = 0;
        totalJobs = _totalJobs;
    }

    inline void incJobs() {
        totalJobs++;
    }
};

// int main() {
//     int shmid1, shmid2, pid, status;
//     // int *shmdata1, *shmdata2, *shmdata;
//     // int mtx1[10][10], mtx2[10][10];
//     // int i,j,k,r1,r2,c1,c2;
    
//     // ftok to generate unique key
//     // key_t key = ftok("shmfile",65);
//     //  int shmid = shmget(key,1024,0666|IPC_CREAT); gfg
//     shmid1 = shmget(shmkey,sizeof(Q),0666|IPC_CREAT|IPC_EXCL);

//     // void* shmat(int shmid, const void* shmaddr, int shmflg);
//     Q *sharedQ;
//     sharedQ = (Q*)shmat(shmid1,0,0);

//     // shmdata = shmdata1;

//     // printf("\nShmID: %d ShmData: %d \n",shmid1,*shmdata1);
//     // printf("Enter the rows and columns of matrix 1:");
//     // scanf("%d%d",&r1,&c1);
//     // printf("Enter the matrix 1: \n");
//     // for(i=0;i<r1;++i)
//     //     for(j=0;j<c1;++j)
//     //         scanf("%d",&mtx1[i][j]);
//     // printf("\nEnter the rows and columns of matrix 2:");
//     // scanf("%d%d",&r2,&c2);
//     // printf("Enter the matrix 2: \n");   
//     // for(i=0;i<r2;++i)
//     //     for(j=0;j<c2;++j)
//     //         scanf("%d",&mtx2[i][j]);   
//     // printf("\n Hello,note this...");
//     // if(r2!=c1) {
//     //     printf("\nCannot Multiply");
//     //     return 0;
//     // }

//     // for(i=0;i<r1/2;i++)
//     //     for(j=0;j<c1;j++) {
//     //     *shmdata1 = 0;
//     //     for(k=0;k<c1;k++)
//     //         *shmdata1 += mtx1[i][k]*mtx2[k][j];
//     //         shmdata1 += sizeof(int);
//     //     }

//     pid = fork();
//     if(pid == 0) {
//         sharedQ->incJobs();
//         // for(i=r1/2;i<r1;i++)
//         //     for(j=0;j<c2;j++) {
//         //         *shmdata1 = 0;
//         //         for(k=0;k<c1;k++)
//         //             *shmdata1 += mtx1[i][k]*mtx2[k][j];
//         //             shmdata1 += sizeof(int);
//         //     }       
//     } else {
//         wait(NULL);
//         cout << sharedQ->totalJobs << '\n';
//         sharedQ->incJobs();
//     }

//     cout << sharedQ->totalJobs << '\n';

//     // while((pid = wait(&status))!= -1);
//     // shmdata1 = shmdata;

//     // printf("\n\n\nResult from %d\n", getpid());
//     // for(i=0;i<r1;++i) {
//     //     printf("\n    ");
//     //     for(j=0;j<c2;j++,shmdata1+=sizeof(int))
//     //         printf("%d ",*shmdata1);
//     // }

//     shmdt((void*)sharedQ);
//     // shmdt((void*)shmdata2);
//     shmctl(shmid1,IPC_RMID,NULL);
//     //shmctl(shmid2,IPC_RMID,NULL);
//     return 1;
// } 

/*
ftok(): is use to generate a unique key.

shmget(): int shmget(key_t,size_tsize,intshmflg); upon successful completion, shmget() returns an identifier for the shared memory segment.

shmat(): Before you can use a shared memory segment, you have to attach yourself
to it using shmat(). void *shmat(int shmid ,void *shmaddr ,int shmflg);
shmid is shared memory id. shmaddr specifies specific address to use but we should set
it to zero and OS will automatically choose the address.

shmdt(): When you’re done with the shared memory segment, your program should
detach itself from it using shmdt(). int shmdt(void *shmaddr);

shmctl(): when you detach from shared memory,it is not destroyed. So, to destroy
shmctl() is used. shmctl(int shmid,IPC_RMID,NULL);
*/

int main() {
    bitset<8> bb;

}