#include <stdio.h>
#include <stdlib.h>
#include "fbsplash.h"
#include "svg_parser.h"
#include "svg_renderer.h"
#define RASTERATOPS_WORDMARK_DATA
#include "rasteratops-wordmark.h"
int main(int argc, char **argv) {
    if (argc != 5) return 2;
    unsigned w=atoi(argv[1]), h=atoi(argv[2]); int rotation=atoi(argv[3]);
    Framebuffer fb={0}; fb.vinfo.xres=w; fb.vinfo.yres=h; fb.vinfo.bits_per_pixel=32;
    fb.finfo.line_length=w*4; fb.screensize=w*h*4; fb.buffer=calloc(1,fb.screensize);
    DisplayInfo *d=calculate_display_info(&fb);
    if (!fb.buffer || !d) return 3;
    for (size_t i=0;i<sizeof(rasteratops_paths)/sizeof(rasteratops_paths[0]);++i) {
        SVGPath *s=parse_svg_path(rasteratops_paths[i],"rgb(230,230,230)");
        if (!s || s->num_paths != 1) return 4;
        if(rotation) rotate_svg_path(s,rotation);
        render_svg_path(&fb,s,d); free_svg_path(s);
    }
    FILE *f=fopen(argv[4],"wb"); if(!f) return 5;
    fprintf(f,"P6\n%u %u\n255\n",w,h); unsigned pixels=0;
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
        unsigned color=((unsigned*)fb.buffer)[y*w+x];
        unsigned char rgb[]={color>>16,color>>8,color}; fwrite(rgb,1,3,f);
        if(color){++pixels;if(x==0||y==0||x==w-1||y==h-1)return 6;}
    }
    fclose(f);free(d);free(fb.buffer);if(pixels==0)return 7;
    printf("PASS %ux%u rotation=%d visible_pixels=%u\n",w,h,rotation,pixels);return 0;
}
