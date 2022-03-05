#include <iostream>
#include <vector>
#include <cstdlib>
#include <cassert>

// Multithreads / multiprocesses
#include <pthread.h>
#include <sys/wait.h>

// For semaphores
#include <semaphore.h>

// For shared memory
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#include <unistd.h>

using namespace std;

pthread_mutexattr_t mattr; 

const int MAX_ID = 1e8;

const int MAX_PTIME = 5;       // seconds
const int MIN_PTIME = 1;

const int MIN_PSLEEP_TIME = 200;
const int MAX_PSLEEP_TIME = 500;    // us

const int MIN_INITIAL_JOBS = 300;
const int MAX_INITIAL_JOBS = 500;

int MAX_JOBS = 500;

const int MAX_COMPLETION_TIME = 250; // ms

// Bound calculated considering the limits on the number of threads allowed
const int MAX_PRODUCERS = 500;
const int MAX_WORKERS = 500;

// status states ?
const int NOT_CREATED = 0;
const int NOT_SCHEDULED = 1;   
const int SCHEDULED = 2;
const int HAS_DEPENDENCY = 3;
const int CREATED = 4;

// Array of nodes 

// index in the array --> 500

struct node {
    int jobId;
    int completionTime;  // maybe change to int in ms ?
    int numJobs;            // num of dependent jobs 
    int parentIdx;          // ptr to parent
    pthread_mutex_t lock;   // for changing status, numJobs : [1.addition 2.completion] of dependentJobs
    int status;             //  
};

struct joblist {
    node* jobs;
    int totalJobs;
    int jobsDone;
    bool allJobsCreated;    // flag to check if all producers have exited or not
    pthread_mutex_t lock;
};

int getRandomInRange(const int lo, const int hi) {
    assert(lo<=hi);
    return rand()%(hi - lo + 1) + lo;
}

void initJob(node* curJob) {
    // TODO : ensure all ids are unique
    curJob->jobId = getRandomInRange(0, MAX_ID);
    curJob->numJobs = 0;
    curJob->status = CREATED;
    curJob->parentIdx = -1;
    curJob->completionTime = getRandomInRange(0, MAX_COMPLETION_TIME);
}

void copyJob(node* dest, node* src)
{
    if(src == NULL || dest == NULL ) {
        cout<<"Null pointer in copyJob\n";
        return ;
    }
    dest->completionTime = src->completionTime;
    dest->jobId = src->jobId;
    dest->numJobs = src->numJobs;
    dest->status = src->status;
    dest->parentIdx = src->parentIdx;
    
    return;
}

void *producer(void *param)
{
    // 2.2 Each p in P runs for a random time [10, 20]
    // 2.3 It adds random dependency jobs to the jobs NOT SCHEDULED (add new children to job node)
    // 2.4 It sleeps for 0..500 ms between each addition

    joblist* T = (joblist *)param;
    int timeToRun = getRandomInRange(MIN_PTIME, MAX_PTIME); // run for 10 to 20 secs
    cout<<"Time to run : "<<timeToRun<<endl;
    int startTime = time(NULL);
    int curTime = time(NULL);
    while(((curTime = time(NULL)) - startTime) <= timeToRun) {
        cerr << "Jag ghumeyaaaaa thare jaisa...." << endl;
        
        // First we generate a random index and then we do linear probing on the array
        // Until we find a suitable position        
        int idx = getRandomInRange(0, MAX_JOBS - 1);
        int parentJobIdx = -1;
        for(int i = idx, k = 0; k < MAX_JOBS; i++, k++) {
            // TODO : decide what does NOT_SCHEDULED mean
            // TODO : check if we need a mutex lock ? {Probably not}
            if(i == MAX_JOBS)
                i = 0;
            pthread_mutex_lock(&(T->jobs[i].lock));
            if(T->jobs[i].status == NOT_SCHEDULED || T->jobs[i].status == HAS_DEPENDENCY) {
                parentJobIdx = i;
                // A producer is going to add a new dependency
                T->jobs[i].status = HAS_DEPENDENCY;
                // no consumer should start work
                // producers can still add new dependecies
                pthread_mutex_unlock(&(T->jobs[i].lock));
                break;
            }
            pthread_mutex_unlock(&(T->jobs[i].lock));
        }

        // we have a job thats NOT scheduled!
        if(parentJobIdx != -1) {
            int idx = getRandomInRange(0, MAX_JOBS - 1);
            int newJobIdx = -1;
            for(int i = idx, k = 0; k < MAX_JOBS; i++, k++) {
                if(i == MAX_JOBS)
                    i = 0;

                pthread_mutex_lock(&(T->jobs[i].lock));
                if(T->jobs[i].status == NOT_CREATED) {
                    newJobIdx = i;
                    T->jobs[i].status = CREATED; 
                    // 
                    // Producer is going to init it
                    pthread_mutex_unlock(&(T->jobs[i].lock));
                    break;
                }
                pthread_mutex_unlock(&(T->jobs[i].lock));
            }
            
            // we have a free index for a new job!
            if(newJobIdx != -1) {
                node* newlyCreatedJob = new node;
                initJob(newlyCreatedJob); 
                
                node& newJob = T->jobs[newJobIdx];
                node& parentJob = T->jobs[parentJobIdx];
                                
                pthread_mutex_lock(&(newJob.lock));
                pthread_mutex_lock(&(parentJob.lock));
                
                copyJob(&newJob, newlyCreatedJob);
                newJob.parentIdx = parentJobIdx;
                int numDependentJobs = parentJob.numJobs;
                // parentJob.dependentJobs[numDependentJobs] = newJobIdx;
                parentJob.numJobs++;
                parentJob.status = HAS_DEPENDENCY;  //  consumer can work on it
                newJob.status = NOT_SCHEDULED;
                // multiple producer and at most one consumer
                pthread_mutex_unlock(&(parentJob.lock));
                pthread_mutex_unlock(&(newJob.lock));
                
                pthread_mutex_lock(&(T->lock));
                T->totalJobs++;
                pthread_mutex_unlock(&(T->lock));
                cout<<"Job : "<<newJob.jobId<<" created with parent idx "<<newJob.parentIdx<<" ("<<T->jobs[newJob.parentIdx].jobId<<")" <<endl;
            } else {
                node& parentJob = T->jobs[parentJobIdx];
                // new job creation unsuccessful
                pthread_mutex_lock(&(parentJob.lock));            
                parentJob.status = (parentJob.numJobs == 0) ? NOT_SCHEDULED : HAS_DEPENDENCY;  //  consumer may work on it
                pthread_mutex_unlock(&(parentJob.lock));
            }
        }
        // Sleep for 0 to 500 ms
        usleep(getRandomInRange(MIN_PSLEEP_TIME, MAX_PSLEEP_TIME) * 1000);
    }
    return NULL;
}

