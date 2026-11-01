#ifndef PIXEL_QUEUE_H
#define PIXEL_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#include "ppu.h"

typedef struct PixelInfo
{
	uint8_t color;
	Palette palette;
	bool bg_priority;
} PixelInfo;

typedef struct PixelQueue
{
	PixelInfo* array;
	uint8_t capacity;
	uint8_t size;
} PixelQueue;

bool is_queue_full(PixelQueue* queue);
bool is_queue_empty(PixelQueue* queue);
PixelQueue* create_queue(uint8_t capacity);
void enqueue(PixelQueue* queue, PixelInfo* pixel);
bool dequeue(PixelQueue* queue, PixelInfo* pixel);
bool queue_get(PixelQueue* queue, uint8_t index, PixelInfo* pixel);
bool queue_set(PixelQueue* queue, uint8_t index, PixelInfo* pixel);
void destroy_queue(PixelQueue* queue);

#endif