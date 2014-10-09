#ifndef MPI_H
#define MPI_H

namespace ConcurrencyPsi {

#ifndef USE_MPI
class Mpi {

public:

	Mpi(int* argcPtr, char*** argvPtr)
	{
		noMpi();
	}

	SizeType rank() const
	{
		noMpi();
		return 0;
	}

	SizeType size() const
	{
		noMpi();
		return 1;
	}

	void sync(double& v) const
	{
		noMpi();
	}

private:

	void noMpi() const
	{
		throw PsimagLite::RuntimeError("Please add -DUSE_MPI to the Makefile\n");
	}

	static bool init_;
}; // class Mpi
#else
#include <mpi.h>
class Mpi {

public:

	Mpi(int* argcPtr, char*** argvPtr)
	{
		if (init_) {
			std::cerr<<"Mpi class is a singleton\n";
			throw PsimagLite::RuntimeError("Refusing to create multiple objects\n");
		}

		MPI_Init(argcPtr, argvPtr);
		init_ = true;
	}

	~Mpi()
	{
		std::cerr<<"Calling finalize\n";
		MPI_Finalize();
	}

	SizeType rank() const
	{
		int rank = 0;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		return rank;
	}

	SizeType size() const
	{
		int MPIsize;
		MPI_Comm_size(MPI_COMM_WORLD, &MPIsize);
		return MPIsize;
	}

	void sync(double& v) const
	{
		double w = v;
		std::cerr<<"Calling sync with v = "<<v<<"\n";
		MPI_Reduce(&v,&w,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
		v = w;
	}

private:

	static bool init_;
}; // class Mpi

bool Mpi::init_ = false;
#endif

} // namespace ConcurrencyPsi

#endif // MPI_H

