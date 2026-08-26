#include <stdio.h>
#include <time.h>
#include <ctype.h>


#include <unistd.h>
#include <X11/Xlib.h>

#define GL2D_IMPLEMENTATION
#include "gl2d.h"

#define GL3D_IMPLEMENTATION
#include "gl3d.h"

#include "font.h"

char *my_strupr(char *s) {
    char *p = s;
    while (*p) {
        *p = toupper((unsigned char)*p);
        p++;
    }
    return s;
}

void gl2d_draw_char(FastCanvas *fc,char ch,int x,int y,int w,int h,unsigned long  color,int size) {
    for(int k=0;k<45;k++) {
        if(font[k][0]==ch) {
            for(int j=0;j<h;j++) {
                for(int i=0;i<w;i++) {
                    if(font[k][j*w+i+1]==1) {
                        gl2d_fill_rect(fc,i*size+x,j*size+y,size,size,color);
                    }
                }
            }
            break;
        }
    }
}

void gl2d_draw_text(FastCanvas *fc,char *t,int x,int y,int w,int h,unsigned long color,int size) {
    for(int k=0;t[k];k++) {
        if(t[k]=='\b') {
            x-=(w+1)*size;
            if(x-(w+1)*size<0) {
                y+=(h+1)*size;
            }
        } else if(t[k]=='\r') {
            x=0;
        } else if(t[k]=='\n') {
            y+=(h+1)*size;
        } else {
            gl2d_draw_char(fc,t[k],x,y,w,h,color,size);
            x+=(w+1)*size;
            if(x+(w+1)*size>fc->width) {
                x=0;
                y+=(h+1)*size;
            }
        }
    }
}

int main() {
    FastCanvas canvas;
    if (!gl2d_init(&canvas)) return 1;

    // Grab input focus cleanly so keyboard events route to overlay
    XSetInputFocus(canvas.display, canvas.win, RevertToParent, CurrentTime);

    MouseState mouse = {0};
    KeyState keyboard = {0};

    Cube3D cube = {
        .pos = {0.0f, 0.0f, 0.0f},
        .size = {120.0f, 120.0f, 120.0f},
        .rot = {0.61548f, 0.7854f, 0.0f},
        .face_colors = {
            0xFF0000FF, // +X: Red
            0xFF00FF00, // -X: Green
            0xFFFF0000, // +Y: Blue
            0xFFFFFF00, // -Y: Yellow
            0xFFFF00FF, // +Z: Magenta
            0xFF00FFFF  // -Z: Cyan
        }
    };

    // Map keycodes for movement & exiting
    KeyCode kc_esc = XKeysymToKeycode(canvas.display, XK_Escape);
    KeyCode kc_w   = XKeysymToKeycode(canvas.display, XK_w);
    KeyCode kc_s   = XKeysymToKeycode(canvas.display, XK_s);
    KeyCode kc_a   = XKeysymToKeycode(canvas.display, XK_a);
    KeyCode kc_d   = XKeysymToKeycode(canvas.display, XK_d);

    int running = 1;
    while (running) {
        gl2d_poll_events(&canvas, &mouse, &keyboard);

        // Exit on Escape or left click
        if (keyboard.keys[kc_esc]) {
            running = 0;
        }

        // Simultaneous key detection for rotation control
        if (keyboard.keys[kc_w]) cube.rot.x -= 0.03f;
        if (keyboard.keys[kc_s]) cube.rot.x += 0.03f;
        if (keyboard.keys[kc_a]) cube.rot.y -= 0.03f;
        if (keyboard.keys[kc_d]) cube.rot.y += 0.03f;

        // Clear screen canvas with dark translucent/solid background
        gl2d_cls(&canvas, 0x00000000);

        // Draw 3D Rotating Cube
        gl3d_draw_cube(&canvas, &cube, 1.0f);

        // Render instruction overlay text using the new gl2d_draw_text function
        gl2d_draw_text(&canvas, "USE W/A/S/D TO ROTATE CUBE\r\nPRESS ESC TO QUIT", 0, 0, 3, 5, 0xFFFFFFFF, 2);

        gl2d_update(&canvas);
        usleep(16000); // ~60 FPS
    }

    gl2d_quit(&canvas);
    return 0;
}
