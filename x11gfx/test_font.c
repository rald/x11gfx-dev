#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "font.h"

#define GL2D_IMPLEMENTATION
#include "gl2d.h"

char *my_strupr(char *s) {
    char *p = s;
    while (*p) {
        *p = toupper((unsigned char)*p);
        p++;
    }
    return s;
}

void dchar(FastCanvas *fc,char ch,int x,int y,unsigned long  color,int size) {
  for(int k=0;k<45;k++) {
    if(font[k][0]==ch) {
      for(int j=0;j<5;j++) {
        for(int i=0;i<3;i++) {
          if(font[k][j*3+i+1]==1) {
            frect(fc,i*size+x,j*size+y,size,size,color);
          }
        }
      }
      break;
    }
  }
}

void dtext(FastCanvas *fc,char *t,int x,int y,unsigned long color,int size) {
  for(int k=0;t[k];k++) {
    dchar(fc,t[k],x,y,color,size);
    x+=4*size;
    if(x+4*size>fc->width) {
      x=0;
      y+=6*size;
    }
  }
}

int main() {

time_t rawtime;
    struct tm *info;
    char buffer[80];

    FastCanvas canvas;

    if (!gl2d_init(&canvas)) {
        return 1;
    }

    MouseState mouse = {0};


    int running = 1;
    while (running) {
        gl2d_poll_events(&canvas, &mouse);

        cls(&canvas, 0x00000000);

        time(&rawtime);
        info = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%A, %B %d, %Y %l:%M:%S %p", info);

        my_strupr(buffer);

        dtext(&canvas,buffer,
            canvas.width-strlen(buffer)*4*4,
            0,0x40FFFFFF,4
        );

        gl2d_update(&canvas);
        usleep(16000); // ~60 FPS
    }

    gl2d_quit(&canvas);
    return 0;
}
