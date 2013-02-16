/*

Standalone GLCD interface for Leonardo and Beerbot Screens
Use this if you want to just debug the GLCD alone
@Govind


*/


#include <glcd.h>
//#include "glcd_Buildinfo.h"
//#include "include/glcd_io.h"
//#include "include/glcd_errno.h"
#include "fonts/SystemFont5x7.h"       // system font
//#include "fonts/allFonts.h"         // system and arial14 fonts are used
#include "bitmaps/allBitmaps.h"       // all images in the bitmap dir 

#define BEERBOT_VERSION  1
#define TIME_TEXT_DISP    5
#define TIME_SPLASH_DISP    5000
#define SPLASH_MSG1 " Maxus "
#define SPLASH_MSG2 "Technology"
#define VEND_MSG "Please Collect Your Beer"
#define CONNECT_MSG "Connecting to GSM + WiFi ..."
#define READY_MSG "READY!!!"
#define STATUS_GSM_NO_SIM "GSM : No SIM"
#define STATUS_GSM_OK     "GSM : OK"
#define STATUS_GSM_NOK    "GSM : NOK"
#define STATUS_WiFi_OK    "WiFi: OK"
#define STATUS_WiFi_NOK   "WiFi: NOK"
#define STATUS_NET_OK     "WWW : OK"
#define STATUS_NET_NOK    "WWW : NOK"
#define STATUS_COSM_OK    "COSM: OK"
#define STATUS_COSM_NOK   "COSM: NOK"
#define STATUS_GSM  1
#define STATUS_WiFi  2
#define STATUS_NET  3
#define STATUS_COSM  4

const char* statusGsm ;
const char* statusWiFi ;
const char* statusNet ;
const char* statusCosm ;

typedef enum{
  DispDummy,
  DispConnecting,
  DispError,
  DispReady,
  DispVending  
} 
e_DisplayStates;

e_DisplayStates DispState;
e_DisplayStates DispStatePrev;
boolean updateDispStat;

Image_t icon;
gText textArea;              // a text area to be defined later in the sketch
gText textAreaArray[3];      // an array of text areas  
gText countdownArea =  gText(GLCD.CenterX, GLCD.CenterY,1,1,System5x7); // text area for countdown digits

//#include <avr/pgmspace.h>
//#define P(name)   static const prog_char name[] PROGMEM   // declare a static string in AVR Progmem

void updateDispStatus(byte type, byte value)
// Call this to update the status message in Ready or Error States
{

  switch(type)
  {
  case STATUS_GSM:
    switch(value)
    {
    case 0: 
      statusGsm = STATUS_GSM_NOK; 
      break;
    case 1: 
      statusGsm = STATUS_GSM_OK; 
      break;
    case 3: 
      statusGsm = STATUS_GSM_NO_SIM; 
      break;
    }
    break;
  case STATUS_WiFi:
    switch(value)
    {
    case 0: 
      statusWiFi = STATUS_WiFi_NOK; 
      break;
    case 1: 
      statusWiFi = STATUS_WiFi_OK; 
      break;
    }
    break;

  case STATUS_NET:
      switch(value)
    {
    case 0: 
      statusNet = STATUS_NET_NOK; 
      break;
    case 1: 
      statusNet = STATUS_NET_OK; 
      break;
    }
    break;
  case STATUS_COSM:
      switch(value)
    {
    case 0: 
      statusCosm = STATUS_COSM_NOK; 
      break;
    case 1: 
      statusCosm = STATUS_COSM_OK; 
      break;
    }
    break;

  }
  updateDispStat = true;
}

void setup()
{
  delay(5);	// allow the hardware time settle
  Serial.begin(9600);
  //while(!Serial) {
  //}; // wait on USB serial port to be ready on Leonardo
}


void  loop()
{   // run over and over again
  int status;
  status = GLCD.Init();   // initialise the library, non inverted writes pixels onto a clear screen


  if(status) // did the initialization fail?
  {
    Serial.println("GLCD initialization Failed: ");
    Serial.println(" (status code: ");
    Serial.print(status);
    Serial.println(')');
  }

  // Init is over
  //introScreen();
  spashScreen(); 
  //GLCD.SelectFont(System5x7, BLACK); // font for the default text area 
  //GLCD.SelectFont(System5x7, BLACK);
  while(1){
    DispScreenUpdate();

  }
}


void spashScreen(void)
{
  statusGsm = STATUS_GSM_NOK;
  statusWiFi = STATUS_WiFi_NOK;
  statusNet = STATUS_NET_NOK;
  statusCosm = STATUS_COSM_NOK;
  GLCD.ClearScreen(); 
  //GLCD.SelectFont(System5x7, BLACK); Arial_14
  // Display title for some time
  GLCD.SelectFont(System5x7); // you can also make your own fonts, see playground for details   
  GLCD.CursorToXY(GLCD.Width/2 - 44, 3);
  GLCD.print("BeerBot version");
  GLCD.print(BEERBOT_VERSION, DEC);
  GLCD.DrawRoundRect(8,0,GLCD.Width-19,17, 5);  // rounded rectangle around text area   
  countdown(TIME_TEXT_DISP);  
  GLCD.ClearScreen(); 
  // Set up text area and welcome message
  //GLCD.DrawRoundRect(GLCD.CenterX + 2, 0, GLCD.CenterX -3, GLCD.Bottom, 5);  // rounded rectangle around text area 
  //GLCD.DrawRoundRect(GLCD.CenterX, 0, GLCD.CenterX, GLCD.Bottom, 5); 
  //textArea.DefineArea(GLCD.CenterX + 5, 3, GLCD.Right-2, GLCD.Bottom-4, SCROLL_UP); 
  textArea.DefineArea(GLCD.CenterX, 0, GLCD.Right, GLCD.Bottom, SCROLL_UP); 
  textArea.SelectFont(System5x7, BLACK);
  textArea.CursorTo(0,0);
  textArea.println("Brought to you by");
  textArea.println(SPLASH_MSG1);
  textArea.println(SPLASH_MSG2);
  // start scribble
  scribble(TIME_SPLASH_DISP);  // run for 5 seconds  
  // move to status screen
  DispState = DispVending;//DispConnecting;
  DispStatePrev = DispDummy;
  updateDispStatus(1,1);
}

