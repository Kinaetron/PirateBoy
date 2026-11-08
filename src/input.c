#include "input.h"

static InputState state;
static SDL_Gamepad* gamepad = NULL;

void input_event_handler(SDL_Event* event)
{
	if (event->type == SDL_EVENT_GAMEPAD_ADDED)
	{
		if (gamepad == NULL) 
		{
			gamepad = SDL_OpenGamepad(event->gdevice.which);
			if (!gamepad) {
				SDL_Log("Failed to open gamepad ID %u: %s", (unsigned int)event->gdevice.which, SDL_GetError());
			}
		}
	}
	else if (event->type == SDL_EVENT_GAMEPAD_REMOVED)
	{
		if (gamepad && (SDL_GetGamepadID(gamepad) == event->gdevice.which))
		{
			SDL_CloseGamepad(gamepad);
			gamepad = NULL;
		}
	}
}

bool input_update(void)
{
	InputState previous = state;

	const bool* keys = SDL_GetKeyboardState(NULL);

	bool pad_up = false, pad_down = false, pad_left = false, pad_right = false;
	bool pad_a = false, pad_b = false, pad_start = false, pad_select = false;

	if (gamepad)
	{
		pad_up		= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
		pad_down	= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
		pad_left	= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
		pad_right	= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
		pad_a		= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST);
		pad_b		= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
		pad_start	= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START);
		pad_select	= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK);
	}

	state.up	  = pad_up || keys[SDL_SCANCODE_UP];
	state.down	  = pad_down || keys[SDL_SCANCODE_DOWN];
	state.left	  = pad_left || keys[SDL_SCANCODE_LEFT];
	state.right   = pad_right || keys[SDL_SCANCODE_RIGHT];
	state.a		  = pad_a || keys[SDL_SCANCODE_Z];
	state.b		  = pad_b || keys[SDL_SCANCODE_X];
	state.start   = pad_start || keys[SDL_SCANCODE_RETURN];
	state.select  = pad_select || keys[SDL_SCANCODE_RSHIFT];

	if (state.left && state.right) 
	{ 
		state.left = false; 
		state.right = false; 
	}
	if (state.up && state.down) 
	{ 
		state.up = false;    
		state.down = false; 
	}

	return (!previous.up && state.up)
		|| (!previous.down && state.down)
		|| (!previous.left && state.left)
		|| (!previous.right && state.right)
		|| (!previous.a && state.a)
		|| (!previous.b && state.b)
		|| (!previous.start && state.start)
		|| (!previous.select && state.select);
}

InputState* input_state(void) {
	return &state;
}

void input_destroy(void)
{
	if (gamepad) { 
		SDL_CloseGamepad(gamepad); gamepad = NULL; 
	}
}