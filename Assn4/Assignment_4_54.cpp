#include <iostream>
#include <vector>
#include <cstdlib>

// Multithreads / multiprocesses
#include <pthread.h>
#include <sys/wait.h>

// For semaphores
#include <semaphore.h>

// For shared memory
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

// For sleep and some keys like IPC_PRIVATE
#include <unistd.h>

using namespace std;

pthread_mutexattr_t mattr; 

const int MAX_ID = 1e8;

const int MAX_PTIME = 20;       // seconds
const int MIN_PTIME = 10;

const int PSLEEP_TIME = 500;    // us

const int MIN_JOBS = 300;
const int MAX_JOBS = 500;

const int MAX_COMPLETION_TIME = 250; // ms

// status states ?
const int NOT_CREATED = 0;
const int NOT_SCHEDULED = 1;   
const int CREATED = 2;
const int SCHEDULED = 3;
const int COMPLETED = 4;
const int HAS_DEPENDENCY = 5;
const int PRODUCER_TAKEN = 6;

// Array of nodes 

// index in the array --> 500

struct node {
    int jobId;
    int completionTime;  // maybe change to int in ms ?
    int numJobs;            // num of dependent jobs
    int dependentJobs[MAX_JOBS]; // ptr to child 
    int parentIdx;          // ptr to parent
    pthread_mutex_t lock;   // for changing status, numJobs : [1.addition 2.completion] of dependentJobs
    int status;             //  
    // sem_t prwSemMutex;      
    // sem_t cSemMutex;
    // int pread_count;
    /*
        Add other data if reqd.
    */
};

struct joblist {
    node jobs[MAX_JOBS];
    int totalJobs;
    int jobsDone;
    // int jobsCreated;
    bool allJobsCreated;    // flag to check if all producers have exited or not
    pthread_mutex_t lock;
};


int getRandomJobId() {
    // TODO : use a set and prevent repititions
    return rand() % MAX_ID + 1;
}

void initJob(node* curJob) {
    curJob->jobId = getRandomJobId();
    curJob->numJobs = 0;
    curJob->status = CREATED;
    curJob->parentIdx = -1;
    for(int i=0; i<MAX_JOBS; ++i)
    {
        curJob->dependentJobs[i] = -1;
    }
    curJob->completionTime = (rand())%(PSLEEP_TIME + 1);
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
    
    for(int i = 0; i < MAX_JOBS; i++)
        dest->dependentJobs[i] = src->dependentJobs[i];
    return;
}

