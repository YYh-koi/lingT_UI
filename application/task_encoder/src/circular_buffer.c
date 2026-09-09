#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "circular_buffer.h"
#include "cmsis_os.h"
#include "gplib_mm_gplus.h"

// The definition of our circular buffer structure is hidden from the user
struct circular_buf_t
{
	unsigned int* buffer;
	size_t head;
	size_t tail;
	size_t max; // of the buffer
	bool full;
};

//#pragma mark - Private Functions -

static inline size_t advance_headtail_value(size_t value, size_t max)
{
	return (value + 1) % max;
}

static void advance_head_pointer(cbuf_handle_t me)
{
	if(circular_buf_full(me))
	{
		me->tail = advance_headtail_value(me->tail, me->max);
	}

	me->head = advance_headtail_value(me->head, me->max);
	me->full = (me->head == me->tail);
}

//#pragma mark - APIs -

cbuf_handle_t circular_buf_init(unsigned int* buffer, size_t size)
{
	cbuf_handle_t cbuf = gp_malloc_align(sizeof(circular_buf_t), 64);

	//DBG_PRINT("-----------------------------circular_buf_init size %d\r\n",size);
		
	if(cbuf)
	{
		cbuf->buffer = buffer;
		cbuf->max = size;
		circular_buf_reset(cbuf);
	}

	return cbuf;
}

void circular_buf_free(cbuf_handle_t me)
{
	if(!me)
	{
		//DBG_PRINT("[%s]me is null\r\n",__FUNCTION__);
		return;
	}

	gp_free((void *)me);
}

void circular_buf_reset(cbuf_handle_t me)
{
	me->head = 0;
	me->tail = 0;
	me->full = false;
}

size_t circular_buf_size(cbuf_handle_t me)
{
	size_t size = 0;

	if(!me)
	{
		//DBG_PRINT("[%s]me is null\r\n",__FUNCTION__);
		return 0;
	}

	size = me->max;

	if(!circular_buf_full(me))
	{
		if(me->head >= me->tail)
		{
			size = (me->head - me->tail);
		}
		else
		{
			size = (me->max + me->head - me->tail);
		}
	}

	return size;
}

size_t circular_buf_capacity(cbuf_handle_t me)
{

	return me->max;
}

void circular_buf_put(cbuf_handle_t me, unsigned int data)
{
	if(!me)
	{
		//DBG_PRINT("[%s]me is null\r\n",__FUNCTION__);
		return;
	}

	me->buffer[me->head] = data;

	advance_head_pointer(me);
}

int circular_buf_try_put(cbuf_handle_t me, unsigned int data)
{
	int r = -1;

	if(!me)
	{
		//DBG_PRINT("[%s]me is null\r\n",__FUNCTION__);
		return r;
	}

	osENTER_CRITICAL();

	if(!circular_buf_full(me))
	{
		me->buffer[me->head] = data;
		advance_head_pointer(me);
		r = 0;
	}

	osEXIT_CRITICAL();

	return r;
}

int circular_buf_get(cbuf_handle_t me, unsigned int* data)
{
	int r = -1;

	if(!me)
	{
		//DBG_PRINT("[%s]me is null\r\n",__FUNCTION__);
		return r;
	}

	osENTER_CRITICAL();

	if(!circular_buf_empty(me))
	{
		*data = me->buffer[me->tail];
		me->buffer[me->tail] = 0;
		me->tail = advance_headtail_value(me->tail, me->max);
		me->full = false;
		r = 0;
	}

	osEXIT_CRITICAL();

	return r;
}

bool circular_buf_empty(cbuf_handle_t me)
{
	if(!me)
	{
		//DBG_PRINT("[%s]me is null\r\n",__FUNCTION__);
		return 0;
	}

	return (!circular_buf_full(me) && (me->head == me->tail));
}

bool circular_buf_full(cbuf_handle_t me)
{
	return me->full;
}

int circular_buf_peek(cbuf_handle_t me, unsigned int* data, unsigned int index)
{
	int r = -1;
	size_t pos = 0;

	if(!me)
	{
		//DBG_PRINT("[%s]me is null\r\n",__FUNCTION__);
		return r;
	}

	osENTER_CRITICAL();

	if(!circular_buf_empty(me))
	{
		pos = (me->tail + index) % me->max;
		*data = me->buffer[pos];
		r = 0;
	}

	osEXIT_CRITICAL();

	return r;
}

void circular_buf_set_drop_index(cbuf_handle_t me, unsigned int index)
{
	me->tail = (me->tail + index) % me->max;
}

