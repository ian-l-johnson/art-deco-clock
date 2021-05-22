/*****************************************************************************

 pi Clock (uses Allegro5)

 Clock modes:
    1. time,date,day
    2. calendar


 Notes:
        Top left is X,Y=0,0
        +----------+
        | 0,0      |
        |          |
        +----------+


  Common Screen sizes:

    1280x800  10" portable LCD
    ??        5" pi LCD


  Softmaxx blue  5E 74 9D
 ****************************************************************************/
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <pthread.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_opengl.h>



//....DEBUG..............................................................

#define DEBUG_FULLSCREEN        1   // 1=fullscreen,  0=windowed
#define DEBUG_WINDOWSIZE_X   1280
#define DEBUG_WINDOWSIZE_Y    800
#define DEBUG_OVERRIDE_MODE     0   // 0=no override, 'C'=cal, 'T'=time-date

//.......................................................................


#define VERSION           "1.2"   // version string
#define DAYS_PER_WEEK        7    // days per week
#define EVENT_TIMECHANGE   600    // time changed event

#define UNUSED_PARM(x) (void)(x)        /**< suppress compiler warning */
#define MILLISECONDS      1000 // uS

#if 0
   // computer font
   #define TIME_FONT  "./fonts/DIG7.ttf"
   #define TIME_FONT_SIZE 280
   #define TIME_X  680
   #define TIME_Y  40

   #define DATE_FONT "./fonts/DIG7.ttf"
   #define DATE_FONT_SIZE 170
   #define DATE_X  440
   #define DATE_Y  300

   #define TZ_FONT    "./fonts/DIG7.ttf"
   #define TZ_FONT_SIZE 140
   #define TZ_X  DATE_X+240
   #define TZ_Y  DATE_Y+22
#else
   // art deco font
   #define TIME_FONT_SCALE  0.58
   #define TIME_X_SCALE  0.70
   #define TIME_Y_SCALE  0.08
   #define TIME_FONT  "./fonts/COPASETI.ttf"
   #define TIME_FONT_R   200
   #define TIME_FONT_G   200
   #define TIME_FONT_B   200

   #define DATE_FONT_SCALE  0.28
   #define DATE_X_SCALE  0.53
   #define DATE_Y_SCALE  0.63
   #define DATE_FONT "./fonts/COPASETI.ttf"
   #define DATE_FONT_R   170
   #define DATE_FONT_G   180
   #define DATE_FONT_B   200

   #define CAL_FONT_SCALE  0.10
   #define CAL_FONT_WIDTH_MULT 3.7 //3.3  // multiplier used to stretch out days horizontally
   #define CAL_X_SCALE   0.53
   #define CAL_Y_SCALE   0.63
   #define CAL_FONT "./fonts/COPASETI.ttf"
   #define CAL_FONT_R    200
   #define CAL_FONT_G    200
   #define CAL_FONT_B    200
#endif

typedef struct {
  uint16_t font_size;
  uint16_t x_pos;
  uint16_t y_pos;
} wordInfo_s;

typedef enum {
   piClock_dateTime,
   piClock_calendar
} piClock_Display_Types_e;

wordInfo_s TimeInf;
wordInfo_s DateInf;
wordInfo_s CalInf;

ALLEGRO_BITMAP *background;
ALLEGRO_FONT *Time_font;
ALLEGRO_FONT *Date_font;
ALLEGRO_FONT *Cal_font;
ALLEGRO_EVENT_SOURCE my_event_source;
ALLEGRO_EVENT event;
ALLEGRO_EVENT my_event;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_DISPLAY *display = NULL;

piClock_Display_Types_e piClock_Display_Mode;
pthread_t timing_pthread;
char g_timeStr[20];

int cur_hour;
int cur_min;
int cur_day;
int cur_mon;
int num_days;
int cur_wday;
int start_day;


