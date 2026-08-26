// Compile with: gcc main.c -o game -I. -L. -lX11 -lXext -lXfixes -lm -O3

#define GL2D_IMPLEMENTATION
#include "gl2d.h"

int main() {
    FastCanvas canvas;

    // 1. Start the graphics engine and transparent overlay window[cite: 1]
    if (!gl2d_start(&canvas)) {
        return 1;
    }

    // Animation Loop Variables
    int ball_x = 100, ball_y = 100;
    int vel_x = 15, vel_y = 10;
    int radius = 30;

    int running = 1;
    while (running) {
        // Clear buffer completely transparent (ARGB: 0x00000000) using fast_cls[cite: 1]
        fast_cls(&canvas, 0x00000000);

        // Update animation coordinates
        ball_x += vel_x;
        ball_y += vel_y;
        if (ball_x - radius < 0 || ball_x + radius > canvas.width) vel_x = -vel_x;
        if (ball_y - radius < 0 || ball_y + radius > canvas.height) vel_y = -vel_y;

        // Draw graphics primitives[cite: 1]
        fast_filled_circle(&canvas, ball_x, ball_y, radius, 0xFF000080); // Opaque Cyan
        fast_circle(&canvas, ball_x, ball_y, radius, 0xFFFFFFFF);       // Opaque Magenta
        fast_filled_rect(&canvas, 200, 150, 100, 60, 0xFF00FF00);       // Opaque Green
        fast_rect(&canvas, 320, 150, 100, 60, 0xFFFF8800);             // Opaque Orange
        fast_line(&canvas, canvas.width/2, canvas.height/2, ball_x, ball_y, 0xFFFF0000); // Opaque Red

        // 2. Refresh frame buffer onto the screen using gl2d_update
        gl2d_update(&canvas);

        usleep(16000); // ~60 FPS
    }

    // 3. Clean up display context and allocated memory
    gl2d_quit(&canvas);
    return 0;
}
