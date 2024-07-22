//
//  threading.h
//  CXXPlayground
//
//  Created by Keven on 2/5/24.
//
//


#ifndef threading_h
#define threading_h

#pragma once

/*
 *   Allows posix thread usage on windows as well as other operating systems.
 * Use this header if you want to make your code more platform independent.
 *
 *   Also provides a custom platform-independent "event" handler via
 * pthread conditional waits.
 */

#include "c99defs.h"

#ifndef _MSC_VER
#include <errno.h>
#endif
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
		
	static inline long os_atomic_inc_long(volatile long *val)
	{
		return __atomic_add_fetch(val, 1, __ATOMIC_SEQ_CST);
	}
	
	static inline long os_atomic_dec_long(volatile long *val)
	{
		return __atomic_sub_fetch(val, 1, __ATOMIC_SEQ_CST);
	}
	
	static inline void os_atomic_store_long(volatile long *ptr, long val)
	{
		__atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
	}
	
	static inline long os_atomic_set_long(volatile long *ptr, long val)
	{
		return __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
	}
	
	static inline long os_atomic_exchange_long(volatile long *ptr, long val)
	{
		return os_atomic_set_long(ptr, val);
	}
	
	static inline long os_atomic_load_long(const volatile long *ptr)
	{
		return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
	}
	
	static inline bool os_atomic_compare_swap_long(volatile long *val, long old_val,
												   long new_val)
	{
		return __atomic_compare_exchange_n(val, &old_val, new_val, false,
										   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	}
	
	static inline bool os_atomic_compare_exchange_long(volatile long *val,
													   long *old_val, long new_val)
	{
		return __atomic_compare_exchange_n(val, old_val, new_val, false,
										   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	}
	
	static inline void os_atomic_store_bool(volatile bool *ptr, bool val)
	{
		__atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
	}
	
	static inline bool os_atomic_set_bool(volatile bool *ptr, bool val)
	{
		return __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
	}
	
	static inline bool os_atomic_exchange_bool(volatile bool *ptr, bool val)
	{
		return os_atomic_set_bool(ptr, val);
	}
	
	static inline bool os_atomic_load_bool(const volatile bool *ptr)
	{
		return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
	}

	
	/* this may seem strange, but you can't use it unless it's an initializer */
	static inline void pthread_mutex_init_value(pthread_mutex_t *mutex)
	{
		pthread_mutex_t init_val = PTHREAD_MUTEX_INITIALIZER;
		if (!mutex)
			return;
		
		*mutex = init_val;
	}
	
	static inline int pthread_mutex_init_recursive(pthread_mutex_t *mutex)
	{
		pthread_mutexattr_t attr;
		int ret = pthread_mutexattr_init(&attr);
		if (ret == 0) {
			ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			if (ret == 0) {
				ret = pthread_mutex_init(mutex, &attr);
			}
			
			pthread_mutexattr_destroy(&attr);
		}
		
		return ret;
	}
	
	enum os_event_type {
		OS_EVENT_TYPE_AUTO,
		OS_EVENT_TYPE_MANUAL,
	};
	
	struct os_event_data;
	struct os_sem_data;
	typedef struct os_event_data os_event_t;
	typedef struct os_sem_data os_sem_t;
	
	EXPORT int os_event_init(os_event_t **event, enum os_event_type type);
	EXPORT void os_event_destroy(os_event_t *event);
	EXPORT int os_event_wait(os_event_t *event);
	EXPORT int os_event_timedwait(os_event_t *event, unsigned long milliseconds);
	EXPORT int os_event_try(os_event_t *event);
	EXPORT int os_event_signal(os_event_t *event);
	EXPORT void os_event_reset(os_event_t *event);
	
	EXPORT int os_sem_init(os_sem_t **sem, int value);
	EXPORT void os_sem_destroy(os_sem_t *sem);
	EXPORT int os_sem_post(os_sem_t *sem);
	EXPORT int os_sem_wait(os_sem_t *sem);
	
	EXPORT void os_set_thread_name(const char *name);
	
#ifdef _MSC_VER
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif
	
#ifdef __cplusplus
}
#endif


#endif /* threading_h */