//----------------------------------------------------------------------------
void display_calendar(uint8_t startDay, uint8_t curDay, uint8_t numDays, uint16_t max_X, uint16_t max_Y)
{
   ALLEGRO_COLOR weekDayCol,weekEndCol;
   ALLEGRO_COLOR todayColor;
   ALLEGRO_COLOR weekdayColor;
   char m1[2],m2[2],m3[2];  // first 3 letters of a month
   int fontWidth;
   int fontHeight;
   int i;


   // init colors
   weekdayColor = al_map_rgb(DATE_FONT_R, DATE_FONT_G, DATE_FONT_B); // TODO 2 change to DoW_FONT_R
   todayColor   = al_map_rgb(200, 60, 60); // TODO 2 change to #define
   weekDayCol   = al_map_rgb(CAL_FONT_R, CAL_FONT_G, CAL_FONT_B);
   weekEndCol   = al_map_rgb(CAL_FONT_R-60, CAL_FONT_G-60, CAL_FONT_B-60);

   // erase screen
   al_draw_filled_rectangle(0, 0, max_X, max_Y, al_map_rgb(0, 0, 0));

   // display background image
   al_draw_scaled_bitmap(background,
         0,0, al_get_bitmap_width(background), al_get_bitmap_height(background),
         0,0, max_X, max_Y, 0);

   fontWidth = (al_get_text_width(Cal_font, "3") * CAL_FONT_WIDTH_MULT );
   fontHeight = al_get_font_line_height(Cal_font)   * 1.2;


   int Xpos_start = ((max_X - (DAYS_PER_WEEK * fontWidth))/2) + 32;
   //   int Xpos_start = max_X * 0.16;
   int Ypos_start = max_Y * 0.09;

   // display days of week
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*0),Ypos_start, ALLEGRO_ALIGN_CENTRE, "Mo");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*1),Ypos_start, ALLEGRO_ALIGN_CENTRE, "Tu");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*2),Ypos_start, ALLEGRO_ALIGN_CENTRE, "We");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*3),Ypos_start, ALLEGRO_ALIGN_CENTRE, "Th");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*4),Ypos_start, ALLEGRO_ALIGN_CENTRE, "Fr");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*5),Ypos_start, ALLEGRO_ALIGN_CENTRE, "Sa");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*6),Ypos_start, ALLEGRO_ALIGN_CENTRE, "Su");


   // display Month
   m1[1]=0; m2[1]=0; m3[1]=0;
   switch(cur_mon)
   {
      case(1):  m1[0]='J';m2[0]='a';m3[0]='n'; break;
      case(2):  m1[0]='F';m2[0]='e';m3[0]='b'; break;
      case(3):  m1[0]='M';m2[0]='a';m3[0]='r'; break;
      case(4):  m1[0]='A';m2[0]='p';m3[0]='r'; break;
      case(5):  m1[0]='M';m2[0]='a';m3[0]='y'; break;
      case(6):  m1[0]='J';m2[0]='u';m3[0]='n'; break;
      case(7):  m1[0]='J';m2[0]='u';m3[0]='l'; break;
      case(8):  m1[0]='A';m2[0]='u';m3[0]='g'; break;
      case(9):  m1[0]='S';m2[0]='e';m3[0]='p'; break;
      case(10): m1[0]='O';m2[0]='c';m3[0]='t'; break;
      case(11): m1[0]='N';m2[0]='o';m3[0]='v'; break;
      case(12): m1[0]='D';m2[0]='e';m3[0]='c'; break;
      default:  m1[0]='?';m2[0]='?';m3[0]='?'; break;
   }
   al_draw_text(Cal_font, weekdayColor, Xpos_start-140,Ypos_start+210, ALLEGRO_ALIGN_CENTRE, m1);
   al_draw_text(Cal_font, weekdayColor, Xpos_start-140,Ypos_start+290, ALLEGRO_ALIGN_CENTRE, m2);
   al_draw_text(Cal_font, weekdayColor, Xpos_start-140,Ypos_start+370, ALLEGRO_ALIGN_CENTRE, m3);

   // display Year
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*7.53),Ypos_start+170, ALLEGRO_ALIGN_CENTRE, "2");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*7.53),Ypos_start+250, ALLEGRO_ALIGN_CENTRE, "0");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*7.53),Ypos_start+330, ALLEGRO_ALIGN_CENTRE, "2");
   al_draw_text(Cal_font, weekdayColor, Xpos_start+(fontWidth*7.53),Ypos_start+410, ALLEGRO_ALIGN_CENTRE, "1");


   int x,y;
   int CAL_YPOS_START =  max_Y * 0.22;
   char tmpStr[9];

   x = startDay-1;
   y = 0;
   for(i=1; i<numDays+1; i++)
   {
      sprintf(tmpStr,"%d", i);
      if(i==curDay)
         al_draw_text(Cal_font, todayColor, (x*fontWidth)+Xpos_start,(y*fontHeight)+CAL_YPOS_START, ALLEGRO_ALIGN_CENTRE, tmpStr);
      else
      {
         if(x<5)
            al_draw_text(Cal_font, weekDayCol, (x*fontWidth)+Xpos_start,(y*fontHeight)+CAL_YPOS_START, ALLEGRO_ALIGN_CENTRE, tmpStr);
         else
            al_draw_text(Cal_font, weekEndCol, (x*fontWidth)+Xpos_start,(y*fontHeight)+CAL_YPOS_START, ALLEGRO_ALIGN_CENTRE, tmpStr);
      }
      x++;
      if(x>=DAYS_PER_WEEK)
      {
         x=0;
         y++;
         if(y>DAYS_PER_WEEK)
            y=0;
      }
   }

   // update display
   al_flip_display();
}


