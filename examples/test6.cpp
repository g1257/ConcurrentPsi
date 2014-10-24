#include <iostream>
#include "../src/Parallelizer.h"
#include "../src/CriticalStorage.h"

class Kernel1 {

public:

	typedef double RealType;

	Kernel1(SizeType total)
	    : total1_(total)
	{}

	template<typename SomeCriticalStorageType>
	void operator()(SizeType index,
	                SizeType threadNum,
	                SomeCriticalStorageType& cs) const
	{
		int totalKernel=100;
		int bigNumber = 1e6;
		RealType sum = 0.0;
		for (int j = 0; j < totalKernel; ++j) {
			for (int mm = 0; mm < bigNumber; mm++)
				sum += (index + 1.0)*1e-6;
		}

		cs.value(0,threadNum) += sum;
	}

	SizeType size() const { return total1_; }

private:

	SizeType total1_;
};

class Kernel2 {

public:

	typedef double RealType;

	Kernel2(SizeType total)
	    : total2_(total)
	{}

	template<typename SomeCriticalStorageType>
	void operator()(SizeType index,
	                SizeType threadNum,
	                SomeCriticalStorageType& cs) const
	{
		int totalKernel=100;
		int bigNumber = 1e6;
		RealType sum = 0.0;
		for (int j = 0; j < totalKernel; ++j) {
			for (int mm = 0; mm < bigNumber; mm++)
				sum += (index + 1.0)*1e-6;
		}

		cs.value(0,threadNum) += sum;
	}

	SizeType size() const { return total2_; }

private:

	SizeType total2_;
};

int main(int argc, char* argv[])
{
	typedef ConcurrentPsi::Parallelizer<ConcurrentPsi::TYPE_PTHREADS,
	        Kernel1> Parallelizer1Type;
	typedef Kernel1::RealType RealType;
	typedef Parallelizer1Type::CriticalStorageType Storage1Type;
	typedef Storage1Type::ConcurrentPatternType ConcurrentPatternType;
	typedef ConcurrentPsi::Parallelizer<ConcurrentPsi::TYPE_PTHREADS,
	        Kernel2> Parallelizer2Type;
	typedef Parallelizer2Type::CriticalStorageType Storage2Type;

	if (argc < 4) {
		std::cerr<<"USAGE "<<argv[0]<<" total1 total2 nthreads1 ";
		std::cerr<<"[nthreads2]\n";
		return 1;
	}

	SizeType total1 = atoi(argv[1]);
	SizeType total2 = atoi(argv[2]);
	SizeType nthreads1 = atoi(argv[3]);
	SizeType nthreads2 = (argc == 5) ? atoi(argv[4]) : 1;
	std::cout<<"two loops in-tandem example \n";

	Kernel1 kernel1(total1);
	Parallelizer1Type p1(kernel1,nthreads1,&argc,&argv);
	Storage1Type cs1;
	ConcurrentPatternType pattern1(ConcurrentPatternType::PATTERN_REDUCE,
	                               ConcurrentPatternType::PATTERN_SUM);
	RealType tmp1 = 0;
	cs1.push(&tmp1, pattern1);
	p1.launch(cs1); // <-- LAUNCHER BLOCKS

	cs1.sync();

	if (Parallelizer1Type::canPrint())
		std::cout<<argv[0]<<" Result1 is "<<tmp1<<"\n";

	Kernel2 kernel2(total2);
	Parallelizer2Type p2(kernel2,nthreads2,&argc,&argv);
	Storage2Type cs2;
	ConcurrentPatternType pattern2(ConcurrentPatternType::PATTERN_REDUCE,
	                               ConcurrentPatternType::PATTERN_SUM);
	RealType tmp2 = 0;
	cs2.push(&tmp2, pattern2);
	p2.launch(cs2);

	cs2.sync();

	if (Parallelizer1Type::canPrint())
		std::cout<<argv[0]<<" Result2 is "<<tmp2<<"\n";
}

