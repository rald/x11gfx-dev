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

// Function Prototypes
int gl2d_start(FastCanvas *canvas);
void gl2d_update(FastCanvas *canvas);
void gl2d_quit(FastCanvas *canvas);

static inline void fast_pset(FastCanvas *canvas, int x, int y, unsigned long color);
static inline unsigned long fast_pget(FastCanvas *canvas, int x, int y);
static inline void fast_cls(FastCanvas *canvas, unsigned long color);
static inline void fast_line(FastCanvas *canvas, int x0, int y0, int x1, int y1, unsigned long color);
static inline void fast_rect(FastCanvas *canvas, int x, int y, int w, int h, unsigned long color);
static inline void fast_filled_rect(FastCanvas *canvas, int x, int y, int w, int h, unsigned long color);
static inline void fast_circle(FastCanvas *canvas, int xc, int yc, int r, unsigned long color);
static inline void fast_filled_circle(FastCanvas *canvas, int xc, int yc, int r, unsigned long color);

#ifdef GL2D_IMPLEMENTATION

// GL2D START: Initialize Display, 32-bit Transparent Window, XImage, and Canvas Context
int gl2d_start(FastCanvas *canvas) {
    canvas->display = XOpenDisplay(NULL);
    if (!canvas->display) {
        fprintf(stderr, "Cannot open display\n");
        return 0;
    }

    int screen = DefaultScreen(canvas->display);
    Window root = RootWindow(canvas->display, screen);
    canvas->width = DisplayWidth(canvas->display, screen);
    canvas->height = DisplayHeight(canvas->display, screen);

    // 1. Match a 32-bit visual to support transparency (ARGB)
    XVisualInfo vinfo;
    if (!XMatchVisualInfo(canvas->display, screen, 32, TrueColor, &vinfo)) {
        fprintf(stderr, "No 32-bit visual found! Transparency won't work.\n");
        XCloseDisplay(canvas->display);
        return 0;
    }

    // 2. Set up window attributes
    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(canvas->display, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0; 
    attrs.border_pixel = 0;
    attrs.override_redirect = True; 

    canvas->win = XCreateWindow(
        canvas->display, root, 
        0, 0, canvas->width, canvas->height, 0, 
        vinfo.depth, InputOutput, vinfo.visual, 
        CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect, &attrs
    );

    // 3. Make the window click-through using XFixes
    XRectangle rect;
    XserverRegion region = XFixesCreateRegion(canvas->display, &rect, 1);
    XFixesSetWindowShapeRegion(canvas->display, canvas->win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(canvas->display, region);

    XMapWindow(canvas->display, canvas->win);

    // Create a custom GC matching our 32-bit window depth
    canvas->gc = XCreateGC(canvas->display, canvas->win, 0, NULL);

    // 4. Create an XImage buffer explicitly matching the 32-bit visual and depth
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

// GL2D UPDATE: Push the frame buffer to the screen overlay
void gl2d_update(FastCanvas *canvas) {
    XPutImage(canvas->display, canvas->win, canvas->gc, canvas->img, 0, 0, 0, 0, canvas->width, canvas->height);
    XFlush(canvas->display);
}

// GL2D QUIT: Clean up and release all X11 and memory resources
void gl2d_quit(FastCanvas *canvas) {
    if (canvas->display) {
        if (canvas->gc) XFreeGC(canvas->display, canvas->gc);
        if (canvas->img) XDestroyImage(canvas->img);
        XCloseDisplay(canvas->display);
    }
}

// FAST PSET: Direct memory write with centralized bounds checking
static inline void fast_pset(FastCanvas *canvas, int x, int y, unsigned long color) {
    if (x >= 0 && x < canvas->width && y >= 0 && y < canvas->height) {
        uint8_t *pixel_addr = canvas->data + (y * canvas->stride) + (x * canvas->bpp);
        *(uint32_t *)pixel_addr = (uint32_t)color;
    }
}

// FAST PGET: Direct memory read with bounds checking
static inline unsigned long fast_pget(FastCanvas *canvas, int x, int y) {
    if (x >= 0 && x < canvas->width && y >= 0 && y < canvas->height) {
        uint8_t *pixel_addr = canvas->data + (y * canvas->stride) + (x * canvas->bpp);
        return (unsigned long)(*(uint32_t *)pixel_addr);
    }
    return 0;
}

// FAST CLS: Clear the entire canvas with a specific color
static inline void fast_cls(FastCanvas *canvas, unsigned long color) {
    uint32_t *buffer = (uint32_t *)canvas->data;
    int total_pixels = canvas->width * canvas->height;
    for (int i = 0; i < total_pixels; i++) {
        buffer[i] = (uint32_t)color;
    }
}

// FAST LINE: Draw a line using Bresenham's algorithm with fast_pset
static inline void fast_line(FastCanvas *canvas, int x0, int y0, int x1, int y1, unsigned long color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        fast_pset(canvas, x0, y0, color);
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

// FAST RECT: Draw an outlined rectangle
static inline void fast_rect(FastCanvas *canvas, int x, int y, int w, int h, unsigned long color) {
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    fast_line(canvas, x, y, x1, y, color);       // Top
    fast_line(canvas, x, y1, x1, y1, color);    // Bottom
    fast_line(canvas, x, y, x, y1, color);       // Left
    fast_line(canvas, x1, y, x1, y1, color);    // Right
}

// FAST FILLED RECT: Draw a solid filled rectangle
static inline void fast_filled_rect(FastCanvas *canvas, int x, int y, int w, int h, unsigned long color) {
    int x_end = x + w;
    int y_end = y + h;
    
    for (int cy = y; cy < y_end; cy++) {
        for (int cx = x; cx < x_end; cx++) {
            fast_pset(canvas, cx, cy, color);
        }
    }
}

// Helper macro for plotting all 8 symmetric circle octants safely via fast_pset
#define PLOT_OCTANTS(canvas, cx, cy, px, py, col) \
    do { \
        fast_pset(canvas, (cx)+(px), (cy)+(py), col); \
        fast_pset(canvas, (cx)-(px), (cy)+(py), col); \
        fast_pset(canvas, (cx)+(px), (cy)-(py), col); \
        fast_pset(canvas, (cx)-(px), (cy)-(py), col); \
        fast_pset(canvas, (cx)+(py), (cy)+(px), col); \
        fast_pset(canvas, (cx)-(py), (cy)+(px), col); \
        fast_pset(canvas, (cx)+(py), (cy)-(px), col); \
        fast_pset(canvas, (cx)-(py), (cy)-(px), col); \
    } while(0)

// FAST CIRCLE: Draw an outlined circle using midpoint circle algorithm
static inline void fast_circle(FastCanvas *canvas, int xc, int yc, int r, unsigned long color) {
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

// FAST FILLED CIRCLE: Draw a solid filled circle
static inline void fast_filled_circle(FastCanvas *canvas, int xc, int yc, int r, unsigned long color) {
    int r2 = r * r;
    for (int y = yc - r; y <= yc + r; y++) {
        for (int x = xc - r; x <= xc + r; x++) {
            int dx = x - xc;
            int dy = y - yc;
            if (dx * dx + dy * dy <= r2) {
                fast_pset(canvas, x, y, color);
            }
        }
    }
}

#endif // GL2D_IMPLEMENTATION

#endif // GL2D_H
