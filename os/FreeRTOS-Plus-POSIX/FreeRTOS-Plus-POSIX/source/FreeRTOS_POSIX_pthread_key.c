/*
 * Amazon FreeRTOS POSIX V1.1.0
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file FreeRTOS_POSIX_pthread_key.c
 * @brief Implementation of thread key functions in pthread.h
 */

/* C standard library includes. */
#include <stddef.h>
#include <string.h>

/* FreeRTOS+POSIX includes. */
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/errno.h"
#include "FreeRTOS_POSIX/pthread.h"
#include <atomic.h>

/* Table of the key information.  */
struct pthread_key_struct __pthread_keys[PTHREAD_KEYS_MAX];

/*-----------------------------------------------------------*/

int pthread_key_create (pthread_key_t *key, void (*destr) (void *))
{
	size_t cnt;
	/* Find a slot in __pthread_keys which is unused.  */
	for (cnt = 0; cnt < PTHREAD_KEYS_MAX; ++cnt)
    {
		unsigned int seq = __pthread_keys[cnt].seq;
		if (KEY_UNUSED (seq) && KEY_USABLE (seq)
          /* We found an unused slot.  Try to allocate it.  */
          && ! Atomic_CompareAndSwap_u32 (&__pthread_keys[cnt].seq,
                                                     seq + 1, seq))
        {
			/* Remember the destructor.  */
			__pthread_keys[cnt].destr = destr;
			/* Return the key to the caller.  */
			*key = cnt;
			/* The call succeeded.  */
			return 0;
        }
    }
  return EAGAIN;
}

/*-----------------------------------------------------------*/

int pthread_key_delete (pthread_key_t key)
{
	int result = EINVAL;
	if (key < PTHREAD_KEYS_MAX)
    {
		unsigned int seq = __pthread_keys[key].seq;
		if ( (! KEY_UNUSED (seq), 1)
			&& ! Atomic_CompareAndSwap_u32 (&__pthread_keys[key].seq,
                                                     seq + 1, seq))
		{
			/* We deleted a valid key.  */
			result = 0;
		}
	}
	return result;
}

/*-----------------------------------------------------------*/

void *pthread_getspecific (pthread_key_t key)
{
	struct pthread_key_data *data;
	void* result;
	/* Special case access to the first 2nd-level block.  This is the
		usual case.  */
	if (key < PTHREAD_KEY_2NDLEVEL_SIZE)
		//data = pthread_self()->specific_1stblock[key];
		data =
	else
    {
		unsigned int idx1st, idx2nd;
		struct pthread_key_data *level2

		/* Verify the key is sane.  */
		if (key >= PTHREAD_KEYS_MAX)
			/* Not valid.  */
			return NULL;
		idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
		idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;
		/* If the sequence number doesn't match or the key cannot be defined
			for this thread since the second level array is not allocated
			return NULL, too.  */
		level2 = THREAD_GETMEM_NC (THREAD_SELF, specific, idx1st);
		if (level2 == NULL)
			/* Not allocated, therefore no data.  */
			return NULL;
		/* There is data.  */
		data = &level2[idx2nd];
    }
	result = data->data;
	if (result != NULL)
    {
		unsigned int seq = data->seq;
		if (seq != __pthread_keys[key].seq)
			result = data->data = NULL;
    }
	return result;
}
