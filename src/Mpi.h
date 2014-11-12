#ifndef MPI_H
#define MPI_H
#ifdef USE_MPI
#include <mpi.h>
#endif
#include <unistd.h>

namespace ConcurrentPsi {

class Mpi {

public:

#ifndef USE_MPI

	enum {MPI_DOUBLE, MPI_SUM};

	typedef int DummyType;
	typedef DummyType CommType;

	static CommType commWorld() { noMpi(); return 1; }

	static void MPI_Init(int*, char***) { noMpi(); }

	static void MPI_Finalize() { noMpi(); }

	static void MPI_Comm_rank(CommType, int*) { noMpi(); }

	static void MPI_Comm_size(CommType, int*) { noMpi(); }

	static void MPI_Barrier(CommType) { noMpi(); }

	static void MPI_Comm_split(CommType, int, int, CommType*)
	{
		noMpi();
	}

	template<typename T>
	void MPI_Reduce(T*,T*,int,int,int,int,CommType) const { noMpi(); }

#else
	typedef MPI_Comm CommType;

	static CommType commWorld()
	{
		return MPI_COMM_WORLD;
	}
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

	CommType split(int size, CommType comm, int option)
	{
		int me0 = rank(Mpi::commWorld());
		int me = rank(comm);
		int color = (option == 0) ? me/size : me % size;
		int key = 0;
		CommType newcomm1;
		MPI_Comm_split(comm, color, key, &newcomm1);
		int localrank = rank(newcomm1);
		std::cout<<"size= "<<size<<" me0="<<me0<<" localrank= "<<localrank;
		std::cout<<" me= "<<me<<" color="<<color<<" comm="<<comm<<" "<<newcomm1<<"\n";
		waitForPrinting();
		if (me0 == 0) {
			std::cout<<"--------------------\n";
			std::cout.flush();
		}

		sleep(1);
		return newcomm1;
	}

	static void waitForPrinting()
	{
		MPI_Barrier(commWorld());
		sleep(1);
	}

private:

	static void noMpi()
	{
		throw PsimagLite::RuntimeError("Please add -DUSE_MPI to the Makefile\n");
	}

	static bool init_;
}; // class Mpi

bool Mpi::init_ = false; // <--- FIXME: linkage in a header file

} // namespace ConcurrentPsi

#endif // MPI_H