//----------------------------------------------------------------------------
void display_time(char *time_str, uint16_t max_X, uint16_t max_Y)
{
   char time2str[20];
   int fontWidth;


   // erase screen
   al_draw_filled_rectangle(0, 0, max_X, max_Y, al_map_rgb(0, 0, 0));

   // display time
   fontWidth = al_get_text_width(Time_font, time_str);
   al_draw_text(Time_font, al_map_rgb(TIME_FONT_R, TIME_FONT_G, TIME_FONT_B),
         ((max_X / 2) - (fontWidth / 2)), TimeInf.y_pos, ALLEGRO_ALIGN_LEFT, time_str);

   // get today's date
   time_t rawtime;
   struct tm *timeinfo;
   time(&rawtime);
   timeinfo = localtime(&rawtime);
   char wday[10];
   switch(timeinfo->tm_wday)
   {
      case (0):
         strcpy(wday, "Sun");
         break;
      case (1):
         strcpy(wday, "Mon");
         break;
      case (2):
         strcpy(wday, "Tue");
         break;
      case (3):
         strcpy(wday, "Wed");
         break;
      case (4):
         strcpy(wday, "Thu");
         break;
      case (5):
         strcpy(wday, "Fri");
         break;
      case (6):
         strcpy(wday, "Sat");
         break;
      default:
         strcpy(wday, "   ");
         break;
   }

   // format date for display
   sprintf(time2str,"%s %d.%d.%d", wday, (timeinfo->tm_mon)+1, timeinfo->tm_mday, timeinfo->tm_year-100);

   // display date
   fontWidth = al_get_text_width(Date_font, time2str);
   al_draw_text(Date_font, al_map_rgb(DATE_FONT_R, DATE_FONT_G, DATE_FONT_B),
         ((max_X / 2) - (fontWidth / 2)), DateInf.y_pos, ALLEGRO_ALIGN_LEFT, time2str);

   // refresh display
   al_flip_display();
}


