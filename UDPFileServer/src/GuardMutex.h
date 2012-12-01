/*
 * GuardMutex.h
 *
 *  Created on: Nov 24, 2012
 *      Author: codell
 */

#ifndef GUARDMUTEX_H_
#define GUARDMUTEX_H_

#include <pthread.h>

class GuardMutex {
public:
	GuardMutex(pthread_mutex_t *mutex) {
		_mutex = mutex;
		pthread_mutex_lock(_mutex);
	}
	virtual ~GuardMutex() {
		pthread_mutex_unlock(_mutex);
	}
private:
	pthread_mutex_t *_mutex;
};

#endif /* GUARDMUTEX_H_ */
