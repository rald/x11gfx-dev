// Compile with: gcc -o anim anim.c -lX11 -lXext -lXfixes -O3
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

// Fast Canvas Context Tracker
typedef struct {
    XImage *img;
    uint8_t *data;
    int bpp;
    int stride;
} FastCanvas;

// FAST PSET: Direct memory write
static inline void fast_pset(FastCanvas *canvas, int x, int y, unsigned long color) {
    uint8_t *pixel_addr = canvas->data + (y * canvas->stride) + (x * canvas->bpp);
    *(uint32_t *)pixel_addr = (uint32_t)color;
}

// FAST PGET: Direct memory read
static inline unsigned long fast_pget(FastCanvas *canvas, int x, int y) {
    uint8_t *pixel_addr = canvas->data + (y * canvas->stride) + (x * canvas->bpp);
    return (unsigned long)(*(uint32_t *)pixel_addr);
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    // 1. Match a 32-bit visual to support transparency (ARGB)
    XVisualInfo vinfo;
    if (!XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo)) {
        fprintf(stderr, "No 32-bit visual found! Transparency won't work.\n");
        return 1;
    }

    // 2. Set up window attributes
    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(display, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0; 
    attrs.border_pixel = 0;
    attrs.override_redirect = True; 

    Window win = XCreateWindow(
        display, root, 
        0, 0, width, height, 0, 
        vinfo.depth, InputOutput, vinfo.visual, 
        CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect, &attrs
    );

    // 3. Make the window click-through using XFixes
    XRectangle rect;
    XserverRegion region = XFixesCreateRegion(display, &rect, 1);
    XFixesSetWindowShapeRegion(display, win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(display, region);

    XMapWindow(display, win);

    // Create a custom GC matching our 32-bit window depth
    GC gc = XCreateGC(display, win, 0, NULL);

    // 4. Create an XImage buffer explicitly matching the 32-bit visual and depth
    XImage *ximage = XCreateImage(
        display, 
        vinfo.visual, 
        vinfo.depth, 
        ZPixmap, 
        0, 
        NULL, 
        width, height, 
        32, 0
    );

    ximage->data = (char *)malloc(ximage->bytes_per_line * height);
    if (!ximage->data) {
        fprintf(stderr, "Failed to allocate image data buffer\n");
        XFreeGC(display, gc);
        XCloseDisplay(display);
        return 1;
    }

    // Initialize our FastCanvas helper struct
    FastCanvas canvas = {
        .img = ximage,
        .data = (uint8_t *)ximage->data,
        .bpp = ximage->bits_per_pixel / 8,
        .stride = ximage->bytes_per_line
    };

    // Animation Loop Variables
    int ball_x = 100, ball_y = 100;
    int vel_x = 15, vel_y = 10;
    int radius = 30;

    int running = 1;
    while (running) {
        // Clear buffer completely transparent (ARGB: 0x00000000) using fast pset or memset
        uint32_t *buffer = (uint32_t *)ximage->data;
        int total_pixels = width * height;
        for (int i = 0; i < total_pixels; i++) {
            buffer[i] = 0x00000000; 
        }

        // Update animation coordinates
        ball_x += vel_x;
        ball_y += vel_y;
        if (ball_x - radius < 0 || ball_x + radius > width) vel_x = -vel_x;
        if (ball_y - radius < 0 || ball_y + radius > height) vel_y = -vel_y;

        // Draw solid circle onto local transparent XImage buffer using fast_pset
        for (int y = ball_y - radius; y <= ball_y + radius; y++) {
            for (int x = ball_x - radius; x <= ball_x + radius; x++) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    int dx = x - ball_x;
                    int dy = y - ball_y;
                    if (dx*dx + dy*dy <= radius*radius) {
                        // Opaque Cyan: 0xFF (Alpha) + 0x00FFFF (Cyan)
                        fast_pset(&canvas, x, y, 0xFF00FFFF); 
                    }
                }
            }
        }

        // Example demonstration of fast_pget (reading the center of the ball)
        unsigned long center_pixel = fast_pget(&canvas, ball_x, ball_y);
        (void)center_pixel; // Suppress unused variable warning if not printing

        // Push frame to screen overlay
        XPutImage(display, win, gc, ximage, 0, 0, 0, 0, width, height);
        XFlush(display);

        usleep(16000); // ~60 FPS
    }

    XFreeGC(display, gc);
    XDestroyImage(ximage);
    XCloseDisplay(display);
    return 0;
}