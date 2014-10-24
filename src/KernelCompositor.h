#ifndef KERNELCOMPOSITOR_H
#define KERNELCOMPOSITOR_H

namespace ConcurrentPsi {

template<typename Kernel1Type_, typename Kernel2Type_>
class KernelCompositor {

	template<typename StorageType>
	class InternalStorage {

		typedef typename Kernel1Type_::RealType RealType;

	public:

		InternalStorage(StorageType& cs, SizeType offset)
		    : cs_(cs),offset_(offset)
		{}

		RealType& value(SizeType storageIndex,SizeType threadNum)
		{
			return cs_.value(storageIndex + offset_,threadNum);
		}

	private:

		StorageType& cs_;
		SizeType offset_;
	};

public:

	typedef typename Kernel1Type_::RealType RealType;

	KernelCompositor(SizeType total1, SizeType total2, SizeType storage1)
	    : kernel1_(total1),kernel2_(total2),storage1_(storage1)
	{}

	SizeType size() const { return kernel1_.size() + kernel2_.size(); }

	template<typename SomeCriticalStorageType>
	void operator()(SizeType index,
	                SizeType threadNum,
	                SomeCriticalStorageType& cs) const
	{
		SizeType total1 = kernel1_.size();

		if (index < total1) {
			return kernel1_(index,threadNum,cs);
		} else {
			InternalStorage<SomeCriticalStorageType> cs2(cs,storage1_);
			return kernel2_(index - total1,threadNum,cs2);
		}
	}

private:

	Kernel1Type_ kernel1_;
	Kernel2Type_ kernel2_;
	SizeType storage1_;
}; // class KernelCompositor

} // namespace ConcurrentPsi

#endif // KERNELCOMPOSITOR_H
