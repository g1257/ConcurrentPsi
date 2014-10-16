#include <iostream>
#include "../src/Parallelizer.h"
#include "../src/CriticalStorage.h"

class KernelInner {

public:

	typedef double RealType;

	KernelInner(SizeType totalInner)
	    : totalInner_(totalInner),
	      outerIndex_(0)
	{}

	template<typename SomeCriticalStorageType>
	void operator()(SizeType index,
	                SizeType threadNum,
	                SomeCriticalStorageType& cs) const
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

	typedef ConcurrencyPsi::Parallelizer<ConcurrencyPsi::TYPE_PTHREADS,
	                                     KernelInner> ParallelizerInnerType;
	typedef ParallelizerInnerType::CriticalStorageType InnerStorageType;

public:

	typedef double RealType;

	KernelOuter(SizeType totalOuter, SizeType totalInner, SizeType nthreads)
	    : totalOuter_(totalOuter),
	      kernelInner_(totalInner),
	      pInner_(kernelInner_,nthreads)
	{}

	template<typename SomeCriticalStorageType>
	void operator()(SizeType index, SizeType threadNum, SomeCriticalStorageType& cs)
	{
		//kernelInner_.setOuter(index);
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
	typedef ConcurrencyPsi::Parallelizer<ConcurrencyPsi::TYPE_PTHREADS,
	                                     KernelOuter> ParallelizerOuterType;
	typedef KernelOuter::RealType RealType;

	if (argc < 4) {
		std::cerr<<"USAGE "<<argv[0]<<" totalOuter totalInner nthreadsInner ";
		std::cerr<<"[nthreadsOuter]\n";
		return 1;
	}

	SizeType totalOuter = atoi(argv[1]);
	SizeType totalInner = atoi(argv[2]);
	SizeType nthreads = atoi(argv[3]);
	SizeType outerPthreads = (argc == 5) ? atoi(argv[4]) : 1;
	std::cout<<"nested loop example \n";
	KernelOuter kernelOuter(totalOuter, totalInner, nthreads);
	ParallelizerOuterType pOuter(kernelOuter,outerPthreads,&argc,&argv);
	ParallelizerOuterType::CriticalStorageType cs;
	RealType tmp = 0;
	cs.push(&tmp);
	pOuter.launch(cs);

	pOuter.sync(cs);

	if (ParallelizerOuterType::canPrint())
		std::cout<<argv[0]<<" Final Result is "<<tmp<<"\n";
}

