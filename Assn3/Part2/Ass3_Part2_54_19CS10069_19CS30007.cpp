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
#include <pthread.h>

using namespace std;

pthread_mutexattr_t mattr; 

const int SIZE = 8;
const int N = 1000;
const int MAX_PRINT_SIZE = 10;
const int resultantMatrixProducer = -10;
set<int> matrixId;

// bitset function
void custom_set(uint8_t &bitset)
{
    bitset = 255;
    return;
}

// To find the block to be multiplied : using first set bit
size_t custom_find_first_one(uint8_t bitset)
{
    size_t i;
    uint8_t one = 1;
    for(i = 0; i < 8; ++i)
    {
        if( (one<<i) & bitset)
        {
            break;
        }
    }
    return i;
}

// flip a bit
void custom_flip(uint8_t &bitset, int flip)
{
    if(flip <0 || flip > 7 ) 
        return;
    uint8_t one = 1;
    bitset = bitset ^ (one << flip);
    return ;
}


// generated by a producer and put into the job queue
struct job {
    int producerNumber;
    uint8_t status;     // 8 bits for representing multiplication status
    long long matrix[N][N];   // -9 <= matrix[i][j] <= 9
    int matrixId;       // rand(1...1e5)
    int resultIdx;      // where the resultant matrix is stored
};

// Print a job
void printJob(job* J) {
    cout<<"----------------------------Print Job----------------------------\n";
    cout<< "Producer Number : "<< ((J->producerNumber == resultantMatrixProducer) ? "Generated as resultant": to_string(J->producerNumber)) <<"\n";
    cout<<"Matrix Id : "<<J->matrixId<<"\n";
    cout<<"Status : "<<(uint)(J->status)<<"\n";

    if(N < MAX_PRINT_SIZE) {
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < N; j++)
                printf("%4lld ", J->matrix[i][j]);
            cout << '\n'; 
        }
    }
    cout<<"----------------------------Printed-----------------------------"<<endl;
}

// initialize a job
void initJob(job* J, int _producerNumber = -1, int _matrixId = -1, int seed = 0, bool zero = false) {
    J->producerNumber = _producerNumber;
    J->matrixId = _matrixId;
    // srand(seed);
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            J->matrix[i][j] = (zero) ? 0 : (rand()%19 - 9);
        }
    }
    J->resultIdx = -1;
    // all ones , that means all of them needs to be completed       
    custom_set(J->status);
    return;
}

// Create a Copy 
void copyJob(job* src, job* dest)
{
    if(src == NULL || dest == NULL ) {
        cout<<"Null pointer in copyJob\n";
        return ;
    }
    dest->producerNumber = src->producerNumber;
    dest->status = src->status;
    dest->matrixId = src->matrixId;
    dest->resultIdx = src->resultIdx;
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            dest->matrix[i][j] = src->matrix[i][j];
        }
    }
    return;
}

// The required shared mem segment
struct SHM
{
    job jobs[SIZE];             // the queue
    int producerPtr, workerPtr; // tail, head
    int size;                   // no of elements
    int jobCreated;             
    int totalJobs;             
    int jobsDone;
    int curSegCounter;          // counts how many segments have been merged to resultant matrix (0 to 8)
    pthread_mutex_t mutex_lock; // for mutual exclusion 
};

// Function to return random ids for matrices
int getrandId() {
    int ans = -1;
    do {
        ans = rand()%100000 + 1;
    } while(matrixId.count(ans));
    matrixId.insert(ans);
    return ans;
}

// SHM initializer : called only by the parent
void SHMInit(int _totalJobs, SHM* Q) {
    Q->producerPtr = Q->workerPtr = 0;
    Q->jobCreated = 0;
    Q->size = 0;
    Q->jobsDone = 0;
    Q->totalJobs = _totalJobs;
    Q->curSegCounter = 0;
    
    pthread_mutexattr_init(&mattr);
    
    if(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)!=0){
    	cerr<<"Lock sharing Unsuccessful\n";
    	exit(1);
    }
    if(pthread_mutex_init(&(Q->mutex_lock), &mattr)!=0){
    	cerr<<"Error : mutex_lock init() failed\n";
    	exit(1);
    }
}

