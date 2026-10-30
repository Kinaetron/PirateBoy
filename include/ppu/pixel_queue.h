#ifndef PIXEL_QUEUE_H
#define PIXEL_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct PixelQueue
{
	uint8_t* array;
	uint8_t capacity;
	uint8_t size;
} PixelQueue;

bool is_queue_full(PixelQueue* queue);
bool is_queue_empty(PixelQueue* queue);
PixelQueue* create_queue(uint8_t capacity);
void enqueue(PixelQueue* queue, uint8_t pixel);
bool dequeue(PixelQueue* queue, uint8_t* pixel);
void destroy_queue(PixelQueue* queue);

#endif