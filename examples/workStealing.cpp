#include <pthread.h> //for pthread_t
#include <stdio.h>      // printf, fgets
#include <stdlib.h>     // atoi
#include <unistd.h>   // for usleep
#include <iostream>
#include <string>
#include <sstream>  // for convert number to string
#include <queue>    // for deque used in working stealing thread safe class
#define MAX_THREADS 16

class TaskKernel {

public:

	TaskKernel(int index): kernelIndex_(index){}

	~TaskKernel() {}

	void executeTask()
	{
		std::stringstream sstk;
		sstk << kernelIndex_;
		std::string str=sstk.str()+" is kernelIndex.\n";
		usleep(kernelIndex_*100000);
		std::cout<<str;
	}

private:

	int kernelIndex_;

};

template<typename ExternalKernelType>
class WorkStealingQ {

public:

	WorkStealingQ()
	{
		pthread_mutex_init(&mutexq_, NULL);
	}

	~WorkStealingQ()
	{
		pthread_mutex_destroy(&mutexq_);
	}

	void push(ExternalKernelType* new_job)
	{
		pthread_mutex_lock (&mutexq_);
		jobDeque_.push_front(new_job);
		pthread_mutex_unlock (&mutexq_);
	}

	//pass in pointer res by reference, in our threadpool and workstealing Q,
	// we only deal with pointer.
	bool canPop(ExternalKernelType* &res)
	{
		pthread_mutex_lock (&mutexq_);
		if (jobDeque_.empty()){
			//std::cout<<"In WorkStealingQ class, in can_pop, deque is empty\n";
			pthread_mutex_unlock (&mutexq_);
			return false;
		}
		res=jobDeque_.front();
		jobDeque_.pop_front();
		res->executeTask();
		pthread_mutex_unlock (&mutexq_);
		return true;
	}

	bool canSteal(ExternalKernelType* &res)
	{
		pthread_mutex_lock (&mutexq_);
		if (jobDeque_.empty()){
			//std::cout<<"In WorkStealingQ class, in can_steal, deque is empty\n";
			pthread_mutex_unlock (&mutexq_);
			return false;
		}
		res=jobDeque_.back();
		jobDeque_.pop_back();
		pthread_mutex_unlock (&mutexq_);
		return true;
	}

	bool empty()
	{
		pthread_mutex_lock (&mutexq_);
		bool t;
		t = jobDeque_.empty();
		pthread_mutex_unlock (&mutexq_);
		return t;
	}

private:

	pthread_mutex_t mutexq_;
	std::deque<ExternalKernelType*> jobDeque_;
};

template<typename ExternalKernelType>
struct HolderStruct{
	long threadParameter;
	bool* ptr_poolStart;
	bool* ptr_submissionDone;
	ExternalKernelType* ptr_etk;
	WorkStealingQ<ExternalKernelType>* ptr_wsq;
	WorkStealingQ<ExternalKernelType>* ptr_wsq_array[MAX_THREADS];
	int* ptr_busyThreads;
	int maxTp;
};

template<typename ExternalKernelType>
class ThreadPool {

	typedef HolderStruct<ExternalKernelType> HolderStructType;
	typedef WorkStealingQ<ExternalKernelType> WorkStealingQType;
	typedef std::vector<pthread_t> VectorPthreadType;

public:

	ThreadPool(int t_num)
	    : poolStart_(false),
	      poolDone_(false),
	      jobDone_(false),
	      submissionDone_(false),
	      currentQ_(0),
	      tUsed_(t_num),
	      busyThreads_(t_num),
	      taskObj_(2)
	{
		int rc;
		long t;
		ptrPoolStart_ = &poolStart_;
		ptrSubmissionDone_ = &submissionDone_;
		ptrTaskObj_ = &taskObj_;
		for (int t=0;t<t_num;t++) ptrJobQArray_[t] = &jobQArray_[t];
		for (t=t_num;t<MAX_THREADS;t++) ptrJobQArray_[t] = NULL;
		ptrBusyThreads_=&busyThreads_;

		pthread_attr_init(&attr_); // the attr may not be necessary
		pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_JOINABLE);

		for (t=0;t<t_num;t++) {
			std::cout<<" In threadpool loop,before launch thread t="<<t<<"\n";
			hsArray_[t].threadParameter=t;
			hsArray_[t].ptr_poolStart=ptrPoolStart_;
			hsArray_[t].ptr_submissionDone=ptrSubmissionDone_;
			//Here is the problem, there is no object yet,
			// only class "taskKernel" is defined
			hsArray_[t].ptr_etk=ptrTaskObj_;
			hsArray_[t].ptr_wsq=&jobQArray_[t];
			hsArray_[t].ptr_busyThreads=ptrBusyThreads_;
			hsArray_[t].maxTp=t_num;

			for (int tt=0;tt<MAX_THREADS;tt++){
				hsArray_[t].ptr_wsq_array[tt]=ptrJobQArray_[tt];
			};

			rc = pthread_create(&threads_[t],
			                    &attr_,
			                    &listenToEnv,
			                    (void *)&hsArray_[t]);
			if (rc){
				printf("ERROR; return code from pthread_create() is %d\n", rc);
				exit(-1);
			}
			usleep(1000000); //if comment out this line, the thread launching mess up
			std::cout<<" In loop,after launch thread t="<<t<<" wait 1 s.\n";
		}
	}

	~ThreadPool()
	{
		poolDone_ = true;
	}

	void start()
	{
		poolStart_ = true;
	}

	bool isBusy() const { return (busyThreads_ != 0); }

	void finalize()
	{
		submissionDone_ = true;
		joiner();
	}

	void receive(ExternalKernelType* ptr_taskObj)
	{
		jobQArray_[currentQ_%tUsed_].push(ptr_taskObj);
		currentQ_ += 1;
	}