// Check if workers' jobs are complete
inline bool isJobFinished(SHM* Q) {
    return (Q->jobsDone >= Q->totalJobs - 1) && (Q->size == 1);
}

// Check if producers' jobs are complete
inline bool isJobCreated(SHM* Q) {
    return (Q->jobCreated >= Q->totalJobs);
}

// Creation of a job
void createJob(SHM* Q, job* newJob, int _producerNumber, int seed = 0, bool zero = false) {

    // Keep at least one empty space in queue to avoid deadlock
    while(Q->size + 1 >= SIZE);

    // many producers
    if(pthread_mutex_lock(&(Q->mutex_lock))!=0)
    {
        cerr<<"Mutex Lock Error\n";
        exit(1);
    }
    
    // check if more jobs are to be created
    if(isJobCreated(Q) || Q->size + 1 >= SIZE) {
        cerr<<"I entered mutex but ";
        if(isJobCreated(Q))
        {
            cerr<<" Jobs already created ";
        }
        if(Q->size + 1 >= SIZE)
        {
            cerr<<" Queue is still full ";
        }
        cerr<<"\n";
        if(pthread_mutex_unlock(&(Q->mutex_lock))!=0) {
            cerr<<"Mutex Unlock Error\n";
            exit(1);
        }
        return;
    }

    cerr<<"Cur Size : "<<Q->size<<endl;
    
    // copy job
    copyJob(newJob, &(Q->jobs[Q->producerPtr]));
    
    // increment producerPtr, jobCreated and queue size
    Q->producerPtr = (Q->producerPtr + 1)%SIZE;
    (Q->jobCreated)++;
    (Q->size)++;
    
    // print details
    cout << newJob->producerNumber << " (producer) successfully created job with matrixid " << newJob->matrixId <<  "\n";
    cout << "Current process id is: " << getpid() << '\n';
    printJob(newJob);
    
    if(pthread_mutex_unlock(&(Q->mutex_lock))!=0)
    {
        cerr<<"Mutex Unlock Error\n";
        exit(1);
    }

    return;
}

