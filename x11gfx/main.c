#define GL2D_IMPLEMENTATION
#include "gl2d.h"

int main() {
    FastCanvas canvas;

    if (!gl2d_init(&canvas)) {
        return 1;
    }

    MouseState mouse = {0};

    int ball_x = 100, ball_y = 100;
    int vel_x = 15, vel_y = 10;
    int radius = 30;

    int rect_x = 200, rect_y = 150, rect_w = 100, rect_h = 60;
    int circle_cx = 450, circle_cy = 180, circle_r = 40;
    
    int rect_clicked = 0;
    int circle_clicked = 0;

    int running = 1;
    while (running) {
        gl2d_poll_events(&canvas, &mouse);

        // Enable input region specifically over the interactive rectangle bounds
        // (Allows clicking through the rest of the transparent screen to background windows)
        gl2d_set_input_region(&canvas, rect_x, rect_y, rect_w, rect_h, 1);

        if (mouse.left_button) {
            if (gl2d_inrect(mouse.x, mouse.y, rect_x, rect_y, rect_w, rect_h)) {
                rect_clicked = !rect_clicked;
            }

            if (gl2d_incirc(mouse.x, mouse.y, circle_cx, circle_cy, circle_r)) {
                circle_clicked = !circle_clicked;
            }
            
            usleep(150000); // Debounce
        }

        fast_cls(&canvas, 0x00000000);

        ball_x += vel_x;
        ball_y += vel_y;
        if (ball_x - radius < 0 || ball_x + radius > canvas.width) vel_x = -vel_x;
        if (ball_y - radius < 0 || ball_y + radius > canvas.height) vel_y = -vel_y;

        fast_filled_circle(&canvas, ball_x, ball_y, radius, 0xFF000080); 
        fast_circle(&canvas, ball_x, ball_y, radius, 0xFFFFFFFF);       
        fast_line(&canvas, canvas.width/2, canvas.height/2, ball_x, ball_y, 0xFFFF0000);

        unsigned long rect_color = rect_clicked ? 0xFFFF00FF : 0xFF00FF00; 
        fast_filled_rect(&canvas, rect_x, rect_y, rect_w, rect_h, rect_color);       
        fast_rect(&canvas, rect_x, rect_y, rect_w, rect_h, 0xFFFFFFFF);             

        unsigned long circle_color = circle_clicked ? 0xFF00FFFF : 0xFFFF8800; 
        fast_filled_circle(&canvas, circle_cx, circle_cy, circle_r, circle_color);
        fast_circle(&canvas, circle_cx, circle_cy, circle_r, 0xFFFFFFFF);

        gl2d_update(&canvas);
        usleep(16000); // ~60 FPS
    }

    gl2d_quit(&canvas);
    return 0;
}