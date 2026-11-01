#include <stdlib.h>
#include "ppu/pixel_queue.h"

PixelQueue* create_queue(uint8_t capacity)
{
	PixelQueue* queue = calloc(1, sizeof(PixelQueue));
	if (queue == NULL) {
		return NULL;
	}

	queue->array = calloc(capacity, sizeof(PixelInfo));
	if (queue->array == NULL)
	{
		free(queue);
		return NULL;
	}

	queue->capacity = capacity;

	return queue;
}

void enqueue(PixelQueue* queue, PixelInfo* pixel)
{
	if (is_queue_full(queue)) {
		return;
	}

	queue->array[queue->size++] = *pixel;
}

bool dequeue(PixelQueue* queue, PixelInfo* pixel)
{
	if (is_queue_empty(queue)) {
		return false;
	}

	*pixel = queue->array[0];

	for (uint8_t i = 1; i < queue->size; i++) {
		queue->array[i - 1] = queue->array[i];
	}

	queue->size--;

	return true;
}

bool queue_get(PixelQueue* queue, uint8_t index, PixelInfo* pixel)
{
	if (index >= queue->size) {
		return false;
	}

	*pixel = queue->array[index];

	return true;
}

bool queue_set(PixelQueue* queue, uint8_t index, PixelInfo* pixel)
{
	if (index >= queue->size) {
		return false;
	}

	queue->array[index] = *pixel;

	return true;
}

bool is_queue_full(PixelQueue* queue) {
	return queue->size == queue->capacity;
}

bool is_queue_empty(PixelQueue* queue) {
	return queue->size == 0;
}

void destroy_queue(PixelQueue* queue)
{
	if (queue == NULL) {
		return;
	}

	free(queue->array);
	free(queue);
}