void *producer(void *param)
{
    // 2.2 Each p in P runs for a random time [10, 20]
    // 2.3 It adds random dependency jobs to the jobs NOT SCHEDULED (add new children to job node)
    // 2.4 It sleeps for 0..500 ms between each addition

    joblist* T = (joblist *)param;
    int timeToRun = rand()%11 + 10; // run for 10 to 20 secs
    cout<<"Time to run : "<<timeToRun<<endl;
    int startTime = time(NULL);
    int curTime = time(NULL);
    while(((curTime = time(NULL)) - startTime) <= timeToRun) {
        cout << "Jag ghumeyaaaaa thare jaisa...." << endl;
        
        // First we generate a random index and then we do linear probing on the array
        // Until we find a suitable position        
        int idx = rand() % MAX_JOBS;
        int parentJobIdx = -1;
        for(int i = idx, k = 0; k < MAX_JOBS; i++, k++) {
            // TODO : decide what does NOT_SCHEDULED mean
            // TODO : check if we need a mutex lock ? {Probably not}
            if(i == MAX_JOBS)
                i = 0;
            pthread_mutex_lock(&(T->jobs[i].lock));
            if(T->jobs[i].status == NOT_SCHEDULED) {
                parentJobIdx = i;
                // A producer is going to add a new dependency
                T->jobs[i].status = HAS_DEPENDENCY;     // TODO : if
                // no consumer should start work
                // producers can still add new dependecies
                break;
            }
            pthread_mutex_unlock(&(T->jobs[i].lock));
        }

        // we have a job thats NOT scheduled!
        if(parentJobIdx != -1) {

            int idx = rand() % MAX_JOBS;
            int newJobIdx = -1;
            for(int i = idx, k = 0; k < MAX_JOBS; i++, k++) {
                // TODO : decide what does NOT_CREATED mean
                // TODO : check if we need a mutex lock ? {Probably not}
                if(i == MAX_JOBS)
                    i = 0;

                pthread_mutex_lock(&(T->jobs[i].lock));
                if(T->jobs[i].status == NOT_CREATED) {
                    newJobIdx = i;
                    T->jobs[i].status = CREATED;
                    // Producer is going to init it
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
                parentJob.dependentJobs[numDependentJobs] = newJobIdx;
                parentJob.numJobs++;
                parentJob.status = NOT_SCHEDULED;  //  consumer can work on it
                newJob.status = NOT_SCHEDULED;
                // multiple producer and at most one consumer
                pthread_mutex_unlock(&(newJob.lock));
                pthread_mutex_unlock(&(parentJob.lock));
                
                pthread_mutex_lock(&(T->lock));
                T->totalJobs++;
                pthread_mutex_unlock(&(T->lock));
                cout<<"Job : "<<newJob.jobId<<" created with parent "<<newJob.parentIdx<<endl;
            }  
        }
        else
        {
            node& parentJob = T->jobs[parentJobIdx];
            // new job creation unsuccessful
            pthread_mutex_lock(&(parentJob.lock));            
            parentJob.status = NOT_SCHEDULED;  //  consumer can work on it
            pthread_mutex_unlock(&(parentJob.lock));
        }

        // Sleep for 0 to 500 ms
        usleep((rand()%(PSLEEP_TIME + 1)) * 1000);
    }
}
bool jobsStillLeft(joblist* T )
{
    // logic for job still left
    // either all jobs are not created or all jobs are not done
    bool res = (!T->allJobsCreated) || (T->jobsDone < T->totalJobs);
    cout<<"Res "<<res<<"\n";
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
        cout << "Jag ghumeyaaaaa thare jaisa na koiii" << endl;
        bool toWork = false;
        for(int i=0; i<MAX_JOBS;++i)
        {
            toWork = false;
            node &curJob = T->jobs[i];
            // curJobs.numJobs == 0
            pthread_mutex_lock(&curJob.lock);
            if(curJob.jobId != -1 && curJob.numJobs == 0 && curJob.status == NOT_SCHEDULED)
            {
                curJob.status = SCHEDULED;
                toWork = true;
            }
            pthread_mutex_unlock(&(curJob.lock));

            if(toWork){
                // do the job
                usleep(curJob.completionTime*1000);    // 1 ms = 1000 us
                cout<<"JobId : "<<curJob.jobId<<" completed"<<endl;
                pthread_mutex_lock(&(T->lock));
                T->jobsDone++;
                pthread_mutex_unlock(&(T->lock));
                
                pthread_mutex_lock(&(curJob.lock));
                curJob.status = COMPLETED;

                if(curJob.parentIdx != -1)
                {
                    int parentJobIdx = curJob.parentIdx;

                    // Inform the parent about job completion
                    pthread_mutex_lock(&(T->jobs[parentJobIdx].lock));
                    T->jobs[parentJobIdx].numJobs--;
                    pthread_mutex_unlock(&(T->jobs[parentJobIdx].lock));
                }
                pthread_mutex_unlock(&(curJob.lock));
                break;
            }
        }
        sleep(5);            
    }
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
    node &rootjob = T->jobs[0];
    rootjob.completionTime = (rand())%(PSLEEP_TIME + 1);

}


int main() {
    // 1.1 Create a job dependency tree T in a shared memory segment.
    // 1.2 {Each parent waits for its child} : using pthread_join?
    int shmid = shmget(IPC_PRIVATE, sizeof(joblist), 0666 | IPC_CREAT);
    joblist *T = (joblist *)shmat(shmid, NULL, 0);
   
    int P, Y; 
    cin >> P >> Y;

    int pid = fork();
    if(pid == 0) {

        // Child process (say B) spawns Y (input) consumer threads
        pthread_t pThreadId[Y];

        for(int i = 0; i < Y; i++)
            pthread_create(&pThreadId[i], NULL, consumer, T);

        for(int i = 0; i < Y; i++)
            pthread_join(pThreadId[i], NULL);
        
        pthread_mutex_lock(&(T->lock));
        T->allJobsCreated = true;
        pthread_mutex_unlock(&(T->lock));
        
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
        cout<<"All producer threads joined"<<endl;
        wait(NULL);
    }

    // 5.1 Write to the terminal accordingly : post regular updates : job addition, start, completion.
    // 5.2 Producers stop : allJobsCreated(), Consumers stop : allJobsDone()

    pthread_mutex_destroy(&(T->lock));
    for(int i = 0; i < MAX_JOBS; i++)
        pthread_mutex_destroy(&(T->jobs[i].lock));

    pthread_mutexattr_destroy(&mattr);

    shmdt(T);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}