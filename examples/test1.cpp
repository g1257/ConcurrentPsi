#include <iostream>
#include "../src/Parallelizer.h"
#include "../src/CriticalStorage.h"

class KernelInner {

	typedef double RealType;

public:

	typedef ConcurrencyPsi::CriticalStorage<RealType> CriticalStorageType;

	KernelInner(SizeType totalInner)
	    : totalInner_(totalInner),
	      outerIndex_(0)
	{}

	void operator()(SizeType index,
	                SizeType threadNum,
	                CriticalStorageType& cs) const
	{
		int totalKernel=100;
		int bigNumber = 1000000;
		for (int j = 0; j < totalKernel; ++j) {
			for (int mm = 0; mm < bigNumber; mm++)
				cs.value(0,threadNum) += (index + 1.0)*(outerIndex_+1.0)*1e-6;
		}
	}

	SizeType size() const { return totalInner_; }

	void setOuter(SizeType outerIndex)
	{
		outerIndex_ = outerIndex;
	}

private:

	SizeType totalInner_;
	SizeType outerIndex_;
};

class KernelOuter {

	typedef ConcurrencyPsi::Parallelizer<ConcurrencyPsi::TYPE_SERIAL,
	                                     KernelInner> ParallelizerInnerType;
	typedef KernelInner::CriticalStorageType InnerStorageType;

public:

	typedef double RealType;
	typedef ConcurrencyPsi::CriticalStorage<RealType> CriticalStorageType;

	KernelOuter(SizeType totalOuter, SizeType totalInner, SizeType nthreads)
	    : totalOuter_(totalOuter),
	      kernelInner_(totalInner),
	      pInner_(kernelInner_,nthreads)
	{}

	void operator()(SizeType index, SizeType threadNum, CriticalStorageType& cs)
	{
		kernelInner_.setOuter(index);
		InnerStorageType storageInner;
		RealType tmp = 0;
		storageInner.push(&tmp);
		pInner_.launch(storageInner);
		pInner_.sync(storageInner);
		cs.value(0, threadNum) += tmp;
	}

	SizeType size() const { return totalOuter_; }

private:

	SizeType totalOuter_;
	KernelInner kernelInner_;
	ParallelizerInnerType pInner_;
};

int main(int argc, char* argv[])
{
	typedef ConcurrencyPsi::Parallelizer<ConcurrencyPsi::TYPE_MPI,
	                                     KernelOuter> ParallelizerOuterType;
	typedef KernelOuter::RealType RealType;

	if (argc < 4) {
		std::cerr<<"USAGE "<<argv[0]<<" totalOuter totalInner nthreads\n";
		return 1;
	}

	SizeType totalOuter = atoi(argv[1]);
	SizeType totalInner = atoi(argv[2]);
	SizeType nthreads = atoi(argv[3]);
	std::cout<<"nested loop example \n";
	KernelOuter kernelOuter(totalOuter, totalInner, nthreads);
	ParallelizerOuterType pOuter(kernelOuter,1,&argc,&argv);
	KernelOuter::CriticalStorageType cs;
	RealType tmp = 0;
	cs.push(&tmp);
	pOuter.launch(cs);

	pOuter.sync(cs);

	if (ParallelizerOuterType::canPrint())
		std::cout<<argv[0]<<" Final Result is "<<tmp<<"\n";
}

