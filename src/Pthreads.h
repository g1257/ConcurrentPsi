#ifndef PTHREADS_H
#define PTHREADS_H

#ifdef USE_PTHREADS
#include <pthread.h>
#endif

namespace ConcurrentPsi {

class Pthreads {

public:

#ifndef USE_PTHREADS
	typedef int DummyType;
	typedef DummyType PthreadType;
	typedef DummyType PthreadAttrType;
	static int pthread_create(PthreadType *,
	                           const PthreadAttrType*,
	                           void *(*) (void *),
	                           void *) { return 1; }
	static int pthread_join(PthreadType thread, void** retval) { return 1; }
#else
	typedef pthread_t PthreadType;
	typedef pthread_attr_t PthreadAttrType;
#endif

	static int create(PthreadType *thread,
	           const PthreadAttrType *attr,
	           void *(*start_routine) (void *),
	           void *arg)
	{
		checkForPthreadsSupport();
		return pthread_create(thread,attr,start_routine,arg);
	}

	static int join(PthreadType thread, void** retval)
	{
		checkForPthreadsSupport();
		return pthread_join(thread,retval);
	}

private:

	static void checkForPthreadsSupport()
	{
#ifndef USE_PTHREADS
		throw PsimagLite::RuntimeError("Please add -DUSE_PTHREADS to the Makefile\n");
#endif
	}
}; // class Pthreads

} // namespace ConcurrentPsi

#endif // PTHREADS_H

