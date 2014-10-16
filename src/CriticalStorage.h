#ifndef CRITICALSTORAGE_H
#define CRITICALSTORAGE_H
#include "Vector.h"
#include "Mpi.h"
#include <cassert>
#include "CriticalReal.h"
#include "ParallelTypeEnum.h"

namespace ConcurrencyPsi {

template<ParallelTypeEnum type, typename RealType>
class CriticalStorageImpl {

public:

	typedef CriticalReal<type,RealType> CriticalRealType;

	CriticalStorageImpl() {}

	~CriticalStorageImpl()
	{
		for (SizeType i = 0; i < values_.size(); ++i) {
			delete values_[i];
			values_[i] = 0;
		}
	}

	void push(RealType* v)
	{
		CriticalRealType* cr = new CriticalRealType(v);
		values_.push_back(cr);
	}

	void prepare(SizeType nthreads)
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->prepare(nthreads);
	}

	RealType& value(SizeType i, SizeType threadNum)
	{
		assert(values_.size() > i);
		return values_[i]->operator()(threadNum);
	}

	void sync()
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->sync();
	}

	template<typename MpiType>
	void sync(MpiType& mpi)
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->sync(mpi);
	}

private:

	CriticalStorageImpl(const CriticalStorageImpl&);

	CriticalStorageImpl& operator=(const CriticalStorageImpl& other);

	typename PsimagLite::Vector<CriticalRealType*>::Type values_;

}; // class CriticalStorageImpl

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

	void prepare(MpiType* mpi)
	{
		mpi_ = mpi;
	}

	void push(RealType* v)
	{
		csImpl_.push(v);
	}

	void sync()
	{
		assert(mpi_);
		csImpl_.sync(*mpi_);
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
}; // class CriticalStorage

} // namespace ConcurrencyPsi

#endif // CRITICALSTORAGE_H

