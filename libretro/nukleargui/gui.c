/* nuklear - v1.00 - public domain */

extern pauseg,vkon,guion;
extern int NPAGE,SHIFTON;
extern unsigned char *keymat,  *revmat,  *joy;
extern int vkey_pressed;

static void
gui(struct nk_context *ctx)
{
   struct nk_panel layout;

 if (nk_begin(ctx, &layout, "Keyboard", nk_rect(10, 50, 364, 210),
NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
 //       NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_MOVABLE))
    {

	if(pauseg==1 && SHOWKEY==1)SHOWKEY=-1;

	// VKB IN GAME
	if(pauseg==0 && SHOWKEY==1)
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

	// GUI IN PAUSE
	if(pauseg==1 && SHOWKEY==-1)
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;
            static char buffer[64];
            static int len;

            nk_layout_row_static(ctx, 50, 80, 1);
            if (nk_button_label(ctx, "Return", NK_BUTTON_DEFAULT)){
                fprintf(stdout, "quit GUI\n");
		pauseg=0;
		guion=0;
	    }
            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, buffer, &len, 64, 0);

            {struct nk_panel combo;
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_label(ctx, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_combo_begin_color(ctx, &combo, background, 400)) {
                nk_layout_row_dynamic(ctx, 120, 1);
                background = nk_color_picker(ctx, background, NK_RGBA);
                nk_layout_row_dynamic(ctx, 25, 1);
                background.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, background.r, 255, 1,1);
                background.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, background.g, 255, 1,1);
                background.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, background.b, 255, 1,1);
                background.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, background.a, 255, 1,1);
                nk_combo_end(ctx);
            }}
        }

   	nk_end(ctx);
   }
}

