#ifndef MPI_H
#define MPI_H
#ifdef USE_MPI
#include <mpi.h>
#endif

namespace ConcurrencyPsi {

class Mpi {

public:

#ifndef USE_MPI

	enum {COMM_WORLD, MPI_DOUBLE, MPI_SUM};

	typedef int DummyType;
	typedef DummyType CommType;

	void MPI_Init(int* argcPtr, char*** argvPtr) { noMpi(); }
	void MPI_Finalize() { noMpi(); }
	void MPI_Comm_rank(CommType comm, int* rank) const { noMpi(); }
	void MPI_Comm_size(CommType comm, int* rank) const { noMpi(); }
	void MPI_Comm_split(CommType comm, int color, int key, CommType* newcomm1)
	{
		noMpi();
	}

	template<typename T>
	void MPI_Reduce(T* v,T* w,int,int,int,int,CommType) const { noMpi(); }

#else
	typedef MPI_Comm CommType;
	CommType COMM_WORLD = MPI_COMM_WORLD;
#endif

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
		MPI_Finalize();
	}

	SizeType rank(CommType comm) const
	{
		int rank = 0;
		MPI_Comm_rank(comm, &rank);
		return rank;
	}

	SizeType size(CommType comm) const
	{
		int MPIsize = 0;
		MPI_Comm_size(comm, &MPIsize);
		return MPIsize;
	}

	void sync(double& v, CommType comm) const
	{
		double w = v;
		MPI_Reduce(&v,&w,1,MPI_DOUBLE,MPI_SUM,0,comm);
		v = w;
	}

	CommType split(int size, CommType comm)
	{
		int me = rank(comm);
		int color = me % size;
		int key = 0;
		CommType newcomm1;
		MPI_Comm_split(comm, color, key, &newcomm1);
		return newcomm1;
	}

private:

	void noMpi() const
	{
		throw PsimagLite::RuntimeError("Please add -DUSE_MPI to the Makefile\n");
	}

	static bool init_;
}; // class Mpi

bool Mpi::init_ = false; // <--- FIXME: linkage in a header file


} // namespace ConcurrencyPsi

#endif // MPI_H

