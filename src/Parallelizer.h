#ifndef PARALLELIZER_H
#define PARALLELIZER_H
#include "Vector.h"
#include "Pthreads.h"
#include <cassert>
#include "CriticalStorage.h"
#include "TypeToString.h"
#include "MpiHolder.h"

namespace ConcurrentPsi {

template<ParallelTypeEnum type,typename KernelType>
class ParallelizerBase {

protected:

	typedef typename KernelType::RealType RealType;

public:

	typedef CriticalStorage<type,RealType> CriticalStorageType;

	static bool canPrint() { return true; }
};

template<typename KernelType>
class ParallelizerBase<TYPE_MPI, KernelType> : public MpiHolder {

protected:

	typedef typename KernelType::RealType RealType;

	ParallelizerBase(int* argcPtr, char*** argvPtr, SizeType mpiSizeArg)
	    : MpiHolder(argcPtr,argvPtr,mpiSizeArg)
	{}

public:

	typedef CriticalStorage<TYPE_MPI,RealType> CriticalStorageType;

};

template<ParallelTypeEnum type,typename KernelType>
class Parallelizer : public ParallelizerBase<type,KernelType> {

	typedef ParallelizerBase<type,KernelType> BaseType;
	typedef typename BaseType::CriticalStorageType CriticalStorageType;

public:

	Parallelizer(KernelType& kernel,
	             int n = 1,
	             int* argcPtr = 0,
	             char*** argvPtr = 0)
	    : kernel_(kernel)
	{}

	void launch(CriticalStorageType& cs)
	{
		SizeType total = kernel_.size();
		for (SizeType i = 0; i < total; ++i) {
			kernel_(i,0,cs);
		}
	}

private:

	KernelType& kernel_;
}; // class Parallelizer

template<typename KernelType>
class Parallelizer<TYPE_PTHREADS,KernelType>
        : public ParallelizerBase<TYPE_PTHREADS,KernelType> {

	typedef ParallelizerBase<TYPE_PTHREADS,KernelType> BaseType;
	typedef typename BaseType::CriticalStorageType CriticalStorageType_;

	struct PthreadFunctionStruct {
		PthreadFunctionStruct()
		    :  kernel(0), blockSize(0), total(0), threadNum(0), criticalStorage(0)
		{}

		KernelType* kernel;
		SizeType blockSize;
		SizeType total;
		int threadNum;
		CriticalStorageType_* criticalStorage;
	};

public:

	typedef CriticalStorageType_ CriticalStorageType;

	Parallelizer(KernelType& kernel,
	             SizeType nPthreads,
	             int* = 0,
	             char*** = 0)
	    : kernel_(kernel),nthreads_(nPthreads)
	{}

	void launch(CriticalStorageType& cs)
	{
		PthreadFunctionStruct pfs[nthreads_];
		Pthreads::PthreadType threadId[nthreads_];

		SizeType total = kernel_.size();

		cs.prepare(nthreads_);

		for (SizeType j=0; j < nthreads_; j++) {
			int ret=0;
			pfs[j].threadNum = j;
			pfs[j].kernel = &kernel_;
			pfs[j].total = total;
			pfs[j].blockSize = total/nthreads_;
			if (total%nthreads_!=0) pfs[j].blockSize++;
			pfs[j].criticalStorage = &cs;
			if ((ret=Pthreads::create(&threadId[j],0,threadFunctionWrapper,&pfs[j])))
				std::cerr<<"Thread creation failed: "<<ret<<"\n";
		}

		for (SizeType j=0; j < nthreads_; j++) Pthreads::join(threadId[j], 0);
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
			                   pfs->threadNum,
			                   *pfs->criticalStorage);
		}

		return 0;
	}

	KernelType& kernel_;
	SizeType nthreads_;
}; // class Parallelizer

template<typename KernelType>
class Parallelizer<TYPE_MPI,KernelType> : public ParallelizerBase<TYPE_MPI,KernelType> {

	typedef ParallelizerBase<TYPE_MPI,KernelType> BaseType;
	typedef Mpi MpiType;

public:

	typedef typename BaseType::CriticalStorageType CriticalStorageType;

	Parallelizer(KernelType& kernel,
	             int mpiSizeArg,
	             int* argcPtr = 0,
	             char*** argvPtr = 0)
	    : BaseType(argcPtr,argvPtr,mpiSizeArg),
	      kernel_(kernel),
	      mpiComm_(BaseType::currentGroup())
	{}

	void launch(CriticalStorageType& cs)
	{
		SizeType total = kernel_.size();

		std::cerr<<"Calling launch with comm="<<mpiComm_<<"\n";
		MpiType::waitForPrinting();

		cs.prepare(BaseType::mpiPtr(), mpiComm_);
		SizeType procs = BaseType::mpi().size(mpiComm_);
		SizeType block = static_cast<SizeType>(total/procs);
		if (total % procs !=0) block++;
		SizeType mpiRank = BaseType::mpi().rank(mpiComm_);

		for (SizeType i = 0; i < block; ++i) {
			SizeType index = i + block*mpiRank;
			if (index >= total) continue;
			kernel_(index,0,cs);
		}
	}

	static bool canPrint()
	{
		return (BaseType::mpi().rank(Mpi::commWorld()) == 0);
	}

private:

	Parallelizer(const Parallelizer&);

	Parallelizer& operator=(const Parallelizer& other);

	KernelType& kernel_;
	Mpi::CommType mpiComm_;
}; // class Parallelizer

} // namespace ConcurrentPsi

#endif // PARALLELIZER_H

