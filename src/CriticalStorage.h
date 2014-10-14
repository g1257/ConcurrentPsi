#ifndef CRITICALSTORAGE_H
#define CRITICALSTORAGE_H
#include "Vector.h"
#include <cassert>
#include "CriticalReal.h"

namespace ConcurrencyPsi {

template<typename RealType>
class CriticalStorage {

public:

	typedef CriticalReal<RealType> CriticalRealType;

	CriticalStorage()
	{}

	~CriticalStorage()
	{
		for (SizeType i = 0; i < values_.size(); ++i) {
			delete values_[i];
			values_[i] = 0;
		}
	}

	void push(RealType* v)
	{
		CriticalRealType* cr = new CriticalReal<RealType>(v);
		values_.push_back(cr);
	}

	void prepareForPthreads(SizeType nthreads)
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->prepareForPthreads(nthreads);
	}

	RealType& value(SizeType i, SizeType threadNum)
	{
		assert(values_.size() > i);
		return values_[i]->operator()(threadNum);
	}

	void syncPthreads()
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->syncPthreads();
	}

	void syncSerial()
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->syncSerial();
	}

	template<typename MpiType>
	void syncMpi(MpiType& mpi)
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->syncMpi(mpi);
	}

private:

	CriticalStorage(const CriticalStorage&);

	CriticalStorage& operator=(const CriticalStorage& other );

	typename PsimagLite::Vector<CriticalRealType*>::Type values_;

}; // class CriticalStorage

} // namespace ConcurrencyPsi

#endif // CRITICALSTORAGE_H

