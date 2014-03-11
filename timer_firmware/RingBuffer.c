#include "main.h"
#include "RingBuffer.h"

RingBuffer* rbInit(uint32_t size)
{
	RingBuffer *rb = (RingBuffer*)malloc(sizeof(RingBuffer));
	rb->buf   = (uint8_t*)malloc(size);
	rb->size  = size;
	rb->start = 0;
	rb->end   = 0;
	return rb;
}

void rbFree(RingBuffer *rb)
{
	free(rb->buf);
	free(rb);
}

bool rbIsEmpty(RingBuffer *rb)
{
	return rb->start == rb->end;
}

bool rbIsFull(RingBuffer *rb)
{
	return (rb->end + 1) % rb->size == rb->start;
}

void rbWrite(RingBuffer *rb, uint8_t ch)
{
	rb->buf[rb->end] = ch;
	rb->end = (rb->end + 1) % rb->size;
}

uint8_t rbRead(RingBuffer *rb)
{
	uint8_t ch = rb->buf[rb->start];
	rb->start = (rb->start + 1) % rb->size;
	return ch;
}