bool jobsStillLeft(joblist* T )
{
    // logic for job still left
    // either all jobs are not created or all jobs are not done
    bool res = (!T->allJobsCreated) || (T->jobsDone < T->totalJobs);
    cerr<<"Res "<<res<<"\n";
    cerr<<"All jobs created "<<T->allJobsCreated<<endl;
    return res;
}
void* consumer(void* param)
{
    joblist* T = (joblist *)param;
    // get a job
    // check if all of its dependencies are done
    // if yes then complete it 
    // repeat
    bool jobReceived = false;
    while(jobsStillLeft(T))
    {   
        cerr << "Jag ghumeyaaaaa thare jaisa na koiii" << endl;
        bool toWork = false;
        for(int i=0; i<MAX_JOBS;++i)
        {
            toWork = false;
            node &curJob = T->jobs[i];
            // curJobs.numJobs == 0
            pthread_mutex_lock(&curJob.lock);
            if(curJob.status == NOT_SCHEDULED)
            {
                assert(curJob.jobId != -1 && curJob.numJobs == 0);
                curJob.status = SCHEDULED;
                toWork = true;
            }
            pthread_mutex_unlock(&(curJob.lock));

            if(toWork){
                // do the job
                cout << "Starting job with jobId " << curJob.jobId << endl;
                usleep(curJob.completionTime*1000);    // 1 ms = 1000 us
                cout<<"JobId : "<<curJob.jobId<<" completed"<<endl;
                pthread_mutex_lock(&(T->lock));
                T->jobsDone++;
                pthread_mutex_unlock(&(T->lock));
                
                pthread_mutex_lock(&(curJob.lock));
                curJob.status = NOT_CREATED; // space deallocated
                curJob.jobId = -1;
                if(curJob.parentIdx != -1)
                {
                    int parentJobIdx = curJob.parentIdx;
                    node& parentJob = T->jobs[parentJobIdx];
                    // Inform the parent about job completion
                    pthread_mutex_lock(&(parentJob.lock));
                    parentJob.numJobs--;
                    parentJob.status = (parentJob.numJobs == 0) ? NOT_SCHEDULED : HAS_DEPENDENCY;  //  consumer may work on it
                    pthread_mutex_unlock(&(parentJob.lock));
                }
                pthread_mutex_unlock(&(curJob.lock));
                break;
            }
            else{
                cerr<<"I am unemployed :("<<endl;
            }
        }
    }
    return NULL;
}


