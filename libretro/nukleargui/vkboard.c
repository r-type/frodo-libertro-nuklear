
#include "libretro-core.h"

extern int NPAGE,SHIFTON;
extern unsigned char *keymat,  *revmat,  *joy;
extern int vkey_pressed;

/* nuklear - v1.00 - public domain */
static void
vkeyboard(struct nk_context *ctx)
{
    struct nk_panel layout;
    if (nk_begin(ctx, &layout, "Keyboard", nk_rect(10, 50, 364, 210),
NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
 //       NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_MOVABLE))
    {

        size_t x = 0,y = 0;
        int page = (NPAGE == -1) ? 0 : 50;

        nk_layout_row_dynamic(ctx, 32, 10);

	vkey_pressed=-1;

   	for(y=0;y<NLIGN;y++)
   	{
   	   for(x=0;x<NPLGN;x++)
  	   {               
		
		if (nk_button_text(ctx,SHIFTON==-1?MVk[(y*NPLGN)+x+page].norml:MVk[(y*NPLGN)+x+page].shift , \
				       SHIFTON==-1?strlen(MVk[(y*NPLGN)+x+page].norml):strlen(MVk[(y*NPLGN)+x+page].shift),\
					 NK_BUTTON_DEFAULT)) {

			LOGI("(%s) pressed! (%d,%d)\n",SHIFTON==-1?MVk[(y*NPLGN)+x+page].norml:MVk[(y*NPLGN)+x+page].shift,x,y);
			vkey_pressed=MVk[(y*NPLGN)+x+page].val;
		}

  	   }
	}



    }
    nk_end(ctx);
}

