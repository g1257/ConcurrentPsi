#include <iostream>
#include "../src/Parallelizer.h"
#include "../src/CriticalStorage.h"

class KernelInner {

public:

	typedef ConcurrencyPsi::CriticalStorage CriticalStorageType;
	typedef CriticalStorageType::ValueType ValueType;

	KernelInner(SizeType totalInner)
	    : totalInner_(totalInner),
	      outerIndex_(0)
	{}

	void operator()(SizeType index,
	                ValueType& v,
	                CriticalStorageType& cs) const
	{
		int totalKernel=100;
		int bigNumber = 1000000;
		for (int j = 0; j < totalKernel; ++j) {
			for (int mm = 0; mm < bigNumber; mm++)
				v += (index + 1.0)*(outerIndex_+1.0)*1e-6;
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
	typedef KernelInner::CriticalStorageType InnerStorageType;

public:

	typedef ConcurrencyPsi::CriticalStorage CriticalStorageType;
	typedef CriticalStorageType::ValueType ValueType;

	KernelOuter(SizeType totalOuter, SizeType totalInner, SizeType nthreads)
	    : totalOuter_(totalOuter),
	      kernelInner_(totalInner),
	      pInner_(kernelInner_,nthreads)
	{}

	void operator()(SizeType index, ValueType& v, CriticalStorageType& cs)
	{
		//kernelInner_.setOuter(index);
		InnerStorageType storageInner;
		pInner_.launch(storageInner);
		pInner_.sync(storageInner);
		v += storageInner.value();
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
	if (argc < 4) {
		std::cerr<<"USAGE "<<argv[0]<<" totalOuter totalInner nthreads\n";
		return 1;
	}

	SizeType totalOuter = atoi(argv[1]);
	SizeType totalInner = atoi(argv[2]);
	SizeType nthreads = atoi(argv[3]);
	SizeType outerPthreads = (argc == 5) ? atoi(argv[4]) : 1;
	std::cout<<"nested loop example \n";
	KernelOuter kernelOuter(totalOuter, totalInner, nthreads);
	ParallelizerOuterType pOuter(kernelOuter,outerPthreads,&argc,&argv);
	KernelOuter::CriticalStorageType cs;
	pOuter.launch(cs);

	pOuter.sync(cs);

	if (ParallelizerOuterType::canPrint())
		std::cout<<argv[0]<<" Final Result is "<<cs.value()<<"\n";
}

