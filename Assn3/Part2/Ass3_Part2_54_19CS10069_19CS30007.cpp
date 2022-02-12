#include <iostream>
#include <bitset>
#include <set>

#include <random>
#include <ctime>

#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

using namespace std;

const int SIZE = 8;
const int N = 1000;
const int resultantMatrixProducer = -10;
set<int> matrixId;


// generated by a producer and put into the job queue
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

    void print() {

    }  
};

struct jobQueue
{
    job jobs[SIZE];
    int producerPtr, workerPtr; // tail, head
    int size; // job created
    int jobCreated;
    int totalJobs;

    jobQueue(int _totalJobs=0) {
        producerPtr = workerPtr = 0;
        jobCreated = size = 0;
        totalJobs = _totalJobs;
    }

    static int getrandId() {
        int ans = -1;
        do {
            ans = rand()%100000 + 1;
        } while(matrixId.count(ans));
        matrixId.insert(ans);
        return ans;
    }
    inline bool isJobFinished()
    {
        return (jobCreated == totalJobs);
    }
    bool createJob(int _producerNumber, int seed = 0, bool zero = false) {
        if(size == SIZE) {
            // size + 1 == SIZE
            return false;
        }
        jobs[producerPtr] = job(_producerNumber, getrandId(), seed, zero);
        producerPtr = (producerPtr + 1)%SIZE;
        jobCreated++;
        size++;
        return true;
    }

    bool completeJob() {
        if(size == 0)
            return false;
        if(jobCreated == totalJobs)     // 1
            return true;
        
        job &curJob = jobs[workerPtr];
        job &nextJob = jobs[(workerPtr + 1)%SIZE];

        if(curJob.resultIdx == -1) {
            if(size == SIZE)
                return false;
            curJob.resultIdx = jobs[workerPtr].resultIdx = jobs[(workerPtr + 1)%SIZE].resultIdx = producerPtr;
            jobs[producerPtr] = job(resultantMatrixProducer, getrandId(), 0, true); 
            producerPtr = (producerPtr + 1)%SIZE;
            size = size++;
        }

        int resIdx = curJob.resultIdx;
        job &resJob = jobs[resIdx];
        
        size_t firstOn = curJob.status._Find_first();
        int i = firstOn & 1, j = firstOn & 2, k = firstOn & 4;

        // Compute D (i, j, k) = A(i, k) * B(k, j)

        int row1 = (N / 2)*i + (N / 2);
        int col2 = (N / 2)*j + (N / 2);
        int col1 = (N / 2)*k + (N / 2);
        
        for(int row = (N / 2)*i; row < row1; row++) {
            for(int col = (N / 2)*j; col < col2; col++) {
                resJob.matrix[row][col] = 0;
                for(int cur = (N / 2)*k; cur < col1; cur++) {
                    // mutex
                    resJob.matrix[row][col] += curJob.matrix[row][cur] * nextJob.matrix[cur][col];  
                }
            }
        }
        curJob.status.flip(firstOn);
        nextJob.status.flip(firstOn);

        if(curJob.status.none()) {
            // all done
            size -= 2;
            curJob = job();
            nextJob = job();
            workerPtr = (workerPtr + 2)%SIZE;
        }
        return true;
    }
};

void producer(int producerId, jobQueue *Q) {
    // if queue not full and jobs fully not created
    // create a job and push into q
    
    // Producer: 
    // 1. generate random job
    // 2. Wait for 0-3 s
    // 3. Try to insert in queue
    //      Wait until queue is full : 
    //      Print job details
    //      Increase job_counter

    while(!Q->isJobFinished()) {
        int waitTime = rand()%4;
        sleep(waitTime);
        if(Q->createJob(producerId, producerId)) {
            cout << producerId << " successfully created job! : )\n";
        } else {
            cout << producerId << " couldnt create job.. trying again\n"; 
        }
    }
    return;
}

void worker(int workerId, jobQueue *Q) {
    // wait: if q is full or empty ? or all 8 blocks are already taken
    // wait if 1 matrix only and all jobs done?
    
    // Worker:
    // 1. Wait for 0-3 s
    // 2. Retrieve 2 blocks of the first 2 matrices

    while(!(Q->isJobFinished())) {
        int waitTime = rand()%4;
        sleep(waitTime);
        if(Q->completeJob()) {
            cout << workerId << " successfully complete job! : )\n";
        } else {
            cout << workerId << " couldnt complete job.. trying again\n"; 
        }
    }
    return ;
}

int main(int argc, char *argv[]) {
    // Create nP, nW processes
    int nP, nW;
    cin >> nP >> nW;
    int nMatrices;
    cin >> nMatrices;

    // SHM queue https://2k8618.blogspot.com/2011/02/matrix-multiplication-using-shared.html
    // SHM job_created


    // Create (SHM) shared memory: shared among all
    // 1. queue: finite size (SIZE)
    // 2. job_created counter : no of matrices gen
    key_t shmkey = 154;
    int shmid1 = shmget(shmkey,sizeof(jobQueue),0666|IPC_CREAT);
    jobQueue *sharedJobQ;
    sharedJobQ = (jobQueue*) shmat(shmid1, NULL, 0);
    // shared
    for(int i = 0; i < nP ; i++) {
        int fork_status = fork();
        if(fork_status == 0) {
            producer(i, sharedJobQ);
            shmdt((void *)sharedJobQ);
            exit(0);            
        }
    }
    
    for(int i = 0; i < nW; i++) {
        int fork_status = fork();
        if(fork_status == 0) {
            worker(i, sharedJobQ);
            shmdt((void *)sharedJobQ);
            exit(0);
        }
    }
    // check this : wait until 1 matrix and all jobs completed?
    while(wait(NULL) > 0);
 
    shmdt((void *)sharedJobQ);
    shmctl(shmid1, IPC_RMID, NULL);
    return 0;
}