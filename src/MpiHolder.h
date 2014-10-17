#ifndef MPIHOLDER_H
#define MPIHOLDER_H
#include "Mpi.h"
#include <cassert>
#include "ParallelTypeEnum.h"
#include <cstdlib>
#include "Vector.h"
#include "String.h"

namespace ConcurrencyPsi {

template<ParallelTypeEnum type>
class MpiHolder {

protected:

	MpiHolder(int* argcPtr, char*** argvPtr)
	{}
};

template<>
class MpiHolder<TYPE_MPI> {

	typedef Mpi MpiType;
	typedef Mpi::CommType CommType;

protected:

	MpiHolder(int* argcPtr, char*** argvPtr)
	{
		if (!mpi_) {
			mpi_ = new MpiType(argcPtr,argvPtr);
			int retCode = atexit(deleteMpi);
			if (retCode != 0) {
				PsimagLite::String str("MpiHolder::ctor() ");
				str += "atexit returned nonzero\n";
				throw PsimagLite::RuntimeError(str);
			}

			groups_.push_back(Mpi::commWorld());
			mpiSizeUsed_ = 1;
			return;
		}
	}

	CommType addGroup(SizeType mpiSizeArg)
	{
		SizeType groupSize = groups_.size();
		assert(groupSize > 0);
		if (groupSize & 1) {
			split(mpiSizeArg);
			assert(groups_.size() == groupSize + 2);
			return groups_[groupSize];
		}

		return groups_[groupSize-1];
	}

	void split(SizeType mpiSizeArg)
	{
		Mpi::CommType prevComm = groups_[groups_.size() - 1];
		if (mpiSizeArg == 0) mpiSizeArg = mpi_->size(prevComm);

		mpiSizeUsed_ *= mpiSizeArg;
		int mpiWorldSize = mpi_->size(Mpi::commWorld());
		if (mpiSizeUsed_ > mpiWorldSize) {
			PsimagLite::String str("MpiHolder<MPI>::split(...) ");
			str += " Not enough mpi processes. Available= "+ ttos(mpiWorldSize);
			str += " Requested (so far)= " + ttos(mpiSizeUsed_) + "\n";
			throw PsimagLite::RuntimeError(str);
		}

		if (mpiWorldSize % mpiSizeArg != 0) {
			PsimagLite::String str("MpiHolder<MPI>::split(...) ");
			str += " The MPI world size = "+ ttos(mpiWorldSize);
			str += " must be a multiple of this level= " + ttos(mpiSizeArg) + "\n";
			throw PsimagLite::RuntimeError(str);
		}

		CommType mpiComm = mpi_->split(mpiSizeArg,prevComm,0);
		groups_.push_back(mpiComm);

		assert(mpiWorldSize % mpiSizeUsed_ == 0);
		CommType mpiComm2 = mpi_->split(mpiSizeArg,prevComm,1);
		groups_.push_back(mpiComm2);
	}

	static MpiType& mpi()
	{
		assert(mpi_);
		return *mpi_;
	}

	static MpiType* mpiPtr() { return mpi_; }

private:

	static void deleteMpi()
	{
		delete mpi_;
		mpi_ = 0;
	}

	static MpiType* mpi_;
	static PsimagLite::Vector<Mpi::CommType>::Type groups_;
	static int mpiSizeUsed_;
};

Mpi* MpiHolder<TYPE_MPI>::mpi_ = 0;

PsimagLite::Vector<Mpi::CommType>::Type MpiHolder<TYPE_MPI>::groups_;

int MpiHolder<TYPE_MPI>::mpiSizeUsed_ = 1;

} // namespace ConcurrencyPsi

#endif // MPIHOLDER_H