//----------------------------------------------------------------------------
//  timing thread: if time has changed, sent 'time changed' event
//----------------------------------------------------------------------------
void* timing_thread(void *ptr)
{
   UNUSED_PARM(ptr);
   time_t rawtime;
   struct tm *timeinfo;
   char displayTime[20];
   int hours;
   displayTime[0] = 0;

   while (1)
   {
      time(&rawtime);
      timeinfo = localtime(&rawtime);

      hours = timeinfo->tm_hour;
      // if 12h clock:
      if(hours > 12)
         hours = hours - 12;
      if(hours == 0)
         hours = 12;
      sprintf(g_timeStr, "%d:%02d", hours, timeinfo->tm_min);
      cur_hour = hours;

      cur_min = timeinfo->tm_min;

      cur_day = timeinfo->tm_mday;
      cur_mon = timeinfo->tm_mon + 1;
      switch(cur_mon)
      {
         case 1:
         case 3:
         case 5:
         case 7:
         case 8:
         case 10:
         case 12:
            num_days = 31;
            break;
         case 4:
         case 6:
         case 9:
         case 11:
            num_days = 30;
            break;
         case 2:
            num_days = 28;
            break;
      }

      cur_wday = timeinfo->tm_wday;

      // calculate the day-of-the-week of the 1st of the current month
      start_day = cur_wday-(((cur_day-1)%DAYS_PER_WEEK)+1);
      if(start_day < 0)
         start_day = start_day+DAYS_PER_WEEK+1;
      else
         start_day++;


      // time has changed...
      if(strcmp(displayTime, g_timeStr) != 0)
      {
         // create update time event
         my_event.user.type = EVENT_TIMECHANGE;
         my_event.user.data1 = ' ';
         if(al_emit_user_event(&my_event_source, &my_event, NULL) == false) fprintf(
               stderr, "ERROR al_emit_user_event()\n");

         // save time for next check
         strcpy(displayTime, g_timeStr);
      }
      usleep(1000 * MILLISECONDS);
   }
}


