/*
 * window_manager.c - Keep track of windows, their order, and dragging.
 *
 *   order[0]            bottom window
 *   order[count - 1]    top window (drawn last, gets the clicks first)
 *
 * Uses: canvas_* (gui/canvas), kmalloc/kfree (memory_management),
 *       strncpy (lib/string).
 */

#include "window_manager.h"
#include "kheap.h"
#include "string.h"
#include "def_window_bounds.h"   // DEFENCE window_bounds
#include "def_window_quota.h"    // DEFENCE window_quota

typedef struct {
    int used;
    int x, y;                   // top-left of the window's content
    canvas_t canvas;
    char title[32];
} window_t;

static window_t windows[MAX_WINDOWS];
static int order[MAX_WINDOWS];  // window ids, bottom to top
static int count;

static int dragging = -1;       // id of the window being dragged, or -1
static int drag_dx, drag_dy;    // mouse position relative to that window
static int was_pressed;         // left button state in the last frame

void wm_init(void) {
    wm_destroy_all();
}

int wm_create_window(int x, int y, int width, int height, const char *title) {
    // DEFENCE window_bounds: kmalloc(width*height*4) must not overflow.
    if (!def_window_size_ok(width, height))
        return -1;

    // DEFENCE window_quota: refuse once every slot is already in use.
    int used = 0;
    for (int i = 0; i < MAX_WINDOWS; i++)
        if (windows[i].used)
            used++;
    if (!def_window_quota_ok(used))
        return -1;

    for (int id = 0; id < MAX_WINDOWS; id++) {
        window_t *w = &windows[id];
        if (w->used)
            continue;

        w->canvas.pixels = kmalloc(width * height * sizeof(uint32_t));
        if (!w->canvas.pixels)
            return -1;
        w->canvas.width  = width;
        w->canvas.height = height;
        canvas_fill(&w->canvas, COLOR_WHITE);

        w->x = x;
        w->y = y;
        strncpy(w->title, title, sizeof(w->title) - 1);
        w->title[sizeof(w->title) - 1] = '\0';
        w->used = 1;

        order[count++] = id;            // new windows start on top
        return id;
    }
    return -1;
}

void wm_destroy_window(int id) {
    if (id < 0 || id >= MAX_WINDOWS || !windows[id].used)
        return;

    kfree(windows[id].canvas.pixels);
    windows[id].used = 0;

    // Remove it from the z-order, closing the gap.
    int i = 0;
    while (order[i] != id)
        i++;
    for (; i < count - 1; i++)
        order[i] = order[i + 1];
    count--;

    if (dragging == id)
        dragging = -1;
}

void wm_destroy_all(void) {
    for (int id = 0; id < MAX_WINDOWS; id++)
        wm_destroy_window(id);
    count = 0;
    dragging = -1;
    was_pressed = 0;
}

canvas_t *wm_window_canvas(int id) {
    if (id < 0 || id >= MAX_WINDOWS || !windows[id].used)
        return 0;
    return &windows[id].canvas;
}

void wm_keep_on_screen(int width, int height) {
    for (int id = 0; id < MAX_WINDOWS; id++) {
        window_t *w = &windows[id];
        if (!w->used)
            continue;
        // Pull it back from the right and bottom edges...
        if (w->x + w->canvas.width > width)
            w->x = width - w->canvas.width;
        if (w->y + w->canvas.height > height)
            w->y = height - w->canvas.height;
        // ...but a window bigger than the screen keeps its title bar visible.
        if (w->x < 0)
            w->x = 0;
        if (w->y < TITLE_BAR_HEIGHT)
            w->y = TITLE_BAR_HEIGHT;
    }
}

static void raise_to_top(int position) {
    int id = order[position];
    for (int i = position; i < count - 1; i++)
        order[i] = order[i + 1];
    order[count - 1] = id;
}

static int inside(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

void wm_handle_mouse(const mouse_state_t *m) {
    int just_pressed = m->left && !was_pressed;
    was_pressed = m->left;

    if (!m->left) {
        dragging = -1;
        return;
    }

    if (dragging >= 0) {                    // still holding: follow the mouse
        windows[dragging].x = m->x - drag_dx;
        windows[dragging].y = m->y - drag_dy;
        return;
    }
    if (!just_pressed)
        return;

    // Find the top-most window under the click.
    for (int i = count - 1; i >= 0; i--) {
        window_t *w = &windows[order[i]];
        int top = w->y - TITLE_BAR_HEIGHT;
        if (!inside(m->x, m->y, w->x, top, w->canvas.width, w->canvas.height + TITLE_BAR_HEIGHT))
            continue;

        if (m->y < w->y) {                  // on the title bar: start dragging
            dragging = order[i];
            drag_dx = m->x - w->x;
            drag_dy = m->y - w->y;
        }
        raise_to_top(i);
        return;
    }
}

static void draw_pointer(canvas_t *s, int x, int y) {
    canvas_fill_rect(s, x, y, 2, 12, COLOR_WHITE);     // an "L" shaped arrow
    canvas_fill_rect(s, x, y, 8, 2, COLOR_WHITE);
    canvas_put_pixel(s, x, y, COLOR_RED);
}

void wm_draw(canvas_t *s, const mouse_state_t *mouse) {
    canvas_fill(s, COLOR_DESKTOP);

    for (int i = 0; i < count; i++) {
        window_t *w = &windows[order[i]];
        int is_top = (i == count - 1);
        uint32_t bar = is_top ? COLOR_BLUE : COLOR_GREY;

        // Border, title bar, title, then the window's own pixels.
        canvas_fill_rect(s, w->x - BORDER, w->y - TITLE_BAR_HEIGHT - BORDER,
                         w->canvas.width + 2 * BORDER,
                         w->canvas.height + TITLE_BAR_HEIGHT + 2 * BORDER, COLOR_GREY);
        canvas_fill_rect(s, w->x, w->y - TITLE_BAR_HEIGHT, w->canvas.width, TITLE_BAR_HEIGHT, bar);
        canvas_draw_string(s, w->x + 6, w->y - 14, w->title, COLOR_WHITE, bar);
        canvas_blit(s, &w->canvas, w->x, w->y);
    }

    draw_pointer(s, mouse->x, mouse->y);
}
