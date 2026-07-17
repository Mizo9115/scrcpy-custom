#include "sidebar.h"

#include <assert.h>
#include <string.h>

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
#define SC_SIDEBAR_BUTTON_COUNT 5
#define SC_SIDEBAR_COMPACT_W 400
#define SC_SIDEBAR_COMPACT_H 200

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
    int total_h = SC_SIDEBAR_BUTTON_COUNT * SC_SIDEBAR_BTN
                + (SC_SIDEBAR_BUTTON_COUNT - 1) * SC_SIDEBAR_GAP;
    int start_y = (dh - total_h) / 2;
    if (my < start_y || my >= start_y + total_h) {
        return -1;
    }
    int rel = my - start_y;
    int slot = rel / (SC_SIDEBAR_BTN + SC_SIDEBAR_GAP);
    if (slot < 0 || slot >= SC_SIDEBAR_BUTTON_COUNT) {
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
        case 1: { /* Volume up */
            struct sc_control_msg down = {
                .type = SC_CONTROL_MSG_TYPE_INJECT_KEYCODE,
                .inject_keycode = {
                    .action = AKEY_EVENT_ACTION_DOWN,
                    .keycode = AKEYCODE_VOLUME_UP,
                    .repeat = 0,
                    .metastate = 0,
                },
            };
            struct sc_control_msg up = {
                .type = SC_CONTROL_MSG_TYPE_INJECT_KEYCODE,
                .inject_keycode = {
                    .action = AKEY_EVENT_ACTION_UP,
                    .keycode = AKEYCODE_VOLUME_UP,
                    .repeat = 0,
                    .metastate = 0,
                },
            };
            if (!sc_controller_push_msg(sb->controller, &down)
                    || !sc_controller_push_msg(sb->controller, &up)) {
                LOGW("Could not inject VOLUME_UP");
            }
            return true;
        }
        case 2: { /* Volume down */
            struct sc_control_msg down = {
                .type = SC_CONTROL_MSG_TYPE_INJECT_KEYCODE,
                .inject_keycode = {
                    .action = AKEY_EVENT_ACTION_DOWN,
                    .keycode = AKEYCODE_VOLUME_DOWN,
                    .repeat = 0,
                    .metastate = 0,
                },
            };
            struct sc_control_msg up = {
                .type = SC_CONTROL_MSG_TYPE_INJECT_KEYCODE,
                .inject_keycode = {
                    .action = AKEY_EVENT_ACTION_UP,
                    .keycode = AKEYCODE_VOLUME_DOWN,
                    .repeat = 0,
                    .metastate = 0,
                },
            };
            if (!sc_controller_push_msg(sb->controller, &down)
                    || !sc_controller_push_msg(sb->controller, &up)) {
                LOGW("Could not inject VOLUME_DOWN");
            }
            return true;
        }
        case 3: { /* Always on top */
            sb->always_on_top = !sb->always_on_top;
            SDL_SetWindowAlwaysOnTop(sb->window,
                                     sb->always_on_top ? SDL_TRUE : SDL_FALSE);
            return true;
        }
        case 4: { /* Window size toggle: default <-> 400x200 */
            if (!sb->compact_window_size) {
                int w;
                int h;
                SDL_GetWindowSize(sb->window, &w, &h);
                if (w > 0 && h > 0) {
                    sb->default_window_w = w;
                    sb->default_window_h = h;
                    sb->default_window_size_saved = true;
                }
                SDL_SetWindowSize(sb->window, SC_SIDEBAR_COMPACT_W,
                                  SC_SIDEBAR_COMPACT_H);
                sb->compact_window_size = true;
            } else {
                if (sb->default_window_size_saved) {
                    SDL_SetWindowSize(sb->window, sb->default_window_w,
                                      sb->default_window_h);
                }
                sb->compact_window_size = false;
            }
            return true;
        }
        default:
            return false;
    }
}

void
sc_sidebar_init(struct sc_sidebar *sb, struct sc_controller *controller,
                SDL_Window *window, bool initial_always_on_top,
                bool initial_screen_dimmed) {
    memset(sb, 0, sizeof(*sb));
    sb->controller = controller;
    sb->window = window;
    sb->enabled = controller != NULL;
    sb->hover_button = -1;
    sb->always_on_top = initial_always_on_top;
    sb->phone_screen_dimmed = initial_screen_dimmed;
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
sidebar_draw_icon_speaker(SDL_Renderer *r, const SDL_Rect *b) {
    int cx = b->x + b->w / 2 - 4;
    int cy = b->y + b->h / 2;
    SDL_Rect body = {cx - 8, cy - 4, 8, 8};
    SDL_RenderFillRect(r, &body);
    SDL_RenderDrawLine(r, cx, cy - 8, cx + 6, cy - 12);
    SDL_RenderDrawLine(r, cx + 6, cy - 12, cx + 6, cy + 12);
    SDL_RenderDrawLine(r, cx + 6, cy + 12, cx, cy + 8);
    SDL_RenderDrawLine(r, cx, cy + 8, cx, cy - 8);
}

static void
sidebar_draw_icon_volume_up(SDL_Renderer *r, const SDL_Rect *b, bool hover) {
    Uint8 c = hover ? 220 : 180;
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    sidebar_draw_icon_speaker(r, b);
    int px = b->x + b->w / 2 + 10;
    int py = b->y + b->h / 2;
    SDL_RenderDrawLine(r, px - 4, py, px + 4, py);
    SDL_RenderDrawLine(r, px, py - 4, px, py + 4);
}

static void
sidebar_draw_icon_volume_down(SDL_Renderer *r, const SDL_Rect *b, bool hover) {
    Uint8 c = hover ? 220 : 180;
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    sidebar_draw_icon_speaker(r, b);
    int px = b->x + b->w / 2 + 10;
    int py = b->y + b->h / 2;
    SDL_RenderDrawLine(r, px - 4, py, px + 4, py);
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

static void
sidebar_draw_icon_resize(SDL_Renderer *r, const SDL_Rect *b, bool hover,
                         bool active) {
    Uint8 c = (active || hover) ? 240 : 180;
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    SDL_Rect outer = {b->x + 8, b->y + 11, b->w - 16, b->h - 18};
    SDL_RenderDrawRect(r, &outer);
    SDL_Rect inner = {b->x + 13, b->y + 16, b->w - 26, b->h - 28};
    SDL_RenderDrawRect(r, &inner);
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

    int total_h = SC_SIDEBAR_BUTTON_COUNT * SC_SIDEBAR_BTN
                + (SC_SIDEBAR_BUTTON_COUNT - 1) * SC_SIDEBAR_GAP;
    int start_y = (dh - total_h) / 2;

    for (int i = 0; i < SC_SIDEBAR_BUTTON_COUNT; ++i) {
        int y = start_y + i * (SC_SIDEBAR_BTN + SC_SIDEBAR_GAP);
        SDL_Rect btn = {SC_SIDEBAR_PAD, y, SC_SIDEBAR_BTN, SC_SIDEBAR_BTN};
        bool hover = (sb->hover_button == i);
        bool active = (i == 3 && sb->always_on_top)
                   || (i == 4 && sb->compact_window_size);
        if (active) {
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
                sidebar_draw_icon_volume_up(renderer, &btn, hover);
                break;
            case 2:
                sidebar_draw_icon_volume_down(renderer, &btn, hover);
                break;
            case 3:
                sidebar_draw_icon_pin(renderer, &btn, hover, sb->always_on_top);
                break;
            case 4:
                sidebar_draw_icon_resize(renderer, &btn, hover,
                                         sb->compact_window_size);
                break;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
