#ifndef CRITICALSTORAGEIMPL_H
#define CRITICALSTORAGEIMPL_H
#include <cassert>
#include "CriticalReal.h"

namespace ConcurrentPsi {

template<ParallelTypeEnum type, typename RealType>
class CriticalStorageImpl {

public:

	typedef CriticalReal<type,RealType> CriticalRealType;
	typedef typename CriticalRealType::ConcurrentPatternType ConcurrentPatternType;

	CriticalStorageImpl() : prepared_(false), synced_(false) {}

	~CriticalStorageImpl()
	{
		if (!synced_) {
			PsimagLite::String str("CriticalStorageImpl::dtor(): ");
			str += " An non-synced object is about to be destroyed.\n";
			str += "\tThis could indicate a client error. ";
			str += "(Did you forget to sync?)\n";
			std::cerr<<str;
		}

		for (SizeType i = 0; i < values_.size(); ++i) {
			delete values_[i];
			values_[i] = 0;
		}
	}

	void push(RealType* v, const ConcurrentPatternType& concurrentPattern)
	{
		if (prepared_) {
			PsimagLite::String str("CriticalStorageImpl::push(...): ");
			str += " Cannot push after prepare has been called\n";
			throw PsimagLite::RuntimeError(str);
		}

		CriticalRealType* cr = new CriticalRealType(v, concurrentPattern);
		values_.push_back(cr);
	}

	void prepare(SizeType nthreads)
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->prepare(nthreads);

		prepared_ = true;
	}

	void setPrepared()
	{
		prepared_ = true;
	}

	RealType& value(SizeType i, SizeType threadNum)
	{
		assert(values_.size() > i);
		assert(prepared_);
		return values_[i]->operator()(threadNum);
	}

	void sync()
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->sync();

		synced_ = true;
	}

	template<typename MpiType>
	void sync(MpiType& mpi, typename MpiType::CommType comm, bool doIt)
	{
		for (SizeType i = 0; i < values_.size(); ++i)
			values_[i]->sync(mpi, comm, doIt);

		synced_ = true;
	}

private:

	CriticalStorageImpl(const CriticalStorageImpl&);

	CriticalStorageImpl& operator=(const CriticalStorageImpl& other);

	bool prepared_;
	bool synced_;
	typename PsimagLite::Vector<CriticalRealType*>::Type values_;

}; // class CriticalStorageImpl

} //namespace ConcurrentPsi

#endif // CRITICALSTORAGEIMPL_H

