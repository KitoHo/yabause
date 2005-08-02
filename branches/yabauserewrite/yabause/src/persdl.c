#include "persdl.h"
#include "SDL.h"
#include "yabause.h"
#include "yui.h"

int PERSDLInit(void);
void PERSDLDeInit(void);
int PERSDLHandleEvents(void);
void PERSDLNothing(void);

PerInterface_struct PERSDL = {
PERCORE_SDL,
"SDL Input Interface",
PERSDLInit,
PERSDLDeInit,
PERSDLHandleEvents
};

void (*PERSDLKeyPressed[SDLK_LAST])(void);
void (*PERSDLKeyReleased[SDLK_LAST])(void);

static inline void PERSDLSetBoth(SDLKey key, void (*fct_ptr1)(void), void (*fct_ptr2)(void)) {
	PERSDLKeyPressed[key] = fct_ptr1;
	PERSDLKeyReleased[key] = fct_ptr2;
}

int PERSDLInit(void) {
	int i;

	for(i = 0;i < SDLK_LAST;i++) {
		PERSDLKeyPressed[i] = PERSDLNothing;
		PERSDLKeyReleased[i] = PERSDLNothing;
	}

	PERSDLSetBoth(SDLK_RIGHT, PerRightPressed, PerRightReleased);
	PERSDLSetBoth(SDLK_LEFT, PerLeftPressed, PerLeftReleased);
	PERSDLSetBoth(SDLK_DOWN, PerDownPressed, PerDownReleased);
	PERSDLSetBoth(SDLK_UP, PerUpPressed, PerUpReleased);
	PERSDLSetBoth(SDLK_j, PerStartPressed, PerStartReleased);
	PERSDLSetBoth(SDLK_k, PerAPressed, PerAReleased);
	PERSDLSetBoth(SDLK_l, PerBPressed, PerBReleased);
	PERSDLSetBoth(SDLK_m, PerCPressed, PerCReleased);
	PERSDLSetBoth(SDLK_u, PerXPressed, PerXReleased);
	PERSDLSetBoth(SDLK_i, PerYPressed, PerYReleased);
	PERSDLSetBoth(SDLK_o, PerZPressed, PerZReleased);
	PERSDLSetBoth(SDLK_z, PerRTriggerPressed, PerRTriggerReleased);
	PERSDLSetBoth(SDLK_x, PerLTriggerPressed, PerLTriggerReleased);

	PERSDLKeyPressed[SDLK_q] = YuiQuit;

	return 0;
}

void PERSDLDeInit(void) {
}

void PERSDLNothing(void) {
}

int PERSDLHandleEvents(void) {
	SDL_Event event;

	if (SDL_PollEvent(&event)) {
		switch(event.type) {
		case SDL_QUIT:
			YuiQuit();
			break;
		case SDL_KEYDOWN:
			PERSDLKeyPressed[event.key.keysym.sym]();
			break;
		case SDL_KEYUP:
			PERSDLKeyReleased[event.key.keysym.sym]();
			break;
		default:
			break;
		}
	} else {
		if (YabauseExec() != 0)
			return -1;
	}
	return 0;
}