void initjoblist(joblist *T) 
{
    pthread_mutexattr_init(&mattr);

    // Lock sharing here : Basically we specify the context in which this has to be shared
    if(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)!=0){
        cerr<<"Lock sharing Unsuccessful\n";
        exit(1);
    }

    //Define the mutex lock
    if(pthread_mutex_init(&(T->lock), &mattr)!=0){
        cerr<<"Error : mutex_lock init() failed\n";
        exit(1);
    }

    T->totalJobs = T->jobsDone = 0;
    T->allJobsCreated = false;

    cout << "Creating initial base tree" << endl;

    for(int i =0; i< MAX_JOBS; ++i)
    {
        T->jobs[i].parentIdx = -1;
        T->jobs[i].jobId = -1;
        T->jobs[i].status = NOT_CREATED;

        //Define the mutex lock
        if(pthread_mutex_init(&(T->jobs[i].lock), &mattr)!=0){
            cerr<<"Error : mutex_lock init() failed\n";
            exit(1);
        }
    }

    int total = getRandomInRange(MIN_INITIAL_JOBS, MAX_INITIAL_JOBS);
    node &rootJob = T->jobs[0];
    initJob(&rootJob);
    rootJob.status = NOT_SCHEDULED;
    T->totalJobs = total;
    rootJob.completionTime = getRandomInRange(0, MAX_COMPLETION_TIME);
    cout << "Created first job with id " << rootJob.jobId << endl;

    for(int jobsCreated = 1; jobsCreated < total; jobsCreated++) {
        // step 1: find an empty index
        int idx = getRandomInRange(0, MAX_JOBS - 1);
        int parentIdx = -1, childIdx = -1;
        for(int k = 0; k < MAX_JOBS; k++, idx++) {
            if(idx == MAX_JOBS)
                idx = 0;
            if(T->jobs[idx].status == NOT_CREATED) {
                childIdx = idx;
                initJob(&(T->jobs[idx]));
                T->jobs[idx].completionTime = getRandomInRange(0, MAX_COMPLETION_TIME);
                break;
            }
        }

        // step 2: find a suitable parent
        idx = getRandomInRange(0, MAX_JOBS - 1);
        for(int k = 0; k < MAX_JOBS; k++, idx++) {
            if(idx == MAX_JOBS)
                idx = 0;
            if(T->jobs[idx].status == NOT_SCHEDULED || T->jobs[idx].status == HAS_DEPENDENCY) {
                parentIdx = idx;
                break;
            }
        }

        // step 3: add link and update status
        T->jobs[parentIdx].status = HAS_DEPENDENCY;
        T->jobs[childIdx].status = NOT_SCHEDULED;
        T->jobs[childIdx].parentIdx = parentIdx;
        T->jobs[parentIdx].numJobs++;

        cout << "Job : " << T->jobs[childIdx].jobId << " created with parent idx " << parentIdx << " (" << T->jobs[parentIdx].jobId << ")" << endl;
    }

    cout << "Initial tree created!" << endl;
}


int main() {
    // 1.1 Create a job dependency tree T in a shared memory segment.
    // 1.2 {Each parent waits for its child} : using pthread_join?
    int P, Y; 
    cin >> P >> Y;

    srand(time(NULL));

    // limit calculation : min time for a job : 200ms
    // min sleep time = 0ms
    // max total execution time for producer : 20s
    // Max jobs = 20 / 0.2 = 100 jobs per producer

    // TODO : Maybe hardcode a limit
    MAX_JOBS = max(MAX_INITIAL_JOBS, 100 * P);
    int shmidAll = shmget(IPC_PRIVATE, sizeof(node) * MAX_JOBS, 0666 | IPC_CREAT);
    node* alljobs = (node* )shmat(shmidAll, NULL, 0);

    int shmid = shmget(IPC_PRIVATE, sizeof(joblist), 0666 | IPC_CREAT);
    joblist *T = (joblist *)shmat(shmid, NULL, 0);

    // Attach the list
    T->jobs = alljobs;

    initjoblist(T);

    int pid = fork();
    if(pid == 0) {
        sleep(2); // TODO remove this
        // Child process (say B) spawns Y (input) consumer threads
        pthread_t pThreadId[Y];

        for(int i = 0; i < Y; i++)
            pthread_create(&pThreadId[i], NULL, consumer, T);

        for(int i = 0; i < Y; i++)
            pthread_join(pThreadId[i], NULL);
        
        cout << "All consumers also finished working! Yayayayaya" << endl;
        
        exit(0);

    } else {
        // Master process (say A) spawns P (input) producer threads 
        pthread_t cThreadId[P];
        cout<<"Master process A"<<endl;
        for(int i = 0; i < P; i++)
            pthread_create(&cThreadId[i], NULL, producer, T);
        cout<<"All producer threads created"<<endl;
        for(int i = 0; i < P; i++)
            pthread_join(cThreadId[i], NULL);

        cout << "All producer threads joined" << endl;
        pthread_mutex_lock(&(T->lock));
        T->allJobsCreated = true;
        pthread_mutex_unlock(&(T->lock));
        
        cout<<"All producer threads joined"<<endl;
        wait(NULL);
    }

    cout<<"Jobs done " <<T->jobsDone<< ", Total jobs : "<< T->totalJobs<<endl;
    pthread_mutex_destroy(&(T->lock));
    for(int i = 0; i < MAX_JOBS; i++)
        pthread_mutex_destroy(&(T->jobs[i].lock));

    pthread_mutexattr_destroy(&mattr);

    shmdt(alljobs);
    shmdt(T);
    shmctl(shmid, IPC_RMID, NULL);
    shmctl(shmidAll, IPC_RMID, NULL);
    return 0;
}