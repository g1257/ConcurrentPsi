#ifndef CRITICALSTORAGE_H
#define CRITICALSTORAGE_H
#include "Vector.h"
#include <cassert>
#include "CriticalReal.h"
#include "ParallelTypeEnum.h"

namespace ConcurrencyPsi {

template<ParallelTypeEnum type, typename RealType>
class CriticalStorage {

public:

	typedef CriticalReal<type,RealType> CriticalRealType;

	CriticalStorage() {}

	~CriticalStorage()
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

	CriticalStorage(const CriticalStorage&);

	CriticalStorage& operator=(const CriticalStorage& other );

	typename PsimagLite::Vector<CriticalRealType*>::Type values_;

}; // class CriticalStorage (serial)


} // namespace ConcurrencyPsi

#endif // CRITICALSTORAGE_H

