#ifndef MPIHOLDER_H
#define MPIHOLDER_H
#include "Mpi.h"
#include <cassert>
#include <cstdlib>
#include "Vector.h"
#include "String.h"

namespace ConcurrencyPsi {

class MpiHolder {

	typedef Mpi MpiType;
	typedef Mpi::CommType CommType;

protected:

	MpiHolder(int* argcPtr, char*** argvPtr, SizeType mpiSizeArg)
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

		mpiSizes_.push_back(mpiSizeArg);
		SizeType groupSize = groups_.size();
		assert(groupSize > 0);
		if (groupSize & 1) {
			split(mpiSizeArg);
			assert(groups_.size() == groupSize + 2);
		}
	}

	~MpiHolder()
	{
		SizeType groupSize = groups_.size();
		if (groupSize > 2) {
			groups_.pop_back();
			groups_.pop_back();
		}

		SizeType nested = mpiSizes_.size();
		if (nested > 0) {
			mpiSizeUsed_ /= mpiSizes_[nested-1];
			mpiSizes_.pop_back();
		}
	}

	static CommType currentGroup()
	{
		SizeType groupSize = groups_.size();
		assert(groupSize > 1);

		if (groupSize & 1)
			return groups_[groupSize];

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
	static PsimagLite::Vector<SizeType>::Type mpiSizes_;
};

Mpi* MpiHolder::mpi_ = 0; // FIXME: linkage in header

PsimagLite::Vector<Mpi::CommType>::Type MpiHolder::groups_; // FIXME: linkage in header

int MpiHolder::mpiSizeUsed_ = 1; // FIXME: linkage in header

PsimagLite::Vector<SizeType>::Type MpiHolder::mpiSizes_; // FIXME: linkage in header
} // namespace ConcurrencyPsi

#endif // MPIHOLDER_H

