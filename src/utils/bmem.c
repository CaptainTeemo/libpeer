//
//  bmem.c
//  CXXPlayground
//
//  Created by Keven on 2/5/24.
//  
//
	

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base.h"
#include "bmem.h"
#include "platform.h"
#include "threading.h"

/*
 * NOTE: totally jacked the mem alignment trick from ffmpeg, credit to them:
 *   http://www.ffmpeg.org/
 */

#define ALIGNMENT 32

/*
 * Attention, intrepid adventurers, exploring the depths of the libobs code!
 *
 * There used to be a TODO comment here saying that we should use memalign on
 * non-Windows platforms. However, since *nix/POSIX systems do not provide an
 * aligned realloc(), this is currently not (easily) achievable.
 * So while the use of posix_memalign()/memalign() would be a fairly trivial
 * change, it would also ruin our memory alignment for some reallocated memory
 * on those platforms.
 */
#if defined(_WIN32)
#define ALIGNED_MALLOC 1
#else
#define ALIGNMENT_HACK 1
#endif

static void *a_malloc(size_t size)
{
#ifdef ALIGNED_MALLOC
	return _aligned_malloc(size, ALIGNMENT);
#elif ALIGNMENT_HACK
	void *ptr = NULL;
	long diff;
	
	ptr = malloc(size + ALIGNMENT);
	if (ptr) {
		diff = ((~(long)ptr) & (ALIGNMENT - 1)) + 1;
		ptr = (char *)ptr + diff;
		((char *)ptr)[-1] = (char)diff;
	}
	
	return ptr;
#else
	return malloc(size);
#endif
}

static void *a_realloc(void *ptr, size_t size)
{
#ifdef ALIGNED_MALLOC
	return _aligned_realloc(ptr, size, ALIGNMENT);
#elif ALIGNMENT_HACK
	long diff;
	
	if (!ptr)
		return a_malloc(size);
	diff = ((char *)ptr)[-1];
	ptr = realloc((char *)ptr - diff, size + diff);
	if (ptr)
		ptr = (char *)ptr + diff;
	return ptr;
#else
	return realloc(ptr, size);
#endif
}

static void a_free(void *ptr)
{
#ifdef ALIGNED_MALLOC
	_aligned_free(ptr);
#elif ALIGNMENT_HACK
	if (ptr)
		free((char *)ptr - ((char *)ptr)[-1]);
#else
	free(ptr);
#endif
}

static long num_allocs = 0;

void *bmalloc(size_t size)
{
	if (!size) {
//		blog(LOG_ERROR,
//			 "bmalloc: Allocating 0 bytes is broken behavior, please "
//			 "fix your code! This will crash in future versions of "
//			 "OBS.");
		size = 1;
	}
	
	void *ptr = a_malloc(size);
	
	if (!ptr) {
//		os_breakpoint();
//		bcrash("Out of memory while trying to allocate %lu bytes",
//			   (unsigned long)size);
	}
	
	os_atomic_inc_long(&num_allocs);
	return ptr;
}

void *brealloc(void *ptr, size_t size)
{
	if (!ptr)
		os_atomic_inc_long(&num_allocs);
	
	if (!size) {
//		blog(LOG_ERROR,
//			 "brealloc: Allocating 0 bytes is broken behavior, please "
//			 "fix your code! This will crash in future versions of "
//			 "OBS.");
		size = 1;
	}
	
	ptr = a_realloc(ptr, size);
	
	if (!ptr) {
//		os_breakpoint();
//		bcrash("Out of memory while trying to allocate %lu bytes",
//			   (unsigned long)size);
	}
	
	return ptr;
}

void bfree(void *ptr)
{
	if (ptr) {
		os_atomic_dec_long(&num_allocs);
		a_free(ptr);
	}
}

long bnum_allocs(void)
{
	return num_allocs;
}

int base_get_alignment(void)
{
	return ALIGNMENT;
}

void *bmemdup(const void *ptr, size_t size)
{
	void *out = bmalloc(size);
	if (size)
		memcpy(out, ptr, size);
	
	return out;
}

OBS_DEPRECATED void base_set_allocator(struct base_allocator *defs)
{
	UNUSED_PARAMETER(defs);
}