void completeJob(SHM* Q, int workerNo) {
    // when to wait
    // 1)Q->size is 0 
    // 2)Q->size is 1 but all jobs not done
    // 3)Q->size > 1 but all accesses are made
    while((Q->size == 0) || ( Q->size > 1 && Q->jobs[Q->workerPtr].status == 0) || (Q->size == 1 && !(isJobFinished(Q))));
    
    // <-> wait 
    if(pthread_mutex_lock(&(Q->mutex_lock))!=0)
    {
        cerr<<"Mutex Lock Error\n";
        exit(1);
    }
    
    // when to exit
    // 1) job finished
    // 2) If other waiting conditions encountered
    // if it is finished
    if(isJobFinished(Q) || (Q->size == 0) || ( Q->size > 1 && Q->jobs[Q->workerPtr].status == 0) || (Q->size == 1 && !(isJobFinished(Q)))) {
        if(pthread_mutex_unlock(&(Q->mutex_lock))!=0)
        {
            cerr<<"Mutex Unlock Error\n";
            exit(1);
        }
        return;
    }

    int workerPtr = Q->workerPtr;
    int producerPtr = Q->producerPtr;
    job &curJob = Q->jobs[workerPtr];
    job &nextJob = Q->jobs[(workerPtr + 1)%SIZE];
    
    
    // the first operation on the two matrices
    if(curJob.resultIdx == -1) {
        // creation of a new mem segment
        curJob.resultIdx = nextJob.resultIdx = producerPtr;
        initJob(&(Q->jobs[producerPtr]), resultantMatrixProducer, getrandId(), 0, true); 
        Q->producerPtr = (Q->producerPtr + 1)%SIZE;
        (Q->size)++;
    }
    
    size_t firstOn = custom_find_first_one(curJob.status);
    
    custom_flip(curJob.status, firstOn);
    custom_flip(nextJob.status, firstOn);

    int resIdx = curJob.resultIdx;
    job &resJob = Q->jobs[resIdx];

    if(firstOn == SIZE)
    {
        cerr<<"All jobs are occupied\n";
        if(pthread_mutex_unlock(&(Q->mutex_lock))!=0)
        {
            cerr<<"Mutex Unlock Error\n";
            exit(1);
        }
        return;
    }
    
    int i = firstOn & 1, j = (firstOn >> 1)&1 , k = (firstOn>>2) & 1;
    printf("---------------------Job Description for Worker %d-----------------------\n",workerNo);
    printf("Reading from matrices %d and %d\n", curJob.matrixId, nextJob.matrixId);
    printf("Worker %d is computing D(%d, %d, %d) for %d\n\n", workerNo, i, j, k, resJob.matrixId);
    fflush(stdout);
    if(pthread_mutex_unlock(&(Q->mutex_lock))!=0)
    {
        cerr<<"Mutex Unlock Error\n";
        exit(1);
    }

    // Compute D (i, j, k) = A(i, k) * B(k, j)
    int row1 = (N / 2)*i + (N / 2);
    int col2 = (N / 2)*j + (N / 2);
    int col1 = (N / 2)*k + (N / 2);
    
    // long long temp[N / 2][N / 2];
    long long **temp = new long long* [N / 2];
    for(int memalloc = 0; memalloc< N/2 ; ++memalloc)
    {
        temp[memalloc] = new long long [N/2];
    }
    for(int row = (N / 2)*i; row < row1; row++) {
        for(int col = (N / 2)*j; col < col2; col++) {
            temp[row - (N / 2)*i][col - (N / 2)*j] = 0;
            for(int cur = (N / 2)*k; cur < col1; cur++) {
                temp[row - (N / 2)*i][col - (N / 2)*j] += curJob.matrix[row][cur] * nextJob.matrix[cur][col];  
            }
        }
    }

    if(pthread_mutex_lock(&(Q->mutex_lock))!=0)
    {
        cerr<<"Mutex Lock Error\n";
        for(int memalloc =0 ;memalloc < (N/2); ++memalloc)
            delete []temp[memalloc];
        delete [] temp;
        exit(1);
    }
    
    printf("Working on matrices %d and %d\n", curJob.matrixId, nextJob.matrixId);
    printf("Worker %d is copying/adding to C(%d, %d) in %d\n", workerNo, i, j, resJob.matrixId);

    fprintf(stderr, "i: %d, j: %d, k: %d\n", i, j, k);
    for(int row = (N / 2)*i; row < row1; row++) {
        for(int col = (N / 2)*j; col < col2; col++) {
            resJob.matrix[row][col] += temp[row - (N / 2)*i][col - (N / 2)*j];
            cerr << resJob.matrix[row][col] << " ";
        }
        cerr<<"\n";
    }

    Q->curSegCounter++;
    printf("Status %d/8 segments done\n", Q->curSegCounter);
    fflush(stdout);

    // All 8 operations have been done
    if(Q->curSegCounter == SIZE) {
        // Removing the first two entries
        cout << "\n***************************RESULT CHECK******************************" << endl;
        printJob(&curJob);
        printJob(&nextJob);
        printJob(&Q->jobs[resIdx]);
        Q->curSegCounter = 0;
        Q->size -= 2;
        (Q->jobsDone)++;
        cout<<"Jobs completed : "<<Q->jobsDone<< " / "<< Q->totalJobs -1<<endl;
        // cout<<"Job Done worked id :"<<Q->workerPtr<< ", worker 2: "<< Q->workerPtr + 1 << "\n";
        Q->workerPtr = (Q->workerPtr + 2)%SIZE;
    }
    for(int memalloc =0 ;memalloc < (N/2); ++memalloc)
        delete []temp[memalloc];
    delete [] temp;
    if(pthread_mutex_unlock(&(Q->mutex_lock))!=0)
    {
        cerr<<"Mutex Unlock Error\n";
        exit(1);
    }
    return;
}

