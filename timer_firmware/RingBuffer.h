#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

typedef struct {
	uint8_t *buf;
	uint32_t size, start, end;
} RingBuffer;

RingBuffer* rbInit(uint32_t size);
void		rbFree(RingBuffer *rb);
bool 		rbIsEmpty(RingBuffer *rb);
bool 		rbIsFull(RingBuffer *rb);
void 		rbWrite(RingBuffer *rb, uint8_t ch);
uint8_t 	rbRead(RingBuffer *rb);

#endif /* RINGBUFFER_H_ */
