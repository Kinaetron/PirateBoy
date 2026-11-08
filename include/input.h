#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <SDL3/SDL.h>

typedef struct
{
	bool up, down, left, right;
	bool a, b, start, select;
} InputState;


void input_event_handler(SDL_Event* event);
void input_update(void);
InputState* input_state(void);
void input_destroy(void);

#endif