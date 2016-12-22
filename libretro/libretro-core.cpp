#include "libretro.h"
#include "libretro-core.h"

//#define RENDER16B
//#undef RENDER16B
#ifdef RENDER16B
#define M16B
#endif



//CORE VAR
#ifdef _WIN32
char slash = '\\';
#else
char slash = '/';
#endif

char RETRO_DIR[512];

//TIME
#ifdef __CELLOS_LV2__
#include "sys/sys_time.h"
#include "sys/timer.h"
#define usleep  sys_timer_usleep
#else
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#endif

extern void Screen_SetFullUpdate(int scr);
extern void kbd_buf_feed(char *s);
extern bool autoboot;
//extern void validkey(int c64_key,int key_up,unsigned char *key_matrix, unsigned char *rev_matrix, unsigned char *joystick);

long frame=0;
unsigned long  Ktime=0 , LastFPSTime=0;

//VIDEO
unsigned int *Retro_Screen;//[1024*1024];

//SOUND
short signed int SNDBUF[1024*2];
int snd_sampler = 44100 / 50;

//PATH
char RPATH[512];

//EMU FLAGS
int NPAGE=-1, KCOL=1, BKGCOLOR=0;
int SHOWKEY=-1;

int MAXPAS=6,SHIFTON=-1,MOUSE_EMULATED=-1,MOUSEMODE=-1,PAS=4;
int SND=1; //SOUND ON/OFF
static int firstps=0;
int pauseg=0; //enter_gui
int touch=-1; // gui mouse btn
//JOY
int al[2][2];//left analog1
int ar[2][2];//right analog1
unsigned char MXjoy[2]; // joy
int NUMjoy=1;

//MOUSE
extern int pushi;  // gui mouse btn
int gmx,gmy; //gui mouse

//KEYBOARD
char Key_Sate[512];
char Key_Sate2[512];