void producer(int producerId, SHM *Q) {
    srand(time(NULL) + producerId);
    while(!isJobCreated(Q)) {
        // 1. init random job
        job* newJob = new job;
        initJob(newJob, producerId, getrandId(), time(NULL));

        // 2. Wait for a random time b/w 0-3 seconds
        int waitTime = rand()%4;
        sleep(waitTime);
        
        // 3. Try to insert into the job queue
        createJob(Q, newJob, producerId, producerId);
    }
    return;
}

void worker(int workerId, SHM *Q) {
    srand(time(NULL) + workerId);
    while(!isJobFinished(Q)) {
        // 1. wait for 0-3 seconds
        int waitTime = rand()%4;
        sleep(waitTime);

        // 2. Retrive first 2 blocks and multiply the reqd part
        completeJob(Q, workerId);
    }
    return ;
}

int main(int argc, char *argv[]) {
    // Create nP, nW processes
    int nP, nW;
    cin >> nP >> nW;
    // nP = 5, nW = 16;
    int nMatrices;
    // nMatrices = 2;
    cin >> nMatrices;

    time_t start_time;
    start_time = time(NULL);
    clock_t start = clock();

    key_t shmkey = 154;
    int shmid1 = shmget(shmkey,sizeof(SHM),0666|IPC_CREAT);
    SHM *sharedJobQ;
    sharedJobQ = (SHM*) shmat(shmid1, NULL, 0);
    
    SHMInit(nMatrices, sharedJobQ);
    // shared
    vector<pid_t> allChildren;
    for(int i = 0; i < nP ; i++) {
        int pid = fork();
        if(pid < 0) {
            perror("Error in fork!\n");
            exit(-1);
        }
        else if(pid == 0) {
            cerr<< "Producer "<<i<<" created\n";
            producer(i, sharedJobQ);
            cerr<< "Producer "<<i<<" exited\n";
            shmdt((void *)sharedJobQ);
            exit(0);            
        } else {
            allChildren.push_back(pid);
        }
    }
    
    for(int i = 0; i < nW; i++) {
        int pid = fork();
        if(pid < 0) {
            perror("Error in fork!\n");
            exit(-1);
        }
        else if(pid == 0) {
            cerr<< "Worker "<<i<<" created\n";
            worker(i, sharedJobQ);
            cerr<< "Worker "<<i<<" exited\n";
            shmdt((void *)sharedJobQ);
            exit(0);
        }
        else {
            allChildren.push_back(pid);
        }
    }

    // wait until 1 matrix and all jobs completed
    while(!isJobFinished(sharedJobQ));
    long long trace = 0;

    for(int i=0; i< N; ++i)
    {
        trace += (sharedJobQ->jobs[sharedJobQ->workerPtr]).matrix[i][i];
    }

    clock_t end = clock();
    double seconds = (float)(end - start) / CLOCKS_PER_SEC;
    cout << "Total time taken(CPU Clock) in seconds: " << seconds << '\n';
    time_t end_time = time(NULL);
    time_t duration = end_time - start_time;
    cout<< "Total time taken in seconds: "<<duration<<endl;
    cout << "Trace = "<<trace<< endl;
    for(auto u: allChildren)
        kill(u, SIGKILL);
 
    pthread_mutex_destroy(&(sharedJobQ->mutex_lock));
    pthread_mutexattr_destroy(&mattr);

    shmdt((void *)sharedJobQ);
    shmctl(shmid1, IPC_RMID, NULL);
    return 0;
}
