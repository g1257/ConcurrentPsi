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

	class EmptyClass {};

protected:

	MpiHolder(int* argcPtr, char*** argvPtr)
	{}

	static EmptyClass& mpi()
	{
		return mpi_;
	}

private:

	static EmptyClass mpi_;
};

template<ParallelTypeEnum type>
typename MpiHolder<type>::EmptyClass MpiHolder<type>::mpi_;

template<>
class MpiHolder<TYPE_MPI> {

	typedef Mpi MpiType;

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

			return;
		}
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
};

Mpi* MpiHolder<TYPE_MPI>::mpi_ = 0;

} // namespace ConcurrencyPsi

#endif // MPIHOLDER_H

