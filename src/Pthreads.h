#ifndef PTHREADS_H
#define PTHREADS_H

#ifdef USE_PTHREADS
#include <pthread.h>
#endif

namespace ConcurrencyPsi {

class Pthreads {

public:

#ifndef USE_PTHREADS
	typedef DummyType PthreadType;
	typedef DummyType PthreadAttrType;
#else
	typedef pthread_t PthreadType;
	typedef pthread_attr_t PthreadAttrType;
#endif

	static int create(PthreadType *thread,
	           const PthreadAttrType *attr,
	           void *(*start_routine) (void *),
	           void *arg)
	{
#ifndef USE_PTHREADS
		noPthreads();
		return -1;
#else
		return pthread_create(thread,attr,start_routine,arg);
#endif
	}

	static int join(PthreadType thread, void **retval)
	{
#ifndef USE_PTHREADS
		noPthreads();
		return -1;
#else
		return pthread_join(thread,retval);
#endif
	}

private:

	static void noPthreads()
	{
		throw PsimagLite::RuntimeError("Please add -DUSE_PTHREADS to the Makefile\n");
	}
}; // class Pthreads

} // namespace ConcurrencyPsi

#endif // PTHREADS_H