void DispScreenUpdate(void)
// Call this periodically to update the screen based on events
{
    
  if(DispState != DispStatePrev){
    switch(DispState)
    {
    case DispConnecting:
      GLCD.ClearScreen();
      icon = sync;  
      GLCD.DrawBitmap(icon, 0,0); 
      textArea.CursorTo(0,0);
      textArea.print(CONNECT_MSG);
      break;

    case DispReady:
      GLCD.ClearScreen();
      icon = beer_icon;  
      GLCD.DrawBitmap(icon, 0,0); 
      break;

    case DispVending:
      GLCD.ClearScreen();
      GLCD.CursorToXY(GLCD.Width/2 - 44, 3);
      GLCD.print("   Vending In  ");
      GLCD.DrawRoundRect(8,0,GLCD.Width-19,17, 5);  // rounded rectangle around text area 
      countdown(TIME_TEXT_DISP); 
      vendBeer();
      GLCD.ClearScreen();
      icon = explosion;  
      GLCD.DrawBitmap(icon, 0,0);
      textArea.CursorTo(0,0);
      textArea.print(VEND_MSG);
      countdown(10);
      GLCD.ClearScreen();
      break;

    default:
      GLCD.ClearScreen();
      icon = error;  // the 32 pixel high icon
      GLCD.DrawBitmap(icon, 0,0); //draw the bitmap at the given x,y position

      break;

    }
    DispStatePrev = DispState;
  }

  if( (updateDispStat == true) && ((DispState == DispError) || (DispState == DispReady)) ){
    textArea.ClearArea();
    textArea.println(statusGsm);
    textArea.println(statusWiFi);
    textArea.println(statusNet);
    textArea.println(statusCosm);
    updateDispStat = false;
  }
  
  if(DispState == DispVending)
    DispState = DispReady;
}

void countdown(int count){
  while(count--){  // do countdown  
    countdownArea.ClearArea(); 
    countdownArea.print(count);
    delay(1000);  
  }  
}

/*
 * scribble drawing routine adapted from TellyMate scribble Video sketch
 * http://www.batsocks.co.uk/downloads/tms_scribble_001.pde
 */
void scribble( const unsigned int duration )
{
  const  float tick = 1/128.0;
  float g_head_pos = 0.0;

  for(unsigned long start = millis();  millis() - start < duration; )
  {
    g_head_pos += tick ;

    float head = g_head_pos ;
    float tail = head - (256 * tick) ;

    // set the pixels at the 'head' of the line...
    byte x = fn_x( head ) ;
    byte y = fn_y( head ) ;
    GLCD.SetDot( x , y , BLACK) ;

    // clear the pixel at the 'tail' of the line...
    x = fn_x( tail ) ;
    y = fn_y( tail ) ;  
    GLCD.SetDot( x , y , WHITE) ;
  }
}

byte fn_x( float tick )
{
  return (byte)(GLCD.Width/4 + (GLCD.Width/4-1) * sin( tick * 1.8 ) * cos( tick * 3.2 )) ;
}

byte fn_y( float tick )
{
  return (byte)(GLCD.Height/2 + (GLCD.Height/2 -1) * cos( tick * 1.2 ) * sin( tick * 3.1 )) ;
}

void introScreen(){  
  icon = beer_icon;  // the 32 pixel high icon
  GLCD.DrawBitmap(icon, 0,0); //draw the bitmap at the given x,y position
  countdown(10);
  GLCD.ClearScreen();
  icon = error;  // the 32 pixel high icon
  GLCD.DrawBitmap(icon, 0,0); //draw the bitmap at the given x,y position
  countdown(10);
  GLCD.ClearScreen();
  icon = explosion;  // the 32 pixel high icon
  GLCD.DrawBitmap(icon, 0,0); //draw the bitmap at the given x,y position
  countdown(10);
  icon = sync;  // the 32 pixel high icon
  GLCD.DrawBitmap(icon, 0,0); //draw the bitmap at the given x,y position
  countdown(10);
  GLCD.ClearScreen();
  GLCD.ClearScreen(); 
  GLCD.CursorToXY(GLCD.Width/2 - 44, 3);
  GLCD.print("GLCD version ");
  GLCD.print(GLCD_VERSION, DEC);
  GLCD.DrawRoundRect(8,0,GLCD.Width-19,17, 5);  // rounded rectangle around text area   
  countdown(3);  
  GLCD.ClearScreen(); 
  scribble(5000);  // run for 5 seconds
  //moveBall(6000); // kick ball for 6 seconds
  GLCD.SelectFont(System5x7, BLACK);
  //showCharacters("5x7 font:", System5x7);
  countdown(3);
  //showCharacters("Arial_14:", Arial_14);
  countdown(3);
  //textAreaDemo();
  //scrollingDemo();
}