private:

	void joiner()
	{
		pthread_attr_destroy(&attr_);
		for (int t=0;t<threads_.size();t++) pthread_join(threads_[t], &status_);
	}

	static void *listenToEnv(void *thread_parameter)
	{
		HolderStructType* hs1 = static_cast<HolderStructType*>(thread_parameter);
		long tp = hs1->threadParameter;
		bool* pStart = hs1->ptr_poolStart;
		bool* sDone = hs1->ptr_submissionDone;
		ExternalKernelType* etk = hs1->ptr_etk;
		WorkStealingQType* wsq_array[MAX_THREADS];
		for (int tt=0; tt<MAX_THREADS; tt++ )wsq_array[tt]= hs1->ptr_wsq_array[tt];
		int* bThreads = hs1->ptr_busyThreads;
		int mTp = hs1->maxTp;

		std::stringstream strstream;
		strstream << tp;
		std::string str;

		int bt;
		bt=1;
		while ( !( *sDone) ){
			if (*pStart){
				if (wsq_array[tp]->empty()){ //race condition here
					*bThreads=*bThreads-bt;
					bt=0;
				}
				if (wsq_array[tp]->canPop(etk) ){
					etk->executeTask();
					str=strstream.str()+". thread a task: execute\n";
					std::cout<<str;
				} else {    // start stealing
					bool stolen(false);
					for (int ios=1;ios<mTp;ios++){
						stolen=wsq_array[(tp+ios)%mTp]->canSteal(etk);
						std::cout<<"stolen="<<stolen<<" tp="<<tp<<" ios="<<ios<<"\n";
						if (stolen) break; //break out from current loop if steal a job
					}
					if (stolen){
						etk->executeTask();
						str=strstream.str()+". thread a task: steal\n";
						std::cout<<str;
					} else {
						usleep(100000);
					}
				}
			} else {
				usleep(1000000);
			}

			usleep(1);
		}

		str=strstream.str()+" in listenToEnv submissionDone=true, poolStart=true\n";
		std::cout<<str;

		return 0;
	}

	VectorPthreadType threads_;
	HolderStructType hsArray_[MAX_THREADS];
	WorkStealingQType jobQArray_[MAX_THREADS];
	WorkStealingQType* ptrJobQArray_[MAX_THREADS];
	pthread_attr_t attr_;
	void* status_;
	bool poolStart_;
	bool poolDone_;
	bool jobDone_;
	bool submissionDone_;
	bool* ptrPoolStart_;
	bool* ptrSubmissionDone_;
	int currentQ_;
	int tUsed_;
	int busyThreads_;
	int *ptrBusyThreads_;
	//Here is the problem, the threadpool should not know how to
	// initialize the object, so the taskKernel constructer should not take argument.
	ExternalKernelType taskObj_;
	ExternalKernelType* ptrTaskObj_;
};

int main(int argc, char* argv[])
{
	if (argc < 3) {
		std::cerr<<"USAGE "<<argv[0]<<" job_num thread_num";
		std::cerr<<"SpareTire\n";
		return 1;
	}

	int job_num = atoi(argv[1]);
	int thread_num = atoi(argv[2]);
	int SpareTire = (argc == 4) ? atoi(argv[3]) : 1;
	std::cout<<"Pthread hello world example (job_num~30, thread_num~4)\n";
	std::cout<<"input: "<<job_num<<" "<<thread_num<<" "<<SpareTire<<"\n";

	TaskKernel etk1(1); //create a taskKernel object,
	TaskKernel etk10(10); //create a taskKernel object,

	ThreadPool<TaskKernel> TB1(thread_num);
	usleep(3000000);
	std::cout<<" wait 3 s to start all threads in threadpool.\n";

	for (int kk=0; kk<15; kk++) {
		TB1.receive(&etk1);
		TB1.receive(&etk10);
		TB1.receive(&etk1);
	}

	TB1.start();
	std::cout<<"reset ptr_taskObj pointer to point to realwork, ";
	std::cout<<"set poolStart=true to let pool starts running real job.\n";

	usleep(5000000);
	std::cout<<"after usleep(5000000),  before: TB1.submissionDone = true  \n";

	if (TB1.isBusy()) {
		std::cout<<"before usleep(2000000)\n";
		usleep(9000000);
	} else {
		std::cout<<"All threads finish, call TB1.joiner and pthread_exit below.  \n";
		TB1.finalize();
		pthread_exit(NULL);
	}
}