//============================================================================
int main(int argc, char **argv)
{
   uint16_t ScreenSize_X, ScreenSize_Y;


   piClock_Display_Mode = piClock_dateTime;  // default to time/date


   // if arg, check it
   if(argc >= 2)
   {
      if((argv[1][0]=='C') || (argv[1][0]=='c'))
      {
         piClock_Display_Mode = piClock_calendar;
      }
   }

   // debug overrides
#if DEBUG_OVERRIDE_MODE
   if(DEBUG_OVERRIDE_MODE=='C')
      piClock_Display_Mode = piClock_calendar;
   if(DEBUG_OVERRIDE_MODE=='T')
      piClock_Display_Mode = piClock_dateTime;
#endif


   printf("\n"
         "piClock %s\n\n"
         "press 'Q' to quit"
         "\n"
         "usage:\n"
         "   piClock T    - time,date view\n"
         "   piClock C    - calendar view\n"
         "\n\n", VERSION);



   // **************** Init Display ************************************
   if(!al_init())
   {
      printf("failed to init Allegro\n");
      return (-9);
   }

   /* try to detect if running in Xwindows
    #include <X11/Xlib.h>
    int main()
    { exit(XOpenDisplay(NULL) ? 0 : 1);  }
    $ gcc -o xprobe xprobe.c -L/usr/X11R6/lib -lX11   */

   al_init_image_addon();
   al_init_primitives_addon();
   al_init_font_addon();
   al_init_ttf_addon();

   ALLEGRO_DISPLAY_MODE disp_data;
   al_get_display_mode(al_get_num_display_modes() - 1, &disp_data);

   ScreenSize_X = disp_data.width;
   ScreenSize_Y = disp_data.height;

   #if DEBUG_FULLSCREEN==0
      ScreenSize_X = DEBUG_WINDOWSIZE_X;
      ScreenSize_Y = DEBUG_WINDOWSIZE_Y;
   #endif

   // fill structs based on max screen size
   TimeInf.font_size = (ScreenSize_Y*TIME_FONT_SCALE);
   TimeInf.x_pos     = (ScreenSize_X*TIME_X_SCALE);
   TimeInf.y_pos     = (ScreenSize_Y*TIME_Y_SCALE);

   DateInf.font_size = (ScreenSize_Y*DATE_FONT_SCALE);
   DateInf.x_pos     = (ScreenSize_X*DATE_X_SCALE);
   DateInf.y_pos     = (ScreenSize_Y*DATE_Y_SCALE);

   CalInf.font_size  = (ScreenSize_Y*CAL_FONT_SCALE);
//   CalInf.x_pos      = (ScreenSize_X*CAL_X_SCALE);
//   CalInf.y_pos      = (ScreenSize_Y*CAL_Y_SCALE);


   // DEBUG
   printf("video resolution is %d x %d\n", ScreenSize_X, ScreenSize_Y);

   // set to full screen/windowed
   #if DEBUG_FULLSCREEN
      al_set_new_display_flags(ALLEGRO_FULLSCREEN);
   #endif

   // set display size
   display = al_create_display(ScreenSize_X, ScreenSize_Y);

   // config display
   al_hide_mouse_cursor(display);  // hide mouse cursor
   al_inhibit_screensaver(true);   // don't let screensaver run

   al_install_keyboard();
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   al_init_user_event_source(&my_event_source);
   al_register_event_source(queue, &my_event_source);

   // load font
   Time_font = al_load_ttf_font(TIME_FONT, TimeInf.font_size, 0);
   if(!Time_font)
   {
      fprintf(stderr, ".Could not load TIME font.\n");
      return -1;
   }

   // load font
   Date_font = al_load_ttf_font(DATE_FONT, DateInf.font_size, 0);
   if(!Date_font)
   {
      fprintf(stderr, ".Could not load DATE font.\n");
      return -1;
   }

   // calendar font
   Cal_font = al_load_ttf_font(CAL_FONT, CalInf.font_size, 0);
   if(!Cal_font)
   {
      fprintf(stderr, ".Could not load CAL font.\n");
      return -1;
   }

   // load images in to memory
   background = al_load_bitmap("./images/artdeco_frame_2.png");
   if(!background)
   {
      printf("Could not load background.\n");
      //return -1;
   }

   /* create a pthread to do processing */
   if(pthread_create(&timing_pthread, NULL, timing_thread, NULL))
   {
      printf("Could NOT start timing pthread\n");
      return -1;
   }

   // main event loop
   while (1)
   {
      al_wait_for_event(queue, &event);

      // process events
      if(event.type == EVENT_TIMECHANGE)
      {
         switch(piClock_Display_Mode)
         {
            case piClock_dateTime:
               display_time(g_timeStr, ScreenSize_X, ScreenSize_Y); // update time string
               break;
            case piClock_calendar:
               display_calendar(start_day, cur_day, num_days, ScreenSize_X, ScreenSize_Y); // update time string
               break;
            default:
               return(-100);
         }
      }

      // quit if display is closed
      if(event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) break;

      // if key is pressed...
      if(event.type == ALLEGRO_EVENT_KEY_DOWN)
      {
         if(event.keyboard.keycode == ALLEGRO_KEY_Q) break; // quit

         /* TEST
          if (event.keyboard.keycode == ALLEGRO_KEY_DOWN)
          {
             printf("ALLEGRO_KEY_DOWN ");
             my_event.user.type = 700;
             my_event.user.data1 = 'D';
             if(al_emit_user_event(&my_event_source, &my_event, NULL) == false)
             fprintf(stderr, "ERROR al_emit_user_event()\n");
             temp_hr--;
          }
          if (event.type == 700)
          {
             sprintf(timeStr,"%d:%02d",temp_hr,cur_min);
             printf("%s\n",timeStr);
             display_time(timeStr);
          }
          */

      }
   } // while(1)

   pthread_cancel(timing_pthread);

   // shut down
   al_destroy_display(display);
   return 0;
}

