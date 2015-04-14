#include "../src/ParallelOutput.h"
#include <pthread.h> //for pthread_t
#include <cstdlib>     // atoi
#include <stdio.h>
#include <unistd.h>   // for usleep
#include <iostream>
#include <string>
#include <sstream>  // for convert number to string
#include <queue>    // for deque used in working stealing thread safe class
#include <cassert>

SizeType ParallelOutput::counter_ = 0;
pthread_mutex_t ParallelOutput::mutexCout_;
ParallelOutput globalParallelOutput;

class TaskKernel {

public:

	TaskKernel(int index): kernelIndex_(index){}

	~TaskKernel() {}

	void executeTask()
	{
		//		std::stringstream sstk;
		//		sstk << kernelIndex_;
		//		std::string str=sstk.str()+" is kernelIndex.\n";
		usleep(kernelIndex_*10000);
		//		std::cout<<str;
	}

private:

	int kernelIndex_;

};

template<typename ExternalKernelType>
class WorkStealingQ {

public:

	WorkStealingQ()
	{
		pthread_mutex_init(&mutexq_, 0);
	}

	~WorkStealingQ()
	{
		pthread_mutex_destroy(&mutexq_);
	}

	void push(ExternalKernelType* new_job)
	{
		pthread_mutex_lock(&mutexq_);
		jobDeque_.push_front(new_job);
		pthread_mutex_unlock(&mutexq_);
	}

	//pass in pointer res by reference, in our threadpool and workstealing Q,
	// we only deal with pointer.
	bool canPop(ExternalKernelType* res)
	{
		pthread_mutex_lock(&mutexq_);
		if (jobDeque_.empty()){
			//std::cout<<"In WorkStealingQ class, in can_pop, deque is empty\n";
			pthread_mutex_unlock(&mutexq_);
			return false;
		}

		res=jobDeque_.front();
		jobDeque_.pop_front();
		res->executeTask();
		pthread_mutex_unlock(&mutexq_);
		return true;
	}

	bool canSteal(ExternalKernelType* &res)
	{
		pthread_mutex_lock (&mutexq_);
		if (jobDeque_.empty()) {
			//std::cout<<"In WorkStealingQ class, in can_steal, deque is empty\n";
			pthread_mutex_unlock(&mutexq_);
			return false;
		}

		res=jobDeque_.back();
		jobDeque_.pop_back();
		pthread_mutex_unlock(&mutexq_);
		return true;
	}

	bool empty()
	{
		pthread_mutex_lock(&mutexq_);
		bool t = jobDeque_.empty();
		pthread_mutex_unlock(&mutexq_);
		return t;
	}

private:

	WorkStealingQ(const WorkStealingQ&);
	WorkStealingQ& operator=(const WorkStealingQ&);

	pthread_mutex_t mutexq_;
	std::deque<ExternalKernelType*> jobDeque_;
};

template<typename ExternalKernelType>
struct HolderStruct{

	HolderStruct(int numThreads) : ptrJobQArray_(numThreads)
	{}

	long threadParameter;
	bool* ptrPoolStart;
	ExternalKernelType* ptrEtk;
	std::vector<WorkStealingQ<ExternalKernelType>*> ptrJobQArray_;
	int* ptrBusyThreads;
	int maxTp;
};

template<typename ExternalKernelType>
class ThreadPool {

	typedef HolderStruct<ExternalKernelType> HolderStructType;
	typedef WorkStealingQ<ExternalKernelType> WorkStealingQType;
	typedef std::vector<pthread_t> VectorPthreadType;
	typedef std::vector<WorkStealingQType*> VectorWorkStealingQType;
	typedef std::vector<HolderStructType*> VectorHolderStructType;

public:

	ThreadPool(int numThreads)
	    : threads_(numThreads),
	      hsArray_(numThreads),
	      ptrJobQArray_(numThreads),
	      poolStart_(false),
	      jobDone_(false),
	      currentQ_(0),
	      tUsed_(numThreads),
	      busyThreads_(numThreads),
	      taskObj_(2)
	{
		pthread_attr_init(&attr_); // the attr may not be necessary
		pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_JOINABLE);

		for (int t=0;t<numThreads;t++)
			ptrJobQArray_[t] = new WorkStealingQType;

		for (int t=0;t<numThreads;t++) {
			std::cout<<" In threadpool loop,before launch thread t="<<t<<"\n";
			hsArray_[t] = new HolderStructType(numThreads);
			HolderStructType hsArray = *hsArray_[t];
			hsArray.threadParameter=t;
			hsArray.ptrPoolStart=&poolStart_;
			//Here is the problem, there is no object yet,
			// only class "taskKernel" is defined
			hsArray.ptrEtk=&taskObj_;
			hsArray.ptrBusyThreads=&busyThreads_;
			hsArray.maxTp=numThreads;

			for (int tt=0;tt<numThreads;tt++){
				hsArray.ptrJobQArray_[tt]=ptrJobQArray_[tt];
			};

			int rc = pthread_create(&threads_[t],
			                        &attr_,
			                        listenToEnv,
			                        (void *)&hsArray);
			if (rc){
				std::cout<<"pthread_create failed\n";
				perror(0);
				exit(-1);
			}

			usleep(100000); //if comment out this line, the thread launching mess up
			//std::cout<<" In loop,after launch thread t="<<t<<" wait 1 s.\n";
		}
	}

	~ThreadPool()
	{
		for (SizeType t=0;t<threads_.size();t++)
			pthread_join(threads_[t], 0);

		pthread_attr_destroy(&attr_);

		for (SizeType i = 0; i < ptrJobQArray_.size(); ++i) {
			delete ptrJobQArray_[i];
			ptrJobQArray_[i] = 0;
		}

		for (SizeType i = 0; i < hsArray_.size(); ++i) {
			delete hsArray_[i];
			hsArray_[i] = 0;
		}
	}

	void start()
	{
		poolStart_ = true;
	}

	bool isBusy() const { return (busyThreads_ != 0); }

	void receive(ExternalKernelType* ptr_taskObj)
	{
		int tmp = currentQ_%tUsed_;
		assert(tmp < ptrJobQArray_.size());
		ptrJobQArray_[tmp]->push(ptr_taskObj);
		currentQ_ += 1;
	}

private:

	ThreadPool(const ThreadPool&);

	ThreadPool& operator=(const ThreadPool&);

	static void *listenToEnv(void *)
	{
		int tp = 0;
		std::stringstream strstream;
		strstream << tp;
		std::string str;
		str=strstream.str()+". thread a task: execute\n";

		ParallelOutput::print(str);
		return 0;
	}

	VectorPthreadType threads_;
	VectorHolderStructType hsArray_;
	VectorWorkStealingQType ptrJobQArray_;
	pthread_attr_t attr_;
	bool poolStart_;
	bool jobDone_;
	int currentQ_;
	int tUsed_;
	int busyThreads_;
	//Here is the problem, the threadpool should not know how to
	// initialize the object, so the taskKernel constructer should not take argument.
	ExternalKernelType taskObj_;
	pthread_mutex_t mutexPoolStart_;
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
	usleep(300000);
	//std::cout<<" wait 3 s to start all threads in threadpool.\n";

	for (int kk=0; kk<15; kk++) {
		TB1.receive(&etk1);
		TB1.receive(&etk10);
		TB1.receive(&etk1);
	}

	TB1.start();

	//std::cout<<"All threads finish, call TB1.joiner and pthread_exit below.  \n";
}

