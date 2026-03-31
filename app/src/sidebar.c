#include "sidebar.h"

#include <assert.h>

#include "android/input.h"
#include "android/keycodes.h"
#include "control_msg.h"
#include "controller.h"
#include "screen.h"
#include "util/log.h"

#define SC_SIDEBAR_EDGE_PX 8
#define SC_SIDEBAR_WIDTH 60
#define SC_SIDEBAR_BTN 44
#define SC_SIDEBAR_GAP 8
#define SC_SIDEBAR_HIDE_DELAY SC_TICK_FROM_MS(500)
#define SC_SIDEBAR_PAD 8

static void
sidebar_mouse_to_drawable(struct sc_screen *screen, int *x, int *y) {
    sc_screen_hidpi_scale_coords(screen, x, y);
}

static int
sidebar_pick_button(struct sc_sidebar *sb, int dw, int dh, int mx, int my) {
    (void) dw;
    if (!sb->panel_visible || mx >= SC_SIDEBAR_WIDTH) {
        return -1;
    }
    int total_h = 4 * SC_SIDEBAR_BTN + 3 * SC_SIDEBAR_GAP;
    int start_y = (dh - total_h) / 2;
    if (my < start_y || my >= start_y + total_h) {
        return -1;
    }
    int rel = my - start_y;
    int slot = rel / (SC_SIDEBAR_BTN + SC_SIDEBAR_GAP);
    if (slot < 0 || slot > 3) {
        return -1;
    }
    int y0 = start_y + slot * (SC_SIDEBAR_BTN + SC_SIDEBAR_GAP);
    if (my >= y0 + SC_SIDEBAR_BTN) {
        return -1;
    }
    int bx0 = SC_SIDEBAR_PAD;
    if (mx < bx0 || mx >= bx0 + SC_SIDEBAR_BTN) {
        return -1;
    }
    return slot;
}

static void
sidebar_update_visibility(struct sc_sidebar *sb, int mx) {
    if (mx < SC_SIDEBAR_EDGE_PX) {
        sb->panel_visible = true;
        sb->hide_deadline = 0;
        return;
    }
    if (sb->panel_visible && mx < SC_SIDEBAR_WIDTH) {
        sb->hide_deadline = 0;
        return;
    }
    if (sb->panel_visible && mx >= SC_SIDEBAR_WIDTH && sb->hide_deadline == 0) {
        sb->hide_deadline = sc_tick_now() + SC_SIDEBAR_HIDE_DELAY;
    }
}

static bool
sidebar_dispatch(struct sc_sidebar *sb, int button_index) {
    assert(sb->controller);

    switch (button_index) {
        case 0: { /* brightness / screen power toggle */
            sb->phone_screen_dimmed = !sb->phone_screen_dimmed;
            struct sc_control_msg msg;
            msg.type = SC_CONTROL_MSG_TYPE_SET_DISPLAY_POWER;
            msg.set_display_power.on = !sb->phone_screen_dimmed;
            if (!sc_controller_push_msg(sb->controller, &msg)) {
                LOGW("Could not push display power toggle");
            }
            return true;
        }
        case 1: { /* Back */
            struct sc_control_msg down = {
                .type = SC_CONTROL_MSG_TYPE_INJECT_KEYCODE,
                .inject_keycode = {
                    .action = AKEY_EVENT_ACTION_DOWN,
                    .keycode = AKEYCODE_BACK,
                    .repeat = 0,
                    .metastate = 0,
                },
            };
            struct sc_control_msg up = {
                .type = SC_CONTROL_MSG_TYPE_INJECT_KEYCODE,
                .inject_keycode = {
                    .action = AKEY_EVENT_ACTION_UP,
                    .keycode = AKEYCODE_BACK,
                    .repeat = 0,
                    .metastate = 0,
                },
            };
            if (!sc_controller_push_msg(sb->controller, &down)
                    || !sc_controller_push_msg(sb->controller, &up)) {
                LOGW("Could not inject BACK");
            }
            return true;
        }
        case 2: { /* Force-close foreground app */
            struct sc_control_msg msg = {
                .type = SC_CONTROL_MSG_TYPE_FORCE_CLOSE_APP,
            };
            if (!sc_controller_push_msg(sb->controller, &msg)) {
                LOGW("Could not request force-close app");
            }
            return true;
        }
        case 3: { /* Always on top */
            sb->always_on_top = !sb->always_on_top;
            SDL_SetWindowAlwaysOnTop(sb->window,
                                     sb->always_on_top ? SDL_TRUE : SDL_FALSE);
            return true;
        }
        default:
            return false;
    }
}

void
sc_sidebar_init(struct sc_sidebar *sb, struct sc_controller *controller,
                SDL_Window *window, bool initial_always_on_top) {
    memset(sb, 0, sizeof(*sb));
    sb->controller = controller;
    sb->window = window;
    sb->enabled = controller != NULL;
    sb->hover_button = -1;
    sb->always_on_top = initial_always_on_top;
    sb->last_mx = -1;
    sb->last_my = -1;
}

void
sc_sidebar_destroy(struct sc_sidebar *sb) {
    (void) sb;
}

