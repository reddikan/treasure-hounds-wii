#include <stdio.h>
#include "../source/gfx.h"
#include "../source/art.h"
int art_atlas_used(void);
int main(void){ art_init(); int used=art_atlas_used(); printf("atlas rows used: %d\n", used);
 FILE*f=fopen("tools/out/atlas.ppm","wb"); int H=used+4; fprintf(f,"P6 %d %d 255\n",1024,H);
 for(int y=0;y<H;y++)for(int x=0;x<1024;x++){unsigned c=gfx_atlas[y*1024+x]; int a=c&255; int bg=((x/8+y/8)&1)?60:90;
  int r=(c>>24)*a/255+bg*(255-a)/255,g=((c>>16)&255)*a/255+bg*(255-a)/255,b=((c>>8)&255)*a/255+bg*(255-a)/255; fputc(r,f);fputc(g,f);fputc(b,f);}
 fclose(f); return 0;}
