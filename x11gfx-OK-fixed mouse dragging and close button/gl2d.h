#ifndef GL2D_H
#define GL2D_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

// Fast Canvas Context Tracker
typedef struct {
    Display *display;
    Window win;
    GC gc;
    XImage *img;
    uint8_t *data;
    int width;
    int height;
    int bpp;
    int stride;
} FastCanvas;

// Mouse State Tracker
typedef struct {
    int x, y;
    int left_button;
    int right_button;
    int middle_button;
} MouseState;

// Keyboard State Tracker (Supports simultaneous keypresses via keycodes)
typedef struct {
    uint8_t keys[256]; 
} KeyState;

// Function Prototypes
int gl2d_init(FastCanvas *canvas);
void gl2d_update(FastCanvas *canvas);
void gl2d_quit(FastCanvas *canvas);
int gl2d_poll_events(FastCanvas *canvas, MouseState *mouse, KeyState *keyboard);
void gl2d_set_input_region(FastCanvas *canvas, int x, int y, int w, int h, int enabled);
void gl2d_set_input_regions(FastCanvas *canvas, XRectangle *rects, int nrects, int enabled);

static inline void gl2d_pset(FastCanvas *canvas, int x, int y, unsigned long color);
static inline unsigned long gl2d_pget(FastCanvas *canvas, int x, int y);
static inline void gl2d_cls(FastCanvas *canvas, unsigned long color);
static inline void gl2d_draw_line(FastCanvas *canvas, int x0, int y0, int x1, int y1, unsigned long color);
static inline void gl2d_draw_rect(FastCanvas *canvas, int x, int y, int w, int h, unsigned long color);
static inline void gl2d_fill_rect(FastCanvas *canvas, int x, int y, int w, int h, unsigned long color);
static inline void gl2d_draw_circle(FastCanvas *canvas, int xc, int yc, int r, unsigned long color);
static inline void gl2d_fill_circle(FastCanvas *canvas, int xc, int yc, int r, unsigned long color);

// Collision Helper Prototypes
static inline int gl2d_inrect(int px, int py, int rx, int ry, int rw, int rh);
static inline int gl2d_incirc(int px, int py, int cx, int cy, int r);

#ifdef GL2D_IMPLEMENTATION

