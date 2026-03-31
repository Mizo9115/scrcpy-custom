#ifndef SC_SIDEBAR_H
#define SC_SIDEBAR_H

#include <stdbool.h>

#include <SDL2/SDL.h>

#include "util/tick.h"

struct sc_controller;
struct sc_screen;

struct sc_sidebar {
    bool enabled;
    struct sc_controller *controller;
    SDL_Window *window;

    bool panel_visible;
    sc_tick hide_deadline;
    int last_mx;
    int last_my;

    int hover_button; /* -1 if none, 0..3 for four buttons */
    bool phone_screen_dimmed;
    bool always_on_top;
};

void
sc_sidebar_init(struct sc_sidebar *sb, struct sc_controller *controller,
                  SDL_Window *window, bool initial_always_on_top);

void
sc_sidebar_destroy(struct sc_sidebar *sb);

/**
 * Handle mouse events before they are forwarded to the device.
 * @return true if the event was consumed (do not forward to Android).
 */
bool
sc_sidebar_handle_event(struct sc_sidebar *sb, struct sc_screen *screen,
                        const SDL_Event *event);

void
sc_sidebar_render(struct sc_sidebar *sb, struct sc_screen *screen,
                    SDL_Renderer *renderer);

#endif
