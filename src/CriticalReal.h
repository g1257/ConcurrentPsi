#ifndef CRITICALREAL_H
#define CRITICALREAL_H
#include "Vector.h"
#include "ParallelTypeEnum.h"

namespace ConcurrencyPsi {

template<ParallelTypeEnum type, typename RealType>
class CriticalReal {

public:

	CriticalReal(RealType* v)
	    : vpointer_(v),values_(1,0.0)
	{
		values_[0] = *vpointer_;
	}

	const RealType& operator()() const
	{
		return *vpointer_;
	}

	const RealType& operator()(SizeType threadNum) const
	{
		assert(threadNum < values_.size());
		return values_[threadNum];
	}

	RealType& operator()(SizeType threadNum)
	{
		assert(threadNum < values_.size());
		return values_[threadNum];
	}

	void prepare(SizeType nthreads)
	{
		if (nthreads <= 1) return;
		assert(vpointer_);
		values_.resize(nthreads,0.0);
		for (SizeType i = 0; i< values_.size(); ++i)
			values_[i] = *vpointer_;
	}

	void sync()
	{
		assert(values_.size() > 0);
		for (SizeType i = 1; i < values_.size(); ++i) {
			values_[0] += values_[i];
		}

		*vpointer_ = values_[0];
	}

	template<typename MpiType>
	void sync(MpiType& mpi, typename MpiType::CommType comm)
	{
		assert(values_.size() > 0);
		mpi.sync(values_[0], comm);

		*vpointer_ = values_[0];
	}

private:

	CriticalReal(const CriticalReal&);

	CriticalReal& operator=(const CriticalReal& other);

	RealType* vpointer_;
	typename PsimagLite::Vector<RealType>::Type values_;

}; // class CriticalReal

} // namespace ConcurrencyPsi

#endif // CRITICALREAL_H