// GL2D INIT: Initialize Display, 32-bit Transparent Window, XImage, and Canvas Context
int gl2d_init(FastCanvas *canvas) {
    canvas->display = XOpenDisplay(NULL);
    if (!canvas->display) {
        fprintf(stderr, "Cannot open display\n");
        return 0;
    }

    int screen = DefaultScreen(canvas->display);
    Window root = RootWindow(canvas->display, screen);
    canvas->width = DisplayWidth(canvas->display, screen);
    canvas->height = DisplayHeight(canvas->display, screen);

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(canvas->display, screen, 32, TrueColor, &vinfo)) {
        fprintf(stderr, "No 32-bit visual found! Transparency won't work.\n");
        XCloseDisplay(canvas->display);
        return 0;
    }

    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(canvas->display, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0; 
    attrs.border_pixel = 0;
    attrs.override_redirect = True; 
    attrs.event_mask = PointerMotionMask | ButtonPressMask | ButtonReleaseMask | 
                       KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask | FocusChangeMask;

    canvas->win = XCreateWindow(
        canvas->display, root, 
        0, 0, canvas->width, canvas->height, 0, 
        vinfo.depth, InputOutput, vinfo.visual, 
        CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWEventMask, &attrs
    );

    XRectangle rect = {0, 0, canvas->width, canvas->height};
    XserverRegion region = XFixesCreateRegion(canvas->display, &rect, 1);
    XFixesSetWindowShapeRegion(canvas->display, canvas->win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(canvas->display, region);

    XMapWindow(canvas->display, canvas->win);
    XRaiseWindow(canvas->display, canvas->win);

    canvas->gc = XCreateGC(canvas->display, canvas->win, 0, NULL);

    canvas->img = XCreateImage(
        canvas->display, 
        vinfo.visual, 
        vinfo.depth, 
        ZPixmap, 
        0, 
        NULL, 
        canvas->width, canvas->height, 
        32, 0
    );

    canvas->img->data = (char *)malloc(canvas->img->bytes_per_line * canvas->height);
    if (!canvas->img->data) {
        fprintf(stderr, "Failed to allocate image data buffer\n");
        XFreeGC(canvas->display, canvas->gc);
        XCloseDisplay(canvas->display);
        return 0;
    }

    canvas->data = (uint8_t *)canvas->img->data;
    canvas->bpp = canvas->img->bits_per_pixel / 8;
    canvas->stride = canvas->img->bytes_per_line;

    return 1;
}

// GL2D UPDATE: Push frame buffer to screen overlay
void gl2d_update(FastCanvas *canvas) {
    XPutImage(canvas->display, canvas->win, canvas->gc, canvas->img, 0, 0, 0, 0, canvas->width, canvas->height);
    XFlush(canvas->display);
}

// GL2D POLL EVENTS: Non-blocking event loop handler for mouse, window, and simultaneous keys
int gl2d_poll_events(FastCanvas *canvas, MouseState *mouse, KeyState *keyboard) {
    XEvent event;
    while (XPending(canvas->display) > 0) {
        XNextEvent(canvas->display, &event);
        switch (event.type) {
            case MotionNotify:
                mouse->x = event.xmotion.x;
                mouse->y = event.xmotion.y;
                break;
            case ButtonPress:
                if (event.xbutton.button == Button1) mouse->left_button = 1;
                if (event.xbutton.button == Button2) mouse->middle_button = 1;
                if (event.xbutton.button == Button3) mouse->right_button = 1;
                break;
            case ButtonRelease:
                if (event.xbutton.button == Button1) mouse->left_button = 0;
                if (event.xbutton.button == Button2) mouse->middle_button = 0;
                if (event.xbutton.button == Button3) mouse->right_button = 0;
                break;
            case KeyPress:
                if (event.xkey.keycode < 256) {
                    keyboard->keys[event.xkey.keycode] = 1;
                }
                break;
            case KeyRelease:
                if (XPending(canvas->display) > 0) {
                    XEvent next_event;
                    XPeekEvent(canvas->display, &next_event);
                    if (next_event.type == KeyPress && next_event.xkey.keycode == event.xkey.keycode && next_event.xkey.time == event.xkey.time) {
                        break; 
                    }
                }
                if (event.xkey.keycode < 256) {
                    keyboard->keys[event.xkey.keycode] = 0;
                }
                break;
        }
    }
    return 1;
}

// GL2D SET INPUT REGION
void gl2d_set_input_region(FastCanvas *canvas, int x, int y, int w, int h, int enabled) {
    XRectangle rect;
    if (enabled) {
        rect.x = x; rect.y = y; rect.width = w; rect.height = h;
    } else {
        rect.x = 0; rect.y = 0; rect.width = 0; rect.height = 0;
    }
    XserverRegion region = XFixesCreateRegion(canvas->display, &rect, enabled ? 1 : 0);
    XFixesSetWindowShapeRegion(canvas->display, canvas->win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(canvas->display, region);
}

// GL2D SET INPUT REGIONS
void gl2d_set_input_regions(FastCanvas *canvas, XRectangle *rects, int nrects, int enabled) {
    XserverRegion region = XFixesCreateRegion(canvas->display, rects, enabled ? nrects : 0);
    XFixesSetWindowShapeRegion(canvas->display, canvas->win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(canvas->display, region);
}

// GL2D QUIT
void gl2d_quit(FastCanvas *canvas) {
    if (canvas->display) {
        if (canvas->gc) XFreeGC(canvas->display, canvas->gc);
        if (canvas->img) XDestroyImage(canvas->img);
        XCloseDisplay(canvas->display);
    }
}

static inline void gl2d_pset(FastCanvas *canvas, int x, int y, unsigned long color) {
    if (x >= 0 && x < canvas->width && y >= 0 && y < canvas->height) {
        uint8_t *pixel_addr = canvas->data + (y * canvas->stride) + (x * canvas->bpp);
        uint32_t alpha = (color >> 24) & 0xFF;
        if (alpha == 0) return;

        uint32_t dest_pixel = *(uint32_t *)pixel_addr;
        uint32_t dr = (dest_pixel >> 16) & 0xFF;
        uint32_t dg = (dest_pixel >> 8) & 0xFF;
        uint32_t db = dest_pixel & 0xFF;

        uint32_t sr = (color >> 16) & 0xFF;
        uint32_t sg = (color >> 8) & 0xFF;
        uint32_t sb = color & 0xFF;

        uint32_t out_r = (sr * alpha + dr * (255 - alpha)) / 255;
        uint32_t out_g = (sg * alpha + dg * (255 - alpha)) / 255;
        uint32_t out_b = (sb * alpha + db * (255 - alpha)) / 255;
        uint32_t out_a = alpha; 

        *(uint32_t *)pixel_addr = (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
    }
}

static inline unsigned long gl2d_pget(FastCanvas *canvas, int x, int y) {
    if (x >= 0 && x < canvas->width && y >= 0 && y < canvas->height) {
        uint8_t *pixel_addr = canvas->data + (y * canvas->stride) + (x * canvas->bpp);
        return (unsigned long)(*(uint32_t *)pixel_addr);
    }
    return 0;
}

static inline void gl2d_cls(FastCanvas *canvas, unsigned long color) {
    uint32_t *buffer = (uint32_t *)canvas->data;
    int total_pixels = canvas->width * canvas->height;
    for (int i = 0; i < total_pixels; i++) {
        buffer[i] = (uint32_t)color;
    }
}

static inline void gl2d_draw_line(FastCanvas *canvas, int x0, int y0, int x1, int y1, unsigned long color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        gl2d_pset(canvas, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static inline void gl2d_draw_rect(FastCanvas *canvas, int x, int y, int w, int h, unsigned long color) {
    if (w <= 0 || h <= 0) return;
    int x1 = x + w - 1;
    int y1 = y + h - 1;

    for (int cx = x; cx <= x1; cx++) {
        gl2d_pset(canvas, cx, y, color);
        gl2d_pset(canvas, cx, y1, color);
    }
    for (int cy = y + 1; cy < y1; cy++) {
        gl2d_pset(canvas, x, cy, color);
        gl2d_pset(canvas, x1, cy, color);
    }
}

static inline void gl2d_fill_rect(FastCanvas *canvas, int x, int y, int w, int h, unsigned long color) {
    if (w <= 0 || h <= 0) return;
    int x_end = x + w;
    int y_end = y + h;
    for (int cy = y; cy < y_end; cy++) {
        for (int cx = x; cx < x_end; cx++) {
            gl2d_pset(canvas, cx, cy, color);
        }
    }
}

#define PLOT_OCTANTS(canvas, cx, cy, px, py, col) \
    do { \
        gl2d_pset(canvas, (cx)+(px), (cy)+(py), col); \
        gl2d_pset(canvas, (cx)-(px), (cy)+(py), col); \
        gl2d_pset(canvas, (cx)+(px), (cy)-(py), col); \
        gl2d_pset(canvas, (cx)-(px), (cy)-(py), col); \
        gl2d_pset(canvas, (cx)+(py), (cy)+(px), col); \
        gl2d_pset(canvas, (cx)-(py), (cy)+(px), col); \
        gl2d_pset(canvas, (cx)+(py), (cy)-(px), col); \
        gl2d_pset(canvas, (cx)-(py), (cy)-(px), col); \
    } while(0)

static inline void gl2d_draw_circle(FastCanvas *canvas, int xc, int yc, int r, unsigned long color) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    PLOT_OCTANTS(canvas, xc, yc, x, y, color);
    while (y >= x) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
        PLOT_OCTANTS(canvas, xc, yc, x, y, color);
    }
}

static inline void gl2d_fill_circle(FastCanvas *canvas, int xc, int yc, int r, unsigned long color) {
    int r2 = r * r;
    for (int y = yc - r; y <= yc + r; y++) {
        for (int x = xc - r; x <= xc + r; x++) {
            int dx = x - xc;
            int dy = y - yc;
            if (dx * dx + dy * dy <= r2) {
                gl2d_pset(canvas, x, y, color);
            }
        }
    }
}

static inline int gl2d_inrect(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

static inline int gl2d_incirc(int px, int py, int cx, int cy, int r) {
    int dx = px - cx;
    int dy = py - cy;
    return (dx * dx + dy * dy <= r * r);
}

#endif // GL2D_IMPLEMENTATION
#endif // GL2D_H