static int mbt[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//STATS GUI
int BOXDEC= 32+2;
int STAT_BASEY;


//FRODO
#include "main.h"
#include "C64.h"
#include "Display.h"
#include "Prefs.h"
#include "SAM.h"
extern C64 *TheC64;
extern void quit_frodo_emu();
int retro_quit=0;

//NUKLEAR APP
extern "C" {
void app_frame();
extern unsigned int *RetroScreen;
extern int app_init(void);
extern int app_free(void);
extern int app_main(void);
extern int app_event(void);
}

int CROP_WIDTH;
int CROP_HEIGHT;
int VIRTUAL_WIDTH;
int retrow=1024; 
int retroh=1024;

#include "vkbd.i"

extern void update_input(void);

const char *retro_save_directory;
const char *retro_system_directory;
const char *retro_content_directory;

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
/*static*/ retro_input_state_t input_state_cb;
/*static*/ retro_input_poll_t input_poll_cb;

void texture_uninit(void)
{

}

void texture_init(void)
{
   //memset(Retro_Screen, 0, sizeof(Retro_Screen));

   gmx=(retrow/2)-1;
   gmy=(retroh/2)-1;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   struct retro_variable variables[] = {
      {
         "Skel_resolution",
         "Internal resolution; 384x288|400x300|640x480|832x576|800x600|960x720|1024x768|1024x1024",

      },
      { NULL, NULL },
   };

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

static void update_variables(void)
{
   struct retro_variable var = {
      .key = "Skel_resolution",
   };

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      char *pch;
      char str[100];
      snprintf(str, sizeof(str), var.value);

      pch = strtok(str, "x");
      if (pch)
         retrow = strtoul(pch, NULL, 0);
      pch = strtok(NULL, "x");
      if (pch)
         retroh = strtoul(pch, NULL, 0);

//FIXME remove force 384x288
retrow=WINDOW_WIDTH;
retroh=WINDOW_HEIGHT;

      fprintf(stderr, "[libretro-test]: Got size: %u x %u.\n", retrow, retroh);

      CROP_WIDTH =retrow;
      CROP_HEIGHT= (retroh-80);
      VIRTUAL_WIDTH = retrow;
      texture_init();
      //reset_screen();
   }

}

#include <ctype.h>

//Args for experimental_cmdline
static char ARGUV[64][1024];
static unsigned char ARGUC=0;

// Args for Core
static char XARGV[64][1024];
static const char* xargv_cmd[64];
int PARAMCOUNT=0;

extern int  skel_main(int argc, char *argv[]);

void Add_Option(const char* option)
{
   static int first=0;

   if(first==0)
   {
      PARAMCOUNT=0;	
      first++;
   }

   sprintf(XARGV[PARAMCOUNT++],"%s\0",option);
}


void parse_cmdline(const char *argv)
{
	char *p,*p2,*start_of_word;
	int c,c2;
	static char buffer[512*4];
	enum states { DULL, IN_WORD, IN_STRING } state = DULL;
	
	strcpy(buffer,argv);
	strcat(buffer," \0");

	for (p = buffer; *p != '\0'; p++)
   {
      c = (unsigned char) *p; /* convert to unsigned char for is* functions */
      switch (state)
      {
         case DULL: /* not in a word, not in a double quoted string */
            if (isspace(c)) /* still not in a word, so ignore this char */
               continue;
            /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
            if (c == '"')
            {
               state = IN_STRING;
               start_of_word = p + 1; /* word starts at *next* char, not this one */
               continue;
            }
            state = IN_WORD;
            start_of_word = p; /* word starts here */
            continue;
         case IN_STRING:
            /* we're in a double quoted string, so keep going until we hit a close " */
            if (c == '"')
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 < p; p2++, c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++; 

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_STRING or we handled the end above */
         case IN_WORD:
            /* we're in a word, so keep going until we get to a space */
            if (isspace(c))
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 <p; p2++,c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               ARGUC++; 

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_WORD or we handled the end above */
      }	
   }
}


int pre_main(const char *argv)
{
   int i;
   bool Only1Arg;

   parse_cmdline(argv); 

   Only1Arg = (strcmp(ARGUV[0],"skelsdl") == 0) ? 0 : 1;

   for (i = 0; i<64; i++)
      xargv_cmd[i] = NULL;

   	Add_Option("skelsdl");

   if(Only1Arg)
   {
     //Add_Option("skelsdl");
      Add_Option(RPATH/*ARGUV[0]*/);
   }
   else
   { // Pass all cmdline args
      for(i = 0; i < ARGUC; i++)
         Add_Option(ARGUV[i]);
   }

   for (i = 0; i < PARAMCOUNT; i++)
   {
      xargv_cmd[i] = (char*)(XARGV[i]);
      LOGI("%2d  %s\n",i,XARGV[i]);
   }

   skel_main(PARAMCOUNT,( char **)xargv_cmd); 

   xargv_cmd[PARAMCOUNT - 2] = NULL;
}

extern "C" long GetTicks(void)
{ // in MSec
#ifndef _ANDROID_

#ifdef __CELLOS_LV2__

   //#warning "GetTick PS3\n"

   unsigned long	ticks_micro;
   uint64_t secs;
   uint64_t nsecs;

   sys_time_get_current_time(&secs, &nsecs);
   ticks_micro =  secs * 1000000UL + (nsecs / 1000);

   return ticks_micro/1000;
#else
   struct timeval tv;
   gettimeofday (&tv, NULL);
   return (tv.tv_sec*1000000 + tv.tv_usec)/1000;
#endif

#else

   struct timespec now;
   clock_gettime(CLOCK_MONOTONIC, &now);
   return (now.tv_sec*1000000 + now.tv_nsec/1000)/1000;
#endif

} 

int slowdown=0;
//NO SURE FIND BETTER WAY TO COME BACK IN MAIN THREAD IN HATARI GUI
void gui_poll_events(void)
{
   Ktime = GetTicks();

   if(Ktime - LastFPSTime >= 1000/50)
   {
	  slowdown=0;
      frame++; 
      LastFPSTime = Ktime;

	//FIXME  Gui endless loop -> no retro_run() call
	//  retro_run_gui();

   }
}

void enter_gui(void)
{
  //save_bkg();
  //	Dialog_DoProperty();
//	pauseg=0;
printf("enter gui!\n");
}

void pause_select(void)
{
   if(pauseg==1 && firstps==0)
   {
      firstps=1;
      enter_gui();
      firstps=0;
   }
}


void retro_key_down(unsigned char retrok)
{

}

void retro_key_up(unsigned char retrok)
{

}

int bitstart=0;
int pushi=0; //mouse button
int keydown=0,keyup=0;
int KBMOD=-1;


#define MATRIX(a,b) (((a) << 3) | (b))

void Process_key/*(void)*/(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick)
{
   int i;

   keydown=0;keyup=0;

   for(i=0;i<320;i++)
   {
      	Key_Sate[i]=input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0,i) ? 0x80: 0;
      
        if(Key_Sate[i]  && Key_Sate2[i]==0)
        {

			if(i==RETROK_RALT){
				KBMOD=-KBMOD;
				printf("Modifier pressed %d \n",KBMOD); 
		        Key_Sate2[i]=1;
				continue;
			}
			
			/*the_app->*/TheC64->TheDisplay->Keymap_KeyDown(i,key_matrix,rev_matrix,joystick);

            //retro_key_down( i );
            		Key_Sate2[i]=1;
			bitstart=1;//
			keydown++;
        }
        else if ( !Key_Sate[i] && Key_Sate2[i]==1 )
        {

			if(i==RETROK_RALT){
				//KBMOD=-KBMOD;
				//printf("Modifier pressed %d \n",KBMOD); 
        		Key_Sate2[i]=0;
				continue;
			}

			
			/*the_app->*/TheC64->TheDisplay->Keymap_KeyUp(i,key_matrix,rev_matrix,joystick);

            //retro_key_up( i );
            		Key_Sate2[i]=0;
			bitstart=0;
			keyup++;

        }
      
   }



}

