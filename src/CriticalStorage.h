#ifndef CRITICALSTORAGE_H
#define CRITICALSTORAGE_H
#include "Vector.h"
#include "Mpi.h"
#include <cassert>
#include "ParallelTypeEnum.h"
#include "CriticalStorageImpl.h"

namespace ConcurrencyPsi {

template<ParallelTypeEnum type, typename RealType>
class CriticalStorage {

	typedef CriticalStorageImpl<type,RealType> CriticalStorageImplType;

public:

	CriticalStorage()
	    : csImpl_()
	{}

	void sync()
	{
		csImpl_.sync();
	}

	void push(RealType* v)
	{
		csImpl_.push(v);
	}

	void prepare(SizeType nthreads)
	{
		csImpl_.prepare(nthreads);
	}

	RealType& value(SizeType i, SizeType threadNum)
	{
		return csImpl_.value(i, threadNum);
	}

private:

	CriticalStorage(const CriticalStorage&);

	CriticalStorage& operator=(const CriticalStorage& other);

	CriticalStorageImplType csImpl_;
}; // class CriticalStorage

template<typename RealType>
class CriticalStorage<TYPE_MPI,RealType> {

	typedef Mpi MpiType;
	typedef CriticalStorageImpl<TYPE_MPI,RealType> CriticalStorageImplType;

public:

	CriticalStorage()
	    : csImpl_(),mpi_(0)
	{}

	void prepare(MpiType* mpi, Mpi::CommType comm)
	{
		mpi_ = mpi;
		comm_ = comm;
		csImpl_.setPrepared();
	}

	void push(RealType* v)
	{
		csImpl_.push(v);
	}

	void sync(bool doIt = true)
	{
		assert(mpi_);
		csImpl_.sync(*mpi_, comm_,doIt);
	}

	RealType& value(SizeType i, SizeType threadNum)
	{
		assert(threadNum == 0);
		return csImpl_.value(i, threadNum);
	}

private:

	CriticalStorage(const CriticalStorage&);

	CriticalStorage& operator=(const CriticalStorage& other);

	CriticalStorageImplType csImpl_;
	MpiType* mpi_;
	Mpi::CommType comm_;
}; // class CriticalStorage (mpi)

} // namespace ConcurrencyPsi

#endif // CRITICALSTORAGE_H

