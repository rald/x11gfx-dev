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

        // Define multiple input regions so both the rectangle and circle catch mouse clicks
        XRectangle interactive_areas[] = {
            { (short)rect_x, (short)rect_y, (unsigned short)rect_w, (unsigned short)rect_h },
            { (short)(circle_cx - circle_r), (short)(circle_cy - circle_r), (unsigned short)(circle_r * 2), (unsigned short)(circle_r * 2) }
        };

        // Enable both interactive regions simultaneously
        gl2d_set_input_regions(&canvas, interactive_areas, 2, 1);

        if (mouse.left_button) {
            if (inrect(mouse.x, mouse.y, rect_x, rect_y, rect_w, rect_h)) {
                rect_clicked = 1;
            }

            if (incirc(mouse.x, mouse.y, circle_cx, circle_cy, circle_r)) {
                circle_clicked = 1;
            }
        } else {
            rect_clicked = 0;
            circle_clicked = 0;
        }

        cls(&canvas, 0x00000000);

        ball_x += vel_x;
        ball_y += vel_y;
        if (ball_x - radius < 0 || ball_x + radius > canvas.width) vel_x = -vel_x;
        if (ball_y - radius < 0 || ball_y + radius > canvas.height) vel_y = -vel_y;

        fcirc(&canvas, ball_x, ball_y, radius, 0xFF000080); 
        circ(&canvas, ball_x, ball_y, radius, 0xFFFFFFFF);       
        line(&canvas, canvas.width/2, canvas.height/2, ball_x, ball_y, 0xFFFF0000);

        unsigned long rect_color = rect_clicked ? 0xFFFF00FF : 0xFF00FF00; 
        frect(&canvas, rect_x, rect_y, rect_w, rect_h, rect_color);       
        rect(&canvas, rect_x, rect_y, rect_w, rect_h, 0xFFFFFFFF);             

        unsigned long circle_color = circle_clicked ? 0xFFFF00FF : 0xFF00FF00;
        fcirc(&canvas, circle_cx, circle_cy, circle_r, circle_color);
        circ(&canvas, circle_cx, circle_cy, circle_r, 0xFFFFFFFF);

        gl2d_update(&canvas);
        usleep(16000); // ~60 FPS
    }

    gl2d_quit(&canvas);
    return 0;
}
