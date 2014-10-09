#ifndef PARALLELIZER_H
#define PARALLELIZER_H
#include "Vector.h"
#include "Mpi.h"
#include <cassert>

namespace ConcurrencyPsi {

enum ParallelTypeEnum {TYPE_SERIAL, TYPE_PTHREADS, TYPE_MPI};

template<int type,typename KernelType>
class Parallelizer {

	typedef typename KernelType::CriticalStorageType CriticalStorageType;

public:

	Parallelizer(KernelType& kernel,
	             int nPthreads = 1,
	             int* argcPtr = 0,
	             char*** argvPtr = 0)
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

	static bool canPrint() { return true; }

private:

	KernelType& kernel_;
}; // class Parallelizer

#ifndef USE_PTHREADS
template<typename KernelType>
class Parallelizer<TYPE_PTHREADS,KernelType> {

	typedef typename KernelType::CriticalStorageType CriticalStorageType;

public:

	Parallelizer(KernelType& kernel,
	             int nPthreads = 1,
	             int* argcPtr = 0,
	             char*** argvPtr = 0)
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

	Parallelizer(KernelType& kernel,
	             SizeType nPthreads,
	             int* argcPtr = 0,
	             char*** argvPtr = 0)
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
		cs.syncPthreads();
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

template<typename KernelType>
class Parallelizer<TYPE_MPI,KernelType> {

	typedef Mpi MpiType;
	typedef typename KernelType::CriticalStorageType CriticalStorageType;

public:

	Parallelizer(KernelType& kernel,
	             int nPthreads = 1,
	             int* argcPtr = 0,
	             char*** argvPtr = 0)
	    : kernel_(kernel)
	{
		if (!mpi_) mpi_ = new MpiType(argcPtr,argvPtr);
		refCounter_++;
	}

	~Parallelizer()
	{
		if (refCounter_ <= 1) {
			delete mpi_;
			mpi_ = 0;
			return;
		}

		refCounter_--;
	}

	void launch(CriticalStorageType& cs)
	{
		SizeType total = kernel_.size();

		assert(mpi_);
		SizeType procs = mpi_->size();
		SizeType block = static_cast<SizeType>(total/procs);
		if (total % procs !=0) block++;
		SizeType mpiRank = mpi_->rank();

		std::cerr<<"block = "<<block<<" mpiRank = "<<mpiRank<<" total = "<<total<<"\n";
		for (SizeType i = 0; i < block; ++i) {
			SizeType index = i + block*mpiRank;
			if (index >= total) continue;
			kernel_(index,cs.value(),cs);
		}
	}

	void sync(CriticalStorageType& cs)
	{
		cs.syncMpi(*mpi_);
	}

	static bool canPrint()
	{
		return (mpi_->rank() == 0);
	}

private:

	static MpiType* mpi_;
	static SizeType refCounter_;
	KernelType& kernel_;
}; // class Parallelizer

template<typename KernelType>
Mpi* Parallelizer<TYPE_MPI,KernelType>::mpi_ = 0;

template<typename KernelType>
SizeType Parallelizer<TYPE_MPI,KernelType>::refCounter_ = 0;

} // namespace ConcurrencyPsi

#endif // PARALLELIZER_H

