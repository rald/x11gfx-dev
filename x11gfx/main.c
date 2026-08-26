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

void gl2d_draw_char(FastCanvas *fc, char ch, int x, int y, int w, int h, unsigned long color, int size) {
    for (int k = 0; k < 46; k++) {
        if (font[k][0] == ch) {
            for (int j = 0; j < h; j++) {
                for (int i = 0; i < w; i++) {
                    if (font[k][j * w + i + 1] == 1) {
                        gl2d_fill_rect(fc, i * size + x, j * size + y, size, size, color);
                    }
                }
            }
            break;
        }
    }
}

void gl2d_draw_text(FastCanvas *fc, char *t, int x, int y, int w, int h, unsigned long color, int size) {
    int start_x = x;
    for (int k = 0; t[k]; k++) {
        if (t[k] == '\b') {
            x -= (w + 1) * size;
            if (x - (w + 1) * size < 0) {
                y += (h + 1) * size;
            }
        } else if (t[k] == '\r') {
            x = start_x;
        } else if (t[k] == '\n') {
            y += (h + 1) * size;
            x = start_x;
        } else {
            gl2d_draw_char(fc, t[k], x, y, w, h, color, size);
            x += (w + 1) * size;
            if (x + (w + 1) * size > fc->width) {
                x = start_x;
                y += (h + 1) * size;
            }
        }
    }
}

int main() {
    FastCanvas canvas;
    if (!gl2d_init(&canvas)) return 1;

    MouseState mouse = {0};
    KeyState keyboard = {0};

    int prev_mouse_x = 0;
    int prev_mouse_y = 0;
    int dragging = 0;

    Cube3D cube = {
        .pos = {0.0f, 0.0f, 0.0f},
        .size = {120.0f, 120.0f, 120.0f},
        .rot = {0.61548f, 0.7854f, 0.0f},
        .face_colors = {
            0xFF0000FF, // +X: Red[cite: 1]
            0xFF00FF00, // -X: Green[cite: 1]
            0xFFFF0000, // +Y: Blue[cite: 1]
            0xFFFFFF00, // -Y: Yellow[cite: 1]
            0xFFFF00FF, // +Z: Magenta[cite: 1]
            0xFF00FFFF  // -Z: Cyan[cite: 1]
        }
    };

    int running = 1;

    while (running) {
        gl2d_poll_events(&canvas, &mouse, &keyboard);

        // Calculate screen-space bounding box for the cube[cite: 1]
        float cx = canvas.width / 2.0f + cube.pos.x;
        float cy = canvas.height / 2.0f - cube.pos.y;
        int cube_bounding_size = (int)(cube.size.x * 1.8f); 
        int cube_x = (int)cx - cube_bounding_size / 2;
        int cube_y = (int)cy - cube_bounding_size / 2;

        int mouse_in_cube = gl2d_inrect(mouse.x, mouse.y, cube_x, cube_y, cube_bounding_size, cube_bounding_size);
        
        int gui_x = 20, gui_y = 20, gui_w = 420, gui_h = 110;
        int mouse_in_gui = gl2d_inrect(mouse.x, mouse.y, gui_x, gui_y, gui_w, gui_h);

        // Close button coordinates (top right of the GUI panel)
        int close_w = 23, close_h = 23;
        int close_x = gui_x + gui_w - close_w - 10;
        int close_y = gui_y + 10;
        int mouse_in_close = gl2d_inrect(mouse.x, mouse.y, close_x, close_y, close_w, close_h);

        // Handle close button click
        if (mouse_in_close && mouse.left_button) {
            running = 0;
        }

        // Handle drag-to-rotate logic
        if (mouse.left_button) {
            if (!dragging && mouse_in_cube) {
                dragging = 1;
            }
            if (dragging) {
                int dx = mouse.x - prev_mouse_x;
                int dy = mouse.y - prev_mouse_y;
                cube.rot.y += dx * 0.01f;
                cube.rot.x += dy * 0.01f;
            }
        } else {
            dragging = 0;
        }

        prev_mouse_x = mouse.x;
        prev_mouse_y = mouse.y;

        // Set input regions for the cube, GUI panel, and close button[cite: 1, 2]
        XRectangle rects[3] = {
            {cube_x, cube_y, cube_bounding_size, cube_bounding_size},
            {gui_x, gui_y, gui_w, gui_h},
            {close_x, close_y, close_w, close_h}
        };
        gl2d_set_input_regions(&canvas, rects, 3, 1);

        // Clear screen canvas with full transparency[cite: 1]
        gl2d_cls(&canvas, 0x00000000);

        // Draw GUI background panel[cite: 1]
        gl2d_fill_rect(&canvas, gui_x, gui_y, gui_w, gui_h, 0xAA111111);
        gl2d_draw_rect(&canvas, gui_x, gui_y, gui_w, gui_h, 0xFF444444);

        // Draw Close Button Box & "X" Text
        unsigned long close_bg = mouse_in_close ? 0xFFFF5555 : 0xFFCC3333;
        gl2d_fill_rect(&canvas, close_x, close_y, close_w, close_h, close_bg);
        gl2d_draw_rect(&canvas, close_x, close_y, close_w, close_h, 0xFFFFFFFF);
        gl2d_draw_text(&canvas, "X", close_x + 9, close_y + 6, 3, 5, 0xFFFFFFFF, 2);

        // Draw 3D Rotating Cube[cite: 1]
        gl3d_draw_cube(&canvas, &cube, 1.0f);

        // Render instruction text overlay[cite: 1]
        gl2d_draw_text(&canvas, "CLICK AND DRAG CUBE TO SPIN\r\nUSE (X) TO CLOSE APP", gui_x + 15, gui_y + 15, 3, 5, 0xFFFFFFFF, 2);

        gl2d_update(&canvas);
        usleep(16000); // ~60 FPS[cite: 1]
    }

    gl2d_quit(&canvas);
    return 0;
}
