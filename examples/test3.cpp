#include <iostream>
#include "../src/Parallelizer.h"
#include "../src/CriticalStorage.h"

class KernelInner {

public:

	typedef double RealType;

	KernelInner(SizeType totalInner)
	    : totalInner_(totalInner)
	{}

	template<typename SomeCriticalStorageType>
	void operator()(SizeType index,
	                SizeType threadNum,
	                SomeCriticalStorageType& cs) const
	{
		int totalKernel=100;
		int bigNumber = 10000;
		RealType outerIndex = cs.value(1,threadNum);
		RealType sum = 0.0;
		for (int j = 0; j < totalKernel; ++j) {
			for (int mm = 0; mm < bigNumber; mm++)
				sum += (index + 1.0)*(outerIndex+1.0)*1e-6;
		}

		cs.value(0,threadNum) += sum;
	}

	SizeType size() const { return totalInner_; }

private:

	SizeType totalInner_;
};

class KernelOuter {

	typedef ConcurrentPsi::Parallelizer<ConcurrentPsi::TYPE_PTHREADS,
	                                     KernelInner> ParallelizerInnerType;
	typedef ParallelizerInnerType::CriticalStorageType InnerStorageType;
	typedef InnerStorageType::ConcurrentPatternType ConcurrentPatternType;

public:

	typedef double RealType;

	KernelOuter(SizeType totalOuter, SizeType totalInner, SizeType nthreads)
	    : totalOuter_(totalOuter),
	      kernelInner_(totalInner),
	      pInner_(kernelInner_,nthreads),
	      innerPattern_(ConcurrentPatternType::PATTERN_REDUCE,
	                    ConcurrentPatternType::PATTERN_SUM)
	{}

	template<typename SomeCriticalStorageType>
	void operator()(SizeType index, SizeType threadNum, SomeCriticalStorageType& cs)
	{
		InnerStorageType storageInner;
		RealType tmp = 0;
		storageInner.push(&tmp, innerPattern_);
		// We can't mark a SizeType as critical yet, only reals, so we must convert
		RealType index2 = index;
		storageInner.push(&index2, innerPattern_);
		pInner_.launch(storageInner);
		storageInner.sync();
		cs.value(0, threadNum) += tmp;
	}

	SizeType size() const { return totalOuter_; }

private:

	SizeType totalOuter_;
	KernelInner kernelInner_;
	ParallelizerInnerType pInner_;
	ConcurrentPatternType innerPattern_;
};

int main(int argc, char* argv[])
{
	typedef ConcurrentPsi::Parallelizer<ConcurrentPsi::TYPE_PTHREADS,
	                                     KernelOuter> ParallelizerOuterType;
	typedef KernelOuter::RealType RealType;
	typedef ParallelizerOuterType::CriticalStorageType OuterStorageType;
	typedef OuterStorageType::ConcurrentPatternType ConcurrentPatternType;

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
	OuterStorageType cs;
	ConcurrentPatternType outerPattern(ConcurrentPatternType::PATTERN_REDUCE,
	                                   ConcurrentPatternType::PATTERN_SUM);
	RealType tmp = 0;
	cs.push(&tmp, outerPattern);
	pOuter.launch(cs);

	cs.sync();

	if (ParallelizerOuterType::canPrint())
		std::cout<<argv[0]<<" Final Result is "<<tmp<<"\n";
}