int Retro_PollEvent(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick)
{
	//   RETRO        B    Y    SLT  STA  UP   DWN  LEFT RGT  A    X    L    R    L2   R2   L3   R3
    //   INDEX        0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
    //   C64          BOOT VKB  M/J  R/S  UP   DWN  LEFT RGT  B1   GUI  F7   F1   F5   F3   SPC  1 

   int SAVPAS=PAS;	
   int i;

   input_poll_cb();

   int mouse_l;
   int mouse_r;
   int16_t mouse_x,mouse_y;
   mouse_x=mouse_y=0;

   if(SHOWKEY==-1 && pauseg==0)Process_key(key_matrix,rev_matrix,joystick);

if(pauseg==0){
/*
   i=0;//Autoboot
   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
      mbt[i]=1;
   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
   {
      mbt[i]=0;
	  kbd_buf_feed("\rLOAD\":*\",8,1:\rRUN\r\0");
	  autoboot=true; 
   }
*/

   i=1;//show vkbd toggle
   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
      mbt[i]=1;
   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
   {
      mbt[i]=0;
      SHOWKEY=-SHOWKEY;
      Screen_SetFullUpdate(0);  
   }
}
   i=2;//mouse/joy toggle
   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
      mbt[i]=1;
   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
      mbt[i]=0;
      MOUSE_EMULATED=-MOUSE_EMULATED;
   }


   i=3;//push r/s
   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 ){
      mbt[i]=1;//validkey(MATRIX(0,3),0,key_matrix,rev_matrix,joystick);
   }
   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
      mbt[i]=0;pauseg=1;//validkey(MATRIX(0,3),1,key_matrix,rev_matrix,joystick);      
   }

#if 0

   if(MOUSE_EMULATED==1){

	  if(slowdown>0)return 1;

      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))mouse_x += PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))mouse_x -= PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))mouse_y += PAS;
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))mouse_y -= PAS;
      mouse_l=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
      mouse_r=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);

      PAS=SAVPAS;

	  slowdown=1;
   }
   else {

      mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      mouse_l    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
      mouse_r    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
   }

   static int mmbL=0,mmbR=0;

   if(mmbL==0 && mouse_l){

      mmbL=1;		
      pushi=1;
	  touch=1;

   }
   else if(mmbL==1 && !mouse_l) {

      mmbL=0;
      pushi=0;
	  touch=-1;
   }

   if(mmbR==0 && mouse_r){
      mmbR=1;		
   }
   else if(mmbR==1 && !mouse_r) {
      mmbR=0;
   }

   gmx+=mouse_x;
   gmy+=mouse_y;
   if(gmx<0)gmx=0;
   if(gmx>retrow-1)gmx=retrow-1;
   if(gmy<0)gmy=0;
   if(gmy>retroh-1)gmy=retroh-1;

#endif

return 1;

}