bool
sc_sidebar_handle_event(struct sc_sidebar *sb, struct sc_screen *screen,
                        const SDL_Event *event) {
    if (!sb->enabled) {
        return false;
    }

    switch (event->type) {
        case SDL_MOUSEMOTION: {
            int mx = event->motion.x;
            int my = event->motion.y;
            sidebar_mouse_to_drawable(screen, &mx, &my);
            sb->last_mx = mx;
            sb->last_my = my;

            int dw, dh;
            SDL_GL_GetDrawableSize(screen->window, &dw, &dh);

            sidebar_update_visibility(sb, mx);

            if (sb->hide_deadline != 0 && sc_tick_now() >= sb->hide_deadline
                    && mx >= SC_SIDEBAR_WIDTH) {
                sb->panel_visible = false;
                sb->hide_deadline = 0;
            }

            sb->hover_button = sidebar_pick_button(sb, dw, dh, mx, my);
            return mx < SC_SIDEBAR_WIDTH;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            if (event->button.button != SDL_BUTTON_LEFT) {
                break;
            }
            int mx = event->button.x;
            int my = event->button.y;
            sidebar_mouse_to_drawable(screen, &mx, &my);
            int dw, dh;
            SDL_GL_GetDrawableSize(screen->window, &dw, &dh);
            int btn = sidebar_pick_button(sb, dw, dh, mx, my);
            if (btn < 0) {
                return mx < SC_SIDEBAR_WIDTH;
            }
            if (event->type == SDL_MOUSEBUTTONDOWN) {
                return true;
            }
            sidebar_dispatch(sb, btn);
            return true;
        }
        default:
            break;
    }
    return false;
}

static void
sidebar_draw_icon_power(SDL_Renderer *r, const SDL_Rect *b, bool hover) {
    Uint8 c = hover ? 220 : 180;
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    int cx = b->x + b->w / 2;
    int cy = b->y + b->h / 2;
    int rad = b->w / 5;
    for (int y = -rad; y <= rad; ++y) {
        for (int x = -rad; x <= rad; ++x) {
            if (x * x + y * y <= rad * rad) {
                SDL_RenderDrawPoint(r, cx + x, cy + y);
            }
        }
    }
    SDL_RenderDrawLine(r, cx, cy - rad - 2, cx, b->y + 4);
}

static void
sidebar_draw_icon_back(SDL_Renderer *r, const SDL_Rect *b, bool hover) {
    Uint8 c = hover ? 220 : 180;
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    int x0 = b->x + 12;
    int y0 = b->y + b->h / 2;
    SDL_RenderDrawLine(r, x0 + 18, y0 - 10, x0, y0);
    SDL_RenderDrawLine(r, x0, y0, x0 + 18, y0 + 10);
}

static void
sidebar_draw_icon_close(SDL_Renderer *r, const SDL_Rect *b, bool hover) {
    Uint8 c = hover ? 220 : 180;
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    int p = 12;
    SDL_RenderDrawLine(r, b->x + p, b->y + p, b->x + b->w - p, b->y + b->h - p);
    SDL_RenderDrawLine(r, b->x + b->w - p, b->y + p, b->x + p, b->y + b->h - p);
}

static void
sidebar_draw_icon_pin(SDL_Renderer *r, const SDL_Rect *b, bool hover,
                      bool active) {
    Uint8 c = (active || hover) ? 240 : 180;
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    int cx = b->x + b->w / 2;
    int top = b->y + 10;
    SDL_RenderDrawLine(r, cx, top + 16, cx, b->y + b->h - 10);
    SDL_Rect head = {cx - 8, top, 16, 10};
    SDL_RenderDrawRect(r, &head);
}

void
sc_sidebar_render(struct sc_sidebar *sb, struct sc_screen *screen,
                    SDL_Renderer *renderer) {
    if (!sb->enabled || !sb->panel_visible) {
        return;
    }

    int dw, dh;
    SDL_GL_GetDrawableSize(screen->window, &dw, &dh);

    if (sb->hide_deadline != 0 && sc_tick_now() >= sb->hide_deadline
            && (sb->last_mx >= SC_SIDEBAR_WIDTH || sb->last_mx < 0)) {
        sb->panel_visible = false;
        sb->hide_deadline = 0;
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Rect panel = {0, 0, SC_SIDEBAR_WIDTH, dh};
    SDL_SetRenderDrawColor(renderer, 20, 20, 24, 200);
    SDL_RenderFillRect(renderer, &panel);

    int total_h = 4 * SC_SIDEBAR_BTN + 3 * SC_SIDEBAR_GAP;
    int start_y = (dh - total_h) / 2;

    for (int i = 0; i < 4; ++i) {
        int y = start_y + i * (SC_SIDEBAR_BTN + SC_SIDEBAR_GAP);
        SDL_Rect btn = {SC_SIDEBAR_PAD, y, SC_SIDEBAR_BTN, SC_SIDEBAR_BTN};
        bool hover = (sb->hover_button == i);
        if (i == 3 && sb->always_on_top) {
            SDL_SetRenderDrawColor(renderer, 60, 80, 120, 220);
        } else if (hover) {
            SDL_SetRenderDrawColor(renderer, 70, 70, 80, 240);
        } else {
            SDL_SetRenderDrawColor(renderer, 45, 45, 52, 200);
        }
        SDL_RenderFillRect(renderer, &btn);
        SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
        SDL_RenderDrawRect(renderer, &btn);

        switch (i) {
            case 0:
                sidebar_draw_icon_power(renderer, &btn, hover);
                break;
            case 1:
                sidebar_draw_icon_back(renderer, &btn, hover);
                break;
            case 2:
                sidebar_draw_icon_close(renderer, &btn, hover);
                break;
            case 3:
                sidebar_draw_icon_pin(renderer, &btn, hover, sb->always_on_top);
                break;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
