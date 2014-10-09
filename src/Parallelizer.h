#ifndef PARALLELIZER_H
#define PARALLELIZER_H
#include "Vector.h"

namespace ConcurrencyPsi {

enum ParallelTypeEnum {TYPE_SERIAL, TYPE_PTHREADS, TYPE_MPI};

template<int type,typename KernelType>
class Parallelizer {

	typedef typename KernelType::CriticalStorageType CriticalStorageType;

public:

	Parallelizer(KernelType& kernel,int nPthreads = 0)
	    : kernel_(kernel)
	{}

	void launch(CriticalStorageType& cs)
	{
		SizeType total = kernel_.size();
		for (SizeType i = 0; i < total; ++i) {
			kernel_(i,cs.value(),cs);
		}
	}

	void sync(CriticalStorageType& cs)
	{
	}

private:

	KernelType& kernel_;
}; // class Parallelizer

#ifndef USE_PTHREADS
template<typename KernelType>
class Parallelizer<TYPE_PTHREADS,KernelType> {

	typedef typename KernelType::CriticalStorageType CriticalStorageType;

public:

	Parallelizer(KernelType& kernel, int nPthreads)
	{
		noPthreads();
	}

	void launch(CriticalStorageType& cs)
	{
		noPthreads();
	}

	void sync(CriticalStorageType& cs)
	{
		noPthreads();
	}

private:

	void noPthreads()
	{
		throw PsimagLite::RuntimeError("Please add -DUSE_PTHREADS to the Makefile\n");
	}
};

#else

#include <pthread.h>
template<typename KernelType>
class Parallelizer<TYPE_PTHREADS,KernelType> {

	typedef typename KernelType::CriticalStorageType CriticalStorageType;

	struct PthreadFunctionStruct {
		KernelType* kernel;
		SizeType blockSize;
		SizeType total;
		int threadNum;
		CriticalStorageType* criticalStorage;
	};

public:

	Parallelizer(KernelType& kernel, SizeType nPthreads)
	    : kernel_(kernel),nthreads_(nPthreads)
	{}

	void launch(CriticalStorageType& cs)
	{
		PthreadFunctionStruct pfs[nthreads_];
		pthread_t threadId[nthreads_];

		SizeType total = kernel_.size();

		cs.prepareForPthreads(nthreads_);

		for (SizeType j=0; j < nthreads_; j++) {
			int ret=0;
			pfs[j].threadNum = j;
			pfs[j].kernel = &kernel_;
			pfs[j].total = total;
			pfs[j].blockSize = total/nthreads_;
			if (total%nthreads_!=0) pfs[j].blockSize++;
			pfs[j].criticalStorage = &cs;
			if ((ret=pthread_create(&threadId[j],
			                        0,
			                        threadFunctionWrapper,
			                        &pfs[j])))
			std::cerr<<"Thread creation failed: "<<ret<<"\n";
		}

		for (SizeType j=0; j < nthreads_; j++) pthread_join(threadId[j], NULL);
	}

	void sync(CriticalStorageType& cs)
	{
		cs.sync();
	}

private:

	static void *threadFunctionWrapper(void *dummyPtr)
	{
		PthreadFunctionStruct* pfs = (PthreadFunctionStruct*) dummyPtr;

		KernelType *kernel = pfs->kernel;

		SizeType blockSize = pfs->blockSize;

		for (SizeType i = 0; i < blockSize; ++i) {
			SizeType index = i + pfs->threadNum*blockSize;
			if (index >= pfs->total) continue;
			kernel->operator()(index,
			                   pfs->criticalStorage->valueAtPthread(pfs->threadNum),
			                   *pfs->criticalStorage);
		}

		return 0;
	}

	KernelType& kernel_;
	SizeType nthreads_;
}; // class Parallelizer
#endif

} // namespace ConcurrencyPsi

#endif // PARALLELIZER_H