void Emu_init(){

#ifdef RETRO_AND
   MOUSEMODE=1;
#endif

   update_variables();

   memset(Key_Sate,0,512);
   memset(Key_Sate2,0,512);

   pre_main(RPATH);

}

void Emu_uninit(){

	quit_frodo_emu();

   texture_uninit();
}

void retro_shutdown_core(void)
{
   LOGI("SHUTDOWN\n");

	quit_frodo_emu();

   texture_uninit();
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

void retro_reset(void){

}

void retro_init(void)
{    	
   const char *system_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
   {
      // if defined, use the system directory			
      retro_system_directory=system_dir;		
   }		   

   const char *content_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
   {
      // if defined, use the system directory			
      retro_content_directory=content_dir;		
   }			

   const char *save_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
   {
      // If save directory is defined use it, otherwise use system directory
      retro_save_directory = *save_dir ? save_dir : retro_system_directory;      
   }
   else
   {
      // make retro_save_directory the same in case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY is not implemented by the frontend
      retro_save_directory=retro_system_directory;
   }

   if(retro_system_directory==NULL)sprintf(RETRO_DIR, "%s\0",".");
   else sprintf(RETRO_DIR, "%s\0", retro_system_directory);

   LOGI("Retro SYSTEM_DIRECTORY %s\n",retro_system_directory);
   LOGI("Retro SAVE_DIRECTORY %s\n",retro_save_directory);
   LOGI("Retro CONTENT_DIRECTORY %s\n",retro_content_directory);

#ifndef RENDER16B
    	enum retro_pixel_format fmt =RETRO_PIXEL_FORMAT_XRGB8888;
#else
    	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
#endif
   
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      fprintf(stderr, "PIXEL FORMAT is not supported.\n");
LOGI("PIXEL FORMAT is not supported.\n");
      exit(0);
   }

	struct retro_input_descriptor inputDescriptors[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3" }
	};
	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &inputDescriptors);

   texture_init();

}

void retro_deinit(void)
{	
   app_free(); 

   Emu_uninit(); 

   LOGI("Retro DeInit\n");
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Frodo";
   info->library_version  = "V4_2";
   info->valid_extensions = "d64|t64|x64|p00|lnx|zip";
   info->need_fullpath    = true;
   info->block_extract = false;

}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   struct retro_game_geometry geom = { retrow, retroh, 1024, 1024,4.0 / 3.0 };
   struct retro_system_timing timing = { 50.0, 44100.0 };

   info->geometry = geom;
   info->timing   = timing;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}
/*
void retro_audiocb(signed short int *sound_buffer,int sndbufsize){
 	int x;
    for(x=0;x<sndbufsize;x++)audio_cb(sound_buffer[x],sound_buffer[x]);	
}
*/


void retro_run(void)
{
   int x;

   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

#ifdef __EMSCRIPTEN__ 
   static int firstinit=0;
   if(firstinit==0){
	firstinit++;
	Emu_init();
   }
#endif

   if(pauseg==0){
	  
      	if(SND==1)
		for(x=0;x<882;x++)
			audio_cb(SNDBUF[x],SNDBUF[x]);

	#ifndef FRODO_SC
		for(x=0;x<312;x++)
	#else
		for(x=0;x<63*312;x++) 
	#endif
		TheC64->thread_func();	
   }

   app_frame();

   video_cb(Retro_Screen,retrow,retroh,retrow<<PIXEL_BYTES);

}

/*
unsigned int lastdown,lastup,lastchar;
static void keyboard_cb(bool down, unsigned keycode,
      uint32_t character, uint16_t mod)
{
   //logging.log(RETRO_LOG_INFO, "Down: %s, Code: %d, Char: %u, Mod: %u.\n",
     //    down ? "yes" : "no", keycode, character, mod);
if(down)lastdown=keycode;
else lastup=keycode;
lastchar=character;
}
*/

bool retro_load_game(const struct retro_game_info *info)
{
   const char *full_path;

   (void)info;

/*
   struct retro_keyboard_callback cb = { keyboard_cb };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
*/
   full_path = info->path;

   strcpy(RPATH,full_path);

   update_variables();

   app_init();

	memset(SNDBUF,0,1024*2*2);
#ifndef __EMSCRIPTEN__ 
	Emu_init();
#endif

   return true;
}

void retro_unload_game(void){

   pauseg=0;
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

