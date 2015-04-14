#ifndef PARALLELLOUTPUT_H
#define PARALLELLOUTPUT_H
#include <string>
#include <iostream>

typedef unsigned int SizeType;

class ParallelOutput {

public:

	ParallelOutput()
	{
		if (counter_ > 0) return;
		counter_ = 1;
		pthread_mutex_init(&mutexCout_, 0);
	}

	~ParallelOutput()
	{
		counter_ = 0;
		pthread_mutex_destroy(&mutexCout_);
	}

	static void print(std::string str)
	{
		pthread_mutex_lock(&mutexCout_);
		std::cout<<str;
		pthread_mutex_unlock(&mutexCout_);
	}

private:

	static SizeType counter_;
	static pthread_mutex_t mutexCout_;
};

#endif // PARALLELLOUTPUT_H

