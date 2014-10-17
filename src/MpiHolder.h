#ifndef MPIHOLDER_H
#define MPIHOLDER_H
#include "Mpi.h"
#include <cassert>
#include <cstdlib>
#include "Vector.h"
#include "String.h"

namespace ConcurrentPsi {

class MpiHolder {

	typedef Mpi MpiType;
	typedef Mpi::CommType CommType;

//protected:
public:

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

			mpiSizeUsed_ = 1;
		}

		std::cerr<<"MpiHolder ctor called with "<<mpiSizeArg<<"\n";
		MpiType::waitForPrinting();
		split(mpiSizeArg);
		mpiSizes_.push_back(mpiSizeArg);
	}

	~MpiHolder()
	{
		SizeType groupSize = groups_.size();
		if (groupSize > 0) {
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
		if (groupSize == 0) {
			PsimagLite::String str("MpiHolder<MPI>::currentGroup(): ");
			str += " called outside a parallel loop\n";
			throw PsimagLite::RuntimeError(str);
		}

		std::cerr<<"currentGroup called when groupSize="<<groupSize<<" ";
		for (SizeType i = 0; i < groups_.size(); ++i)
			std::cerr<<groups_[i]<<" ";
		std::cerr<<"\n";
		MpiType::waitForPrinting();
		return groups_[groupSize - 1];
	}

	void split(SizeType mpiSizeArg)
	{
		Mpi::CommType prevComm = Mpi::commWorld();
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

		bool groupOdd = (groups_.size() & 1);

		if (groupOdd)
			assert(mpiSizes_.size() > 0);

		int option = (groupOdd) ? 1 : 0;
		int mpiSizeArg2 = (groupOdd) ? mpiSizes_[mpiSizes_.size()-1] : mpiSizeArg;
		CommType mpiComm = mpi_->split(mpiSizeArg2,prevComm,option);
		groups_.push_back(mpiComm);
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
} // namespace ConcurrentPsi

#endif // MPIHOLDER_H

