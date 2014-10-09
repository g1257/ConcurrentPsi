#ifndef CRITICALSTORAGE_H
#define CRITICALSTORAGE_H
#include "Vector.h"
#include <cassert>

namespace ConcurrencyPsi {

class CriticalStorage {

public:

	typedef double ValueType;

	CriticalStorage()
	    : value_(1,0.0),currentThread_(0)
	{}

	void prepareForPthreads(SizeType nthreads)
	{
		value_.resize(nthreads,0.0);
	}

	ValueType& value()
	{
		assert(value_.size() > 0);
		return value_[0];
	}

	ValueType& valueAtPthread(SizeType thread)
	{
		assert(thread < value_.size());
		return value_[thread];
	}

	void syncPthreads()
	{
		for (SizeType i = 1; i < value_.size(); ++i) {
			value_[0] += value_[i];
		}
	}

	template<typename MpiType>
	void syncMpi(MpiType& mpi)
	{
		mpi.sync(value_[0]);
	}

	void clear()
	{
		for (SizeType i = 0; i < value_.size(); ++i)
			value_[i] = 0.0;
	}

private:

	PsimagLite::Vector<ValueType>::Type value_;
	SizeType currentThread_;

}; // class CriticalStorage

} // namespace ConcurrencyPsi

#endif // CRITICALSTORAGE_H

